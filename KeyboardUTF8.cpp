#include  "KeyboardUTF8.h"

// MUDANÇA: O construtor é atualizado para aceitar e armazenar a referência do teclado
KeyboardUTF8::KeyboardUTF8(const UnicodeLookup *ut, const size_t uts, const DeadkeyLookup *dt, const size_t dts, USBHIDKeyboard &keyboard) :
  _keyboard(keyboard), // Armazena a referência
  UnicodeTable(ut),
  UnicodeTableSize(uts),
  DeadkeyTable(dt),
  DeadkeyTableSize(dts)
{
  UTF8State = UTF8_Idle;

  // Fill in the keycocdes for hex digits
  for (int i = 0; i < 16; i++)
  {
    HexKeycodes[i] = FindKeycode("0123456789abcdef"[i]) | MOD_ALT;
  }
}

uint16_t KeyboardUTF8::FindKeycode(uint32_t codepoint)
{
  // (Sem mudanças nesta função)
  for (size_t i = 0; i < UnicodeTableSize; i++)
  {
    if (pgm_read_word(&UnicodeTable[i].UnicodeCodepoint) == codepoint)
    {
      return pgm_read_word(&UnicodeTable[i].USBKeycodeWithModifiers);
    }
  }
  return 0;
}

size_t KeyboardUTF8::writeKeycode(uint8_t keycode)
{
  // MUDANÇA: usa _keyboard
  return _keyboard.write(keycode + 136);
}

size_t KeyboardUTF8::writeKeycodeWithModifiers(uint16_t keycode)
{
  // --- INÍCIO DA CORREÇÃO ---
  
  // 1. Pressiona os modificadores necessários
  if (keycode & MOD_SHIFT)
    _keyboard.press(KEY_LEFT_SHIFT);

  if (keycode & MOD_ALTGR)
    _keyboard.press(KEY_RIGHT_ALT);

  if (keycode & MOD_ALT)
    _keyboard.press(KEY_LEFT_ALT);

  // 2. Escreve a tecla principal (que faz um press/release dela)
  size_t result = writeKeycode(keycode & 0xFF);

  // 3. (A PARTE QUE FALTAVA) Solta os modificadores
  if (keycode & MOD_SHIFT)
    _keyboard.release(KEY_LEFT_SHIFT);

  if (keycode & MOD_ALTGR)
    _keyboard.release(KEY_RIGHT_ALT);

  if (keycode & MOD_ALT)
    _keyboard.release(KEY_LEFT_ALT);

  return result;
  
  // --- FIM DA CORREÇÃO ---
}
size_t KeyboardUTF8::writeUnicodeHex(uint32_t codepoint)
{
  // MUDANÇA: usa _keyboard
  _keyboard.press(KEY_LEFT_ALT);
  writeKeycodeWithModifiers(HexKeycodes[(codepoint >> 12) & 0x0F]);
  writeKeycodeWithModifiers(HexKeycodes[(codepoint >>  8) & 0x0F]);
  writeKeycodeWithModifiers(HexKeycodes[(codepoint >>  4) & 0x0F]);
  writeKeycodeWithModifiers(HexKeycodes[(codepoint >>  0) & 0x0F]);
  _keyboard.release(KEY_LEFT_ALT);
  return 1;
}

// (Função writeUnicode() sem mudanças, ela chama as outras que foram modificadas)
size_t KeyboardUTF8::writeUnicode(uint32_t codepoint)
{
  // First, see if there is a single keycode for the codepoint
  uint16_t keycodeWithModifiers = FindKeycode(codepoint);
  if (keycodeWithModifiers != 0)
  {
    return writeKeycodeWithModifiers(keycodeWithModifiers);
  }

  // No direct match.  Check the DeadkeyTable to see if the character can be
  // produced with a deadkey.
  for (size_t i = 0; i < DeadkeyTableSize; i++)
  {
    if (pgm_read_word(&DeadkeyTable[i].UnicodeCodepoint) == codepoint)
    {
      // We have a matching deadkey. We have a keycode for the deadkey so
      // now we look up the keycode for the 'base' (unaccented) key.
      uint32_t baseCodepoint = pgm_read_word(&DeadkeyTable[i].BaseKeyCodepoint);

      uint16_t baseKeycode;
      // Is the base codepoint the same?
      if (baseCodepoint == codepoint)
        baseKeycode = pgm_read_word(&DeadkeyTable[i].DeadkeyKeycodeWithModifiers);
      else
        baseKeycode = FindKeycode(baseCodepoint);
        
      if (baseKeycode != 0)
      {
        // We have a match for the base key codepoint.  Send the deadkey and basekey
        size_t keys = 0;
        keys += writeKeycodeWithModifiers(pgm_read_word(&DeadkeyTable[i].DeadkeyKeycodeWithModifiers));
        keys += writeKeycodeWithModifiers(baseKeycode);
        return keys;
      }
    }
  }

  // No direct keycode and no deadkey.  The only option left is to write the four hex
  // digit Unicode codepoint directly
  return writeUnicodeHex(codepoint);
}

// (Função write(uint8_t c) sem mudanças, ela chama as outras que foram modificadas)
size_t  KeyboardUTF8::write(uint8_t c)
{
  if ((c & 0x80) == 0x00) // Top bit is 0 (ASCII)
  {
    // If any UTF-8 encoding was in progress we throw it out.
    UTF8State = UTF8_Idle;
    return writeUnicode(c);
  }
  else if ((c & 0xC0) == 0x80) // Top 2 bits are 0b10, Subsequent byte
  {
    // Add the 6 bits from this "subsequent byte"
    UnicodeCodepoint <<= 6;
    UnicodeCodepoint |=  c & 0x3F; // Add 6 more bits

    switch (UTF8State)
    {
      case UTF8_2of3:
        UTF8State = UTF8_3of3; // Expecting one more
        break;

      case UTF8_2of4:
        UTF8State = UTF8_3of4; // Expecting two more
        break;

      case UTF8_3of4:
        UTF8State = UTF8_4of4; // Expecting one more
        break;

      case UTF8_2of2:
      case UTF8_3of3:
      case UTF8_4of4:
        // Done with encoding
        UTF8State = UTF8_Idle;
        return writeUnicode(UnicodeCodepoint);

      default: // Not expecting a "subsequent byte"
        UTF8State = UTF8_Idle;
        return 0;
    }
  }
  else if ((c & 0xE0) == 0xC0) // Top 3 bits are 0b110, First of 2 bytes
  {
    UnicodeCodepoint = c & 0x1F;
    UTF8State = UTF8_2of2;
    return 0;
  }
  else if ((c & 0xF0) == 0xE0) // Top 4 bits are 0b1110, First of 3 bytes
  {
    UnicodeCodepoint = c & 0x0F;
    UTF8State = UTF8_2of3;
    return 0;
  }
  else if ((c & 0xF8) == 0xE0) // Top 5 bits are 0b11110, First of 4 bytes
  {
    UnicodeCodepoint = c & 0x07;
    UTF8State = UTF8_2of4;
    return 0;
  }

  return 0;  // Invalid encoding
}
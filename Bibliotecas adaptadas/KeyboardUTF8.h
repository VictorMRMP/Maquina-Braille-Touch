// =======================================================================
// Autor: JohnWasser(https://github.com/JohnWasser/Arduino_KeyboardUTF8)
// Modificado por: Victor Marcelo Riverete Monteiro Padial
// Projeto: "Teclado Touch Braille para auxílio na alfabetização e navegação digital de deficientes visuais sensíveis ao toque e/ou com limitações motoras", realizado para o Trabalho de Conclusão de Curso para o curso de Engenharia Elétrica com ênfase em eletrônica
// Detalhes: Deve estar no mesmo caminho que a biblioteca USBHIDKeyboard.h, provavelmente em .../ArduinoData/packages/esp32/hardware/esp32/2.0.14/libraries/USB/src
// =======================================================================

#pragma once

#include <Arduino.h>
// MUDANÇA: Inclui a biblioteca HID do ESP32 em vez da 'Keyboard.h' padrão
#include "USBHIDKeyboard.h" 

const uint16_t MOD_SHIFT = 1 << (8 + (KEY_LEFT_SHIFT & 0x0F));
const uint16_t MOD_ALTGR = 1 << (8 + (KEY_RIGHT_ALT & 0x0F));
const uint16_t MOD_ALT = 1 << (8 + (KEY_LEFT_ALT & 0x0F));

// Estruturas (sem mudanças)
struct UnicodeLookup
{
  uint16_t UnicodeCodepoint;
  uint16_t USBKeycodeWithModifiers;
};

struct DeadkeyLookup
{
  uint16_t UnicodeCodepoint;
  uint16_t BaseKeyCodepoint;
  uint16_t DeadkeyKeycodeWithModifiers;
};

class KeyboardUTF8 : public Print
{
  public:
    // MUDANÇA: O construtor agora aceita uma referência ao seu objeto USBHIDKeyboard
    KeyboardUTF8(const UnicodeLookup *ul, const size_t uls, const DeadkeyLookup *dl, const size_t dls, USBHIDKeyboard &keyboard);
    
    size_t writeUnicode(uint32_t codepoint);
    size_t writeUnicodeHex(uint32_t codepoint);
    size_t write(uint8_t);
    
    // MUDANÇA: O begin() agora chama o begin() do objeto de teclado real
    void begin() { _keyboard.begin(); }

  private:
    // MUDANÇA: Armazena uma referência ao objeto de teclado fornecido
    USBHIDKeyboard &_keyboard; 

    const UnicodeLookup *UnicodeTable;
    const size_t UnicodeTableSize;
    const DeadkeyLookup *DeadkeyTable;
    const size_t DeadkeyTableSize;
    uint16_t FindKeycode(uint32_t codepoint);
    size_t writeKeycode(uint8_t keycode);
    size_t writeKeycodeWithModifiers(uint16_t keycode);
    enum UTF8States
    {
      UTF8_Idle,
      UTF8_2of2,
      UTF8_2of3,
      UTF8_3of3,
      UTF8_2of4,
      UTF8_3of4,
      UTF8_4of4
    } UTF8State;
    uint32_t UnicodeCodepoint;
    uint16_t HexKeycodes[16];
};

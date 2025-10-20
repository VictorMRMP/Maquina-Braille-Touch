// =======================================================================
// Autor: Victor Marcelo Riverete Monteiro Padial
// Descrição desse código: Identifica as combinações Braille e comandos auxiliares e transforma em caracteres correspondentes de acordo com a Grafia Braille para a Lingua Portuguesa
// Microcontrolador: ESP32S3
// Comunicação: Porta USB HID
// Versão: 3.0
// Projeto: "Teclado Touch Braille para auxílio na alfabetização e navegação digital de deficientes visuais sensíveis ao toque e/ou com limitações motoras", realizado para o Trabalho de Conclusão de Curso para o curso de Engenharia Elétrica com ênfase em eletrônica
// =======================================================================

#include <map>
#include <cctype>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "USB.h"
#include "USBHIDKeyboard.h"

// MUDANÇA: Inclui os arquivos da biblioteca ABNT que devem estar no mesmo caminho que a biblioteca USBHIDKeyboard.h, provavelmente em .../ArduinoData/packages/esp32/hardware/esp32/2.0.14/libraries/USB/src
#include "KeyboardUTF8.h"
#include "Keyboard_BR_1.h" // Agora só contém as tabelas

// MUDANÇA: Instancia os DOIS objetos
// 1. O objeto HID de baixo nível 
USBHIDKeyboard Braille_Keyboard;

// 2. O objeto tradutor ABNT, passando o objeto HID para ele.
//    Ele usará as tabelas de 'Keyboard_BR_1.h' e enviará os 
//    comandos através do 'Braille_Keyboard'.
KeyboardUTF8 Braille_ABNT(
    kbdbr_1, sizeof kbdbr_1 / sizeof kbdbr_1[0], 
    kbdbr_1_deadkeys, sizeof kbdbr_1_deadkeys / sizeof kbdbr_1_deadkeys[0],
    Braille_Keyboard // 
);


//Ordem dos bits (i=0 a 5) para corresponder ao mapa: P6, P5, P4, P1, P2, P3
const int touchPins[] = {12, 11, 9, 3, 8, 4}; 

//Ordem dos bits (i=0 a 6) para corresponder ao mapa: Dir, Baixo, Cima, Esq, Esp, Enter, Apagar
const int auxPins[] = {6, 5, 1, 2, 14, 10, 13}; 

const int numTouchPins = 6;
const int numAuxPins = 7;

// Constantes de calibração (sem mudanças)
int baseCapacitanceTouch[numTouchPins]={764,828,1008,756,720,692};
int baseCapacitanceAux[numAuxPins]={596,630,712,673,635,544,644};
const int thresholdTouch = 1000;
int measurement = 10; 
int slep = 10; 

const int letra_maiscula = 0b000101; 
const int numero = 0b100111;

// Mapa principal de Braille para caracteres
std::map<int, uint8_t> braille_char_map = {
    // Letras (sem mudanças)
    {0b001000, 'a'}, {0b011000, 'b'}, {0b001100, 'c'}, {0b001110, 'd'},
    {0b001010, 'e'}, {0b011100, 'f'}, {0b011110, 'g'}, {0b011010, 'h'},
    {0b010100, 'i'}, {0b010110, 'j'}, {0b101000, 'k'}, {0b111000, 'l'},
    {0b101100, 'm'}, {0b101110, 'n'}, {0b101010, 'o'}, {0b111100, 'p'},
    {0b111110, 'q'}, {0b111010, 'r'}, {0b110100, 's'}, {0b110110, 't'},
    {0b101001, 'u'}, {0b111001, 'v'}, {0b010111, 'w'}, {0b101101, 'x'},
    {0b101111, 'y'}, {0b101011, 'z'},

    // Letras acentuadas (sem mudanças)
    {0b111011, 0xE1}, // 'á'
    {0b111111, 0xE9}, // 'é'
    {0b100100, 0xED}, // 'í'
    {0b100101, 0xF3}, // 'ó'
    //{0b110111, 0xFA}, // 'ú' não funcionando
    {0b011101, 0xE0}, // 'à'
    {0b001001, 0xE2}, // 'â'
    {0b011001, 0xEA}, // 'ê'
    {0b001111, 0xF4}, // 'ô'
    {0b100110, 0xE3}, // 'ã'
    {0b010101, 0xF5}, // 'õ'
    {0b111101, 0xE7}, // 'ç'

    // Sinais especiais (sem mudanças - Braille_ABNT vai traduzir)
    {0b010000, ','}, {0b110000, ';'}, {0b010010, ':'}, {0b100000, '.'},
    {0b010011, '/'}, {0b010001, '?'}, {0b100001, '-'}, {0b100010, '*'},
    {0b110001, '"'}, {0b000011, '$'}, {0b110010, '+'}, {0b110011, '='},
    {0b000111, '>'},
    
    // MUDANÇA: Corrigido para ABNT2 usando os códigos de caractere reais
    // A biblioteca Braille_ABNT saberá como digitar isso.
    {0b100011, 0xBA}, // 'º' (código Unicode/Latin-1)
    {0b011011, 0xB4}, // '´' (código Unicode/Latin-1 para acento agudo)

    // Prefixos (sem mudanças)
    {letra_maiscula, 0x16},
    {numero, 0x17}
};

// Mapa para o modo "Número" (sem mudanças)
std::map<int, uint8_t> braille_number_map = {
    {0b001000, '1'}, {0b011000, '2'}, {0b001100, '3'}, {0b001110, '4'},
    {0b001010, '5'}, {0b011100, '6'}, {0b011110, '7'}, {0b011010, '8'},
    {0b010100, '9'}, {0b010110, '0'} 
};

// Mapa para pinos auxiliares (sem mudanças)
std::map<int, int> aux_map = {
    {0b0000001, KEY_RIGHT_ARROW},
    {0b0000010, KEY_DOWN_ARROW}, 
    {0b0000100, KEY_UP_ARROW},   
    {0b0001000, KEY_LEFT_ARROW}, 
    {0b0010000, 0x20},           // Espaço (ASCII)
    {0b0100000, KEY_RETURN},     
    {0b1000000, KEY_BACKSPACE},  
};

bool useShift = false;
bool useNumber = false;

// Variáveis de tempo (sem mudanças)
unsigned long lastReadingTimeBraille = 0;
unsigned long lastReadingTimeAux = 0;

const unsigned long readingDelay = 750;
const unsigned long readingDelayAux = 500;

// Prototipação
void processBraille(void *pvParameters);
void processAux(void *pvParameters);

void setup() {
    Serial.begin(9600);
    delay(100);
    Braille_Keyboard.begin();
    
    USB.begin();
    touchSetCycles(measurement, slep);
    xTaskCreatePinnedToCore(processBraille, "ProcessBraille", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(processAux, "ProcessAux", 4096, NULL, 1, NULL, 1);
}

void loop() {
    // Loop vazio, tarefas executadas pelo RTOS
}

void processBraille(void *pvParameters) {
    while (true) {
        int braille = 0;
        for (int i = 0; i < numTouchPins; i++) {
            int touchValue = touchRead(touchPins[i]);
            if (touchValue - baseCapacitanceTouch[i] > thresholdTouch) {
                braille |= (1 << i);
            }
        }

        if (braille > 0 && (millis() - lastReadingTimeBraille > readingDelay)) {
            lastReadingTimeBraille = millis(); 
            Serial.printf("Braille lido: %d\n", braille);

            if (braille == letra_maiscula) {
                useShift = true;
            } else if (braille == numero) {
                useNumber = true;
            } else {
                uint8_t output = 0;
                if (useNumber) {
                    auto it = braille_number_map.find(braille);
                    if (it != braille_number_map.end()) output = it->second;
                    useNumber = false;
                } else {
                    auto it = braille_char_map.find(braille);
                    if (it != braille_char_map.end()) output = it->second;

                    if (useShift) {
                        if (islower(output)) output = toupper(output);
                        useShift = false;
                    }
                }

                    if (output) {
                    // 1. Envia o caractere ABNT (que pode pressionar e soltar SHIFT)
                    Braille_ABNT.writeUnicode(output);

                    // Força a liberação de TODAS as teclas (incluindo SHIFT)
                    // para garantir que ele não fique "preso".
                    Braille_Keyboard.releaseAll();
                }
            }
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void processAux(void *pvParameters) {
    while (true) {
        int aux = 0;
        for (int i = 0; i < numAuxPins; i++) {
            int touchValue = touchRead(auxPins[i]);
            if (touchValue - baseCapacitanceAux[i] > thresholdTouch) {
                aux |= (1 << i);
            }
        }

        if (aux > 0 && (millis() - lastReadingTimeAux > readingDelayAux)) {
            lastReadingTimeAux = millis(); 

            auto it = aux_map.find(aux);
            if (it != aux_map.end()) {

                Braille_Keyboard.write(it->second);
            }
        }
        
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

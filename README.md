# Teclado Touch Braille (TCC)

Este repositório contém o código-fonte do projeto de Trabalho de Conclusão de Curso (TCC) intitulado: **"Teclado Touch Braille para auxílio na alfabetização e navegação digital de deficientes visuais sensíveis ao toque e/ou com limitações motoras"**.

* **Autor:** Victor Marcelo Riverete Monteiro Padial
* **Microcontrolador:** ESP32-S3
* **Comunicação:** USB HID (Teclado)

## Objetivo do Projeto

O objetivo deste projeto é um teclado Braille totalmente funcional, construído com sensores capacitivos (touch) e um microcontrolador ESP32-S3. O sistema é projetado para ler as combinações de 6 pontos da cela Braille e 7 teclas auxiliares (setas, espaço, enter, apagar), convertendo-as em caracteres digitais.

O principal diferencial deste projeto é o suporte nativo à **Grafia Braille para a Língua Portuguesa**, permitindo a digitação correta de caracteres acentuados (como `á`, `é`, `ç`, `ã`) e símbolos do padrão ABNT.

## Funcionamento

O firmware utiliza o FreeRTOS do ESP32 para dividir o processamento em duas tarefas principais, rodando em núcleos separados:

1.  **`processBraille` (Núcleo 0)**: Esta tarefa é responsável por ler os 6 pinos de toque que compõem a cela Braille. Ela aguarda um tempo (`readingDelay`) para estabilizar a leitura e então processa a combinação. O código gerencia estados especiais, como "letra maiúscula" e "número", e utiliza mapas (`std::map`) para traduzir a combinação Braille no caractere correspondente.
2.  **`processAux` (Núcleo 1)**: Esta tarefa lê os 7 pinos de toque auxiliares. Ela identifica comandos de navegação (Setas), `Espaço`, `Enter` e `Apagar` e os envia diretamente como comandos de teclado.

### Suporte ao Layout ABNT

Para que o teclado enviasse caracteres acentuados do português (ABNT) corretamente, foi necessário modificar a biblioteca USB padrão do ESP32.

O projeto utiliza uma biblioteca `KeyboardUTF8` (baseada na obra de JohnWasser), que foi adaptada para usar o objeto `USBHIDKeyboard` do ESP32. Esta biblioteca utiliza uma tabela de consulta (`Keyboard_BR_1.h`) que mapeia caracteres Unicode (como `0xE1` para `á`) para a sequência correta de teclas ABNT (ex: pressionar "acento agudo", soltar, pressionar "a").

Os arquivos `USBHIDKeyboard.cpp` e `.h` originais do ESP32 foram modificados para suportar este envio no formato ABNT ao invés do US-ASCII padrão.

## Estrutura dos Arquivos

* `MaquinaBrailleTouchFinal.ino`:
    * Código principal do teclado Braille.
    * Contém a lógica de inicialização das tarefas (FreeRTOS), os mapas de caracteres Braille-para-Português, os mapas das teclas auxiliares e a lógica de processamento de toque para ambas as tarefas.
    * Instancia os objetos `USBHIDKeyboard` (nível baixo) e `KeyboardUTF8` (nível alto/ABNT).

* `TesteDeToque.ino`:
    * Um sketch auxiliar de diagnóstico.
    * Ele é usado para calibrar os sensores de toque. O código lê todos os 13 pinos capacitivos e exibe seus valores "base" e a "diferença" (delta) quando tocados, facilitando a definição dos valores de `baseCapacitance` e `thresholdTouch` usados no arquivo `.ino` principal.

* `USBHIDKeyboard.h` e `USBHIDKeyboard.cpp`:
    * **Arquivos de biblioteca modificados.**
    * Substituem os arquivos padrão da biblioteca USB do ESP32.
    * A modificação principal é a remoção do mapa de caracteres US-ASCII padrão.

* `KeyboardUTF8.h` e `KeyboardUTF8.cpp`:
    * **Novos arquivos de biblioteca.**
    * Adaptação da biblioteca `KeyboardUTF8` para ser compatível com a `USBHIDKeyboard` do ESP32.
    * Recebe um caractere Unicode (ex: 'ç') e usa as tabelas de layout para enviá-lo como comandos de teclado.

* `Keyboard_BR_1.h`:
    * **Novo arquivo de biblioteca (Layout).**
    * Contém as tabelas de consulta (lookup tables) para o layout Português-Brasil ABNT.
    * Define o código de tecla (`keycode`) e os modificadores (`MOD_SHIFT`, `MOD_ALTGR`) para cada caractere, incluindo acentos e símbolos.

## Como Usar e Instalar

1.  **Hardware:**
    * Conecte os 13 sensores capacitivos aos pinos do ESP32-S3 conforme definido nos arrays `touchPins` e `auxPins` em `MaquinaBrailleTouchFinal.ino`.

2.  **Calibração (Opcional, mas recomendado):**
    * Carregue o sketch `TesteDeToque.ino` no ESP32-S3.
    * Abra o Monitor Serial (baud 9600).
    * Anote os valores médios dos pinos sem toque (exibidos na calibração inicial). Estes serão seus valores `baseCapacitanceTouch` e `baseCapacitanceAux`.
    * Toque em cada sensor e anote o valor `Δ` (diferença). Escolha um valor seguro para o `thresholdTouch` (ex: `1000`, como no código).
    * Atualize os valores de calibração no início do `MaquinaBrailleTouchFinal.ino`.

3.  **Instalação da Biblioteca ABNT (Crucial):**
    * Encontre o diretório de instalação da biblioteca `USB` do ESP32. O caminho provável é:
        `.../ArduinoData/packages/esp32/hardware/esp32/2.0.14/libraries/USB/src`
    * **Faça backup** dos arquivos `USBHIDKeyboard.h` e `USBHIDKeyboard.cpp` originais.
    * Copie os arquivos `USBHIDKeyboard.h` e `USBHIDKeyboard.cpp` deste repositório para a pasta `src`, substituindo os originais.
    * Copie os arquivos `KeyboardUTF8.h`, `KeyboardUTF8.cpp`, e `Keyboard_BR_1.h` para esta **mesma** pasta `src`.

4.  **Carregar Firmware Principal:**
    * Abra o arquivo `MaquinaBrailleTouchFinal.ino` no seu Arduino IDE.
    * Compile e carregue o código para o ESP32-S3.
    * O dispositivo será reconhecido pelo sistema operacional como um teclado USB padrão e estará pronto para uso.

## Créditos

* A implementação do layout ABNT/UTF-8 é uma adaptação do trabalho original de **JohnWasser** (disponível em [Arduino\_KeyboardUTF8](https://github.com/JohnWasser/Arduino_KeyboardUTF8)).

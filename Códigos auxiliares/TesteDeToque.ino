// =======================================================================
// Autor: Victor Marcelo Riverete Monteiro Padial
// Descrição desse código: Teste dos pinos capacitivos para verificação do toque a partir da mudança de capacitância
// Microcontrolador: ESP32S3
// Comunicação: Porta Serial COM
// Versão: 1.0
// Projeto: "Teclado Touch Braille para auxílio na alfabetização e navegação digital de deficientes visuais sensíveis ao toque e/ou com limitações motoras", realizado para o Trabalho de Conclusão de Curso para o curso de Engenharia Elétrica com ênfase em eletrônica
// =======================================================================

// --- CONFIGURAÇÕES DOS PINOS ---
const int ACTIVE_TOUCH_PINS[] = {
    // Pinos Braille (6)
    //Pino 3 2 1 4 5 6
           4, 8, 3, 9, 11, 12,
    // Pinos Esq Baixo Dir Cima Esp Enter Apagar
              2,   5,    6, 1,   14, 10   , 13
};
const char* PIN_LABELS[] = {
  "B_Ponto3", "B_Ponto2", "B_Ponto1", "B_Ponto4", "B_Ponto5", "B_Ponto6",
  "Esq", "Baixo", "Dir", "Cima", "Esp", "Enter", "Apagar"
};

// --- CONFIGURAÇÕES DO SENSOR DE TOQUE ---
#define TOUCH_MEAS_CYCLE 10   // Ciclo de medição (↑ = mais sensível)
#define TOUCH_SLEEP_CYCLE 10   // Ciclo de descanso
#define PLOT_INTERVAL_MS 3000   // Intervalo entre leituras
#define BASELINE_SMOOTHING 0.9 // Fator de calibração dinâmica (0.0–1.0)
#define TOUCH_THRESHOLD 1000     // Sensibilidade de toque (ajuste se necessário)

const int NUM_ACTIVE_PINS = sizeof(ACTIVE_TOUCH_PINS) / sizeof(ACTIVE_TOUCH_PINS[0]);

// --- VARIÁVEIS GLOBAIS ---
long baselineValues[NUM_ACTIVE_PINS];
unsigned long previousMillis = 0;

// =======================================================================
//   SETUP
// =======================================================================
void setup() {
  Serial.begin(9600);
  delay(1000);

  Serial.println("\n=== Analisador de Resposta Capacitiva (v4.0) ===");
  Serial.println("Configuração de alta sensibilidade e calibração dinâmica.");
  Serial.println("Não toque na placa durante a calibração inicial!\n");

  // Configuração de sensibilidade
  touchSetCycles(TOUCH_MEAS_CYCLE, TOUCH_SLEEP_CYCLE);

  // Calibração inicial
  for (int i = 0; i < NUM_ACTIVE_PINS; i++) {
    baselineValues[i] = touchRead(ACTIVE_TOUCH_PINS[i]);
    delay(30);
  }

  Serial.println(">>> Calibração inicial concluída! <<<");
  Serial.println("Leituras iniciadas...\n");

  // Cabeçalho da tabela
  for (int i = 0; i < NUM_ACTIVE_PINS; i++) {
    Serial.print(PIN_LABELS[i]);
    Serial.print("\t");
  }
  Serial.println();
}

// =======================================================================
//   LOOP
// =======================================================================
void loop() {
  if (millis() - previousMillis >= PLOT_INTERVAL_MS) {
    previousMillis = millis();

    bool plotterMode = false; // Troque para true se quiser usar o Plotter Serial

    if (!plotterMode) Serial.println("\n--- Leitura Atual ---");

    for (int i = 0; i < NUM_ACTIVE_PINS; i++) {
      long currentValue = touchRead(ACTIVE_TOUCH_PINS[i]);
      long diff = baselineValues[i] - currentValue;

      // Calibração dinâmica (mantém baseline atualizado lentamente)
      baselineValues[i] = BASELINE_SMOOTHING * baselineValues[i] + 
                          (1.0 - BASELINE_SMOOTHING) * currentValue;

      if (!plotterMode) {
        Serial.print(PIN_LABELS[i]);
        Serial.print(": ");
        Serial.print(currentValue);
        Serial.print(" (Δ=");
        Serial.print(diff);
        Serial.print(") ");
        if (diff > TOUCH_THRESHOLD) Serial.print("→ TOQUE DETECTADO");
        Serial.println();
      } else {
        Serial.print(PIN_LABELS[i]);
        Serial.print(":");
        Serial.print(diff);
        Serial.print("\t");
      }
    }

    if (plotterMode) Serial.println(); // Quebra de linha para o plotter
  }
}

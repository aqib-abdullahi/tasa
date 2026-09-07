/*
 * TASA — Trigger-Adaptive Sampling Algorithm
 * Stage 0 skeleton: baseline-rate ADC sampling scaffold.
 *
 * Purpose of this stage:
 *   - Confirm timer-driven ADC sampling works on the simulated target.
 *   - Establish r_b (baseline sample rate) as a real, measurable timer period.
 *   - Print samples over serial so we can sanity-check the signal path
 *     before wiring in synthetic transient injection and the trigger FSM.
 *
 * ADC input (PA0) is currently a potentiometer standing in for the
 * voltage-sensing front end. Next step: replace with injected synthetic
 * PQD waveform samples (see /notes/next-steps.md).
 */

#include <Arduino.h>

// ---- Config -----------------------------------------------------------
#define ADC_PIN PA0

// Baseline sample rate r_b (Hz). Placeholder — tune once trigger detector
// latency (delta, see section 3.3) is characterized.
constexpr uint32_t R_B_HZ = 50;
constexpr uint32_t BASELINE_PERIOD_MS = 1000 / R_B_HZ;

// Pre-trigger ring buffer size N_pre (samples). Placeholder.
constexpr size_t N_PRE = 32;

// ---- State --------------------------------------------------------------
uint16_t ring_buffer[N_PRE];
size_t ring_head = 0;
size_t ring_count = 0;
uint32_t last_sample_ms = 0;
uint32_t sample_index = 0;

void ring_push(uint16_t v) {
  ring_buffer[ring_head] = v;
  ring_head = (ring_head + 1) % N_PRE;
  if (ring_count < N_PRE) ring_count++;
}

void setup() {
  Serial.begin(115200);
  pinMode(ADC_PIN, INPUT_ANALOG);
  delay(200);
  Serial.println("TASA stage 0: baseline sampling scaffold");
  Serial.print("r_b = ");
  Serial.print(R_B_HZ);
  Serial.println(" Hz");
}

void loop() {
  uint32_t now = millis();
  if (now - last_sample_ms < BASELINE_PERIOD_MS) {
    return;
  }
  last_sample_ms = now;

  uint16_t v = analogRead(ADC_PIN);   // 0..4095 on STM32 12-bit ADC
  ring_push(v);
  sample_index++;

  // Stage 0 just reports what it read. detect_trigger() and the
  // BASELINE/TRIGGERED FSM get added here in stage 1.
  Serial.print(sample_index);
  Serial.print(",");
  Serial.println(v);
}

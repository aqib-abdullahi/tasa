/*
 * TASA — Trigger-Adaptive Sampling Algorithm
 * Stage 1: synthetic transient waveform injection.
 *
 * Change from Stage 0: analogRead(ADC_PIN) is replaced with a step through
 * a precomputed synthetic PQD waveform (see notes/generate_waveform.py),
 * containing a stable baseline, one sag event, and one surge event,
 * matching IEEE 1159 style definitions. This lets us test the pipeline
 * against known, labeled ground-truth transients before wiring in the
 * trigger detector (Stage 2).
 *
 * The potentiometer is still wired in the diagram but no longer read —
 * safe to leave connected, or remove later once the FSM stages need
 * different hardware.
 */

#include <Arduino.h>
#include "waveform_table.h"   // provides: waveform[], WAVEFORM_LEN

// ---- Config -----------------------------------------------------------
constexpr uint32_t R_B_HZ = 50;
constexpr uint32_t BASELINE_PERIOD_MS = 1000 / R_B_HZ;
constexpr size_t N_PRE = 32;

// ---- State --------------------------------------------------------------
uint16_t ring_buffer[N_PRE];
size_t ring_head = 0;
size_t ring_count = 0;
uint32_t last_sample_ms = 0;
uint32_t sample_index = 0;
size_t waveform_pos = 0;   // where we are in the synthetic waveform

void ring_push(uint16_t v) {
  ring_buffer[ring_head] = v;
  ring_head = (ring_head + 1) % N_PRE;
  if (ring_count < N_PRE) ring_count++;
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("TASA stage 1: synthetic waveform injection");
  Serial.print("r_b = ");
  Serial.print(R_B_HZ);
  Serial.println(" Hz");
  Serial.print("waveform length = ");
  Serial.print(WAVEFORM_LEN);
  Serial.println(" samples");
}

void loop() {
  uint32_t now = millis();
  if (now - last_sample_ms < BASELINE_PERIOD_MS) {
    return;
  }
  last_sample_ms = now;

  // Instead of analogRead(), pull the next value from the synthetic
  // waveform. Loops back to the start once it reaches the end, so the
  // sag/surge cycle repeats every 20 seconds (1000 samples at 50 Hz).
  uint16_t v = waveform[waveform_pos];
  waveform_pos = (waveform_pos + 1) % WAVEFORM_LEN;

  ring_push(v);
  sample_index++;

  Serial.print(sample_index);
  Serial.print(",");
  Serial.println(v);
}

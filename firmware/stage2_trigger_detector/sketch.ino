/*
 * TASA — Trigger-Adaptive Sampling Algorithm
 * Stage 2: trigger detector.
 *
 * Adds detect_trigger() on top of Stage 1's synthetic waveform playback.
 * Two cheap tests, OR'd together (section 4.2):
 *   - slope test:  |v(t) - v(t-1)| > tau_slope        -> catches fast jumps
 *   - RMS test:    |rolling_rms - baseline_rms| > tau_rms -> catches sustained drift
 *
 * This stage only DETECTS and LOGS triggers — it does not yet change the
 * sample rate or manage a capture buffer. That's Stage 3 (the full FSM).
 * Keeping detection separate from state-switching makes each part easy
 * to verify independently.
 */

#include <Arduino.h>
#include "waveform_table.h"   // provides: waveform[], WAVEFORM_LEN

// ---- Config -----------------------------------------------------------
constexpr uint32_t R_B_HZ = 50;
constexpr uint32_t BASELINE_PERIOD_MS = 1000 / R_B_HZ;
constexpr size_t N_PRE = 32;

// Trigger thresholds — placeholders, will be tuned once we see false
// trigger rate / miss rate results in Stage 5.
// Baseline noise in our synthetic signal is roughly +-10 counts (see
// waveform.csv), so tau_slope must sit comfortably above that.
constexpr int16_t TAU_SLOPE = 40;      // counts, single-sample jump
constexpr float TAU_RMS = 60.0;        // counts, deviation from established baseline
constexpr size_t RMS_WINDOW = 10;      // samples used for rolling RMS

// ---- State --------------------------------------------------------------
uint16_t ring_buffer[N_PRE];
size_t ring_head = 0;
size_t ring_count = 0;
uint32_t last_sample_ms = 0;
uint32_t sample_index = 0;
size_t waveform_pos = 0;

uint16_t last_value = 0;
bool have_last_value = false;

float baseline_rms = 0;        // established once, from the first RMS_WINDOW samples
bool baseline_established = false;
uint16_t rms_window_buf[RMS_WINDOW];
size_t rms_window_head = 0;
size_t rms_window_count = 0;

void ring_push(uint16_t v) {
  ring_buffer[ring_head] = v;
  ring_head = (ring_head + 1) % N_PRE;
  if (ring_count < N_PRE) ring_count++;
}

// Rolling RMS over the last RMS_WINDOW samples.
float rolling_rms() {
  if (rms_window_count == 0) return 0;
  float sum_sq = 0;
  for (size_t i = 0; i < rms_window_count; i++) {
    float val = rms_window_buf[i];
    sum_sq += val * val;
  }
  return sqrt(sum_sq / rms_window_count);
}

void rms_window_push(uint16_t v) {
  rms_window_buf[rms_window_head] = v;
  rms_window_head = (rms_window_head + 1) % RMS_WINDOW;
  if (rms_window_count < RMS_WINDOW) rms_window_count++;
}

// Returns true if this sample looks like the start/continuation of a
// transient event, by either test.
bool detect_trigger(uint16_t v) {
  bool slope_trip = false;
  if (have_last_value) {
    int16_t delta = (int16_t)v - (int16_t)last_value;
    slope_trip = abs(delta) > TAU_SLOPE;
  }

  bool rms_trip = false;
  if (baseline_established) {
    float current_rms = rolling_rms();
    rms_trip = fabs(current_rms - baseline_rms) > TAU_RMS;
  }

  return slope_trip || rms_trip;
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("TASA stage 2: trigger detector");
  Serial.print("tau_slope = "); Serial.println(TAU_SLOPE);
  Serial.print("tau_rms = "); Serial.println(TAU_RMS);
  Serial.println("index,value,triggered");
}

void loop() {
  uint32_t now = millis();
  if (now - last_sample_ms < BASELINE_PERIOD_MS) {
    return;
  }
  last_sample_ms = now;

  uint16_t v = waveform[waveform_pos];
  waveform_pos = (waveform_pos + 1) % WAVEFORM_LEN;

  ring_push(v);
  rms_window_push(v);
  sample_index++;

  // Establish the baseline RMS once we have a full window of (presumed
  // stable, since we start at t=0 in "normal" state) samples.
  if (!baseline_established && rms_window_count == RMS_WINDOW) {
    baseline_rms = rolling_rms();
    baseline_established = true;
    Serial.print("# baseline_rms established = ");
    Serial.println(baseline_rms);
  }

  bool triggered = detect_trigger(v);

  last_value = v;
  have_last_value = true;

  Serial.print(sample_index);
  Serial.print(",");
  Serial.print(v);
  Serial.print(",");
  Serial.println(triggered ? 1 : 0);
}

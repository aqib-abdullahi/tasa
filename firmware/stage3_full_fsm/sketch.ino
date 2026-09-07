/*
 * TASA — Trigger-Adaptive Sampling Algorithm
 * Stage 3: full two-state FSM (sections 4.1, 4.3, 4.4).
 *
 * BASELINE state: sample at r_b, run detect_trigger() every tick,
 *   keep a rolling pre-trigger ring buffer.
 * TRIGGERED state: sample at r_f (faster), append to a capture buffer,
 *   apply hysteresis (H consecutive non-triggering samples) before
 *   flushing the whole capture and returning to BASELINE.
 *
 * This is the core novel contribution of the paper: unlike Stage 2
 * (detect only), we now actually change effective sampling rate and
 * manage a bounded capture buffer, which is what produces the memory
 * and power savings claimed in section 4.5 / 6.4.
 */

#include <Arduino.h>
#include "waveform_table.h"   // provides: waveform[], WAVEFORM_LEN

// ---- Config -----------------------------------------------------------
constexpr uint32_t R_B_HZ = 50;                    // baseline rate
constexpr uint32_t R_F_HZ = 200;                   // full/triggered rate (4x baseline)
constexpr uint32_t BASELINE_PERIOD_MS = 1000 / R_B_HZ;
constexpr uint32_t TRIGGERED_PERIOD_MS = 1000 / R_F_HZ;

constexpr size_t N_PRE = 32;     // pre-trigger ring buffer size
constexpr size_t N_POST = 200;   // max post-trigger capture buffer size
constexpr size_t H = 10;         // hysteresis: consecutive clear samples before flush

constexpr int16_t TAU_SLOPE = 40;
constexpr float TAU_RMS = 60.0;
constexpr size_t RMS_WINDOW = 10;

// ---- FSM states -----------------------------------------------------------
enum State { BASELINE, TRIGGERED };
State state = BASELINE;

// ---- Buffers ----------------------------------------------------------
uint16_t ring_buffer[N_PRE];
size_t ring_head = 0;
size_t ring_count = 0;

uint16_t capture_buffer[N_PRE + N_POST];  // pre-trigger context + post-trigger capture
size_t capture_len = 0;

size_t hysteresis_count = 0;

// ---- Detector state -----------------------------------------------------
uint16_t last_value = 0;
bool have_last_value = false;
float baseline_rms = 0;
bool baseline_established = false;
uint16_t rms_window_buf[RMS_WINDOW];
size_t rms_window_head = 0;
size_t rms_window_count = 0;

// ---- Timing / bookkeeping ------------------------------------------------
uint32_t last_sample_ms = 0;
uint32_t sample_index = 0;
size_t waveform_pos = 0;
uint32_t event_count = 0;

void ring_push(uint16_t v) {
  ring_buffer[ring_head] = v;
  ring_head = (ring_head + 1) % N_PRE;
  if (ring_count < N_PRE) ring_count++;
}

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

// Copy the pre-trigger ring buffer (oldest-to-newest order) into the
// start of the capture buffer. This is the "context before the event"
// that a hard real-time trigger alone would otherwise lose.
void drain_ring_into_capture() {
  capture_len = 0;
  size_t start = (ring_head + N_PRE - ring_count) % N_PRE;
  for (size_t i = 0; i < ring_count; i++) {
    capture_buffer[capture_len++] = ring_buffer[(start + i) % N_PRE];
  }
}

void flush_capture() {
  event_count++;
  Serial.print("# EVENT ");
  Serial.print(event_count);
  Serial.print(" flushed, ");
  Serial.print(capture_len);
  Serial.println(" samples captured (pre+post)");
  // In a real deployment this is where capture_buffer[0..capture_len)
  // gets written to flash/SD. For now we just report it.
  capture_len = 0;
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("TASA stage 3: full FSM");
  Serial.print("r_b="); Serial.print(R_B_HZ);
  Serial.print(" r_f="); Serial.print(R_F_HZ);
  Serial.print(" N_pre="); Serial.print(N_PRE);
  Serial.print(" N_post="); Serial.print(N_POST);
  Serial.print(" H="); Serial.println(H);
  Serial.println("index,value,state");
}

void loop() {
  uint32_t now = millis();
  uint32_t period = (state == BASELINE) ? BASELINE_PERIOD_MS : TRIGGERED_PERIOD_MS;
  if (now - last_sample_ms < period) {
    return;
  }
  last_sample_ms = now;

  uint16_t v = waveform[waveform_pos];
  waveform_pos = (waveform_pos + 1) % WAVEFORM_LEN;
  sample_index++;

  rms_window_push(v);
  if (!baseline_established && rms_window_count == RMS_WINDOW) {
    baseline_rms = rolling_rms();
    baseline_established = true;
  }

  bool trig = detect_trigger(v);
  last_value = v;
  have_last_value = true;

  if (state == BASELINE) {
    ring_push(v);
    if (trig) {
      state = TRIGGERED;
      drain_ring_into_capture();   // pull in pre-trigger context
      capture_buffer[capture_len++] = v;  // this sample too
      hysteresis_count = 0;
    }
  } else {  // TRIGGERED
    if (capture_len < N_PRE + N_POST) {
      capture_buffer[capture_len++] = v;
    }
    if (!trig) {
      hysteresis_count++;
    } else {
      hysteresis_count = 0;
    }
    bool buffer_full = capture_len >= N_PRE + N_POST;
    if (hysteresis_count >= H || buffer_full) {
      flush_capture();
      state = BASELINE;
      ring_push(v);  // keep ring buffer warm for the next event
    }
  }

  Serial.print(sample_index);
  Serial.print(",");
  Serial.print(v);
  Serial.print(",");
  Serial.println(state == BASELINE ? "B" : "T");
}

/*
 * TASA core algorithm — standalone C, no framework dependencies.
 * Extracted from main_stage3.cpp for isolated memory footprint
 * measurement (section 6.2). This compiles to real ARM machine code
 * without needing the Arduino/HAL framework, so `size` reports the
 * algorithm's true cost, not framework overhead.
 *
 * To measure: paste into godbolt.org, select an ARM compiler
 * (e.g. "ARM gcc" targeting a Cortex-M profile, or plain
 * "arm-none-eabi-gcc"), add the "size" tool from the tools dropdown.
 */

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <stddef.h>

#define R_B_HZ 50
#define R_F_HZ 200
#define N_PRE 32
#define N_POST 200
#define H 10
#define TAU_SLOPE 40
#define TAU_RMS 60.0f
#define RMS_WINDOW 10

typedef enum { BASELINE, TRIGGERED } State;

typedef struct {
    State state;

    uint16_t ring_buffer[N_PRE];
    size_t ring_head;
    size_t ring_count;

    uint16_t capture_buffer[N_PRE + N_POST];
    size_t capture_len;

    size_t hysteresis_count;

    uint16_t last_value;
    bool have_last_value;

    float baseline_rms;
    bool baseline_established;
    uint16_t rms_window_buf[RMS_WINDOW];
    size_t rms_window_head;
    size_t rms_window_count;
} TasaContext;

static void ring_push(TasaContext *ctx, uint16_t v) {
    ctx->ring_buffer[ctx->ring_head] = v;
    ctx->ring_head = (ctx->ring_head + 1) % N_PRE;
    if (ctx->ring_count < N_PRE) ctx->ring_count++;
}

static float rolling_rms(TasaContext *ctx) {
    if (ctx->rms_window_count == 0) return 0.0f;
    float sum_sq = 0.0f;
    for (size_t i = 0; i < ctx->rms_window_count; i++) {
        float val = (float)ctx->rms_window_buf[i];
        sum_sq += val * val;
    }
    return sqrtf(sum_sq / (float)ctx->rms_window_count);
}

static void rms_window_push(TasaContext *ctx, uint16_t v) {
    ctx->rms_window_buf[ctx->rms_window_head] = v;
    ctx->rms_window_head = (ctx->rms_window_head + 1) % RMS_WINDOW;
    if (ctx->rms_window_count < RMS_WINDOW) ctx->rms_window_count++;
}

static bool detect_trigger(TasaContext *ctx, uint16_t v) {
    bool slope_trip = false;
    if (ctx->have_last_value) {
        int16_t delta = (int16_t)v - (int16_t)ctx->last_value;
        slope_trip = (delta > TAU_SLOPE) || (-delta > TAU_SLOPE);
    }
    bool rms_trip = false;
    if (ctx->baseline_established) {
        float current_rms = rolling_rms(ctx);
        float dev = current_rms - ctx->baseline_rms;
        if (dev < 0) dev = -dev;
        rms_trip = dev > TAU_RMS;
    }
    return slope_trip || rms_trip;
}

static void drain_ring_into_capture(TasaContext *ctx) {
    ctx->capture_len = 0;
    size_t start = (ctx->ring_head + N_PRE - ctx->ring_count) % N_PRE;
    for (size_t i = 0; i < ctx->ring_count; i++) {
        ctx->capture_buffer[ctx->capture_len++] = ctx->ring_buffer[(start + i) % N_PRE];
    }
}

/* One tick of the FSM. Call this once per ADC sample. Returns true if
 * a capture was just flushed (caller would write capture_buffer to
 * storage here in a real deployment). */
bool tasa_tick(TasaContext *ctx, uint16_t v) {
    bool flushed = false;

    rms_window_push(ctx, v);
    if (!ctx->baseline_established && ctx->rms_window_count == RMS_WINDOW) {
        ctx->baseline_rms = rolling_rms(ctx);
        ctx->baseline_established = true;
    }

    bool trig = detect_trigger(ctx, v);
    ctx->last_value = v;
    ctx->have_last_value = true;

    if (ctx->state == BASELINE) {
        ring_push(ctx, v);
        if (trig) {
            ctx->state = TRIGGERED;
            drain_ring_into_capture(ctx);
            ctx->capture_buffer[ctx->capture_len++] = v;
            ctx->hysteresis_count = 0;
        }
    } else {
        if (ctx->capture_len < N_PRE + N_POST) {
            ctx->capture_buffer[ctx->capture_len++] = v;
        }
        if (!trig) {
            ctx->hysteresis_count++;
        } else {
            ctx->hysteresis_count = 0;
        }
        bool buffer_full = ctx->capture_len >= N_PRE + N_POST;
        if (ctx->hysteresis_count >= H || buffer_full) {
            flushed = true;
            ctx->capture_len = 0;
            ctx->state = BASELINE;
            ring_push(ctx, v);
        }
    }
    return flushed;
}

/* Global instance, so `size` sees real allocated .bss/.data — matches
 * how this would actually sit in a deployed firmware image. */
TasaContext g_tasa_ctx;

int main(void) {
    /* Dummy loop so the compiler can't optimize the struct away entirely.
     * Not meant to run for real — this file's only purpose is to be
     * compiled and measured, not executed. */
    uint16_t dummy = 500;
    for (int i = 0; i < 10; i++) {
        tasa_tick(&g_tasa_ctx, dummy);
        dummy++;
    }
    return 0;
}

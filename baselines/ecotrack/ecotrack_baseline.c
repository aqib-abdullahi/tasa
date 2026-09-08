/*
 * EcoTrack baseline — re-implementation for comparison against TASA.
 *
 * Follows the published FSM design (Giordano et al., ENSSys
 * 2023, "Energy-Aware Adaptive Sampling for Self-Sustainability in
 * Resource-Constrained IoT Devices", https://github.com/ETH-PBL/EcoTrack)
 * and the metric-function definition confirmed in the follow-up paper
 * (Cortesi et al. 2026, Eco-WakeLoc, Eq. 1).
 *
 * States: D (decrease, k halves), Z (hold, k unchanged), I (increase, k++)
 * Metric: m = B*(b[t]-b[t-1]) - (1/b[t] - 1) + (inf if b[t]>=gamma else 0)
 * Transitions: m < beta1 -> D,  m > beta2 -> I,  else -> Z
 *
 * IMPORTANT — purpose of this port: EcoTrack was designed to track a
 * SLOWLY-VARYING resource signal (battery state of charge, changing over
 * hours/days). Here we deliberately feed it OUR fast power-quality signal
 * (normalized to [0,1], standing in for "b[t]") to demonstrate quantitatively
 * — not just assert — that a slow-adaptive AIMD scheme is unsuited to
 * millisecond-scale transient capture (paper section 2.3 / 6.1 gap analysis).
 *
 * Published tuning (from the EcoTrack README example): beta1=-0.203,
 * beta2=0.468, gamma=0.67. We keep these as-is for a faithful comparison;
 * only the input signal changes, not the algorithm's own knobs.
 */

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <stddef.h>

typedef enum { STATE_D, STATE_Z, STATE_I } EcoState;

typedef struct {
    struct {
        float beta1;
        float beta2;
        float gamma;
    } parameters;

    float battery_capacity;   // B
    float b_prev;              // b[t-1]
    bool have_prev;

    EcoState state;
    int k;                     // current "sample count" analogue
    int k_min;
    int k_max;
} EcoTrackState;

int fsm_init(EcoTrackState *s, float battery_capacity, float initial_level) {
    if (!s) return -1;
    s->battery_capacity = battery_capacity;
    s->b_prev = initial_level;
    s->have_prev = false;
    s->state = STATE_Z;
    s->k = 1;
    s->k_min = 0;
    s->k_max = 200;   // matched to TASA's r_f, so k is directly comparable
                       // to "samples per baseline tick" in our context
    return 0;
}

/* Core metric function, Eq. (1) in Cortesi et al. */
static float compute_metric(EcoTrackState *s, float b_t) {
    float batt_diff = s->battery_capacity * (b_t - s->b_prev);
    float low_batt_penalty = (1.0f / b_t) - 1.0f;
    float high_batt_reward = (b_t >= s->parameters.gamma) ? INFINITY : 0.0f;
    return batt_diff - low_batt_penalty + high_batt_reward;
}

/* Call once per tick with the current normalized signal level (analogue
 * of battery state of charge, in [0,1]). Returns updated k. */
int update_algorithm(EcoTrackState *s, float b_t) {
    if (b_t < 0.001f) b_t = 0.001f;  // guard against div-by-zero in metric

    if (!s->have_prev) {
        s->b_prev = b_t;
        s->have_prev = true;
        return s->k;
    }

    float m = compute_metric(s, b_t);

    if (m < s->parameters.beta1) {
        s->state = STATE_D;
        s->k = s->k / 2;                 // multiplicative decrease
    } else if (m > s->parameters.beta2) {
        s->state = STATE_I;
        s->k = s->k + 1;                 // additive increase
        if (s->k > s->k_max) s->k = s->k_max;
    } else {
        s->state = STATE_Z;              // hold
    }
    if (s->k < s->k_min) s->k = s->k_min;

    s->b_prev = b_t;
    return s->k;
}

/* Global instance, so `size`/assembly output shows real allocated
 * .bss/.data -- matches how TASA's footprint was measured, for a fair
 * side-by-side comparison. */
EcoTrackState g_eco_state;

/* --- Test harness: feed our synthetic PQD signal through EcoTrack --- */
#include <stdio.h>

int main(void) {
    EcoTrackState *s = &g_eco_state;
    s->parameters.beta1 = -0.203f;
    s->parameters.beta2 = 0.468f;
    s->parameters.gamma = 0.67f;
    fsm_init(s, 11.4f, 1.0f);

    /* Same waveform shape as waveform.csv, inlined here as normalized
     * per-unit voltage (0..1.5 pu scaled to 0..1 range) for a quick
     * standalone demonstration without needing the full CSV. */
    for (int i = 0; i < 1000; i++) {
        float pu;
        if (i >= 300 && i < 350) pu = 0.4f;        // sag
        else if (i >= 650 && i < 680) pu = 1.3f;   // surge
        else pu = 1.0f;
        float b_t = pu / 1.5f;  // normalize to [0,1] like our ADC scaling

        int k = update_algorithm(s, b_t);
        printf("%d,%.3f,%d,%d\n", i, b_t, k, s->state);
    }
    return 0;
}

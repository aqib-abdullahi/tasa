"""
TASA — synthetic PQD (Power Quality Disturbance) waveform generator.

Generates a synthetic RMS-voltage trend line (per IEEE 1159 style events):
  - stable baseline around nominal voltage
  - a SAG event: RMS drops to 0.1-0.9 pu for a short duration
  - a SURGE event: RMS rises above 1.1 pu for a short duration
  - small random noise throughout (real signals are never perfectly flat)

Output: two files
  - waveform.csv          human-readable: index, time_s, pu, adc_value, label
  - waveform_table.h      C header with the array baked in, ready to #include
"""

import numpy as np

# ---- Config -------------------------------------------------------------
SAMPLE_RATE_HZ = 50        # matches r_b in firmware
DURATION_S = 20            # 20 seconds of synthetic data = 1000 samples
NOMINAL_PU = 1.0           # 1.0 = nominal voltage, in "per-unit"
ADC_MAX = 1023             # matches Wokwi's observed 10-bit ADC range
NOISE_STD = 0.01           # small random jitter, realistic sensor noise

rng = np.random.default_rng(seed=42)  # seed = reproducible results

n_samples = SAMPLE_RATE_HZ * DURATION_S
t = np.arange(n_samples) / SAMPLE_RATE_HZ   # time axis, seconds

# ---- Build baseline: nominal voltage + noise -----------------------------
pu = np.full(n_samples, NOMINAL_PU) + rng.normal(0, NOISE_STD, n_samples)

# ---- Inject a SAG event ---------------------------------------------------
# IEEE 1159: sag = 0.1-0.9 pu, duration 0.5 cycle to 1 minute.
# We'll do a sag to 0.4 pu lasting 1 second, starting at t=6s.
sag_start_s, sag_dur_s, sag_level = 6.0, 1.0, 0.4
sag_start_idx = int(sag_start_s * SAMPLE_RATE_HZ)
sag_end_idx = int((sag_start_s + sag_dur_s) * SAMPLE_RATE_HZ)
pu[sag_start_idx:sag_end_idx] = sag_level + rng.normal(0, NOISE_STD, sag_end_idx - sag_start_idx)

# ---- Inject a SURGE event --------------------------------------------------
# IEEE 1159: swell/surge = 1.1-1.8 pu, similar duration range.
# Surge to 1.3 pu lasting 0.6s, starting at t=13s.
surge_start_s, surge_dur_s, surge_level = 13.0, 0.6, 1.3
surge_start_idx = int(surge_start_s * SAMPLE_RATE_HZ)
surge_end_idx = int((surge_start_s + surge_dur_s) * SAMPLE_RATE_HZ)
pu[surge_start_idx:surge_end_idx] = surge_level + rng.normal(0, NOISE_STD, surge_end_idx - surge_start_idx)

# ---- Label each sample (ground truth, for later accuracy scoring) --------
label = np.array(["normal"] * n_samples, dtype=object)
label[sag_start_idx:sag_end_idx] = "sag"
label[surge_start_idx:surge_end_idx] = "surge"

# ---- Convert per-unit voltage to a simulated ADC reading ------------------
# Scale so nominal (1.0 pu) sits comfortably mid-range, headroom for surges.
adc = np.clip((pu / 1.5) * ADC_MAX, 0, ADC_MAX).astype(int)

# ---- Write CSV (for inspection / later accuracy scoring) ------------------
with open("notes/waveform.csv", "w") as f:
    f.write("index,time_s,pu,adc_value,label\n")
    for i in range(n_samples):
        f.write(f"{i},{t[i]:.2f},{pu[i]:.4f},{adc[i]},{label[i]}\n")

# ---- Write C header with the lookup table ----------------------------------
with open("notes/waveform_table.h", "w") as f:
    f.write("// Auto-generated synthetic PQD waveform. See notes/generate_waveform.py\n")
    f.write(f"// {n_samples} samples at {SAMPLE_RATE_HZ} Hz = {DURATION_S} s. Values are simulated ADC counts (0-{ADC_MAX}).\n")
    f.write("#pragma once\n")
    f.write(f"constexpr size_t WAVEFORM_LEN = {n_samples};\n")
    f.write("const uint16_t waveform[WAVEFORM_LEN] = {\n  ")
    for i, val in enumerate(adc):
        f.write(f"{val}, ")
        if (i + 1) % 20 == 0:
            f.write("\n  ")
    f.write("\n};\n")

print(f"Generated {n_samples} samples.")
print(f"Sag: idx {sag_start_idx}-{sag_end_idx} (t={sag_start_s}s-{sag_start_s+sag_dur_s}s)")
print(f"Surge: idx {surge_start_idx}-{surge_end_idx} (t={surge_start_s}s-{surge_start_s+surge_dur_s}s)")
print("Wrote notes/waveform.csv and notes/waveform_table.h")

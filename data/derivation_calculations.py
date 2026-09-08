"""
Reproduces the RAM and sampling-rate figures directly from raw data.
Run: python3 derivation_calculations.py
No dependencies beyond the Python standard library.
"""

# ============================================================
# PART 1: RAM footprint (588 bytes) — struct field breakdown
# Cross-check against tasa_core_struct_assembly.txt line:
#   g_tasa_ctx: .space 588
# ============================================================

fields = [
    ("state (enum)",                  4),
    ("ring_buffer[32] uint16_t",      64),
    ("ring_head (size_t)",             4),
    ("ring_count (size_t)",            4),
    ("capture_buffer[232] uint16_t", 464),  # N_pre(32) + N_post(200)
    ("capture_len (size_t)",           4),
    ("hysteresis_count (size_t)",      4),
    ("last_value (uint16_t)",          2),
    ("have_last_value (bool)",         1),
    ("baseline_rms (float)",           4),
    ("baseline_established (bool)",    1),
    ("rms_window_buf[10] uint16_t",   20),
    ("rms_window_head (size_t)",       4),
    ("rms_window_count (size_t)",      4),
]

print("=" * 60)
print("PART 1: RAM footprint breakdown")
print("=" * 60)
raw_sum = sum(size for _, size in fields)
for name, size in fields:
    print(f"  {name:32s} {size:4d} bytes")
print(f"  {'RAW SUM':32s} {raw_sum:4d} bytes")
print(f"  {'Compiler-reported (.space)':32s} {588:4d} bytes")
print(f"  {'Alignment/padding overhead':32s} {588 - raw_sum:4d} bytes")

# Sampling rate — both methods, from raw log data
# Source: RAW_stage3_firmware_log.txt
#   Sag triggered window:   index 301 to 366 inclusive
#   Surge triggered window: index 651 to 696 inclusive
#   Total waveform cycle:   1000 samples

print()
print("=" * 60)
print("PART 2: Sampling rate — two candidate methods")
print("=" * 60)

sag_start, sag_end = 301, 366
surge_start, surge_end = 651, 696

sag_samples = sag_end - sag_start + 1
surge_samples = surge_end - surge_start + 1
triggered_samples = sag_samples + surge_samples
total_samples = 1000
baseline_samples = total_samples - triggered_samples

r_b, r_f = 50, 200  # Hz, from firmware config

print(f"Sag triggered samples:    {sag_samples} (index {sag_start}-{sag_end})")
print(f"Surge triggered samples:  {surge_samples} (index {surge_start}-{surge_end})")
print(f"Total triggered samples:  {triggered_samples} / {total_samples}")
print(f"Total baseline samples:   {baseline_samples} / {total_samples}")

print()
print(" TIME-weighted ")
baseline_period_ms = 1000 / r_b
triggered_period_ms = 1000 / r_f
baseline_time_ms = baseline_samples * baseline_period_ms
triggered_time_ms = triggered_samples * triggered_period_ms
total_time_ms = baseline_time_ms + triggered_time_ms
p_trig_time = triggered_time_ms / total_time_ms
r_avg_2 = total_samples / (total_time_ms / 1000)

print(f"Baseline time:  {baseline_samples} x {baseline_period_ms}ms = {baseline_time_ms:.0f} ms")
print(f"Triggered time: {triggered_samples} x {triggered_period_ms}ms = {triggered_time_ms:.0f} ms")
print(f"Total elapsed:  {total_time_ms:.0f} ms = {total_time_ms/1000:.3f} s")
print(f"p_trig (time fraction) = {triggered_time_ms:.0f}/{total_time_ms:.0f} = {p_trig_time:.4f}")
print(f"r_avg  = {total_samples} samples / {total_time_ms/1000:.3f} s = {r_avg_2:.2f} Hz")
print(f"Reduction vs 200 Hz: {(1 - r_avg_2/200)*100:.1f}%")

print()
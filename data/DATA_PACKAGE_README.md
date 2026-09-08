# TASA Data Package — Source Data Behind Reported Figures

This package contains the raw, unprocessed data behind two reported
numbers: the 588-byte static RAM footprint and the 66.8 Hz average
sampling rate (currently under review — see note below).

## Files in this package

### 1. RAW_stage3_firmware_log.txt
The actual serial output from the Stage 3 TASA firmware, running in
the Wokwi STM32F103 simulator. Format: `sample_index,adc_value,state`
where state is B (BASELINE) or T (TRIGGERED). Lines starting with `#`
are firmware-printed event flush notifications.

This is the PRIMARY SOURCE for the sampling-rate calculation. The two
load-bearing facts extracted from it:
  - Sag event: TRIGGERED from index 301 to 366 inclusive (66 samples)
  - Surge event: TRIGGERED from index 651 to 696 inclusive (46 samples)

### 2. waveform.csv
The ground-truth synthetic test signal (1000 samples, 50 Hz, 20 s),
with per-sample labels (normal/sag/surge). This is what was fed into
the firmware's ADC read path. Independent of the RAM/rate figures
below, but is the source of truth for recall/precision calculations
elsewhere in the paper.

### 3. ecotrack_output.csv
Output of the standalone EcoTrack re-implementation (compiled with
gcc, run outside the firmware simulator) against the same signal.
Format: `sample_index,normalized_input,k,state`. Source for the
EcoTrack collapse-to-zero claim.

### 4. tasa_core_struct_assembly.txt
Raw ARM assembly output from Compiler Explorer (godbolt.org), compiling
the isolated TasaContext struct with `-mcpu=cortex-m3 -mthumb -Os`.
Contains the line `g_tasa_ctx: .space 588`, the PRIMARY SOURCE for the
588-byte RAM figure. This is real compiler output, not a hand estimate.

### 5. derivation_calculations.py
Runnable Python script reproducing both the RAM breakdown and BOTH
candidate sampling-rate calculations (see note below) directly from
the raw data in files 1-4. Run it yourself to verify independently.

- **time-correct: 54.6 Hz.** Weights r_b and r_f by the
  FRACTION OF ELAPSED TIME spent triggered, accounting for the fact
  that triggered samples are taken 4x faster (5 ms each) than baseline
  samples (20 ms each), so 112 triggered samples represent much less
  real time than 112 baseline samples would.

The paper's own formula (r_avg = r_b(1-p_trig) + r_f*p_trig) is only a
correct time-average if p_trig is a TIME fraction. 
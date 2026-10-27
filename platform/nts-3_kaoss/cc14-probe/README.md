# CC14 Probe — NTS-3 diagnostic (experimental)

This diagnostic is not a Shermanish effect. It checks whether paired MSB/LSB MIDI CC values survive host mapping into a custom effect parameter.

## Setup
1. Build/install cc14-probe. Disable all other effects and start at very low speaker volume. This effect generates a deliberately quiet triangle tone (no sax input required).
2. In the NTS-3 editor verify that X RAW, Y RAW, D RAW have linear full-range 0..16383 mappings for X, Y and DEPTH respectively. Mapping is essential; the MIDI hardware may quantize the mapped values.
3. Set PAIR=0 (X/CC16+48), FINE/COARSE=0 (fine). Keep the coarse/MSB pot still and sweep its paired LSB pot. A smoothly changing pitch across ~110–1126 Hz suggests many fine steps survived. If pitch jumps in only a few large steps, the host has quantized.
4. Repeat PAIR=1 for CC17+49 and PAIR=2 for CC18+50. Set FINE/COARSE=1 to verify MSB separately.
5. Read the current raw parameter values in your Android editor if available. Note whether increments are 1, 8, 16, 128, or something else. If all 128 fine steps survive, decoding six independent controls is feasible.

**Important:** Some NTS-3 firmware may remap 14-bit CC to a lower resolution, or interpret the CC LSB only as fine adjustment. An audible change alone does not establish 128 independent steps. Do not assume this diagnostic can intercept MIDI: it only receives host-mapped effect parameters.

No changes to Sensai7 firmware are required for this experiment. NTS-3 output level is intentionally low, but begin with speakers quiet.

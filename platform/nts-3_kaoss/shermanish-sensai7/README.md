# Shermanish S7 — Sensai7 six-pot edition

**A second, separately installable Shermanish effect**, sharing the same two resonant filters, feedback, stereo detuning, and dual envelope DSP with the [naked NTS-3 edition](../shermanish/README.md). No Sensai7 firmware change is required.

The NTS-3 appears to retain only **10 of the 14 MIDI CC bits**. Thus each CC pair carries one 128-position coarse control and one **8-position fine control**. The eight fine destinations are interpolated smoothly in the DSP. The effect does not receive raw MIDI messages.

## Sensai7 physical pots

Select the matching effect page on the Sensai7 (FX1 shown; other pages use their corresponding CCs). Keep the NTS-3's mapping curves **linear**, not logarithmic.

| Physical pot | FX1 MIDI CC | Decoded Shermanish control | Resolution |
|---|---|---|---|
| 1 | 16 (X MSB) | Main filter frequency | 128 positions |
| 2 | 48 (X LSB) | Filter interval | 8 positions, ~110 ms glide |
| 3 | 17 (Y MSB) | Resonance / feedback | 128 positions |
| 4 | 49 (Y LSB) | Input drive / saturation | 8 positions, ~52 ms smoothing |
| 5 | 18 (DEPTH MSB) | Follower ↔ triggered ADSR morph | 128 positions |
| 6 | 50 (DEPTH LSB) | Envelope modulation amount | 8 positions, ~74 ms smoothing |

The interval steps are **−24, −12, −7, 0, +5, +7, +12, +24 semitones** (before interval shift). Drive and envelope amount use eight increasing levels; these are **target** levels, with smooth movement between them.

## NTS-3 parameters and mappings

The first three SDK parameters are packed transport channels, not independent 0–1000 parameters:

| NTS-3 parameter | Mapping | Meaning |
|---|---|---|
| X FREQ+INTERVAL | X, linear, 0–16383 | Coarse frequency + fine interval |
| Y REGEN+DRIVE | Y, linear, 0–16383 | Coarse resonance + fine drive |
| D MORPH+ENV | DEPTH, linear, 0–16383 | Coarse morph + fine envelope amount |
| INTERVAL SHIFT | none | Fine interval transpose, −12 to +12 semitones |
| DRIVE MAX | none | Scale the 8-step drive range, default 100% |
| ENV MAX | none | Scale the 8-step envelope range, default 100% |
| ENV TIME | none | Envelope timing and tempo divisions (0–9) |
| ROUTING | none | Parallel / serial / cross-coupled (0–2) |

The host maps the incoming 14-bit CC pair onto a 10-bit position. The effect then rounds the SDK's 0–16383 parameter back to 0–1023, and splits it into **7 coarse bits + 3 fine bits**. When the NTS-3 requests a parameter string, it displays e.g. **C064 F3**.

**Important:** Use the NTS-3's X/Y/DEPTH assignments and linear curves as above. Do not assign the packed parameters to a logarithmic curve; this would destroy the bit separation. The normal XY pad will also modify the packed controls, so use the [naked version](../shermanish/README.md) when performing without Sensai7.

## Test and listening

1. Install the S7 build as a user effect, assign it to one FX slot, and select that slot's page on Sensai7.
2. Start with low monitor volume and one effect enabled. Confirm that pots 1, 3, 5 sweep frequency, resonance and envelope morph.
3. Hold pot 1 stationary and sweep pot 2: you should hear **eight smoothly gliding filter intervals**. Repeat pot 4 (drive) and pot 6 (envelope amount) while holding their MSB partners.
4. Turn an MSB pot while holding its LSB partner. The corresponding fine target should remain unchanged; check this before relying on the effect live.
5. If you hear wrong jumps, use the Android editor to inspect packed values. Quantization, custom curves or host rounding can make some boundary values ambiguous; the 10-bit packing is based on your observed eight LSB steps and still requires on-device verification.

The output is **fully wet**; mix dry sax and loop channels on the Zoom L6max. Never feed the NTS-3 return back into the mixer's SUB OUT. Strong feedback can create loud self-resonance; test quietly first.

## Build and test

\`\`\`sh
g++ -std=c++11 -O2 -Iplatform/nts-3_kaoss/common platform/nts-3_kaoss/shermanish-sensai7/test_host.cc -o /tmp/shermanish-s7-test
/tmp/shermanish-s7-test
./docker/run_cmd.sh build nts-3_kaoss/shermanish-sensai7
\`\`\`

Output: \`platform/nts-3_kaoss/shermanish-sensai7/shermanish_s7.nts3unit\`.

The GitHub Actions workflow builds both the naked and Sensai7 editions as separate downloadable artifacts.

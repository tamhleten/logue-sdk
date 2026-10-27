# Shermanish — naked NTS-3 version

This is the standalone Sherman Filterbank-inspired dual resonant bandpass effect for the **NTS-3 alone**, with no Sensai7 board required. It has a separate unit ID from the Sensai7 edition, so both can be installed and compared.

## Controls

| Control | Parameter | Behaviour |
|---|---|---|
| Pad X | FREQ | Main resonant filter frequency |
| Pad Y | REGEN | Resonance and saturated feedback |
| FX DEPTH | ENV MORPH | 0 = audio envelope follower, 1000 = triggered ADSR; smooth blend |
| Extra 1 | DRIVE | Input saturation |
| Extra 2 | INTERVAL | Second filter offset (−24 to +24 semitones) |
| Extra 3 | ENV AMT | Filter modulation strength |
| Extra 4 | ENV TIME | Envelope timing / clock division (0–9) |
| Extra 5 | ROUTING | 0 parallel, 1 serial, 2 cross-coupled |

The audio follower and triggered ADSR keep running in parallel; the morph chooses their mix. The effect output is wet; mix dry sax and loops on the L6max. The two filters use slightly opposed stereo detuning.

For the controller edition with three 7-bit coarse controls and three smoothed 3-bit fine controls, see `../shermanish-sensai7/README.md`.

## Signal chain

Sax microphone → L6max. Looper and echo returns → L6max. Mix independently at SUB OUT → NTS-3 → L6max return → speakers. **Keep the NTS-3 return out of SUB OUT** to prevent unwanted mixer feedback.

## Build

`./docker/run_cmd.sh build nts-3_kaoss/shermanish`

Output: `platform/nts-3_kaoss/shermanish/shermanish.nts3unit`.

The two versions use the same DSP source, but the Sensai7 version is compiled with longer smoothing times for its stepped fine controls. **Begin at low speaker volume** when testing high resonance.

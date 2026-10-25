# Shermanish v0.1 for NTS-3

Sherman Filterbank-inspired (not a circuit clone) dual bandpass resonator with saturation, harmonic interval tuning, controlled local feedback, envelope following, and slow wobble.

## Intended L6max routing
Sax microphone into L6max. AUX 1 to external looper and reverse/echo, returned to an L6max channel. Independently mix dry sax and processed looper return to stereo SUB OUT feeding NTS-3. Return NTS-3 to L6max, **excluded from SUB mix**. Mono source is fine; the resonators add stereo movement.

## Parameters
- X / FREQ: exponential frequency sweep, roughly 55 Hz to 2.5 kHz.
- Y / INTERVAL: -24 to +24 semitones, continuous, filter 2 relative to filter 1.
- Depth / REGEN: resonance plus local feedback (start low).
- DRIVE: input distortion.
- ROUTING: 0 parallel; 1 serial; 2 cross-coupled.
- ENV AMT: input envelope opens cutoff.
- WOBBLE: 0.7 Hz slow frequency movement.
- DRY WET: true dry/wet, defaults fully wet.

## Build
Run `./docker/run_cmd.sh build nts-3_kaoss/shermanish` or use the repository's GitHub Actions workflow.
Artifact: `platform/nts-3_kaoss/shermanish/shermanish.nts3unit`.

## First test
FX1 alone. Fully wet, X around 360, Y around zero, Depth at 300-450, DRIVE near default, ROUTING=0. Use a bari loop to excite the filters; slowly increase Depth and play X and Y. Keep monitoring level very low initially: high resonance can produce prolonged, loud pitched tones.

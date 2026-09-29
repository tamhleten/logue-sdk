# Scrap Metal

A hands-free, bari-sax-oriented NTS-3 effect for anti-melodic metallic textures.
It runs a dynamically crushed branch in parallel with a Hann-window pitch shifter,
then applies ring modulation and an attack-retriggered low-pass gate.

## Controls

- **X / Ring Freq:** 60-600 Hz; default 170 Hz.
- **Y / Destruction:** increases sample-rate reduction, bit reduction, ring depth,
  and the effect of the input envelope; default 0.750.
- **Depth / Dry Wet:** true dry/wet balance; default fully wet.
- **Pitch:** -24 to 0 semitones; default -12 for baritone sax.
- **Sensitivity:** attack detector threshold; default 0.035.
- **Gate Rate:** 2-20 steps per second; default 8.
- **Pattern:** four eight-step mechanical gate patterns.
- **Pitch Mix:** balance between crushed and pitch-shifted branches; default 0.550.

The envelope follower has fast attack and slower release. Harder playing makes the
crusher coarser and raises the ring-modulator frequency. Each new sax attack
restarts the gate at step one, so no touch-pad gesture is needed while playing.

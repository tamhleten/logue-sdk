# NTS-3 Ricochet

A tempo-linked bouncing-ball delay for short, accelerating echo gestures.
Ricochet captures the attack of a note and plays a finite series of progressively
closer and quieter repetitions, ending on the selected beat boundary.

## Arming and triggering

- Tap the pad and release it to arm one shot. The next qualifying attack fires
  one ricochet and disarms the effect.
- Keep a finger on the pad to remain armed. A later attack can fire another
  ricochet after the signal has dropped below the trigger reset level.
- NTS-3 Hold or per-effect Freeze keeps that continuous armed state active after
  the finger is removed. Release Hold/Freeze to return to one-shot operation.
- Merely arming the effect does not capture audio. Capture begins at the next
  detected attack.

## Controls

- X — total gesture length, 0.25 to 2.00 beats; default 1.00 beat
- Y — interval ratio, 0.500 to 0.900; lower values accelerate more violently
- Depth — true dry/wet crossfade: `D100` dry, `BALN` 50/50, `W100` wet
- Decay — amplitude retained by each successive bounce; default 0.680
- Bounces — 4 to 12; default 8
- Threshold — attack threshold from -48 to -12 dBFS; default -28 dBFS
- Grain ms — captured attack length from 20 to 100 ms; default 55 ms

All continuous controls are smoothed. Gesture timing follows the NTS-3 tempo;
at 120 BPM, the default one-beat ricochet lasts 500 ms.

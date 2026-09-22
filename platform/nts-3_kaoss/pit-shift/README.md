# NTS-3 Pit Shift

Native stereo delay-line pitch shifter for the NTS-3 kaoss pad kit. It follows
the STK-style algorithm described in Ben Roth's “Implementing a Pitch Shifter
in SuperCollider”: two linearly interpolated delay taps are half a window apart.

The implementation keeps the 5024-sample delay but uses complementary Hann
windows linked to the delay-sweep phase. Each tap therefore reaches zero gain
at its delay-wrap point, giving smoother transitions than the article's
independently cycling triangular envelope.

Controls:

- X — centered pitch shift: left runs from 0.25x to 1.00x, right runs from
  1.00x to 8.00x
- Y — window/grain length from 1024 samples (about 21 ms) at the bottom to
  5024 samples (about 105 ms) at the top
- Depth — dry/wet crossfade from dry at 0.000 to fully wet at 1.000

X uses an exponential pitch mapping on each side of center, so octave intervals
receive even pad travel and unison is exactly at the physical midpoint.
Shorter Y settings give tighter transients and a rougher, more granular sound;
longer settings are smoother and more diffuse. The default remains the original
5024-sample window.

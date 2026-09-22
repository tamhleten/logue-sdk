# NTS-3 Pit Shift

Native stereo delay-line pitch shifter for the NTS-3 kaoss pad kit. It follows
the STK-style algorithm described in Ben Roth's “Implementing a Pitch Shifter
in SuperCollider”: two linearly interpolated delay taps are half a window apart.

The implementation keeps the 5024-sample delay but uses complementary Hann
windows linked to the delay-sweep phase. Each tap therefore reaches zero gain
at its delay-wrap point, giving smoother transitions than the article's
independently cycling triangular envelope.

Controls:

- X — `shift` ratio from 0.25x to 8.00x (1.00x is the original pitch)
- Y — unused
- Depth — wet output level from 0.000 to 1.000

The unit outputs only the processed signal. The NTS-3 generic-effect routing
provides the dry path.

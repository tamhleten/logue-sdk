# NTS-3 Dub Echo

Native stereo port of the supplied SuperCollider `LocalIn`/`LocalOut` dub echo.
The processed output is filtered at 400 Hz and 5 kHz, softly saturated, delayed
with complementary wandering stereo times, channel-swapped, and returned to the
feedback path.

Controls:

- X — main delay length from 40 to 2000 ms, exponentially mapped
- Y — complementary stereo separation from 0.0 to 30.0 ms, exponentially mapped
- Depth — true linear dry/wet crossfade: `D100` is dry only, `BALN` is
  50/50, and `W100` is wet only
- Feedback — 0.000 to 0.980, available as an additional parameter; default 0.700
- Mod Rate — 0.10 to 30.00 Hz, available as an additional parameter; default
  12.00 Hz, matching the SuperCollider patch

Defaults reproduce the example: 750 ms length, 0.700 feedback, 1.2 ms
separation, and 12 Hz modulation. Delay, separation, dry/wet, feedback, and
modulation-rate changes are smoothed. Moving X therefore produces a short
tape-like glide instead of a discontinuity or click.

# NTS-3 Afterimage

Hands-free phrase-shadow effect for collective improvisation. It continuously
listens to the stereo input, captures the end of a played phrase, and answers
once the player leaves a short gap. The shadow ducks when live playing resumes,
so it occupies the spaces rather than covering the next phrase.

Controls:

- X — shadow pitch from -12 to +12 semitones; default +7 semitones
- Y — maximum captured phrase length from 250 to 3000 ms; default 1500 ms
- Depth — additive shadow level; the dry sax always remains present
- Touch/footswitch while a shadow is sounding — toggle hold; the captured
  shadow loops until hold is switched off. Touches made before the first
  afterimage are ignored, so setting X and Y does not accidentally enable it.

The detector uses a slowly adapting noise floor, hysteresis, a 200 ms phrase-end
gap, and a 150 ms minimum phrase length. A four-second stereo rolling buffer is
stored in SDRAM. Playback is faded, pitch-shifted with complementary Hann
windows, and automatically ducked by new live input.

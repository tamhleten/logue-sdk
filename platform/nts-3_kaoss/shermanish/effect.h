#pragma once

#include <math.h>
#include <stdint.h>
#include "processor.h"

// Sherman-inspired dual resonator. Not a circuit model of the analogue unit.
// Zero external RAM: four state-variable filters and bounded local regeneration.
class Effect : public Processor {
public:
  static constexpr float SAMPLE_RATE = 48000.f;
  static constexpr float PI = 3.14159265358979323846f;
  enum { PITCH = 0, INTERVAL, REGEN, DRIVE, ROUTING, ENVELOPE,
         WOBBLE, DRY_WET, NUM_PARAMS };

  struct Params {
    float pitch, semitones, regen, drive, env, wobble, mix;
    int route;
    Params() : pitch(360.f), semitones(0.f), regen(0.40f), drive(2.4f),
               env(0.f), wobble(0.f), mix(1.f), route(0) {}
  };

  uint32_t getBufferSize() const override final { return 0U; }
  void init(float *) override final { params = Params(); reset(); }
  void teardown() override final {}
  void reset() override final {
    for (int ch = 0; ch < 2; ++ch) {
      for (int j = 0; j < 2; ++j) filters[ch][j] = Svf();
    }
    envelope = 0.f;
    lfo_phase = 0.f;
    smooth_pitch = params.pitch;
    smooth_semitones = params.semitones;
    smooth_regen = params.regen;
    smooth_drive = params.drive;
    smooth_env = params.env;
    smooth_wobble = params.wobble;
    smooth_mix = params.mix;
  }
  void setParameter(uint8_t index, int32_t value) override final {
    switch (index) {
      case PITCH: params.pitch = static_cast<float>(value); break;
      case INTERVAL: params.semitones = static_cast<float>(value) * .1f; break;
      case REGEN: params.regen = static_cast<float>(value) * .001f; break;
      case DRIVE: params.drive = 1.f + static_cast<float>(value) * .019f; break;
      case ROUTING: params.route = value; break;
      case ENVELOPE: params.env = static_cast<float>(value) * .001f; break;
      case WOBBLE: params.wobble = static_cast<float>(value) * .001f; break;
      case DRY_WET: params.mix = (static_cast<float>(value) + 1000.f) * .0005f; break;
      default: break;
    }
  }

  void process(const float *__restrict in, float *__restrict out,
               uint32_t frames) override final {
    // Calculate transcendental functions once per buffer, never per sample.
    // Coefficients are interpolated per sample to avoid zipper clicks.
    smooth_pitch += .11f * (params.pitch - smooth_pitch);
    smooth_semitones += .11f * (params.semitones - smooth_semitones);
    smooth_env += .11f * (params.env - smooth_env);
    smooth_wobble += .11f * (params.wobble - smooth_wobble);
    const float hz1 = 55.f * exp2f(smooth_pitch * (5.50635267f / 1000.f));
    const float env_octaves = smooth_env * envelope * 1.3f;
    const float wobble_octaves = smooth_wobble * .16f * sinf(lfo_phase);
    const float octave_offset = env_octaves + wobble_octaves;
    const float hz2 = hz1 * exp2f(smooth_semitones / 12.f);
    for (int ch = 0; ch < 2; ++ch) {
      // When fed mono, deliberate mild stereo detuning creates moving beats.
      const float stereo_offset = ch == 0 ? .985f : 1.015f;
      const float modulation = exp2f(octave_offset);
      filters[ch][0].targetG(hz1 * modulation * stereo_offset);
      filters[ch][1].targetG(hz2 * modulation / stereo_offset);
    }
    lfo_phase += 2.f * PI * .7f * static_cast<float>(frames) / SAMPLE_RATE;
    if (lfo_phase >= 2.f * PI) lfo_phase -= 2.f * PI;

    const float target_regen = params.regen;
    const float target_drive = params.drive;
    const float target_mix = params.mix;
    const int route = params.route;
    for (uint32_t i = 0; i < frames; ++i) {
      smooth_regen += .0008f * (target_regen - smooth_regen);
      smooth_drive += .0008f * (target_drive - smooth_drive);
      smooth_mix += .0008f * (target_mix - smooth_mix);
      const float k = 0.62f - .600f * smooth_regen;
      // At the highest positions positive feedback can sustain a resonance;
      // inside tanh is bounded and the filters have finite bandwidth.
      const float feedback = .02f + .74f * smooth_regen * smooth_regen;
      for (int ch = 0; ch < 2; ++ch) {
        Svf &a = filters[ch][0];
        Svf &b = filters[ch][1];
        const float src = in[2U * i + static_cast<uint32_t>(ch)];
        const float magnitude = fabsf(src);
        const float attack = magnitude > envelope ? .014f : .00028f;
        envelope += attack * (magnitude - envelope);
        a.moveG(); b.moveG();
        const float driven = softclip(smooth_drive * src);
        const float previous_a = a.last;
        const float previous_b = b.last;
        const float cross = route == 2 ? .26f : 0.f;
        const float excitation_a = driven + feedback * softclip(2.f * previous_a)
                                   + cross * softclip(2.f * previous_b);
        const float band_a = a.tick(softclip(excitation_a), k);
        const float excitation_b = (route == 1 ? softclip(2.f * band_a) : driven)
                                   + feedback * softclip(2.f * previous_b)
                                   + cross * softclip(2.f * previous_a);
        const float band_b = b.tick(softclip(excitation_b), k);
        const float combined = route == 1 ? band_b : .58f * (band_a + band_b);
        const float wet = .72f * softclip(2.5f * combined);
        out[2U * i + static_cast<uint32_t>(ch)] =
            (1.f - smooth_mix) * src + smooth_mix * wet;
      }
    }
  }

private:
  static float softclip(float x) {
    // tanh-like, monotonic and bounded. No branches or costly expf in DSP loop.
    x = x > 3.f ? 3.f : (x < -3.f ? -3.f : x);
    const float x2 = x * x;
    return x * (27.f + x2) / (27.f + 9.f * x2);
  }
  struct Svf {
    float ic1 = 0.f, ic2 = 0.f, last = 0.f;
    float g = .015f, g_target = .015f;
    void targetG(float hz) {
      if (hz < 28.f) hz = 28.f;
      if (hz > 5800.f) hz = 5800.f;
      g_target = tanf(PI * hz / SAMPLE_RATE);
    }
    void moveG() { g += .0015f * (g_target - g); }
    float tick(float input, float k) {
      const float a1 = 1.f / (1.f + g * (g + k));
      const float a2 = g * a1;
      const float a3 = g * a2;
      const float v3 = input - ic2;
      const float v1 = a1 * ic1 + a2 * v3;
      const float v2 = ic2 + a2 * ic1 + a3 * v3;
      ic1 = 2.f * v1 - ic1;
      ic2 = 2.f * v2 - ic2;
      // Bound internal state under unexpected hot input and high feedback.
      if (ic1 > 16.f) ic1 = 16.f;
      if (ic1 < -16.f) ic1 = -16.f;
      if (ic2 > 16.f) ic2 = 16.f;
      if (ic2 < -16.f) ic2 = -16.f;
      last = v1;
      return v1;
    }
  };
  Svf filters[2][2];
  Params params;
  float envelope = 0.f, lfo_phase = 0.f;
  float smooth_pitch = 360.f, smooth_semitones = 0.f;
  float smooth_regen = .4f, smooth_drive = 2.4f;
  float smooth_env = 0.f, smooth_wobble = 0.f, smooth_mix = 1.f;
};

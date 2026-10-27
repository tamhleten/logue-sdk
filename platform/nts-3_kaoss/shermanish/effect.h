#pragma once
#include <math.h>
#include <stdint.h>
#include "processor.h"

// Sherman Filterbank-inspired dual resonator with audio-following and
// tempo-driven envelopes. Not an analogue circuit emulation.
// Input/output: stereo interleaved, including duplicated mono feeds.
class Effect : public Processor {
public:
  static constexpr float SAMPLE_RATE = 48000.f;
  static constexpr float PI = 3.14159265358979323846f;
  enum { PITCH = 0, INTERVAL, REGEN, DRIVE, ROUTING, ENVELOPE,
         ENV_MODE, DRY_WET, NUM_PARAMS };

  enum { ENV_OFF = 0, FOLLOW_FAST = 1, FOLLOW_SLOW = 2,
         TRIG_PUNCH = 3, TRIG_SWELL = 4,
         CLOCK_16TH = 5, CLOCK_8TH = 6, CLOCK_QUARTER = 7,
         CLOCK_HALF = 8, CLOCK_BAR = 9 };
  enum Stage { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE };

  struct Params {
    float pitch, semitones, regen, drive, env, mix;
    int route, mode;
    Params() : pitch(360.f), semitones(0.f), regen(.40f), drive(2.406f),
               env(.45f), mix(1.f), route(0), mode(FOLLOW_FAST) {}
  };

  uint32_t getBufferSize() const override final { return 0U; }
  void init(float *) override final { params = Params(); reset(); }
  void teardown() override final {}
  void reset() override final {
    for (int ch = 0; ch < 2; ++ch)
      for (int j = 0; j < 2; ++j) filters[ch][j] = Svf();
    detector = 0.f;
    follower = 0.f;
    env_value = 0.f;
    clock_phase = 0.f;
    stage = IDLE;
    gate = false;
    pending_tick = false;
    smooth_pitch = params.pitch;
    smooth_semitones = params.semitones;
    smooth_regen = params.regen;
    smooth_drive = params.drive;
    smooth_env = params.env;
    smooth_mix = params.mix;
    smooth_bpm = target_bpm;
  }
  void setParameter(uint8_t index, int32_t value) override final {
    switch (index) {
      case PITCH: params.pitch = static_cast<float>(value); break;
      case INTERVAL: params.semitones = static_cast<float>(value) * .1f; break;
      case REGEN: params.regen = static_cast<float>(value) * .001f; break;
      case DRIVE: params.drive = 1.f + static_cast<float>(value) * .019f; break;
      case ROUTING: params.route = value; break;
      case ENVELOPE: params.env = static_cast<float>(value) * .001f; break;
      case ENV_MODE: params.mode = value; break;
      case DRY_WET: params.mix = (static_cast<float>(value) + 1000.f) * .0005f; break;
      default: break;
    }
  }

  void setTempo(float bpm) override final {
    if (bpm < 30.f) bpm = 30.f;
    if (bpm > 300.f) bpm = 300.f;
    target_bpm = bpm;
  }
  void tempo4ppqnTick(uint32_t counter) override final {
    received_tick_counter = counter;
    pending_tick = true;
  }

  void process(const float *__restrict in, float *__restrict out,
               uint32_t frames) override final {
    smooth_pitch += .11f * (params.pitch - smooth_pitch);
    smooth_semitones += .11f * (params.semitones - smooth_semitones);
    const float hz1 = 55.f * exp2f(smooth_pitch * (5.50635267f / 1000.f));
    const float hz2 = hz1 * exp2f(smooth_semitones / 12.f);
    const int mode = params.mode;
    const float amount = params.env;
    const int route = params.route;

    // The input envelope is calculated per sample, and filter coefficients
    // updated once each audio block. Even 64 frames is only 1.33 ms at 48 kHz.
    const float modulation = exp2f(3.5f * smooth_env * env_value);
    for (int ch = 0; ch < 2; ++ch) {
      // Small opposed detuning creates width from a single mono sax microphone.
      const float stereo_offset = ch == 0 ? .985f : 1.015f;
      filters[ch][0].targetG(hz1 * modulation * stereo_offset);
      filters[ch][1].targetG(hz2 * modulation / stereo_offset);
    }

    // Align tempo modes to NTS-3's 4 PPQN tick callbacks if provided.
    // Also run an internal BPM phase accumulator if tick callbacks are absent.
    if (pending_tick) {
      const uint32_t counter = received_tick_counter;
      pending_tick = false;
      const unsigned ticks = ticksPerCycle(mode);
      if (ticks > 0U && (counter % ticks) == 0U)
        clock_phase = 0.f;
    }

    const float target_regen = params.regen;
    const float target_drive = params.drive;
    const float target_mix = params.mix;
    for (uint32_t i = 0; i < frames; ++i) {
      smooth_regen += .0008f * (target_regen - smooth_regen);
      smooth_drive += .0008f * (target_drive - smooth_drive);
      smooth_mix += .0008f * (target_mix - smooth_mix);
      smooth_env += .0008f * (amount - smooth_env);
      smooth_bpm += .00015f * (target_bpm - smooth_bpm);

      const float left = in[2U * i];
      const float right = in[2U * i + 1U];
      const float peak = fmaxf(fabsf(left), fabsf(right));
      const float det_coeff = peak > detector ? .006f : .00025f;
      detector += det_coeff * (peak - detector);

      if (mode == FOLLOW_FAST || mode == FOLLOW_SLOW) {
        const float desired = clamp01(4.2f * detector);
        const float coef = mode == FOLLOW_FAST ?
              (desired > follower ? .0045f : .00019f) :
              (desired > follower ? .00034f : .000027f);
        follower += coef * (desired - follower);
        env_value = clamp01(follower);
      } else if (mode == TRIG_PUNCH || mode == TRIG_SWELL) {
        if (!gate && detector >= .027f) {
          gate = true; stage = ATTACK;
        } else if (gate && detector < .010f) {
          gate = false; stage = RELEASE;
        }
        advanceTriggered(mode);
      } else if (mode >= CLOCK_16TH && mode <= CLOCK_BAR) {
        const float cycle_beats = beatsPerCycle(mode);
        clock_phase += smooth_bpm / (60.f * SAMPLE_RATE * cycle_beats);
        if (clock_phase >= 1.f) clock_phase -= 1.f;
        // Clock also makes an AR-like amplitude gate in the wet branch.
        const float attack = .085f;
        const float p = clock_phase;
        if (p < attack) env_value = p / attack;
        else {
          const float fall = (1.f - p) / (1.f - attack);
          env_value = fall * fall;
        }
      } else {
        env_value = 0.f;
      }

      const float k = .62f - .600f * smooth_regen;
      const float feedback = .02f + .74f * smooth_regen * smooth_regen;
      const float level_mod = (mode >= CLOCK_16TH && mode <= CLOCK_BAR) ?
          (1.f - .80f * fabsf(smooth_env) * (1.f - env_value)) : 1.f;

      for (int ch = 0; ch < 2; ++ch) {
        Svf &a = filters[ch][0];
        Svf &b = filters[ch][1];
        const float src = in[2U * i + static_cast<uint32_t>(ch)];
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
        const float wet = .72f * softclip(2.5f * combined) * level_mod;
        out[2U * i + static_cast<uint32_t>(ch)] =
            (1.f - smooth_mix) * src + smooth_mix * wet;
      }
    }
  }

  float currentEnvelope() const { return env_value; }
  float currentClockPhase() const { return clock_phase; }
  float currentBpm() const { return smooth_bpm; }

private:
  static float clamp01(float x) { return x < 0.f ? 0.f : (x > 1.f ? 1.f : x); }
  static float softclip(float x) {
    x = x > 3.f ? 3.f : (x < -3.f ? -3.f : x);
    const float x2 = x * x;
    return x * (27.f + x2) / (27.f + 9.f * x2);
  }
  static unsigned ticksPerCycle(int mode) {
    if (mode == CLOCK_16TH) return 1U;
    if (mode == CLOCK_8TH) return 2U;
    if (mode == CLOCK_QUARTER) return 4U;
    if (mode == CLOCK_HALF) return 8U;
    if (mode == CLOCK_BAR) return 16U;
    return 0U;
  }
  static float beatsPerCycle(int mode) {
    if (mode == CLOCK_16TH) return .25f;
    if (mode == CLOCK_8TH) return .5f;
    if (mode == CLOCK_QUARTER) return 1.f;
    if (mode == CLOCK_HALF) return 2.f;
    return 4.f;
  }
  void advanceTriggered(int mode) {
    const bool slow = mode == TRIG_SWELL;
    const float attack = slow ? .220f : .007f;
    const float decay = slow ? .42f : .180f;
    const float sustain = slow ? .65f : .45f;
    const float release = slow ? 1.2f : .34f;
    switch (stage) {
      case IDLE: env_value = 0.f; break;
      case ATTACK:
        env_value += 1.f / (attack * SAMPLE_RATE);
        if (env_value >= 1.f) { env_value = 1.f; stage = DECAY; }
        break;
      case DECAY:
        env_value -= (1.f - sustain) / (decay * SAMPLE_RATE);
        if (env_value <= sustain) { env_value = sustain; stage = SUSTAIN; }
        break;
      case SUSTAIN: env_value = sustain; break;
      case RELEASE:
        env_value -= 1.f / (release * SAMPLE_RATE);
        if (env_value <= 0.f) { env_value = 0.f; stage = IDLE; }
        break;
    }
  }

  struct Svf {
    float ic1 = 0.f, ic2 = 0.f, last = 0.f;
    float g = .015f, g_target = .015f;
    void targetG(float hz) {
      if (hz < 28.f) hz = 28.f;
      if (hz > 5800.f) hz = 5800.f;
      g_target = tanf(PI * hz / SAMPLE_RATE);
    }
    void moveG() { g += .0025f * (g_target - g); }
    float tick(float input, float k) {
      const float a1 = 1.f / (1.f + g * (g + k));
      const float a2 = g * a1;
      const float a3 = g * a2;
      const float v3 = input - ic2;
      const float v1 = a1 * ic1 + a2 * v3;
      const float v2 = ic2 + a2 * ic1 + a3 * v3;
      ic1 = 2.f * v1 - ic1;
      ic2 = 2.f * v2 - ic2;
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
  float detector = 0.f, follower = 0.f, env_value = 0.f;
  Stage stage = IDLE;
  bool gate = false;
  volatile bool pending_tick = false;
  volatile uint32_t received_tick_counter = 0U;
  float target_bpm = 120.f, smooth_bpm = 120.f, clock_phase = 0.f;
  float smooth_pitch = 360.f, smooth_semitones = 0.f;
  float smooth_regen = .4f, smooth_drive = 2.406f;
  float smooth_env = .45f, smooth_mix = 1.f;
};

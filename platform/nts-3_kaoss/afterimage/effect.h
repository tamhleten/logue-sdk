#pragma once

#include <math.h>
#include <stdint.h>

#include "processor.h"

class Effect : public Processor
{
public:
  static constexpr float SAMPLE_RATE = 48000.f;
  static constexpr uint32_t RING_SIZE = 4U * 48000U;
  static constexpr uint32_t PITCH_DELAY = 4096U;
  static constexpr uint32_t PITCH_BUFFER_SIZE = PITCH_DELAY + 2U;
  static constexpr uint32_t WINDOW_TABLE_SIZE = PITCH_DELAY + 1U;
  static constexpr uint32_t BUFFER_SIZE = 2U * RING_SIZE +
                                          2U * PITCH_BUFFER_SIZE +
                                          WINDOW_TABLE_SIZE;
  static constexpr uint32_t END_HOLD_SAMPLES = 9600U;  // 200 ms
  static constexpr uint32_t MIN_PHRASE_SAMPLES = 7200U; // 150 ms
  static constexpr uint32_t PRE_ROLL_SAMPLES = 2400U;   // 50 ms

  enum
  {
    PITCH = 0U,
    MEMORY,
    SHADOW_LEVEL,
    NUM_PARAMS
  };

  struct Params
  {
    float ratio;
    uint32_t memory_samples;
    float shadow_level;

    void reset()
    {
      ratio = pitchRatio(700);
      memory_samples = 72000U;
      shadow_level = 0.5f;
    }

    Params() { reset(); }
  };

  uint32_t getBufferSize() const override final { return BUFFER_SIZE; }

  static float pitchRatio(int32_t cents)
  {
    return exp2f(static_cast<float>(cents) / 1200.f);
  }

  void setParameter(uint8_t index, int32_t value) override final
  {
    switch (index)
    {
    case PITCH:
      params.ratio = pitchRatio(value);
      break;
    case MEMORY:
      params.memory_samples = static_cast<uint32_t>(value) * 48U;
      break;
    case SHADOW_LEVEL:
      params.shadow_level = static_cast<float>(value) * 0.001f;
      break;
    default:
      break;
    }
  }

  const char *getParameterStrValue(uint8_t, int32_t) const override final
  {
    return nullptr;
  }

  void init(float *allocated_buffer) override final
  {
    ring_l = allocated_buffer;
    ring_r = ring_l ? ring_l + RING_SIZE : nullptr;
    pitch_l = ring_r ? ring_r + RING_SIZE : nullptr;
    pitch_r = pitch_l ? pitch_l + PITCH_BUFFER_SIZE : nullptr;
    window_table = pitch_r ? pitch_r + PITCH_BUFFER_SIZE : nullptr;
    params.reset();

    if (window_table)
    {
      static constexpr float TWO_PI = 6.28318530717958647692f;
      for (uint32_t i = 0; i < WINDOW_TABLE_SIZE; ++i)
      {
        const float phase = static_cast<float>(i) /
                            static_cast<float>(PITCH_DELAY);
        window_table[i] = 0.5f * (1.f + cosf(TWO_PI * phase));
      }
    }
    reset();
  }

  void teardown() override final
  {
    ring_l = nullptr;
    ring_r = nullptr;
    pitch_l = nullptr;
    pitch_r = nullptr;
    window_table = nullptr;
  }

  void reset() override final
  {
    write_index = 0U;
    phrase_active = false;
    phrase_span = 0U;
    captured_span = 0U;
    silence_count = 0U;
    last_sound_index = 0U;
    have_capture = false;
    capture_start = 0U;
    capture_length = 0U;
    playback_active = false;
    playback_index = 0U;
    playback_remaining = 0U;
    playback_total = 0U;
    tail_remaining = 0U;
    hold_enabled = false;
    hold_toggle_pending = false;
    envelope = 0.f;
    noise_floor = 0.001f;
    duck_gain = 1.f;
    current_level = params.shadow_level;
    pitch_write_index = 0U;
    pitch_phase = 0.f;
    current_ratio = params.ratio;

    if (ring_l)
      for (uint32_t i = 0; i < BUFFER_SIZE; ++i)
        ring_l[i] = 0.f;
  }

  void process(const float *__restrict in,
               float *__restrict out,
               uint32_t frames) override final
  {
    if (!ring_l || !ring_r || !pitch_l || !pitch_r || !window_table)
    {
      for (uint32_t i = 0; i < 2U * frames; ++i)
        out[i] = in[i];
      return;
    }

    const Params target = params;

    if (hold_toggle_pending)
    {
      hold_toggle_pending = false;
      // Ignore ordinary pad setup before a shadow exists.  Touch/footswitch
      // toggles Hold only while an afterimage is actually sounding.
      if (playback_active || tail_remaining > 0U)
      {
        hold_enabled = !hold_enabled;
        if (hold_enabled && have_capture && !playback_active)
          startPlayback();
      }
    }

    for (uint32_t i = 0; i < frames; ++i)
    {
      const float input_l = in[2U * i];
      const float input_r = in[2U * i + 1U];
      const float mono_abs = 0.5f * (fabsf(input_l) + fabsf(input_r));

      const float env_coeff = mono_abs > envelope ? 0.05f : 0.001f;
      envelope += env_coeff * (mono_abs - envelope);

      if (!phrase_active && !(hold_enabled && have_capture))
        noise_floor += 0.00001f * (mono_abs - noise_floor);

      const float onset_threshold = maximum(0.012f, 6.f * noise_floor);
      const float release_threshold = maximum(0.006f, 3.f * noise_floor);
      const uint32_t current_index = write_index;

      if (!(hold_enabled && have_capture))
      {
        ring_l[current_index] = input_l;
        ring_r[current_index] = input_r;
        advanceRing(write_index);
        updatePhraseDetector(current_index, onset_threshold,
                             release_threshold, target.memory_samples);
      }

      float source_l = 0.f;
      float source_r = 0.f;
      float phrase_fade = 0.f;

      if (playback_active)
      {
        source_l = ring_l[playback_index];
        source_r = ring_r[playback_index];
        advanceRing(playback_index);

        const uint32_t played = playback_total - playback_remaining;
        static constexpr uint32_t FADE_IN = 2400U;
        static constexpr uint32_t FADE_OUT = 4800U;
        const float fade_in = minimum(1.f, static_cast<float>(played) /
                                           static_cast<float>(FADE_IN));
        const float fade_out = minimum(1.f,
            static_cast<float>(playback_remaining) /
            static_cast<float>(FADE_OUT));
        phrase_fade = minimum(fade_in, fade_out);

        if (--playback_remaining == 0U)
        {
          if (hold_enabled)
            startPlayback();
          else
          {
            playback_active = false;
            tail_remaining = PITCH_DELAY;
          }
        }
      }
      else if (tail_remaining > 0U)
      {
        phrase_fade = static_cast<float>(tail_remaining) /
                      static_cast<float>(PITCH_DELAY);
        --tail_remaining;
      }

      float shifted_l = 0.f;
      float shifted_r = 0.f;
      processPitch(source_l, source_r, target.ratio, shifted_l, shifted_r);

      const bool live_input = phrase_active || envelope > release_threshold;
      const float duck_target = live_input ? 0.f : 1.f;
      const float duck_coeff = duck_target < duck_gain ? 0.002f : 0.0005f;
      duck_gain += duck_coeff * (duck_target - duck_gain);
      current_level += 0.001f * (target.shadow_level - current_level);

      const float shadow_gain = 0.7f * current_level * duck_gain * phrase_fade;
      out[2U * i] = input_l + shadow_gain * shifted_l;
      out[2U * i + 1U] = input_r + shadow_gain * shifted_r;
    }
  }

  void touchEvent(uint8_t, uint8_t phase, uint32_t, uint32_t) override final
  {
    // k_unit_touch_phase_began is zero in the logue SDK runtime ABI.
    if (phase == 0U)
      hold_toggle_pending = true;
  }

  bool isPhraseActive() const { return phrase_active; }
  bool isPlaybackActive() const
  {
    return playback_active || tail_remaining > 0U;
  }
  bool isHoldEnabled() const { return hold_enabled; }
  uint32_t capturedSamples() const { return capture_length; }
  float inputEnvelope() const { return envelope; }
  float currentDuckGain() const { return duck_gain; }

private:
  static float minimum(float a, float b) { return a < b ? a : b; }
  static float maximum(float a, float b) { return a > b ? a : b; }

  static void advanceRing(uint32_t &index)
  {
    if (++index >= RING_SIZE)
      index = 0U;
  }

  void updatePhraseDetector(uint32_t current_index,
                            float onset_threshold,
                            float release_threshold,
                            uint32_t memory_samples)
  {
    if (!phrase_active)
    {
      if (envelope >= onset_threshold)
      {
        phrase_active = true;
        phrase_span = PRE_ROLL_SAMPLES;
        captured_span = PRE_ROLL_SAMPLES;
        silence_count = 0U;
        last_sound_index = current_index;
      }
      return;
    }

    const uint32_t maximum_span = RING_SIZE - END_HOLD_SAMPLES - 1U;
    if (phrase_span < maximum_span)
      ++phrase_span;

    if (envelope >= release_threshold)
    {
      silence_count = 0U;
      captured_span = phrase_span;
      last_sound_index = current_index;
      return;
    }

    if (++silence_count < END_HOLD_SAMPLES)
      return;

    if (captured_span >= MIN_PHRASE_SAMPLES)
    {
      capture_length = captured_span < memory_samples ?
                       captured_span : memory_samples;
      capture_start = (last_sound_index + RING_SIZE + 1U - capture_length) %
                      RING_SIZE;
      have_capture = true;
      startPlayback();
    }

    phrase_active = false;
    phrase_span = 0U;
    captured_span = 0U;
    silence_count = 0U;
  }

  void startPlayback()
  {
    if (!have_capture || capture_length == 0U)
      return;
    playback_active = true;
    playback_index = capture_start;
    playback_remaining = capture_length;
    playback_total = capture_length;
    tail_remaining = 0U;
  }

  float readPitchDelay(const float *buffer, float delay) const
  {
    float position = static_cast<float>(pitch_write_index) - delay;
    while (position < 0.f)
      position += static_cast<float>(PITCH_BUFFER_SIZE);
    while (position >= static_cast<float>(PITCH_BUFFER_SIZE))
      position -= static_cast<float>(PITCH_BUFFER_SIZE);

    const uint32_t index_a = static_cast<uint32_t>(position);
    uint32_t index_b = index_a + 1U;
    if (index_b >= PITCH_BUFFER_SIZE)
      index_b = 0U;
    const float fraction = position - static_cast<float>(index_a);
    return buffer[index_a] + (buffer[index_b] - buffer[index_a]) * fraction;
  }

  float readWindow(float position) const
  {
    const uint32_t index_a = static_cast<uint32_t>(position);
    const uint32_t index_b = index_a + 1U;
    const float fraction = position - static_cast<float>(index_a);
    return window_table[index_a] +
           (window_table[index_b] - window_table[index_a]) * fraction;
  }

  void processPitch(float input_l, float input_r, float target_ratio,
                    float &output_l, float &output_r)
  {
    current_ratio += 0.002f * (target_ratio - current_ratio);
    pitch_phase += (1.f - current_ratio) / static_cast<float>(PITCH_DELAY);
    while (pitch_phase >= 1.f)
      pitch_phase -= 1.f;
    while (pitch_phase < 0.f)
      pitch_phase += 1.f;

    const float delay_a = pitch_phase * static_cast<float>(PITCH_DELAY);
    float phase_b = pitch_phase + 0.5f;
    if (phase_b >= 1.f)
      phase_b -= 1.f;
    const float delay_b = phase_b * static_cast<float>(PITCH_DELAY);
    const float window_b =
        readWindow(pitch_phase * static_cast<float>(PITCH_DELAY));
    const float window_a = 1.f - window_b;

    pitch_l[pitch_write_index] = input_l;
    pitch_r[pitch_write_index] = input_r;
    output_l = window_a * readPitchDelay(pitch_l, delay_a) +
               window_b * readPitchDelay(pitch_l, delay_b);
    output_r = window_a * readPitchDelay(pitch_r, delay_a) +
               window_b * readPitchDelay(pitch_r, delay_b);

    if (++pitch_write_index >= PITCH_BUFFER_SIZE)
      pitch_write_index = 0U;
  }

  float *ring_l = nullptr;
  float *ring_r = nullptr;
  float *pitch_l = nullptr;
  float *pitch_r = nullptr;
  float *window_table = nullptr;
  Params params;

  uint32_t write_index = 0U;
  bool phrase_active = false;
  uint32_t phrase_span = 0U;
  uint32_t captured_span = 0U;
  uint32_t silence_count = 0U;
  uint32_t last_sound_index = 0U;
  bool have_capture = false;
  uint32_t capture_start = 0U;
  uint32_t capture_length = 0U;

  bool playback_active = false;
  uint32_t playback_index = 0U;
  uint32_t playback_remaining = 0U;
  uint32_t playback_total = 0U;
  uint32_t tail_remaining = 0U;
  bool hold_enabled = false;
  bool hold_toggle_pending = false;

  float envelope = 0.f;
  float noise_floor = 0.001f;
  float duck_gain = 1.f;
  float current_level = 0.5f;

  uint32_t pitch_write_index = 0U;
  float pitch_phase = 0.f;
  float current_ratio = 1.f;
};

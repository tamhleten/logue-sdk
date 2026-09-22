#pragma once

#include <math.h>
#include <stdint.h>

#include "processor.h"

class Effect : public Processor
{
public:
  static constexpr float SAMPLE_RATE = 48000.f;
  static constexpr uint32_t DELAY_LENGTH = 5024U;
  static constexpr uint32_t MIN_WINDOW_LENGTH = 1024U;
  static constexpr uint32_t CHANNEL_BUFFER_SIZE = DELAY_LENGTH + 2U;
  static constexpr uint32_t AUDIO_BUFFER_SIZE = 2U * CHANNEL_BUFFER_SIZE;
  static constexpr uint32_t WINDOW_TABLE_SIZE = DELAY_LENGTH + 1U;
  static constexpr uint32_t BUFFER_SIZE = AUDIO_BUFFER_SIZE + WINDOW_TABLE_SIZE;

  enum
  {
    SHIFT = 0U,
    WINDOW_LENGTH,
    WET_LEVEL,
    NUM_PARAMS
  };

  struct Params
  {
    float ratio;
    float window_length;
    float wet;

    void reset()
    {
      ratio = 1.f;
      window_length = static_cast<float>(DELAY_LENGTH);
      wet = 1.f;
    }

    Params() { reset(); }
  };

  uint32_t getBufferSize() const override final { return BUFFER_SIZE; }

  void setParameter(uint8_t index, int32_t value) override final
  {
    switch (index)
    {
    case SHIFT:
      params.ratio = shiftRatio(value);
      break;
    case WINDOW_LENGTH:
      params.window_length = static_cast<float>(value);
      break;
    case WET_LEVEL:
      params.wet = static_cast<float>(value) * 0.001f;
      break;
    default:
      break;
    }
  }

  static float shiftRatio(int32_t value)
  {
    // The exposed parameter is a centered pad position (-1000 .. +1000).
    // Give each half of X its own exponential pitch range so that unity is
    // exactly at the physical midpoint while retaining 0.25x and 8.00x.
    const float position = static_cast<float>(value) * 0.001f;
    return exp2f(position < 0.f ? 2.f * position : 3.f * position);
  }

  const char *getParameterStrValue(uint8_t, int32_t) const override final
  {
    return nullptr;
  }

  void init(float *allocated_buffer) override final
  {
    buffer_l = allocated_buffer;
    buffer_r = allocated_buffer ? allocated_buffer + CHANNEL_BUFFER_SIZE : nullptr;
    window_table = allocated_buffer ? allocated_buffer + AUDIO_BUFFER_SIZE : nullptr;
    params.reset();
    current_ratio = 1.f;
    current_window_length = static_cast<float>(DELAY_LENGTH);
    sweep_phase = 0.f;
    write_index = 0U;

    if (window_table)
    {
      static constexpr float TWO_PI = 6.28318530717958647692f;
      for (uint32_t i = 0; i < WINDOW_TABLE_SIZE; ++i)
      {
        const float phase = static_cast<float>(i) /
                            static_cast<float>(DELAY_LENGTH);
        window_table[i] = 0.5f * (1.f + cosf(TWO_PI * phase));
      }
    }
  }

  void teardown() override final
  {
    buffer_l = nullptr;
    buffer_r = nullptr;
    window_table = nullptr;
  }

  void reset() override final
  {
    sweep_phase = 0.f;
    write_index = 0U;
    current_ratio = params.ratio;
    current_window_length = params.window_length;

    if (buffer_l)
      for (uint32_t i = 0; i < AUDIO_BUFFER_SIZE; ++i)
        buffer_l[i] = 0.f;
  }

  void process(const float *__restrict in,
               float *__restrict out,
               uint32_t frames) override final
  {
    if (!buffer_l || !buffer_r || !window_table)
    {
      for (uint32_t i = 0; i < 2U * frames; ++i)
        out[i] = 0.f;
      return;
    }

    const Params target = params;

    for (uint32_t i = 0; i < frames; ++i)
    {
      // SuperCollider's example uses XLine for smooth shift changes.  This
      // one-pole slew gives NTS-3 pad movements the same useful behaviour.
      current_ratio += 0.002f * (target.ratio - current_ratio);
      current_window_length +=
          0.002f * (target.window_length - current_window_length);

      // Keep the sweep as normalized phase so changing the window length does
      // not reset the grains or introduce a phase jump.
      sweep_phase += (1.f - current_ratio) / current_window_length;
      while (sweep_phase >= 1.f)
        sweep_phase -= 1.f;
      while (sweep_phase < 0.f)
        sweep_phase += 1.f;

      // Two delay taps are half a grain apart.
      const float delay_a = sweep_phase * current_window_length;
      float phase_b = sweep_phase + 0.5f;
      if (phase_b >= 1.f)
        phase_b -= 1.f;
      const float delay_b = phase_b * current_window_length;

      // Phase-linked Hann crossfade.  At either delay tap's wrap point that
      // tap has zero gain, eliminating the discontinuity.  The complementary
      // windows sum to one throughout the sweep.
      const float window_b =
          readWindow(sweep_phase * static_cast<float>(DELAY_LENGTH));
      const float window_a = 1.f - window_b;

      // Write first so a zero-length tap really is the current input sample.
      buffer_l[write_index] = in[2U * i];
      buffer_r[write_index] = in[2U * i + 1U];

      const float wet_l = window_a * readDelay(buffer_l, delay_a) +
                          window_b * readDelay(buffer_l, delay_b);
      const float wet_r = window_a * readDelay(buffer_r, delay_a) +
                          window_b * readDelay(buffer_r, delay_b);

      ++write_index;
      if (write_index >= CHANNEL_BUFFER_SIZE)
        write_index = 0U;

      // Depth is a true dry/wet crossfade: zero is the untouched input and
      // one is the fully pitch-shifted signal.
      const float dry = 1.f - target.wet;
      out[2U * i] = dry * in[2U * i] + target.wet * wet_l;
      out[2U * i + 1U] = dry * in[2U * i + 1U] + target.wet * wet_r;
    }
  }

private:
  float readDelay(const float *channel, float delay_samples) const
  {
    float position = static_cast<float>(write_index) - delay_samples;
    while (position < 0.f)
      position += static_cast<float>(CHANNEL_BUFFER_SIZE);
    while (position >= static_cast<float>(CHANNEL_BUFFER_SIZE))
      position -= static_cast<float>(CHANNEL_BUFFER_SIZE);

    const uint32_t index_a = static_cast<uint32_t>(position);
    uint32_t index_b = index_a + 1U;
    if (index_b >= CHANNEL_BUFFER_SIZE)
      index_b = 0U;

    const float fraction = position - static_cast<float>(index_a);
    return channel[index_a] + (channel[index_b] - channel[index_a]) * fraction;
  }

  float readWindow(float position) const
  {
    const uint32_t index_a = static_cast<uint32_t>(position);
    const uint32_t index_b = index_a + 1U;
    const float fraction = position - static_cast<float>(index_a);
    return window_table[index_a] +
           (window_table[index_b] - window_table[index_a]) * fraction;
  }

  float *buffer_l = nullptr;
  float *buffer_r = nullptr;
  float *window_table = nullptr;
  Params params;
  uint32_t write_index = 0U;
  float sweep_phase = 0.f;
  float current_ratio = 1.f;
  float current_window_length = static_cast<float>(DELAY_LENGTH);
};

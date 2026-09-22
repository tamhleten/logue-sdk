#pragma once

#include <math.h>
#include <stdint.h>

#include "processor.h"

class Effect : public Processor
{
public:
  static constexpr float SAMPLE_RATE = 48000.f;
  static constexpr uint32_t DELAY_LENGTH = 5024U;
  static constexpr uint32_t HALF_LENGTH = DELAY_LENGTH / 2U;
  static constexpr uint32_t CHANNEL_BUFFER_SIZE = DELAY_LENGTH + 2U;
  static constexpr uint32_t AUDIO_BUFFER_SIZE = 2U * CHANNEL_BUFFER_SIZE;
  static constexpr uint32_t WINDOW_TABLE_SIZE = DELAY_LENGTH + 1U;
  static constexpr uint32_t BUFFER_SIZE = AUDIO_BUFFER_SIZE + WINDOW_TABLE_SIZE;

  enum
  {
    SHIFT = 0U,
    WET_LEVEL,
    NUM_PARAMS
  };

  struct Params
  {
    float ratio;
    float wet;

    void reset()
    {
      ratio = 1.f;
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
      params.ratio = static_cast<float>(value) * 0.01f;
      break;
    case WET_LEVEL:
      params.wet = static_cast<float>(value) * 0.001f;
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
    buffer_l = allocated_buffer;
    buffer_r = allocated_buffer ? allocated_buffer + CHANNEL_BUFFER_SIZE : nullptr;
    window_table = allocated_buffer ? allocated_buffer + AUDIO_BUFFER_SIZE : nullptr;
    params.reset();
    current_ratio = 1.f;
    sweep_samples = 0.f;
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
    sweep_samples = 0.f;
    write_index = 0U;
    current_ratio = params.ratio;

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

      // Sweep.ar(Impulse.kr(0), 1.0 - shift), expressed in samples.
      sweep_samples += 1.f - current_ratio;
      while (sweep_samples >= static_cast<float>(DELAY_LENGTH))
        sweep_samples -= static_cast<float>(DELAY_LENGTH);
      while (sweep_samples < 0.f)
        sweep_samples += static_cast<float>(DELAY_LENGTH);

      // Wrap.ar([sweep, sweep + halfLength], 0, delayLength).
      const float delay_a = sweep_samples;
      float delay_b = sweep_samples + static_cast<float>(HALF_LENGTH);
      if (delay_b >= static_cast<float>(DELAY_LENGTH))
        delay_b -= static_cast<float>(DELAY_LENGTH);

      // Phase-linked Hann crossfade.  At either delay tap's wrap point that
      // tap has zero gain, eliminating the discontinuity.  The complementary
      // windows sum to one throughout the sweep.
      const float window_b = readWindow(sweep_samples);
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

      // Generic FX routing supplies the dry path; this unit returns wet only.
      out[2U * i] = target.wet * wet_l;
      out[2U * i + 1U] = target.wet * wet_r;
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
  float sweep_samples = 0.f;
  float current_ratio = 1.f;
};

#pragma once

#include <math.h>
#include <stdint.h>

#include "processor.h"

class Effect : public Processor
{
public:
  static constexpr float SAMPLE_RATE = 48000.f;
  static constexpr uint32_t DELAY_BUFFER_SIZE = 100000U;
  static constexpr uint32_t BUFFER_SIZE = 2U * DELAY_BUFFER_SIZE;

  enum
  {
    LENGTH = 0U,
    FEEDBACK,
    DRY_WET,
    SEPARATION,
    MOD_RATE,
    NUM_PARAMS
  };

  struct Params
  {
    float delay_samples;
    float separation_samples;
    float mix;
    float feedback;
    float modulation_rate;

    void reset()
    {
      delay_samples = 750.f * 48.f;
      separation_samples = 1.2f * 48.f;
      mix = 0.5f;
      feedback = 0.7f;
      modulation_rate = 12.f;
    }

    Params() { reset(); }
  };

  uint32_t getBufferSize() const override final { return BUFFER_SIZE; }

  void setParameter(uint8_t index, int32_t value) override final
  {
    switch (index)
    {
    case LENGTH:
      params.delay_samples = static_cast<float>(value) * 48.f;
      break;
    case FEEDBACK:
      params.feedback = static_cast<float>(value) * 0.001f;
      break;
    case DRY_WET:
      params.mix = (static_cast<float>(value) + 1000.f) * 0.0005f;
      break;
    case SEPARATION:
      // Header value is in tenths of a millisecond.
      params.separation_samples = static_cast<float>(value) * 4.8f;
      break;
    case MOD_RATE:
      params.modulation_rate = static_cast<float>(value) * 0.01f;
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
    delay_l = allocated_buffer;
    delay_r = delay_l ? delay_l + DELAY_BUFFER_SIZE : nullptr;
    params.reset();
    reset();
  }

  void teardown() override final
  {
    delay_l = nullptr;
    delay_r = nullptr;
  }

  void reset() override final
  {
    write_index = 0U;
    hp_state_l = 0.f;
    hp_state_r = 0.f;
    lp_state_l = 0.f;
    lp_state_r = 0.f;
    current_delay = params.delay_samples;
    current_separation = params.separation_samples;
    current_mix = params.mix;
    current_feedback = params.feedback;
    current_rate = params.modulation_rate;
    noise_phase = 0.f;
    noise_from = 0.f;
    noise_to = randomBipolar();

    if (delay_l)
      for (uint32_t i = 0; i < BUFFER_SIZE; ++i)
        delay_l[i] = 0.f;
  }

  void process(const float *__restrict in,
               float *__restrict out,
               uint32_t frames) override final
  {
    if (!delay_l || !delay_r)
    {
      for (uint32_t i = 0; i < 2U * frames; ++i)
        out[i] = in[i];
      return;
    }

    const Params target = params;

    for (uint32_t i = 0; i < frames; ++i)
    {
      // Around 20-40 ms of one-pole smoothing. Delay changes intentionally
      // glide like tape rather than jumping between read positions.
      current_delay += 0.0005f * (target.delay_samples - current_delay);
      current_separation +=
          0.001f * (target.separation_samples - current_separation);
      current_mix += 0.001f * (target.mix - current_mix);
      current_feedback += 0.001f * (target.feedback - current_feedback);
      current_rate += 0.001f * (target.modulation_rate - current_rate);

      const float noise = nextSmoothNoise(current_rate);
      const float position = 0.5f * (noise + 1.f);
      const float delay_for_l = current_delay +
                                position * current_separation;
      const float delay_for_r = current_delay +
                                (1.f - position) * current_separation;

      // Equivalent to DelayC(...).reverse in the SuperCollider patch:
      // right history returns on the left, and left history on the right.
      const float feedback_l = readDelayCubic(delay_r, delay_for_r);
      const float feedback_r = readDelayCubic(delay_l, delay_for_l);

      const float summed_l = in[2U * i] + current_feedback * feedback_l;
      const float summed_r = in[2U * i + 1U] +
                             current_feedback * feedback_r;

      // LeakDC + HPF(400) + LPF(5000), with the 400 Hz HPF also removing DC.
      hp_state_l += HP_COEFF * (summed_l - hp_state_l);
      hp_state_r += HP_COEFF * (summed_r - hp_state_r);
      const float high_l = summed_l - hp_state_l;
      const float high_r = summed_r - hp_state_r;
      lp_state_l += LP_COEFF * (high_l - lp_state_l);
      lp_state_r += LP_COEFF * (high_r - lp_state_r);

      const float wet_l = fastTanh(lp_state_l);
      const float wet_r = fastTanh(lp_state_r);
      delay_l[write_index] = wet_l;
      delay_r[write_index] = wet_r;

      if (++write_index >= DELAY_BUFFER_SIZE)
        write_index = 0U;

      const float dry = 1.f - current_mix;
      out[2U * i] = dry * in[2U * i] + current_mix * wet_l;
      out[2U * i + 1U] = dry * in[2U * i + 1U] +
                         current_mix * wet_r;
    }
  }

  float currentDelaySamples() const { return current_delay; }
  float currentSeparationSamples() const { return current_separation; }
  float currentMix() const { return current_mix; }
  float currentFeedback() const { return current_feedback; }

private:
  static constexpr float HP_COEFF = 0.051001575f;
  static constexpr float LP_COEFF = 0.480297357f;

  float readDelayCubic(const float *buffer, float delay_samples) const
  {
    float position = static_cast<float>(write_index) - delay_samples;
    while (position < 0.f)
      position += static_cast<float>(DELAY_BUFFER_SIZE);
    while (position >= static_cast<float>(DELAY_BUFFER_SIZE))
      position -= static_cast<float>(DELAY_BUFFER_SIZE);

    const uint32_t index_1 = static_cast<uint32_t>(position);
    const float fraction = position - static_cast<float>(index_1);
    const uint32_t index_0 = index_1 == 0U ?
                             DELAY_BUFFER_SIZE - 1U : index_1 - 1U;
    const uint32_t index_2 = index_1 + 1U >= DELAY_BUFFER_SIZE ?
                             0U : index_1 + 1U;
    const uint32_t index_3 = index_2 + 1U >= DELAY_BUFFER_SIZE ?
                             0U : index_2 + 1U;

    const float y0 = buffer[index_0];
    const float y1 = buffer[index_1];
    const float y2 = buffer[index_2];
    const float y3 = buffer[index_3];
    const float a0 = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
    const float a1 = y0 - 2.5f * y1 + 2.f * y2 - 0.5f * y3;
    const float a2 = -0.5f * y0 + 0.5f * y2;
    return ((a0 * fraction + a1) * fraction + a2) * fraction + y1;
  }

  static float fastTanh(float value)
  {
    if (value > 3.f)
      value = 3.f;
    else if (value < -3.f)
      value = -3.f;
    const float squared = value * value;
    return value * (27.f + squared) / (27.f + 9.f * squared);
  }

  float nextSmoothNoise(float rate)
  {
    noise_phase += rate / SAMPLE_RATE;
    while (noise_phase >= 1.f)
    {
      noise_phase -= 1.f;
      noise_from = noise_to;
      noise_to = randomBipolar();
    }
    const float smooth = noise_phase * noise_phase *
                         (3.f - 2.f * noise_phase);
    return noise_from + (noise_to - noise_from) * smooth;
  }

  float randomBipolar()
  {
    random_state = random_state * 1664525U + 1013904223U;
    const float unit = static_cast<float>((random_state >> 8) & 0x00FFFFFFU) /
                       16777215.f;
    return 2.f * unit - 1.f;
  }

  float *delay_l = nullptr;
  float *delay_r = nullptr;
  Params params;
  uint32_t write_index = 0U;

  float hp_state_l = 0.f;
  float hp_state_r = 0.f;
  float lp_state_l = 0.f;
  float lp_state_r = 0.f;

  float current_delay = 36000.f;
  float current_separation = 57.6f;
  float current_mix = 0.5f;
  float current_feedback = 0.7f;
  float current_rate = 12.f;

  uint32_t random_state = 0x41C64E6DU;
  float noise_phase = 0.f;
  float noise_from = 0.f;
  float noise_to = 0.f;
};

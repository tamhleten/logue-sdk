#pragma once

#include <math.h>
#include <stdint.h>
#include "processor.h"

class Effect : public Processor
{
public:
  static constexpr float SAMPLE_RATE = 48000.f;
  static constexpr uint32_t WINDOW_LENGTH = 5024U;
  static constexpr uint32_t CHANNEL_BUFFER_SIZE = WINDOW_LENGTH + 2U;
  static constexpr uint32_t AUDIO_BUFFER_SIZE = 2U * CHANNEL_BUFFER_SIZE;
  static constexpr uint32_t WINDOW_TABLE_SIZE = WINDOW_LENGTH + 1U;
  static constexpr uint32_t BUFFER_SIZE = AUDIO_BUFFER_SIZE + WINDOW_TABLE_SIZE;

  enum
  {
    RING_FREQUENCY = 0U,
    DESTRUCTION,
    DRY_WET,
    PITCH,
    SENSITIVITY,
    GATE_RATE,
    PATTERN,
    PITCH_MIX,
    NUM_PARAMS
  };

  struct Params
  {
    float ring_hz;
    float destruction;
    float wet;
    float pitch_ratio;
    float threshold;
    float gate_rate;
    uint8_t pattern;
    float pitch_mix;

    void reset()
    {
      ring_hz = 170.f;
      destruction = 0.75f;
      wet = 1.f;
      pitch_ratio = 0.5f;
      threshold = 0.035f;
      gate_rate = 8.f;
      pattern = 0U;
      pitch_mix = 0.55f;
    }
    Params() { reset(); }
  };

  uint32_t getBufferSize() const override final { return BUFFER_SIZE; }

  void setParameter(uint8_t index, int32_t value) override final
  {
    switch (index)
    {
    case RING_FREQUENCY: params.ring_hz = static_cast<float>(value); break;
    case DESTRUCTION: params.destruction = static_cast<float>(value) * 0.001f; break;
    case DRY_WET: params.wet = 0.0005f * static_cast<float>(value + 1000); break;
    case PITCH: params.pitch_ratio = exp2f(static_cast<float>(value) / 12.f); break;
    case SENSITIVITY: params.threshold = static_cast<float>(value) * 0.001f; break;
    case GATE_RATE: params.gate_rate = static_cast<float>(value) * 0.1f; break;
    case PATTERN: params.pattern = static_cast<uint8_t>(value); break;
    case PITCH_MIX: params.pitch_mix = static_cast<float>(value) * 0.001f; break;
    default: break;
    }
  }

  const char *getParameterStrValue(uint8_t, int32_t) const override final { return nullptr; }

  void init(float *allocated_buffer) override final
  {
    buffer_l = allocated_buffer;
    buffer_r = allocated_buffer ? allocated_buffer + CHANNEL_BUFFER_SIZE : nullptr;
    window_table = allocated_buffer ? allocated_buffer + AUDIO_BUFFER_SIZE : nullptr;
    params.reset();
    if (window_table)
    {
      for (uint32_t i = 0; i < WINDOW_TABLE_SIZE; ++i)
      {
        const float phase = static_cast<float>(i) / static_cast<float>(WINDOW_LENGTH);
        window_table[i] = 0.5f * (1.f + cosf(TWO_PI * phase));
      }
    }
    reset();
  }

  void teardown() override final
  {
    buffer_l = nullptr;
    buffer_r = nullptr;
    window_table = nullptr;
  }

  void reset() override final
  {
    if (buffer_l)
      for (uint32_t i = 0; i < AUDIO_BUFFER_SIZE; ++i) buffer_l[i] = 0.f;
    write_index = 0U;
    sweep_phase = 0.f;
    ring_phase = 0.f;
    envelope = 0.f;
    current_ring_hz = params.ring_hz;
    current_destruction = params.destruction;
    current_wet = params.wet;
    current_ratio = params.pitch_ratio;
    current_pitch_mix = params.pitch_mix;
    current_gate_rate = params.gate_rate;
    held_l = held_r = 0.f;
    crush_phase = 0.f;
    gate_phase = 0.f;
    gate_value = 1.f;
    gate_step = 0U;
    note_active = false;
    silence_samples = 0U;
    attack_count = 0U;
    lpg_l = lpg_r = 0.f;
  }

  void process(const float *__restrict in, float *__restrict out,
               uint32_t frames) override final
  {
    if (!buffer_l || !buffer_r || !window_table)
    {
      for (uint32_t i = 0; i < 2U * frames; ++i) out[i] = 0.f;
      return;
    }

    const Params target = params;
    for (uint32_t i = 0; i < frames; ++i)
    {
      const float input_l = in[2U * i];
      const float input_r = in[2U * i + 1U];
      const float peak = fmaxf(fabsf(input_l), fabsf(input_r));

      const float env_coeff = peak > envelope ? 0.0042f : 0.00026f;
      envelope += env_coeff * (peak - envelope);
      updateAttackDetector(target.threshold);

      current_ring_hz += 0.0015f * (target.ring_hz - current_ring_hz);
      current_destruction += 0.002f * (target.destruction - current_destruction);
      current_wet += 0.002f * (target.wet - current_wet);
      current_ratio += 0.0015f * (target.pitch_ratio - current_ratio);
      current_pitch_mix += 0.002f * (target.pitch_mix - current_pitch_mix);
      current_gate_rate += 0.002f * (target.gate_rate - current_gate_rate);

      const float env_drive = clamp01((envelope - 0.25f * target.threshold) /
                                      fmaxf(0.02f, 4.f * target.threshold));
      const float corruption = clamp01(current_destruction * (0.65f + 0.7f * env_drive));

      const float pitched_l = pitchSample(buffer_l, input_l);
      const float pitched_r = pitchSample(buffer_r, input_r);
      advancePitch();

      const float hold_period = 4.f + 22.f * (1.f - corruption) * (1.f - corruption);
      crush_phase += 1.f;
      if (crush_phase >= hold_period)
      {
        crush_phase -= hold_period;
        const float levels = 8.f + 504.f * (1.f - corruption) * (1.f - corruption);
        held_l = floorf(input_l * levels + 0.5f) / levels;
        held_r = floorf(input_r * levels + 0.5f) / levels;
      }

      const float branch_l = (1.f - current_pitch_mix) * held_l + current_pitch_mix * pitched_l;
      const float branch_r = (1.f - current_pitch_mix) * held_r + current_pitch_mix * pitched_r;

      const float dynamic_ring_hz = current_ring_hz * (1.f + 0.42f * env_drive);
      ring_phase += dynamic_ring_hz / SAMPLE_RATE;
      if (ring_phase >= 1.f) ring_phase -= 1.f;
      const float ring_l = sinf(TWO_PI * ring_phase);
      const float ring_r = sinf(TWO_PI * ring_phase + HALF_PI);
      const float ring_depth = 0.65f + 0.35f * corruption;
      float metal_l = branch_l * ((1.f - ring_depth) + ring_depth * ring_l);
      float metal_r = branch_r * ((1.f - ring_depth) + ring_depth * ring_r);

      updateGate(target.pattern);
      const float cutoff = 100.f + gate_value * (1800.f + 4200.f * (1.f - corruption));
      const float g = (TWO_PI * cutoff / SAMPLE_RATE) /
                      (1.f + TWO_PI * cutoff / SAMPLE_RATE);
      lpg_l += g * (metal_l - lpg_l);
      lpg_r += g * (metal_r - lpg_r);
      const float gate_gain = 0.012f + 0.988f * gate_value;
      metal_l = softClip(2.2f * gate_gain * lpg_l);
      metal_r = softClip(2.2f * gate_gain * lpg_r);

      const float dry = 1.f - current_wet;
      out[2U * i] = dry * input_l + current_wet * metal_l;
      out[2U * i + 1U] = dry * input_r + current_wet * metal_r;
    }
  }

  float currentRingHz() const { return current_ring_hz; }
  float currentDestruction() const { return current_destruction; }
  float currentEnvelope() const { return envelope; }
  uint32_t detectedAttacks() const { return attack_count; }
  uint8_t currentGateStep() const { return gate_step; }

private:
  static constexpr float TWO_PI = 6.28318530717958647692f;
  static constexpr float HALF_PI = 1.57079632679489661923f;

  static float clamp01(float value)
  { return value < 0.f ? 0.f : (value > 1.f ? 1.f : value); }

  static float softClip(float value)
  { return value / (1.f + fabsf(value)); }

  void updateAttackDetector(float threshold)
  {
    if (!note_active && envelope > threshold)
    {
      note_active = true;
      silence_samples = 0U;
      gate_phase = 0.f;
      gate_step = 0U;
      gate_value = 1.f;
      ++attack_count;
    }
    else if (note_active)
    {
      if (envelope < threshold * 0.38f)
      {
        if (++silence_samples > 2400U) note_active = false;
      }
      else silence_samples = 0U;
    }
  }

  float pitchSample(float *channel, float input)
  {
    buffer_l[write_index] = (channel == buffer_l) ? input : buffer_l[write_index];
    buffer_r[write_index] = (channel == buffer_r) ? input : buffer_r[write_index];
    const float delay_a = sweep_phase * static_cast<float>(WINDOW_LENGTH);
    float phase_b = sweep_phase + 0.5f;
    if (phase_b >= 1.f) phase_b -= 1.f;
    const float delay_b = phase_b * static_cast<float>(WINDOW_LENGTH);
    const float window_b = readWindow(sweep_phase * static_cast<float>(WINDOW_LENGTH));
    const float window_a = 1.f - window_b;
    return window_a * readDelay(channel, delay_a) + window_b * readDelay(channel, delay_b);
  }

  void advancePitch()
  {
    sweep_phase += (1.f - current_ratio) / static_cast<float>(WINDOW_LENGTH);
    while (sweep_phase >= 1.f) sweep_phase -= 1.f;
    while (sweep_phase < 0.f) sweep_phase += 1.f;
    if (++write_index >= CHANNEL_BUFFER_SIZE) write_index = 0U;
  }

  float readDelay(const float *channel, float delay_samples) const
  {
    float position = static_cast<float>(write_index) - delay_samples;
    while (position < 0.f) position += static_cast<float>(CHANNEL_BUFFER_SIZE);
    while (position >= static_cast<float>(CHANNEL_BUFFER_SIZE)) position -= static_cast<float>(CHANNEL_BUFFER_SIZE);
    const uint32_t a = static_cast<uint32_t>(position);
    const uint32_t b = (a + 1U) < CHANNEL_BUFFER_SIZE ? a + 1U : 0U;
    const float fraction = position - static_cast<float>(a);
    return channel[a] + (channel[b] - channel[a]) * fraction;
  }

  float readWindow(float position) const
  {
    const uint32_t a = static_cast<uint32_t>(position);
    const uint32_t b = a + 1U;
    const float fraction = position - static_cast<float>(a);
    return window_table[a] + (window_table[b] - window_table[a]) * fraction;
  }

  void updateGate(uint8_t pattern)
  {
    static const float patterns[4][8] = {
      {1.f, 0.f, 1.f, 0.12f, 1.f, 0.f, 0.35f, 0.f},
      {1.f, 0.f, 0.f, 1.f, 0.2f, 0.f, 1.f, 0.f},
      {1.f, 0.3f, 0.f, 0.65f, 0.f, 1.f, 0.f, 0.18f},
      {1.f, 0.f, 0.55f, 0.f, 0.2f, 0.8f, 0.f, 0.f}
    };
    gate_phase += current_gate_rate / SAMPLE_RATE;
    if (gate_phase >= 1.f)
    {
      gate_phase -= 1.f;
      gate_step = static_cast<uint8_t>((gate_step + 1U) & 7U);
    }
    const float target = patterns[pattern & 3U][gate_step];
    const float coeff = target > gate_value ? 0.018f : 0.0013f;
    gate_value += coeff * (target - gate_value);
  }

  float *buffer_l = nullptr;
  float *buffer_r = nullptr;
  float *window_table = nullptr;
  Params params;
  uint32_t write_index = 0U;
  float sweep_phase = 0.f;
  float ring_phase = 0.f;
  float envelope = 0.f;
  float current_ring_hz = 170.f;
  float current_destruction = 0.75f;
  float current_wet = 1.f;
  float current_ratio = 0.5f;
  float current_pitch_mix = 0.55f;
  float current_gate_rate = 8.f;
  float held_l = 0.f;
  float held_r = 0.f;
  float crush_phase = 0.f;
  float gate_phase = 0.f;
  float gate_value = 1.f;
  uint8_t gate_step = 0U;
  bool note_active = false;
  uint32_t silence_samples = 0U;
  uint32_t attack_count = 0U;
  float lpg_l = 0.f;
  float lpg_r = 0.f;
};

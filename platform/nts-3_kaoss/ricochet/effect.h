#pragma once

#include <math.h>
#include <stdint.h>

#include "processor.h"

class Effect : public Processor
{
public:
  static constexpr float SAMPLE_RATE = 48000.f;
  static constexpr uint32_t MAX_GRAIN_SAMPLES = 5760U;
  static constexpr uint32_t BUFFER_SIZE = 2U * MAX_GRAIN_SAMPLES;
  static constexpr uint32_t MAX_BOUNCES = 12U;

  enum
  {
    BEATS = 0U,
    ACCELERATION,
    DRY_WET,
    DECAY,
    BOUNCES,
    THRESHOLD,
    GRAIN_LENGTH,
    NUM_PARAMS
  };

  struct Params
  {
    float beats;
    float acceleration;
    float mix;
    float decay;
    uint32_t bounces;
    float threshold;
    float grain_ms;

    void reset()
    {
      beats = 1.f;
      acceleration = 0.7f;
      mix = 0.5f;
      decay = 0.68f;
      bounces = 8U;
      threshold = dbToLinear(-28.f);
      grain_ms = 55.f;
    }

    static float dbToLinear(float db)
    {
      return powf(10.f, db * 0.05f);
    }

    Params() { reset(); }
  };

  uint32_t getBufferSize() const override final { return BUFFER_SIZE; }

  void setParameter(uint8_t index, int32_t value) override final
  {
    switch (index)
    {
    case BEATS:
      params.beats = static_cast<float>(value) * 0.01f;
      break;
    case ACCELERATION:
      params.acceleration = static_cast<float>(value) * 0.001f;
      break;
    case DRY_WET:
      params.mix = (static_cast<float>(value) + 1000.f) * 0.0005f;
      break;
    case DECAY:
      params.decay = static_cast<float>(value) * 0.001f;
      break;
    case BOUNCES:
      params.bounces = static_cast<uint32_t>(value);
      break;
    case THRESHOLD:
      params.threshold = Params::dbToLinear(static_cast<float>(value));
      break;
    case GRAIN_LENGTH:
      params.grain_ms = static_cast<float>(value);
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
    grain_l = allocated_buffer;
    grain_r = grain_l ? grain_l + MAX_GRAIN_SAMPLES : nullptr;
    params.reset();
    reset();
  }

  void teardown() override final
  {
    grain_l = nullptr;
    grain_r = nullptr;
  }

  void reset() override final
  {
    current_beats = params.beats;
    current_acceleration = params.acceleration;
    current_mix = params.mix;
    current_decay = params.decay;
    current_threshold = params.threshold;
    current_grain_ms = params.grain_ms;
    current_bpm = target_bpm;

    armed = false;
    arm_pending = false;
    release_pending = false;
    touch_continuous = false;
    triggered_during_touch = false;
    onset_ready = true;
    quiet_samples = 0U;
    envelope = 0.f;
    gesture_active = false;
    capture_active = false;
    capture_position = 0U;
    capture_length = 0U;
    gesture_age = 0U;
    gesture_duration = 0U;
    tap_count = 0U;
    trigger_count = 0U;

    if (grain_l)
      for (uint32_t i = 0U; i < BUFFER_SIZE; ++i)
        grain_l[i] = 0.f;
  }

  void setTempo(float tempo) override final
  {
    if (tempo < 30.f)
      tempo = 30.f;
    else if (tempo > 300.f)
      tempo = 300.f;
    target_bpm = tempo;
  }

  void touchEvent(uint8_t, uint8_t phase, uint32_t, uint32_t) override final
  {
    // Began=0, moved=1, ended=2, stationary=3, cancelled=4.
    if (phase == 0U)
    {
      touch_continuous = true;
      triggered_during_touch = false;
      arm_pending = true;
    }
    else if (phase == 1U || phase == 3U)
    {
      // Stationary refreshes keep NTS-3 hold/freeze operation continuous.
      touch_continuous = true;
      if (!armed && !gesture_active)
        arm_pending = true;
    }
    else if (phase == 2U)
    {
      touch_continuous = false;
      release_pending = true;
    }
    else if (phase == 4U)
    {
      // A UI mode change can cancel the physical lifecycle while freeze keeps
      // the effect active. Preserve the continuous-arm state in that case.
      touch_continuous = true;
    }
  }

  void process(const float *__restrict in,
               float *__restrict out,
               uint32_t frames) override final
  {
    if (!grain_l || !grain_r)
    {
      for (uint32_t i = 0U; i < 2U * frames; ++i)
        out[i] = in[i];
      return;
    }

    const Params target = params;

    for (uint32_t i = 0U; i < frames; ++i)
    {
      smoothControls(target);

      if (arm_pending)
      {
        armed = true;
        onset_ready = envelope < 0.4f * current_threshold;
        quiet_samples = 0U;
        arm_pending = false;
      }

      if (release_pending)
      {
        if (triggered_during_touch)
          armed = false;
        release_pending = false;
      }

      const float input_l = in[2U * i];
      const float input_r = in[2U * i + 1U];
      const float absolute_l = fabsf(input_l);
      const float absolute_r = fabsf(input_r);
      const float peak = absolute_l > absolute_r ? absolute_l : absolute_r;
      const float envelope_coeff = peak > envelope ? 0.2f : 0.001f;
      envelope += envelope_coeff * (peak - envelope);

      const float reset_level = 0.4f * current_threshold;
      if (envelope < reset_level)
      {
        if (quiet_samples < QUIET_SAMPLES_REQUIRED)
          ++quiet_samples;
        if (quiet_samples >= QUIET_SAMPLES_REQUIRED)
          onset_ready = true;
      }
      else
      {
        quiet_samples = 0U;
      }

      if (onset_ready && peak >= current_threshold)
      {
        onset_ready = false;
        quiet_samples = 0U;
        if (armed && !gesture_active)
        {
          startGesture();
          if (touch_continuous)
            triggered_during_touch = true;
          else
            armed = false;
        }
      }

      if (capture_active)
      {
        grain_l[capture_position] = input_l;
        grain_r[capture_position] = input_r;
        if (++capture_position >= capture_length)
          capture_active = false;
      }

      float wet_l = 0.f;
      float wet_r = 0.f;
      if (gesture_active)
      {
        for (uint32_t tap = 0U; tap < tap_count; ++tap)
        {
          if (gesture_age < tap_start[tap])
            continue;
          const uint32_t grain_position = gesture_age - tap_start[tap];
          if (grain_position >= capture_length)
            continue;
          const float window = grainWindow(grain_position, capture_length);
          const float gain = tap_gain[tap] * window;
          wet_l += gain * grain_l[grain_position];
          wet_r += gain * grain_r[grain_position];
        }

        if (++gesture_age >= gesture_duration)
        {
          gesture_active = false;
          capture_active = false;
        }
      }

      wet_l = fastTanh(0.9f * wet_l);
      wet_r = fastTanh(0.9f * wet_r);
      const float dry = 1.f - current_mix;
      out[2U * i] = dry * input_l + current_mix * wet_l;
      out[2U * i + 1U] = dry * input_r + current_mix * wet_r;
    }
  }

  bool isArmed() const { return armed; }
  bool isGestureActive() const { return gesture_active; }
  bool isContinuousTouch() const { return touch_continuous; }
  uint32_t triggerCount() const { return trigger_count; }
  uint32_t tapCount() const { return tap_count; }
  uint32_t tapStart(uint32_t index) const { return tap_start[index]; }
  float tapGain(uint32_t index) const { return tap_gain[index]; }
  uint32_t capturedSamples() const { return capture_length; }
  uint32_t gestureSamples() const { return gesture_duration; }
  float currentMix() const { return current_mix; }

private:
  static constexpr uint32_t QUIET_SAMPLES_REQUIRED = 480U;

  void smoothControls(const Params &target)
  {
    current_beats += 0.001f * (target.beats - current_beats);
    current_acceleration +=
        0.001f * (target.acceleration - current_acceleration);
    current_mix += 0.001f * (target.mix - current_mix);
    current_decay += 0.001f * (target.decay - current_decay);
    current_threshold += 0.001f * (target.threshold - current_threshold);
    current_grain_ms += 0.001f * (target.grain_ms - current_grain_ms);
    current_bpm += 0.001f * (target_bpm - current_bpm);
  }

  void startGesture()
  {
    tap_count = params.bounces;
    if (tap_count < 1U)
      tap_count = 1U;
    else if (tap_count > MAX_BOUNCES)
      tap_count = MAX_BOUNCES;

    gesture_duration = static_cast<uint32_t>(
        current_beats * (60.f / current_bpm) * SAMPLE_RATE + 0.5f);
    if (gesture_duration < 2400U)
      gesture_duration = 2400U;
    else if (gesture_duration > 192000U)
      gesture_duration = 192000U;

    float ratio_power = 1.f;
    float interval_sum = 0.f;
    for (uint32_t tap = 0U; tap < tap_count; ++tap)
    {
      interval_sum += ratio_power;
      ratio_power *= current_acceleration;
    }

    uint32_t requested_grain = static_cast<uint32_t>(
        current_grain_ms * 0.001f * SAMPLE_RATE + 0.5f);
    if (requested_grain > MAX_GRAIN_SAMPLES)
      requested_grain = MAX_GRAIN_SAMPLES;
    const uint32_t schedule_limited_grain = static_cast<uint32_t>(
        static_cast<float>(gesture_duration) / (interval_sum + 1.f));
    capture_length = requested_grain < schedule_limited_grain ?
                     requested_grain : schedule_limited_grain;
    if (capture_length < 32U)
      capture_length = 32U;

    const float available = static_cast<float>(gesture_duration - capture_length);
    const float first_interval = available / interval_sum;
    float interval = first_interval;
    float accumulated = 0.f;
    float gain = 1.f;
    for (uint32_t tap = 0U; tap < tap_count; ++tap)
    {
      accumulated += interval;
      tap_start[tap] = static_cast<uint32_t>(accumulated + 0.5f);
      tap_gain[tap] = gain;
      interval *= current_acceleration;
      gain *= current_decay;
    }

    // Eliminate accumulated rounding error so the last grain ends on the beat.
    tap_start[tap_count - 1U] = gesture_duration - capture_length;
    for (uint32_t i = 0U; i < capture_length; ++i)
    {
      grain_l[i] = 0.f;
      grain_r[i] = 0.f;
    }

    capture_position = 0U;
    capture_active = true;
    gesture_age = 0U;
    gesture_active = true;
    ++trigger_count;
  }

  static float grainWindow(uint32_t position, uint32_t length)
  {
    uint32_t fade = length / 8U;
    if (fade > 24U)
      fade = 24U;
    if (fade < 1U)
      return 1.f;
    if (position < fade)
      return static_cast<float>(position + 1U) / static_cast<float>(fade);
    const uint32_t remaining = length - position;
    if (remaining <= fade)
      return static_cast<float>(remaining) / static_cast<float>(fade);
    return 1.f;
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

  float *grain_l = nullptr;
  float *grain_r = nullptr;
  Params params;

  float target_bpm = 120.f;
  float current_bpm = 120.f;
  float current_beats = 1.f;
  float current_acceleration = 0.7f;
  float current_mix = 0.5f;
  float current_decay = 0.68f;
  float current_threshold = 0.039810717f;
  float current_grain_ms = 55.f;

  volatile bool arm_pending = false;
  volatile bool release_pending = false;
  volatile bool touch_continuous = false;
  bool triggered_during_touch = false;
  bool armed = false;
  bool onset_ready = true;
  uint32_t quiet_samples = 0U;
  float envelope = 0.f;

  bool gesture_active = false;
  bool capture_active = false;
  uint32_t capture_position = 0U;
  uint32_t capture_length = 0U;
  uint32_t gesture_age = 0U;
  uint32_t gesture_duration = 0U;
  uint32_t tap_count = 0U;
  uint32_t tap_start[MAX_BOUNCES] = {};
  float tap_gain[MAX_BOUNCES] = {};
  uint32_t trigger_count = 0U;
};

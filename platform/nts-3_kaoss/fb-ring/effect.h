#pragma once

/*
 * Native NTS-3 version of the Pd feedback/ring-mod patch.
 *
 * X     : delay time, 1..1000 ms
 * Y     : ring modulator MIDI pitch, 1..104
 * Depth : feedback gain, 0.000..2.000
 *
 * A new touch fires one excitation.
 * Moving, holding and releasing do not retrigger.
 */

#include <stdint.h>
#include <stddef.h>

#include "processor.h"
#include "unit_genericfx.h"
#include "osc_api.h"

class Effect : public Processor
{
public:
  static constexpr uint32_t DELAY_BUFFER_SIZE = 48002U;

  enum
  {
    DELAY = 0U,
    MOD_PITCH,
    GAIN,
    NUM_PARAMS
  };

  struct Params
  {
    float delay_ms;
    uint8_t mod_note;
    float gain;

    void reset()
    {
      delay_ms = 344.f;
      mod_note = 60;      // C4 ~= 261.626 Hz
      gain = 1.921f;
    }

    Params() { reset(); }
  };

  uint32_t getBufferSize() const override final
  {
    // unit.cc interprets this as a number of floats.
    return DELAY_BUFFER_SIZE;
  }

  inline void setParameter(uint8_t index, int32_t value) override final
  {
    switch (index)
    {
    case DELAY:
      params.delay_ms = static_cast<float>(value);
      break;

    case MOD_PITCH:
      params.mod_note = static_cast<uint8_t>(value);
      break;

    case GAIN:
      params.gain = static_cast<float>(value) * 0.001f;
      break;

    default:
      break;
    }
  }

  inline const char *getParameterStrValue(uint8_t index,
                                          int32_t value) const override final
  {
    (void)index;
    (void)value;
    return nullptr;
  }

  void init(float *allocated_buffer) override final
  {
    buffer = allocated_buffer;

    params.reset();

    write_index = 0U;

    carrier_phase = 0.f;
    mod_phase = 0.f;

    hp_x1 = 0.f;
    hp_y1 = 0.f;

    env = 0.f;
    env_start = 0.f;
    env_age = 0U;
    env_active = false;

    trigger_pending = false;
  }

  void teardown() override final
  {
    buffer = nullptr;
  }

  void reset() override final
  {
    write_index = 0U;

    carrier_phase = 0.f;
    mod_phase = 0.f;

    hp_x1 = 0.f;
    hp_y1 = 0.f;

    env = 0.f;
    env_start = 0.f;
    env_age = 0U;
    env_active = false;

    trigger_pending = false;

    if (buffer)
    {
      for (uint32_t i = 0; i < DELAY_BUFFER_SIZE; ++i)
        buffer[i] = 0.f;
    }
  }

  void process(const float *__restrict in,
               float *__restrict out,
               uint32_t frames) override final
  {
    (void)in;

    // A touch event only requests a trigger.
    // We start it here in the audio callback.
    if (trigger_pending)
    {
      trigger_pending = false;
      trigger();
    }

    const Params p = params;

    // Y behaves like Pd [mtof] -> [osc~].
    const float mod_hz = osc_notehzf(p.mod_note);

    const float carrier_inc = 262.f / 48000.f;
    const float mod_inc = mod_hz / 48000.f;

    // 1 ms = 48 samples at 48 kHz.
    float delay_samples = p.delay_ms * 48.f;

    if (delay_samples < 48.f)
      delay_samples = 48.f;

    if (delay_samples > 48000.f)
      delay_samples = 48000.f;

    for (uint32_t i = 0; i < frames; ++i)
    {
      // ------------------------------------------------------------
      // [delread~ feedback]
      // ------------------------------------------------------------

      const float delayed_raw = readDelay(delay_samples);

      // ------------------------------------------------------------
      // [hip~ 40]
      //
      // Simple one-pole 40 Hz highpass.
      // exp(-2*pi*40/48000) ~= 0.99478
      // ------------------------------------------------------------

      const float delayed =
          0.99478f * (hp_y1 + delayed_raw - hp_x1);

      hp_x1 = delayed_raw;
      hp_y1 = delayed;

      // ------------------------------------------------------------
      // [osc~ 262] * excitation envelope
      // ------------------------------------------------------------

      const float carrier = osc_sinf(carrier_phase);
      carrier_phase += carrier_inc;

      if (carrier_phase >= 1.f)
        carrier_phase -= 1.f;

      const float beep = carrier * nextEnvelopeSample();

      // ------------------------------------------------------------
      // feedback output + beep
      // ------------------------------------------------------------

      const float mixed = delayed + beep;

      // ------------------------------------------------------------
      // modulator:
      // Y MIDI note -> Hz -> sine oscillator
      // ------------------------------------------------------------

      const float modulator = osc_sinf(mod_phase);
      mod_phase += mod_inc;

      if (mod_phase >= 1.f)
        mod_phase -= 1.f;

      // ------------------------------------------------------------
      // ring modulation
      // ------------------------------------------------------------

      float x = mixed * modulator;

      // [clip~ -1 1]
      if (x > 1.f)
        x = 1.f;
      else if (x < -1.f)
        x = -1.f;

      // [*~ -1]
      x = -x;

      // [expr~ $v1 - 1/3 * pow($v1,3)]
      const float shaped =
          x - 0.333333333333f * x * x * x;

      // Depth / GAIN
      const float feedback_write = shaped * p.gain;

      // [delwrite~ feedback]
      buffer[write_index] = feedback_write;

      ++write_index;
      if (write_index >= DELAY_BUFFER_SIZE)
        write_index = 0U;

      // Original desktop patch outputs the highpassed delay,
      // attenuated by 0.1, not the excitation directly.
      const float y = delayed * 0.1f;

      out[2U * i] = y;
      out[2U * i + 1U] = y;
    }
  }

  inline void touchEvent(uint8_t id,
                         uint8_t phase,
                         uint32_t x,
                         uint32_t y) override final
  {
    (void)id;
    (void)x;
    (void)y;

    // This is the native equivalent of our Pd "bang".
    //
    // ONLY a genuinely new finger-down triggers the sound.
    // moved/stationary/ended/cancelled do nothing.
    if (phase == k_unit_touch_phase_began)
      trigger_pending = true;
  }

private:
  // ------------------------------------------------------------
  // Desktop patch excitation:
  //
  // bang:
  //   [0.5 100(
  //
  // and after 500 ms:
  //   [0 100(
  //
  // Result:
  //   0-100 ms   ramp toward 0.5
  //   100-500 ms hold 0.5
  //   500-600 ms ramp to 0
  // ------------------------------------------------------------

  void trigger()
  {
    env_start = env;
    env_age = 0U;
    env_active = true;
  }

  inline float nextEnvelopeSample()
  {
    if (!env_active)
      return env;

    static constexpr uint32_t ATTACK_END  = 4800U;  // 100 ms
    static constexpr uint32_t RELEASE_BEG = 24000U; // 500 ms
    static constexpr uint32_t RELEASE_END = 28800U; // 600 ms

    if (env_age < ATTACK_END)
    {
      const float t =
          static_cast<float>(env_age) /
          static_cast<float>(ATTACK_END);

      env = env_start + (0.5f - env_start) * t;
    }
    else if (env_age < RELEASE_BEG)
    {
      env = 0.5f;
    }
    else if (env_age < RELEASE_END)
    {
      const float t =
          static_cast<float>(env_age - RELEASE_BEG) /
          static_cast<float>(RELEASE_END - RELEASE_BEG);

      env = 0.5f * (1.f - t);
    }
    else
    {
      env = 0.f;
      env_active = false;
    }

    ++env_age;
    return env;
  }

  // Linear interpolated variable delay reader.
  inline float readDelay(float delay_samples) const
  {
    float pos =
        static_cast<float>(write_index) - delay_samples;

    if (pos < 0.f)
      pos += static_cast<float>(DELAY_BUFFER_SIZE);

    uint32_t i0 = static_cast<uint32_t>(pos);
    uint32_t i1 = i0 + 1U;

    if (i1 >= DELAY_BUFFER_SIZE)
      i1 = 0U;

    const float frac = pos - static_cast<float>(i0);

    const float a = buffer[i0];
    const float b = buffer[i1];

    return a + (b - a) * frac;
  }

  float *buffer = nullptr;

  Params params;

  uint32_t write_index = 0U;

  float carrier_phase = 0.f;
  float mod_phase = 0.f;

  float hp_x1 = 0.f;
  float hp_y1 = 0.f;

  float env = 0.f;
  float env_start = 0.f;
  uint32_t env_age = 0U;
  bool env_active = false;

  volatile bool trigger_pending = false;
};
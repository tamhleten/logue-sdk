#include "effect.h"

#include <cmath>
#include <cstdio>
#include <vector>

static constexpr uint32_t BLOCK = 64U;
static constexpr float PI = 3.14159265358979323846f;

static void process(Effect &effect, const std::vector<float> &input,
                    std::vector<float> &output)
{
  output.assign(input.size(), 0.f);
  const uint32_t total = static_cast<uint32_t>(input.size() / 2U);
  for (uint32_t base = 0U; base < total; base += BLOCK)
  {
    const uint32_t remaining = total - base;
    const uint32_t frames = remaining < BLOCK ? remaining : BLOCK;
    effect.process(input.data() + 2U * base,
                   output.data() + 2U * base, frames);
  }
}

static void addAttack(std::vector<float> &audio, uint32_t start,
                      uint32_t length, float amplitude = 0.6f)
{
  for (uint32_t i = 0U; i < length; ++i)
  {
    const float fade = i < 48U ? static_cast<float>(i) / 48.f : 1.f;
    const float sample = amplitude * fade *
                         sinf(2.f * PI * 146.83f * i / 48000.f);
    audio[2U * (start + i)] = sample;
    audio[2U * (start + i) + 1U] = sample;
  }
}

static void configureForTest(Effect &effect)
{
  effect.setParameter(Effect::BEATS, 50);
  effect.setParameter(Effect::ACCELERATION, 700);
  effect.setParameter(Effect::DRY_WET, 1000);
  effect.setParameter(Effect::DECAY, 680);
  effect.setParameter(Effect::BOUNCES, 8);
  effect.setParameter(Effect::THRESHOLD, -40);
  effect.setParameter(Effect::GRAIN_LENGTH, 20);
  effect.reset();
}

static bool dryTest()
{
  Effect effect;
  std::vector<float> memory(effect.getBufferSize(), 0.f);
  effect.init(memory.data());
  effect.setParameter(Effect::DRY_WET, -1000);
  effect.reset();

  std::vector<float> input(2U * 4096U);
  for (uint32_t i = 0U; i < input.size(); ++i)
    input[i] = 0.35f * sinf(0.017f * static_cast<float>(i));
  std::vector<float> output;
  process(effect, input, output);
  for (uint32_t i = 0U; i < input.size(); ++i)
    if (fabsf(input[i] - output[i]) > 0.000001f)
      return false;
  return true;
}

static bool oneShotTest()
{
  Effect effect;
  std::vector<float> memory(effect.getBufferSize(), 0.f);
  effect.init(memory.data());
  configureForTest(effect);

  effect.touchEvent(0U, 0U, 500U, 500U);
  effect.touchEvent(0U, 2U, 500U, 500U);
  std::vector<float> input(2U * 50000U, 0.f);
  addAttack(input, 0U, 1200U);
  addAttack(input, 30000U, 1200U);
  std::vector<float> output;
  process(effect, input, output);

  double echo_energy = 0.0;
  for (uint32_t i = 1500U; i < 14000U; ++i)
    echo_energy += output[2U * i] * output[2U * i] +
                   output[2U * i + 1U] * output[2U * i + 1U];
  std::printf("one-shot triggers=%u echo-energy=%.4f armed=%u\n",
              effect.triggerCount(), echo_energy, effect.isArmed() ? 1U : 0U);
  return effect.triggerCount() == 1U && !effect.isArmed() && echo_energy > 0.01;
}

static bool heldAndFreezeTest()
{
  Effect held;
  std::vector<float> held_memory(held.getBufferSize(), 0.f);
  held.init(held_memory.data());
  configureForTest(held);
  held.touchEvent(0U, 0U, 500U, 500U);

  std::vector<float> input(2U * 50000U, 0.f), output;
  addAttack(input, 0U, 1200U);
  addAttack(input, 30000U, 1200U);
  process(held, input, output);
  const bool held_repeats = held.triggerCount() == 2U && held.isArmed();
  held.touchEvent(0U, 2U, 500U, 500U);
  std::vector<float> silence(2U * 64U, 0.f);
  process(held, silence, output);
  const bool release_disarms = !held.isArmed();

  Effect frozen;
  std::vector<float> frozen_memory(frozen.getBufferSize(), 0.f);
  frozen.init(frozen_memory.data());
  configureForTest(frozen);
  frozen.touchEvent(0U, 0U, 500U, 500U);
  frozen.touchEvent(0U, 4U, 500U, 500U);
  process(frozen, input, output);
  const bool freeze_repeats = frozen.triggerCount() == 2U &&
                              frozen.isContinuousTouch() && frozen.isArmed();
  return held_repeats && release_disarms && freeze_repeats;
}

static bool scheduleTest()
{
  Effect effect;
  std::vector<float> memory(effect.getBufferSize(), 0.f);
  effect.init(memory.data());
  configureForTest(effect);
  effect.touchEvent(0U, 0U, 500U, 500U);
  effect.touchEvent(0U, 2U, 500U, 500U);

  std::vector<float> input(2U * 2048U, 0.f), output;
  addAttack(input, 0U, 1200U);
  process(effect, input, output);
  if (effect.triggerCount() != 1U || effect.tapCount() != 8U)
    return false;

  uint32_t previous_interval = effect.tapStart(0U);
  for (uint32_t tap = 1U; tap < effect.tapCount(); ++tap)
  {
    const uint32_t interval = effect.tapStart(tap) - effect.tapStart(tap - 1U);
    if (interval > previous_interval + 1U)
      return false;
    if (!(effect.tapGain(tap) < effect.tapGain(tap - 1U)))
      return false;
    previous_interval = interval;
  }
  return effect.tapStart(effect.tapCount() - 1U) + effect.capturedSamples() ==
         effect.gestureSamples();
}

static bool smoothingTest()
{
  Effect effect;
  std::vector<float> memory(effect.getBufferSize(), 0.f);
  effect.init(memory.data());
  effect.setParameter(Effect::DRY_WET, 1000);
  std::vector<float> silence(2U * 128U, 0.f), output;
  process(effect, silence, output);
  const bool gliding = effect.currentMix() > 0.5f && effect.currentMix() < 1.f;
  silence.assign(2U * 20000U, 0.f);
  process(effect, silence, output);
  return gliding && fabsf(effect.currentMix() - 1.f) < 0.001f;
}

static bool stabilityTest()
{
  Effect effect;
  std::vector<float> memory(effect.getBufferSize(), 0.f);
  effect.init(memory.data());
  effect.setParameter(Effect::THRESHOLD, -48);
  effect.setParameter(Effect::DRY_WET, 1000);
  effect.setParameter(Effect::BOUNCES, 12);
  effect.setParameter(Effect::DECAY, 900);
  effect.reset();
  effect.touchEvent(0U, 0U, 500U, 500U);

  std::vector<float> input(2U * 120000U, 0.f), output;
  addAttack(input, 0U, 4800U, 2.5f);
  addAttack(input, 60000U, 4800U, 2.5f);
  process(effect, input, output);
  for (float sample : output)
    if (!std::isfinite(sample) || fabsf(sample) > 1.001f)
      return false;
  return true;
}

int main()
{
  const bool dry = dryTest();
  const bool one_shot = oneShotTest();
  const bool held_freeze = heldAndFreezeTest();
  const bool schedule = scheduleTest();
  const bool smoothing = smoothingTest();
  const bool stability = stabilityTest();
  std::printf("dry=%s one-shot=%s held/freeze=%s schedule=%s smoothing=%s stability=%s\n",
              dry ? "ok" : "fail", one_shot ? "ok" : "fail",
              held_freeze ? "ok" : "fail", schedule ? "ok" : "fail",
              smoothing ? "ok" : "fail", stability ? "ok" : "fail");
  return dry && one_shot && held_freeze && schedule && smoothing && stability ?
         0 : 1;
}

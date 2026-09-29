#include "effect.h"
#include <cmath>
#include <cstdio>
#include <vector>

static constexpr unsigned BLOCK = 64U;

static void process(Effect &effect, const std::vector<float> &input,
                    std::vector<float> &output)
{
  output.assign(input.size(), 0.f);
  const unsigned total = static_cast<unsigned>(input.size() / 2U);
  for (unsigned base = 0; base < total; base += BLOCK)
  {
    const unsigned frames = total - base < BLOCK ? total - base : BLOCK;
    effect.process(input.data() + 2U * base, output.data() + 2U * base, frames);
  }
}

static bool dryTest()
{
  Effect effect;
  std::vector<float> memory(effect.getBufferSize(), 0.f);
  effect.init(memory.data());
  effect.setParameter(Effect::DRY_WET, -1000);
  effect.reset();
  std::vector<float> input(2U * 4096U);
  for (unsigned i = 0; i < input.size(); ++i)
    input[i] = 0.4f * std::sin(0.031f * static_cast<float>(i));
  std::vector<float> output;
  process(effect, input, output);
  for (unsigned i = 0; i < input.size(); ++i)
    if (std::fabs(input[i] - output[i]) > 0.000001f) return false;
  return true;
}

static bool saxResponseTest()
{
  Effect effect;
  std::vector<float> memory(effect.getBufferSize(), 0.f);
  effect.init(memory.data());
  effect.reset();
  const unsigned frames = 48000U;
  std::vector<float> input(2U * frames, 0.f);
  for (unsigned i = 0; i < 36000U; ++i)
  {
    const float sample = 0.35f * std::sin(6.28318530718f * 110.f * i / 48000.f);
    input[2U * i] = sample;
    input[2U * i + 1U] = sample;
  }
  std::vector<float> output;
  process(effect, input, output);
  double energy = 0.0;
  double difference = 0.0;
  for (unsigned i = 0; i < 2U * 36000U; ++i)
  {
    energy += output[i] * output[i];
    difference += std::fabs(output[i] - input[i]);
  }
  std::printf("bari energy=%.3f difference=%.3f env=%.4f attacks=%u\n",
              energy, difference, effect.currentEnvelope(), effect.detectedAttacks());
  return energy > 1.0 && difference > 100.0 && effect.detectedAttacks() == 1U;
}

static bool retriggerTest()
{
  Effect effect;
  std::vector<float> memory(effect.getBufferSize(), 0.f);
  effect.init(memory.data());
  effect.reset();
  std::vector<float> input(2U * 50000U, 0.f);
  for (unsigned i = 0; i < 5000U; ++i)
  {
    input[2U * i] = input[2U * i + 1U] = 0.5f;
    input[2U * (35000U + i)] = input[2U * (35000U + i) + 1U] = 0.5f;
  }
  std::vector<float> output;
  process(effect, input, output);
  return effect.detectedAttacks() == 2U;
}

static bool smoothingTest()
{
  Effect effect;
  std::vector<float> memory(effect.getBufferSize(), 0.f);
  effect.init(memory.data());
  effect.setParameter(Effect::RING_FREQUENCY, 600);
  effect.setParameter(Effect::DESTRUCTION, 0);
  std::vector<float> input(2U * 128U, 0.f), output;
  process(effect, input, output);
  const bool gliding = effect.currentRingHz() > 170.f && effect.currentRingHz() < 600.f &&
                       effect.currentDestruction() > 0.f && effect.currentDestruction() < 0.75f;
  input.assign(2U * 20000U, 0.f);
  process(effect, input, output);
  const bool settled = std::fabs(effect.currentRingHz() - 600.f) < 0.1f &&
                       effect.currentDestruction() < 0.001f;
  return gliding && settled;
}

static bool stabilityTest()
{
  Effect effect;
  std::vector<float> memory(effect.getBufferSize(), 0.f);
  effect.init(memory.data());
  effect.setParameter(Effect::DESTRUCTION, 1000);
  effect.setParameter(Effect::DRY_WET, 1000);
  effect.reset();
  std::vector<float> input(2U * 120000U, 0.f);
  for (unsigned i = 0; i < input.size(); ++i)
    input[i] = 2.5f * std::sin(0.017f * static_cast<float>(i));
  std::vector<float> output;
  process(effect, input, output);
  for (float sample : output)
    if (!std::isfinite(sample) || std::fabs(sample) >= 1.f) return false;
  return true;
}

int main()
{
  const bool dry = dryTest();
  const bool sax = saxResponseTest();
  const bool retrigger = retriggerTest();
  const bool smoothing = smoothingTest();
  const bool stability = stabilityTest();
  std::printf("dry=%s sax=%s retrigger=%s smoothing=%s stability=%s\n",
              dry ? "ok" : "fail", sax ? "ok" : "fail",
              retrigger ? "ok" : "fail", smoothing ? "ok" : "fail",
              stability ? "ok" : "fail");
  return dry && sax && retrigger && smoothing && stability ? 0 : 1;
}

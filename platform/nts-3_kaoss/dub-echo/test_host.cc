#include "effect.h"
#include <cmath>
#include <cstdio>
#include <vector>

static constexpr unsigned BLOCK = 64U;

static void process(Effect &effect, const std::vector<float> &input,
                    std::vector<float> &output)
{
  output.assign(input.size(), 0.f);
  const unsigned total_frames = static_cast<unsigned>(input.size() / 2U);
  for (unsigned base = 0; base < total_frames; base += BLOCK)
  {
    const unsigned frames = total_frames - base < BLOCK ?
                            total_frames - base : BLOCK;
    effect.process(input.data() + 2U * base,
                   output.data() + 2U * base, frames);
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
    if (std::fabs(input[i] - output[i]) > 0.000001f)
      return false;
  return true;
}

static bool crossEchoTest()
{
  Effect effect;
  std::vector<float> memory(effect.getBufferSize(), 0.f);
  effect.init(memory.data());
  effect.setParameter(Effect::LENGTH, 100);
  effect.setParameter(Effect::SEPARATION, 0);
  effect.setParameter(Effect::DRY_WET, 1000);
  effect.setParameter(Effect::FEEDBACK, 700);
  effect.reset();

  const unsigned frames = 15000U;
  std::vector<float> input(2U * frames, 0.f);
  input[0] = 0.8f;
  std::vector<float> output;
  process(effect, input, output);

  double first_right = 0.0;
  double first_left = 0.0;
  double second_left = 0.0;
  for (unsigned i = 4750U; i < 5100U; ++i)
  {
    first_left += std::fabs(output[2U * i]);
    first_right += std::fabs(output[2U * i + 1U]);
  }
  for (unsigned i = 9550U; i < 9950U; ++i)
    second_left += std::fabs(output[2U * i]);

  std::printf("cross echoes L1=%.4f R1=%.4f L2=%.4f\n",
              first_left, first_right, second_left);
  return first_right > 0.01 && first_right > 5.0 * first_left &&
         second_left > 0.001;
}

static bool smoothingTest()
{
  Effect effect;
  std::vector<float> memory(effect.getBufferSize(), 0.f);
  effect.init(memory.data());
  effect.setParameter(Effect::LENGTH, 100);
  effect.setParameter(Effect::SEPARATION, 300);
  effect.setParameter(Effect::DRY_WET, 1000);
  effect.setParameter(Effect::FEEDBACK, 980);

  std::vector<float> input(2U * 128U, 0.f);
  std::vector<float> output;
  process(effect, input, output);
  const bool still_gliding = effect.currentDelaySamples() > 4800.f &&
                             effect.currentDelaySamples() < 36000.f &&
                             effect.currentSeparationSamples() > 57.6f &&
                             effect.currentSeparationSamples() < 1440.f &&
                             effect.currentMix() > 0.5f &&
                             effect.currentMix() < 1.f &&
                             effect.currentFeedback() > 0.7f &&
                             effect.currentFeedback() < 0.98f;

  input.assign(2U * 30000U, 0.f);
  process(effect, input, output);
  const bool settled = std::fabs(effect.currentDelaySamples() - 4800.f) < 1.f &&
                       std::fabs(effect.currentSeparationSamples() - 1440.f) < 1.f &&
                       std::fabs(effect.currentMix() - 1.f) < 0.001f &&
                       std::fabs(effect.currentFeedback() - 0.98f) < 0.001f;
  return still_gliding && settled;
}

static bool stabilityTest()
{
  Effect effect;
  std::vector<float> memory(effect.getBufferSize(), 0.f);
  effect.init(memory.data());
  effect.setParameter(Effect::LENGTH, 40);
  effect.setParameter(Effect::SEPARATION, 300);
  effect.setParameter(Effect::DRY_WET, 1000);
  effect.setParameter(Effect::FEEDBACK, 980);
  effect.reset();

  std::vector<float> input(2U * 240000U, 0.f);
  input[0] = 2.f;
  input[1] = -2.f;
  std::vector<float> output;
  process(effect, input, output);
  for (float sample : output)
    if (!std::isfinite(sample) || std::fabs(sample) > 1.01f)
      return false;
  return true;
}

int main()
{
  const bool dry_ok = dryTest();
  const bool cross_ok = crossEchoTest();
  const bool smoothing_ok = smoothingTest();
  const bool stability_ok = stabilityTest();
  std::printf("dry=%s cross=%s smoothing=%s stability=%s\n",
              dry_ok ? "ok" : "fail",
              cross_ok ? "ok" : "fail",
              smoothing_ok ? "ok" : "fail",
              stability_ok ? "ok" : "fail");
  return dry_ok && cross_ok && smoothing_ok && stability_ok ? 0 : 1;
}

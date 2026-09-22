#include "effect.h"
#include <cmath>
#include <cstdio>
#include <vector>

static float measure(int shift_position, int window_length = Effect::DELAY_LENGTH)
{
  Effect effect;
  std::vector<float> memory(effect.getBufferSize(), 0.f);
  effect.init(memory.data());
  effect.setParameter(Effect::SHIFT, shift_position);
  effect.setParameter(Effect::WINDOW_LENGTH, window_length);
  effect.setParameter(Effect::WET_LEVEL, 1000);

  const unsigned frames = 64;
  std::vector<float> input(2 * frames), output(2 * frames);
  float previous = 0.f;
  unsigned crossings = 0;
  const unsigned total = 3 * 48000;
  const unsigned measure_from = 2 * 48000;

  for (unsigned base = 0; base < total; base += frames)
  {
    for (unsigned i = 0; i < frames; ++i)
    {
      const float x = std::sin(2.f * 3.14159265358979323846f * 440.f *
                               static_cast<float>(base + i) / 48000.f);
      input[2 * i] = x;
      input[2 * i + 1] = x;
    }
    effect.process(input.data(), output.data(), frames);
    for (unsigned i = 0; i < frames; ++i)
    {
      const unsigned sample = base + i;
      const float x = output[2 * i];
      if (sample >= measure_from && previous <= 0.f && x > 0.f)
        ++crossings;
      previous = x;
    }
  }
  return static_cast<float>(crossings);
}

static bool dryWetMixOk()
{
  Effect dry_effect;
  Effect half_effect;
  Effect wet_effect;
  std::vector<float> dry_memory(dry_effect.getBufferSize(), 0.f);
  std::vector<float> half_memory(half_effect.getBufferSize(), 0.f);
  std::vector<float> wet_memory(wet_effect.getBufferSize(), 0.f);
  dry_effect.init(dry_memory.data());
  half_effect.init(half_memory.data());
  wet_effect.init(wet_memory.data());
  dry_effect.setParameter(Effect::WET_LEVEL, 0);
  half_effect.setParameter(Effect::WET_LEVEL, 500);
  wet_effect.setParameter(Effect::WET_LEVEL, 1000);

  const unsigned frames = 64;
  std::vector<float> input(2 * frames);
  std::vector<float> dry_output(2 * frames);
  std::vector<float> half_output(2 * frames);
  std::vector<float> wet_output(2 * frames);
  for (unsigned i = 0; i < 2 * frames; ++i)
    input[i] = std::sin(0.17f * static_cast<float>(i));

  dry_effect.process(input.data(), dry_output.data(), frames);
  half_effect.process(input.data(), half_output.data(), frames);
  wet_effect.process(input.data(), wet_output.data(), frames);

  for (unsigned i = 0; i < 2 * frames; ++i)
  {
    if (std::fabs(dry_output[i] - input[i]) > 0.000001f)
      return false;
    const float expected_half = 0.5f * (input[i] + wet_output[i]);
    if (std::fabs(half_output[i] - expected_half) > 0.000001f)
      return false;
  }
  return true;
}

int main()
{
  const float down = measure(-500);
  const float unity = measure(0);
  const float up = measure(333);
  const float short_window_up = measure(333, Effect::MIN_WINDOW_LENGTH);
  std::printf("0.5x: %.0f Hz, 1x: %.0f Hz, 2x: %.0f Hz, "
              "2x short window: %.0f Hz\n",
              down, unity, up, short_window_up);
  const bool mapping_ok = std::fabs(Effect::shiftRatio(-1000) - 0.25f) < 0.0001f &&
                          std::fabs(Effect::shiftRatio(0) - 1.f) < 0.0001f &&
                          std::fabs(Effect::shiftRatio(1000) - 8.f) < 0.0001f;
  return (mapping_ok && dryWetMixOk() && down > 210.f && down < 230.f &&
          unity > 430.f && unity < 450.f &&
          up > 860.f && up < 900.f &&
          short_window_up > 800.f && short_window_up < 950.f) ? 0 : 1;
}

#include "effect.h"
#include <cmath>
#include <cstdio>
#include <vector>

static float measure(int ratio_hundredths)
{
  Effect effect;
  std::vector<float> memory(effect.getBufferSize(), 0.f);
  effect.init(memory.data());
  effect.setParameter(Effect::SHIFT, ratio_hundredths);
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

int main()
{
  const float down = measure(50);
  const float unity = measure(100);
  const float up = measure(200);
  std::printf("0.5x: %.0f Hz, 1x: %.0f Hz, 2x: %.0f Hz\n", down, unity, up);
  return (down > 210.f && down < 230.f &&
          unity > 430.f && unity < 450.f &&
          up > 860.f && up < 900.f) ? 0 : 1;
}

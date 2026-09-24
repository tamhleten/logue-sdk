#include "effect.h"
#include <cmath>
#include <cstdio>
#include <vector>

static constexpr unsigned BLOCK = 64U;

static void processTone(Effect &effect, float frequency, float amplitude,
                        unsigned samples, unsigned &clock,
                        double *output_energy = nullptr,
                        unsigned *crossings = nullptr)
{
  std::vector<float> input(2U * BLOCK, 0.f);
  std::vector<float> output(2U * BLOCK, 0.f);
  float previous = 0.f;
  for (unsigned done = 0; done < samples; done += BLOCK)
  {
    const unsigned frames = samples - done < BLOCK ? samples - done : BLOCK;
    for (unsigned i = 0; i < frames; ++i)
    {
      const float sample = amplitude * std::sin(
          2.f * 3.14159265358979323846f * frequency *
          static_cast<float>(clock + i) / 48000.f);
      input[2U * i] = sample;
      input[2U * i + 1U] = sample;
    }
    effect.process(input.data(), output.data(), frames);
    for (unsigned i = 0; i < frames; ++i)
    {
      const float sample = output[2U * i];
      if (output_energy)
        *output_energy += static_cast<double>(sample) * sample;
      if (crossings && previous <= 0.f && sample > 0.f)
        ++*crossings;
      previous = sample;
    }
    clock += frames;
  }
}

static bool waitForPlayback(Effect &effect, unsigned &clock)
{
  const unsigned limit = Effect::END_HOLD_SAMPLES + 12000U;
  for (unsigned waited = 0; waited < limit; waited += BLOCK)
  {
    processTone(effect, 440.f, 0.f, BLOCK, clock);
    if (effect.isPlaybackActive())
      return true;
  }
  return false;
}

static bool automaticReplayTest()
{
  Effect effect;
  std::vector<float> memory(effect.getBufferSize(), 0.f);
  effect.init(memory.data());
  effect.setParameter(Effect::PITCH, 0);
  effect.setParameter(Effect::MEMORY, 500);
  effect.setParameter(Effect::SHADOW_LEVEL, 1000);
  unsigned clock = 0U;

  effect.touchEvent(0, 0U, 0, 0);
  processTone(effect, 440.f, 0.f, BLOCK, clock);
  if (effect.isHoldEnabled())
    return false;

  processTone(effect, 440.f, 0.25f, 48000U, clock);
  const bool started = waitForPlayback(effect, clock);
  std::printf("capture: active=%d playback=%d samples=%u\n",
              effect.isPhraseActive(), effect.isPlaybackActive(),
              effect.capturedSamples());
  if (!started)
    return false;
  if (effect.capturedSamples() < 23000U || effect.capturedSamples() > 24100U)
    return false;

  double energy = 0.0;
  processTone(effect, 440.f, 0.f, 24000U, clock, &energy);
  std::printf("replay energy: %.3f\n", energy);
  return energy > 10.0;
}

static bool pitchTest()
{
  Effect effect;
  std::vector<float> memory(effect.getBufferSize(), 0.f);
  effect.init(memory.data());
  effect.setParameter(Effect::PITCH, 1200);
  effect.setParameter(Effect::MEMORY, 1000);
  effect.setParameter(Effect::SHADOW_LEVEL, 1000);
  unsigned clock = 0U;

  processTone(effect, 440.f, 0.25f, 48000U, clock);
  if (!waitForPlayback(effect, clock))
    return false;
  processTone(effect, 440.f, 0.f, 4096U, clock);
  unsigned crossings = 0U;
  processTone(effect, 440.f, 0.f, 24000U, clock, nullptr, &crossings);
  const float measured = 2.f * static_cast<float>(crossings);
  std::printf("afterimage +12 semitones: %.0f Hz\n", measured);
  return measured > 820.f && measured < 940.f;
}

static bool holdAndDuckTest()
{
  Effect effect;
  std::vector<float> memory(effect.getBufferSize(), 0.f);
  effect.init(memory.data());
  effect.setParameter(Effect::PITCH, 0);
  effect.setParameter(Effect::MEMORY, 250);
  effect.setParameter(Effect::SHADOW_LEVEL, 1000);
  unsigned clock = 0U;

  processTone(effect, 220.f, 0.25f, 16000U, clock);
  if (!waitForPlayback(effect, clock))
    return false;
  effect.touchEvent(0, 0U, 0, 0);
  processTone(effect, 220.f, 0.f, 64U, clock);
  if (!effect.isHoldEnabled())
    return false;

  processTone(effect, 220.f, 0.f, 30000U, clock);
  if (!effect.isPlaybackActive())
    return false;

  double live_energy = 0.0;
  processTone(effect, 330.f, 0.25f, 12000U, clock, &live_energy);
  return effect.isPlaybackActive() && effect.isPhraseActive() == false &&
         effect.currentDuckGain() < 0.05f && live_energy > 100.0;
}

int main()
{
  const bool ratio_ok = std::fabs(Effect::pitchRatio(-1200) - 0.5f) < 0.0001f &&
                        std::fabs(Effect::pitchRatio(0) - 1.f) < 0.0001f &&
                        std::fabs(Effect::pitchRatio(1200) - 2.f) < 0.0001f;
  const bool replay_ok = automaticReplayTest();
  const bool pitch_ok = pitchTest();
  const bool hold_ok = holdAndDuckTest();
  std::printf("ratio=%s replay=%s pitch=%s hold=%s\n",
              ratio_ok ? "ok" : "fail",
              replay_ok ? "ok" : "fail",
              pitch_ok ? "ok" : "fail",
              hold_ok ? "ok" : "fail");
  return ratio_ok && replay_ok && pitch_ok && hold_ok ? 0 : 1;
}

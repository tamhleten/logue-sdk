#pragma once
#include <stdint.h>
#include <math.h>
#include "processor.h"

// NTS-3 MIDI CC MSB/LSB resolution diagnostic.
// X/Y/DEPTH values are supplied by the NTS-3 host's 14-bit mappings.
// It is NOT a raw MIDI receiver: any information already discarded by the
// NTS-3 firmware cannot be recovered here.
class Effect : public Processor {
public:
  enum { X_VALUE = 0, Y_VALUE, DEPTH_VALUE, SELECT_PAIR,
         AUDITION, OUTPUT_LEVEL, NUM_PARAMS };
  static constexpr float SAMPLE_RATE = 48000.f;

  uint32_t getBufferSize() const override final { return 0U; }
  void init(float*) override final { reset(); }
  void reset() override final {
    phase_ = 0.f;
    current_frequency_ = 150.f;
    output_level_ = 0.f;
    for (unsigned k = 0; k != 3U; ++k) raw_[k] = 8192;
    select_ = 0; audition_ = 0; level_ = 250;
  }
  void setParameter(uint8_t id, int32_t value) override final {
    if (id <= DEPTH_VALUE)
      raw_[id] = value < 0 ? 0 : (value > 16383 ? 16383 : value);
    else if (id == SELECT_PAIR)
      select_ = value < 0 ? 0 : (value > 2 ? 2 : value);
    else if (id == AUDITION)
      audition_ = value < 0 ? 0 : (value > 1 ? 1 : value);
    else if (id == OUTPUT_LEVEL)
      level_ = value < 0 ? 0 : (value > 1000 ? 1000 : value);
  }
  // Display the decoded LSB value in the NTS-3 effects parameter editor,
  // if its firmware requests an SDK custom string for mapped parameters.
  const char *getParameterStrValue(uint8_t id, int32_t value) const override final {
    if (id > DEPTH_VALUE) return nullptr;
    const unsigned lsb = static_cast<unsigned>(value) & 127U;
    char *s = lsb_text_[id];
    s[0]='L'; s[1]=static_cast<char>('0'+(lsb/100U));
    s[2]=static_cast<char>('0'+((lsb/10U)%10U));
    s[3]=static_cast<char>('0'+(lsb%10U)); s[4]='\0';
    return s;
  }
  void process(const float *__restrict in,float *__restrict out,
               uint32_t frames) override final {
    (void)in;
    const uint32_t value=static_cast<uint32_t>(raw_[select_]);
    const uint32_t coarse=(value >> 7U) & 127U;
    const uint32_t fine=value & 127U;
    // Exactly 8 Hz per decoded integer. If only every 8th/16th LSB
    // survived in host mapping, the notes will jump audibly by 64/128 Hz.
    const float target_frequency=110.f+8.f*static_cast<float>(
        audition_ == 0 ? fine : coarse);
    const float target_level=.00010f*static_cast<float>(level_);
    for(uint32_t i=0;i<frames;++i) {
      current_frequency_+=.006f*(target_frequency-current_frequency_);
      output_level_+=.006f*(target_level-output_level_);
      phase_+=current_frequency_*(1.f/SAMPLE_RATE);
      if(phase_>=1.f) phase_-=1.f;
      // Diagnostic triangle wave, deliberately capped to 0.1 full scale.
      const float tone=output_level_*(4.f*fabsf(phase_-.5f)-1.f);
      out[2U*i]=tone;
      out[2U*i+1U]=tone;
    }
  }
  uint32_t rawValue(unsigned pair) const { return static_cast<uint32_t>(raw_[pair%3U]); }
  uint32_t coarse(unsigned pair) const { return (rawValue(pair)>>7U)&127U; }
  uint32_t fine(unsigned pair) const { return rawValue(pair)&127U; }
  float frequency() const { return current_frequency_; }
  float outputLevel() const { return output_level_; }
private:
  int32_t raw_[3] = {8192,8192,8192};
  int32_t select_ = 0, audition_ = 0, level_ = 250;
  float phase_ = 0.f, current_frequency_ = 150.f, output_level_ = 0.f;
  mutable char lsb_text_[3][5] = {{0},{0},{0}};
};

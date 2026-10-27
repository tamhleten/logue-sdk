#pragma once
#include <stdint.h>

// Share the *same* audio algorithm with naked Shermanish. Only the control
// decoder and fine-knob smoothing differ between the two builds.
#define SHERMANISH_SENSAI7 1
#include "../shermanish/effect.h"

// Sensai7's six independent pots send three 14-bit MIDI CC pairs. The NTS-3
// host appears to retain only 10 bits: 7 bits MSB, 3 bits LSB.
// We can recover 128 coarse values + 8 fine values per pair *if* the host
// delivers its mapped 10-bit value monotonically and without a curve.
// No raw MIDI is received by this SDK effect.
class Sensai7Effect : public Processor {
public:
  enum {
    X_PACKED=0, Y_PACKED, DEPTH_PACKED, INTERVAL_SHIFT,
    DRIVE_CEILING, ENV_CEILING, ENV_TIME, ROUTING, NUM_PARAMS
  };
  uint32_t getBufferSize() const override final { return core_.getBufferSize(); }
  void init(float *buffer) override final {
    core_.init(buffer);
    packed_[0]=5941; // coarse=46, fine=3 => unison interval
    packed_[1]=6566; // coarse=51, fine=2 => gentle drive
    packed_[2]=64;   // coarse=0,  fine=4 => follower + mid env amount
    interval_shift_=0; drive_ceiling_=1000; env_ceiling_=1000;
    time_=2; routing_=0;
    refresh();
  }
  void teardown() override final { core_.teardown(); }
  void reset() override final { core_.reset(); }
  void resume() override final { core_.resume(); }
  void suspend() override final { core_.suspend(); }
  void process(const float *__restrict in, float *__restrict out,
               uint32_t frames) override final {
    core_.process(in,out,frames);
  }
  void setParameter(uint8_t id,int32_t value) override final {
    switch(id) {
      case X_PACKED: case Y_PACKED: case DEPTH_PACKED:
        packed_[id]=clamp(value,0,16383); break;
      case INTERVAL_SHIFT: interval_shift_=clamp(value,-120,120); break;
      case DRIVE_CEILING: drive_ceiling_=clamp(value,0,1000); break;
      case ENV_CEILING: env_ceiling_=clamp(value,0,1000); break;
      case ENV_TIME: time_=clamp(value,0,9); break;
      case ROUTING: routing_=clamp(value,0,2); break;
      default: return;
    }
    refresh();
  }
  const char *getParameterStrValue(uint8_t id,int32_t value) const override final {
    if(id>DEPTH_PACKED) return nullptr;
    const unsigned ten=decode10(value);
    const unsigned coarse=ten>>3U, fine=ten&7U;
    char *s=display_[id];
    s[0]='C'; s[1]=static_cast<char>('0'+coarse/100U);
    s[2]=static_cast<char>('0'+(coarse/10U)%10U);
    s[3]=static_cast<char>('0'+coarse%10U);
    s[4]=' '; s[5]='F'; s[6]=static_cast<char>('0'+fine);
    s[7]='\0';
    return s;
  }
  void touchEvent(uint8_t id,uint8_t phase,uint32_t x,uint32_t y) override final {
    core_.touchEvent(id,phase,x,y);
  }
  void setTempo(float bpm) override final { core_.setTempo(bpm); }
  void tempo4ppqnTick(uint32_t counter) override final {
    core_.tempo4ppqnTick(counter);
  }
  // 0..16383 mapped value -> nearest 0..1023 ten-bit code.
  // 32-bit multiplication is safe: 16383*1023 < 2^31.
  static uint16_t decode10(int32_t raw) {
    const uint32_t r=static_cast<uint32_t>(clamp(raw,0,16383));
    return static_cast<uint16_t>((r*1023U+8191U)/16383U);
  }
  static int32_t packedFrom10(unsigned ten) {
    return static_cast<int32_t>((ten*16383U+511U)/1023U);
  }
  unsigned coarse(unsigned pair) const { return decode10(packed_[pair%3U])>>3U; }
  unsigned fine(unsigned pair) const { return decode10(packed_[pair%3U])&7U; }
  const Effect &core() const { return core_; }
  static int32_t intervalForStep(unsigned fine) {
    static const int16_t intervals[8]={-240,-120,-70,0,50,70,120,240};
    return intervals[fine&7U];
  }
  static int32_t driveForStep(unsigned fine) {
    static const int16_t drive[8]={0,40,90,160,260,400,620,1000};
    return drive[fine&7U];
  }
  static int32_t envForStep(unsigned fine) {
    static const int16_t amount[8]={0,90,200,330,480,650,820,1000};
    return amount[fine&7U];
  }
private:
  static int32_t clamp(int32_t v,int32_t lo,int32_t hi) {
    return v<lo?lo:(v>hi?hi:v);
  }
  void refresh() {
    core_.setParameter(Effect::PITCH,
                       static_cast<int32_t>((coarse(0)*1000U+63U)/127U));
    core_.setParameter(Effect::REGEN,
                       static_cast<int32_t>((coarse(1)*1000U+63U)/127U));
    core_.setParameter(Effect::MORPH,
                       static_cast<int32_t>((coarse(2)*1000U+63U)/127U));
    core_.setParameter(Effect::INTERVAL,
      clamp(intervalForStep(fine(0))+interval_shift_,-240,240));
    core_.setParameter(Effect::DRIVE,
      driveForStep(fine(1))*drive_ceiling_/1000);
    core_.setParameter(Effect::ENV_AMT,
      envForStep(fine(2))*env_ceiling_/1000);
    core_.setParameter(Effect::ENV_TIME,time_);
    core_.setParameter(Effect::ROUTING,routing_);
  }
  Effect core_;
  int32_t packed_[3]={5941,6566,64};
  int32_t interval_shift_=0,drive_ceiling_=1000,env_ceiling_=1000;
  int32_t time_=2,routing_=0;
  mutable char display_[3][8]={{0},{0},{0}};
};

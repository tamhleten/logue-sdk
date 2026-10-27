#include "sensai7_effect.h"
#include <cassert>
#include <cmath>
#include <cstdio>
static void render(Sensai7Effect &e,unsigned blocks) {
  float in[128]={0},out[128]={0};
  for(unsigned i=0;i<blocks;++i) {
    e.process(in,out,64);
    for(float sample:out) assert(std::isfinite(sample) && std::fabs(sample)<2.7f);
  }
}
static void setPair(Sensai7Effect &e,unsigned pair,unsigned coarse,unsigned fine) {
  assert(pair<3 && coarse<128 && fine<8);
  e.setParameter(static_cast<uint8_t>(pair),
    Sensai7Effect::packedFrom10(coarse*8U+fine));
}
static void decoderTest() {
  Sensai7Effect e;e.init(nullptr);
  const unsigned coarseValues[]={0,1,17,64,126,127};
  for(unsigned c:coarseValues) for(unsigned f=0;f<8;++f) {
    for(unsigned pair=0;pair<3;++pair) {
      setPair(e,pair,c,f);
      assert(e.coarse(pair)==c);
      assert(e.fine(pair)==f);
    }
  }
  setPair(e,0,40,2);
  setPair(e,1,50,4);
  setPair(e,2,60,6);
  setPair(e,0,41,2); // moving MSB must not change the fine index
  assert(e.fine(0)==2 && e.coarse(0)==41);
  setPair(e,1,50,7); // moving LSB must not change coarse index
  assert(e.coarse(1)==50 && e.fine(1)==7);
  const char *display=e.getParameterStrValue(1,Sensai7Effect::packedFrom10(50*8+7));
  assert(display && display[0]=='C' && display[5]=='F' && display[6]=='7');
}
static void smoothingTest() {
  Sensai7Effect e;e.init(nullptr);
  setPair(e,0,64,3); // 0 semitones
  setPair(e,1,64,2); // drive 90
  setPair(e,2,64,4); // env 480
  render(e,900);
  assert(std::fabs(e.core().currentInterval())<.05f);
  assert(std::fabs(e.core().currentDrive()-2.71f)<.05f);
  assert(std::fabs(e.core().currentEnvAmount()-.48f)<.02f);
  setPair(e,0,64,7); // +24 semitones
  setPair(e,1,64,7); // drive 1000
  setPair(e,2,64,7); // env 1000
  render(e,1);
  const float interval=e.core().currentInterval();
  const float drive=e.core().currentDrive();
  const float env=e.core().currentEnvAmount();
  std::printf("first block: interval %.3f drive %.3f env %.3f\n",interval,drive,env);
  assert(interval>.01f && interval<2.f);
  assert(drive>2.71f && drive<5.f);
  assert(env>.48f && env<.55f);
  render(e,1200);
  std::printf("settled: interval %.3f drive %.3f env %.3f\n",
    e.core().currentInterval(),e.core().currentDrive(),e.core().currentEnvAmount());
  assert(std::fabs(e.core().currentInterval()-24.f)<.02f);
  assert(std::fabs(e.core().currentDrive()-20.f)<.02f);
  assert(std::fabs(e.core().currentEnvAmount()-1.f)<.02f);
  e.setParameter(Sensai7Effect::INTERVAL_SHIFT,-70);
  setPair(e,0,64,3);
  e.setParameter(Sensai7Effect::DRIVE_CEILING,500);
  e.setParameter(Sensai7Effect::ENV_CEILING,500);
  render(e,1200);
  assert(std::fabs(e.core().currentInterval()+7.f)<.02f);
  assert(std::fabs(e.core().currentDrive()-10.5f)<.02f);
  assert(std::fabs(e.core().currentEnvAmount()-.5f)<.02f);
}
int main() {
  decoderTest();
  smoothingTest();
  std::puts("Sensai7 packed CC decoder, independence, and smoothing: PASS");
  return 0;
}

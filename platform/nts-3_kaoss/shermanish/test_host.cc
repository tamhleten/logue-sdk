#include "effect.h"
#include <cmath>
#include <cstdio>
#include <vector>
#include <algorithm>
static void process(Effect &e,const std::vector<float>&in,std::vector<float>&out) {
  out.assign(in.size(),0.f);
  for(unsigned p=0;p<in.size()/2U;p+=64U) {
    const unsigned n=std::min(64U,static_cast<unsigned>(in.size()/2U-p));
    e.process(in.data()+2U*p,out.data()+2U*p,n);
  }
}
static bool safe(const std::vector<float>&x) {
  for(float v:x) if(!std::isfinite(v)||std::fabs(v)>2.7f)return false;
  return true;
}
static bool dryTest() {
  Effect e;e.init(nullptr);e.setParameter(Effect::DRY_WET,-1000);e.reset();
  std::vector<float> in(8000),out;
  for(unsigned i=0;i<in.size();++i)in[i]=.5f*std::sin(.007f*i);
  process(e,in,out);
  for(unsigned i=0;i<in.size();++i)if(std::fabs(in[i]-out[i])>.000001f)return false;
  return true;
}
static bool monoStereoTest() {
  Effect e;e.init(nullptr);e.setParameter(Effect::PITCH,420);
  std::vector<float> in(2U*96000),out;
  for(unsigned i=0;i<96000;++i) {
    float v=.26f*(std::sin(i*6.2831853f*82.4f/48000.f)+.4f*std::sin(i*6.2831853f*164.8f/48000.f));
    in[2*i]=in[2*i+1]=v;
  }
  process(e,in,out);
  double difference=0,energy=0,stereo=0;
  for(unsigned i=0;i<96000;++i) {
    energy+=out[2*i]*out[2*i];
    difference+=std::fabs(out[2*i]-in[2*i]);
    stereo+=std::fabs(out[2*i]-out[2*i+1]);
  }
  std::printf("mono stereo energy %.2f diff %.2f width %.2f\n",energy,difference,stereo);
  return safe(out)&&energy>1&&difference>10&&stereo>.1;
}
static bool feedbackTailTest() {
  Effect e;e.init(nullptr);e.setParameter(Effect::REGEN,950);
  e.setParameter(Effect::PITCH,440);e.setParameter(Effect::DRY_WET,1000);
  std::vector<float> in(2U*96000,0.f),out;
  for(unsigned i=0;i<12000;++i)in[2*i]=in[2*i+1]=.3f*std::sin(i*6.2831853f*110.f/48000.f);
  process(e,in,out);
  double tail=0;
  for(unsigned i=40000;i<90000;++i)tail+=out[2*i]*out[2*i];
  std::printf("feedback tail energy %.5f\n",tail);
  return safe(out)&&tail>.0001;
}
static bool hotStability() {
  Effect e;e.init(nullptr);e.setParameter(Effect::REGEN,1000);
  e.setParameter(Effect::DRIVE,1000);e.setParameter(Effect::ROUTING,2);
  e.setParameter(Effect::ENV_MODE,Effect::CLOCK_16TH);e.setParameter(Effect::ENVELOPE,1000);
  std::vector<float> in(2U*240000),out;
  for(unsigned i=0;i<240000;++i) {
    float v=4.f*(std::sin(.0345f*i)+.7f*std::sin(.1373f*i));
    in[2*i]=v;in[2*i+1]=v;
  }
  process(e,in,out);return safe(out);
}

static bool followerTest() {
  Effect e;e.init(nullptr);
  e.setParameter(Effect::ENV_MODE,Effect::FOLLOW_FAST);
  e.setParameter(Effect::ENVELOPE,800);
  std::vector<float> in(2U*24000),out;
  for(unsigned i=0;i<24000;++i)in[2*i]=in[2*i+1]=.45f*std::sin(2.f*3.14159265f*110.f*i/48000.f);
  process(e,in,out);
  const float active=e.currentEnvelope();
  in.assign(2U*48000,0.f);process(e,in,out);
  const float rest=e.currentEnvelope();
  std::printf("follower active %.4f rest %.4f\n",active,rest);
  return safe(out)&&active>.3f&&rest<active*.4f;
}
static bool triggeredTest() {
  Effect e;e.init(nullptr);
  e.setParameter(Effect::ENV_MODE,Effect::TRIG_PUNCH);
  std::vector<float> in(2U*8000),out;
  for(unsigned i=0;i<8000;++i)in[2*i]=in[2*i+1]=.65f*std::sin(6.2831853f*130.f*i/48000.f);
  process(e,in,out);
  const float active=e.currentEnvelope();
  in.assign(2U*70000,0.f);process(e,in,out);
  const float rest=e.currentEnvelope();
  std::printf("trigger active %.4f rest %.4f\n",active,rest);
  return safe(out)&&active>.30f&&rest<.03f;
}
static bool tempoTest() {
  Effect e;e.init(nullptr);
  e.setParameter(Effect::ENV_MODE,Effect::CLOCK_8TH);
  e.setParameter(Effect::ENVELOPE,800);
  e.setTempo(120.f);
  std::vector<float> silence(2U*3000,0.f),out;
  process(e,silence,out);
  const float phase=e.currentClockPhase();
  e.tempo4ppqnTick(2U);
  std::vector<float> next(2U*64,0.f);
  process(e,next,out);
  const float reset=e.currentClockPhase();
  e.setTempo(60.f);
  silence.assign(2U*48000,0.f);
  process(e,silence,out);
  std::printf("tempo phase %.4f reset %.4f bpm %.2f\n",phase,reset,e.currentBpm());
  return safe(out)&&phase>.2f&&reset<.02f&&fabsf(e.currentBpm()-60.f)<.1f;
}

int main(){
  bool a=dryTest(),b=monoStereoTest(),c=feedbackTailTest(),d=hotStability();
  bool follower=followerTest(),trigger=triggeredTest(),tempo=tempoTest();
  std::printf("dry=%d mono=%d tail=%d hot=%d follower=%d trigger=%d tempo=%d\n",a,b,c,d,follower,trigger,tempo);
  return a&&b&&c&&d&&follower&&trigger&&tempo?0:1;
}

#pragma once
#include <math.h>
#include <stdint.h>
#include "processor.h"

// Sherman Filterbank-inspired dual-bandpass performance instrument.
// The two envelopes run continuously; Y smoothly crossfades their modulation.
// Intended for mono sax/looper via the L6max SUB OUT, with stereo wet output.
// Not a circuit model of the analogue Sherman Filterbank.
class Effect : public Processor {
public:
  static constexpr float SAMPLE_RATE = 48000.f;
  static constexpr float PI = 3.14159265358979323846f;
  enum { PITCH = 0, MORPH, REGEN, DRIVE, INTERVAL, ENV_AMT,
         ENV_TIME, ROUTING, NUM_PARAMS };
  enum Stage { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE };

  struct Params {
    float pitch, morph, regen, drive, semitones, env;
    int time_mode, route;
    Params() : pitch(360.f), morph(0.f), regen(.40f), drive(2.406f),
               semitones(0.f), env(.50f), time_mode(2), route(0) {}
  };

  uint32_t getBufferSize() const override final { return 0U; }
  void init(float *) override final { params = Params(); reset(); }
  void teardown() override final {}
  void reset() override final {
    for (int ch=0;ch<2;++ch) for(int j=0;j<2;++j) filters[ch][j]=Svf();
    detector=0.f; follower=0.f; adsr=0.f; envelope=0.f;
    stage=IDLE; gate=false; clock_phase=0.f; pending_tick=false;
    smooth_pitch=params.pitch; smooth_morph=params.morph;
    smooth_regen=params.regen; smooth_drive=params.drive;
    smooth_interval=params.semitones; smooth_env=params.env;
    smooth_bpm=target_bpm;
  }

  void setParameter(uint8_t index, int32_t value) override final {
    switch(index) {
      case PITCH: params.pitch=static_cast<float>(value); break;
      case MORPH: params.morph=static_cast<float>(value)*.001f; break;
      case REGEN: params.regen=static_cast<float>(value)*.001f; break;
      case DRIVE: params.drive=1.f+static_cast<float>(value)*.019f; break;
      case INTERVAL: params.semitones=static_cast<float>(value)*.1f; break;
      case ENV_AMT: params.env=static_cast<float>(value)*.001f; break;
      case ENV_TIME: params.time_mode=value; break;
      case ROUTING: params.route=value; break;
      default:break;
    }
  }
  void setTempo(float bpm) override final {
    if (bpm<30.f) bpm=30.f;
    if (bpm>300.f) bpm=300.f;
    target_bpm=bpm;
  }
  void tempo4ppqnTick(uint32_t counter) override final {
    last_tick=counter; pending_tick=true;
  }

  void process(const float *__restrict in, float *__restrict out,
               uint32_t frames) override final {
    // Only the filter coefficients depend on expensive trigonometric
    // functions. Update once per block; interpolate coefficients per sample.
    smooth_pitch+=.11f*(params.pitch-smooth_pitch);
    smooth_interval+=.11f*(params.semitones-smooth_interval);
    const float hz1=55.f*exp2f(smooth_pitch*(5.50635267f/1000.f));
    const float hz2=hz1*exp2f(smooth_interval/12.f);
    const float cutoff_mod=exp2f(3.0f*smooth_env*envelope);
    for(int ch=0;ch<2;++ch) {
      const float detune=ch==0?.985f:1.015f;
      filters[ch][0].targetG(hz1*cutoff_mod*detune);
      filters[ch][1].targetG(hz2*cutoff_mod/detune);
    }

    const int time_mode=params.time_mode;
    const int route=params.route;
    const bool clocked=time_mode>=5;
    const float beats=beatsForMode(time_mode);
    const float target_regen=params.regen;
    const float target_drive=params.drive;
    const float target_morph=params.morph;
    const float target_env=params.env;
    if(pending_tick) {
      const uint32_t tick=last_tick;
      pending_tick=false;
      if(clocked && tick%(static_cast<unsigned>(beats*4.f))==0U)
        clock_phase=1.f;  // Retrigger from the following audio sample.
    }

    for(uint32_t i=0;i<frames;++i) {
      smooth_regen+=.0008f*(target_regen-smooth_regen);
      smooth_drive+=.0008f*(target_drive-smooth_drive);
      smooth_env+=.0008f*(target_env-smooth_env);
      smooth_morph+=.0018f*(target_morph-smooth_morph);
      smooth_bpm+=.00015f*(target_bpm-smooth_bpm);

      const float l=in[2U*i],r=in[2U*i+1U];
      const float peak=fmaxf(fabsf(l),fabsf(r));
      const float attack=peak>detector?.009f:.00022f;
      detector+=attack*(peak-detector);

      // Audio follower never stops updating, including while Y is in ADSR.
      const float level=clamp01(4.2f*detector);
      const float coeff=level>follower?.0044f:.00017f;
      follower+=coeff*(level-follower);

      // Audio trigger or metronomic trigger. Tap BPM also determines the
      // ADSR contour length, even in audio-triggered modes.
      const float seconds=beats*60.f/smooth_bpm;
      if(clocked) {
        if(clock_phase>=1.f) {
          clock_phase=0.f;
          beginEnvelope();
        }
        clock_phase+=1.f/(seconds*SAMPLE_RATE);
      } else {
        if(!gate && detector>.029f) {
          gate=true;
          beginEnvelope();
        } else if(gate && detector<.012f) {
          gate=false;
          stage=RELEASE;
        }
      }
      stepEnvelope(seconds,clocked);

      // At Y < 40%, only the follower is heard; at Y > 60%, only
      // the triggered ADSR. Both stay active through the center.
      const float m=smoothstep((smooth_morph-.4f)*5.f);
      envelope=(1.f-m)*follower+m*adsr;

      const float k=.62f-.600f*smooth_regen;
      const float feedback=.02f+.74f*smooth_regen*smooth_regen;
      for(int ch=0;ch<2;++ch) {
        Svf &a=filters[ch][0],&b=filters[ch][1];
        a.moveG();b.moveG();
        const float src=in[2U*i+static_cast<uint32_t>(ch)];
        const float driven=softclip(smooth_drive*src);
        const float aa=a.last, bb=b.last;
        const float cross=route==2?.26f:0.f;
        const float ain=driven+feedback*softclip(2.f*aa)
                        +cross*softclip(2.f*bb);
        const float band_a=a.tick(softclip(ain),k);
        const float bin=(route==1?softclip(2.f*band_a):driven)
                        +feedback*softclip(2.f*bb)
                        +cross*softclip(2.f*aa);
        const float band_b=b.tick(softclip(bin),k);
        const float summed=route==1?band_b:.58f*(band_a+band_b);
        // Always wet; the L6max handles the dry/wet mix.
        out[2U*i+static_cast<uint32_t>(ch)]=
            .72f*softclip(2.5f*summed);
      }
    }
  }

  float currentEnvelope() const { return envelope; }
  float currentFollower() const { return follower; }
  float currentAdsr() const { return adsr; }
  float currentMorph() const { return smoothstep((smooth_morph-.4f)*5.f); }
  float currentClockPhase() const { return clock_phase; }
  float currentBpm() const { return smooth_bpm; }

private:
  static float clamp01(float x) {
    return x<0.f?0.f:(x>1.f?1.f:x);
  }
  static float smoothstep(float x) {
    const float y=clamp01(x);
    return y*y*(3.f-2.f*y);
  }
  static float softclip(float x) {
    x=x>3.f?3.f:(x< -3.f?-3.f:x);
    const float x2=x*x;
    return x*(27.f+x2)/(27.f+9.f*x2);
  }
  static float beatsForMode(int mode) {
    switch(mode%5) {
      case 0:return .25f;   // 1/16
      case 1:return .5f;    // 1/8
      case 2:return 1.f;    // 1/4
      case 3:return 2.f;    // 1/2
      default:return 4.f;   // 1 bar in 4/4
    }
  }
  void beginEnvelope() {
    stage=ATTACK;
    adsr=0.f;
  }
  void stepEnvelope(float seconds,bool clocked) {
    const float step=1.f/SAMPLE_RATE;
    const float a=seconds*.06f;
    const float d=seconds*.50f;
    const float r=seconds*.55f;
    const float sustain=.30f;
    switch(stage) {
      case IDLE:adsr=0.f;break;
      case ATTACK:
        adsr+=step/(a>.003f?a:.003f);
        if(adsr>=1.f){adsr=1.f;stage=DECAY;}
        break;
      case DECAY:
        adsr-=(1.f-sustain)*step/(d>.01f?d:.01f);
        if(adsr<=sustain){adsr=sustain;stage=clocked?RELEASE:SUSTAIN;}
        break;
      case SUSTAIN:
        adsr=sustain;
        break;
      case RELEASE:
        adsr-=step/(r>.025f?r:.025f);
        if(adsr<=0.f){adsr=0.f;stage=IDLE;}
        break;
    }
  }
  struct Svf {
    float ic1=0.f,ic2=0.f,last=0.f,g=.015f,target=.015f;
    void targetG(float hz) {
      if(hz<28.f)hz=28.f;
      if(hz>5800.f)hz=5800.f;
      target=tanf(PI*hz/SAMPLE_RATE);
    }
    void moveG() { g+=.0025f*(target-g); }
    float tick(float input,float k) {
      const float a1=1.f/(1.f+g*(g+k));
      const float a2=g*a1,a3=g*a2;
      const float v3=input-ic2;
      const float v1=a1*ic1+a2*v3;
      const float v2=ic2+a2*ic1+a3*v3;
      ic1=2.f*v1-ic1;ic2=2.f*v2-ic2;
      if(ic1>16.f)ic1=16.f;if(ic1< -16.f)ic1=-16.f;
      if(ic2>16.f)ic2=16.f;if(ic2< -16.f)ic2=-16.f;
      last=v1;return v1;
    }
  };
  Svf filters[2][2];
  Params params;
  float detector=0.f,follower=0.f,adsr=0.f,envelope=0.f;
  Stage stage=IDLE;
  bool gate=false;
  volatile bool pending_tick=false;
  volatile uint32_t last_tick=0U;
  float target_bpm=120.f,smooth_bpm=120.f,clock_phase=0.f;
  float smooth_pitch=360.f,smooth_morph=0.f,smooth_interval=0.f;
  float smooth_regen=.4f,smooth_drive=2.406f,smooth_env=.5f;
};

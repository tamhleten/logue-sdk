#include "effect.h"
#include <cassert>
#include <cmath>
#include <vector>
int main() {
 Effect e;e.init(nullptr);
 for(int k=0;k<3;++k){
  e.setParameter(k,128*71+93);
  assert(e.coarse(k)==71 && e.fine(k)==93);
 }
 e.setParameter(Effect::SELECT_PAIR,2);
 e.setParameter(Effect::AUDITION,0);
 std::vector<float> in(48000*2,0),out(in.size());
 for(int n=0;n<750;n++)e.process(in.data()+n*128,out.data()+n*128,64);
 for(float v:out) assert(std::isfinite(v) && std::fabs(v)<=.101f);
 return 0;
}

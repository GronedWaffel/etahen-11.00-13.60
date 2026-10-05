// Formula cases adapted from OnionHEN's test_fps_formula.cpp; stale/invalid
// sample cases cover the shared record used by this etaHEN port.
#include <onion/fps_formula.hpp>
#include <onion/fps_validate.h>
#include <assert.h>
#include "../Source Code/include/port_fps_cache.hpp"
#include <limits>
using namespace onion::fps;
int main(){
 assert(hz_from_delta(15,1)==15);assert(hz_from_delta(60,1)==60);
 assert(hz_from_delta(1,0)==0);assert(hz_from_delta(0,1)==0);
 assert(hz_from_delta(1,std::numeric_limits<double>::quiet_NaN())==0);
 HybridIn in;assert(!compose(in).valid);
 in.ring_ok=true;in.ring=30;in.global_ok=true;in.global=120;
 assert(compose(in).fps==30); // Do not mistake the global submit counter for rendered frames.
 in.scanout_ok=true;in.scanout=60;in.calibration_ready=true;in.multipass=true;
 assert(compose(in).fps==60 && (compose(in).source&ONION_FPS_SRC_MULTIPASS));
 in.ring=500;in.scanout_ok=false;assert(!compose(in).valid);
 OnionFpsSample s{};s.magic=ONION_FPS_MAGIC;s.valid=1;s.pid=91;s.fps=30;s.unix_ns=100;strcpy(s.title_id,"CUSA12345");
 assert(onion_fps_sample_valid(&s,100));assert(!onion_fps_sample_valid(&s,99));
 assert(!onion_fps_sample_valid(&s,100+ONION_FPS_STALE_NS+1));
 s.fps=std::numeric_limits<float>::quiet_NaN();assert(!onion_fps_sample_valid(&s,100));
 s.fps=60;memset(s.title_id,'A',sizeof(s.title_id));assert(!onion_fps_sample_valid(&s,100));
 // A blocked/stopped background reader must never keep showing stale FPS.
 PortFpsCache cache;OnionFpsSample native{},bc{};
 native.magic=ONION_FPS_MAGIC;native.valid=1;native.pid=91;native.fps=59.94f;
 native.unix_ns=10000000000ull;strcpy(native.title_id,"PPSA04264");
 uint64_t wall=native.unix_ns+1000000000ull;
 cache.publish(&native,wall,1000);assert(cache.read(1000)==599);
 assert(cache.read(1999)==599);assert(cache.read(2000)==0);assert(cache.read(15000)==0);
 cache.publish(&native,native.unix_ns-1,1000);assert(cache.read(1000)==0);
 bc=native;strcpy(bc.title_id,"CUSA00265");bc.unix_ns+=100000000ull;
 assert(port_fps_choose(native,bc,wall)==&bc);
 bc.valid=0;assert(port_fps_choose(native,bc,wall)==&bc);
 cache.publish(port_fps_choose(native,bc,wall),wall,2000);assert(cache.read(2000)==0);
 bc.seq=1;assert(port_fps_choose(native,bc,wall)==&native);
 native.unix_ns=wall+1;assert(!port_fps_choose(native,bc,wall));
 return 0;
}

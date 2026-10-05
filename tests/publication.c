#define PORT_PUBLICATION_TEST 1
#define ETAHEN_TOOLBOX_DIAGNOSTIC 1
#include "../Source Code/libNineS/src/port_publish.c"
static PortPublishRequest setup(void){
 memset(memory,0,sizeof memory);stopped=busy_once=fail_write=write_calls=fail_detach=flushes=events_count=0;
 PortPatch patches[2]={0};
 for(int i=0;i<2;i++){patches[i].address=0x2000+i*0x100;memset(patches[i].before,0x90,14);memset(patches[i].after,0x41+i,14);memcpy(memory+patches[i].address,patches[i].before,14);}
 memcpy(memory+0x1000,patches,sizeof patches);return (PortPublishRequest){PORT_PUBLISH_MAGIC,2,1,0x1000};
}
static int saw(const char *op){for(int i=0;i<events_count;i++)if(!strcmp(operations[i],op))return 1;return 0;}
int main(void){
 PortPublishRequest request=setup();assert(apply_stopped(58,&request,0x3400)==0);assert(!stopped);assert(memory[0x2000]==0x41&&memory[0x2100]==0x42);assert(saw("hook-compare"));
 request=setup();fail_write=2;assert(apply_stopped(58,&request,0x3400)==-1);assert(!stopped);assert(memory[0x2000]==0x90&&memory[0x2100]==0x90);assert(saw("rollback-index"));
 request=setup();memory[0x2000]=0xcc;assert(apply_stopped(58,&request,0x3400)==-1);assert(!stopped);assert(!write_calls);assert(saw("original-hook-mismatch"));
 request=setup();busy_once=1;assert(apply_stopped(58,&request,0x3400)==0);assert(!stopped);assert(saw("thread-in-hook"));
 request=setup();fail_write=3;assert(apply_stopped(58,&request,0x3400)==-1);assert(!stopped);assert(memory[0x2000]==0x90&&memory[0x2100]==0x90);
 request=setup();fail_detach=1;assert(apply_stopped(58,&request,0x3400)==-1);assert(stopped);assert(flushes==1);
 puts("publication success, rollback, mismatch, busy thread, acknowledgement failure and failed detach verified");return 0;
}

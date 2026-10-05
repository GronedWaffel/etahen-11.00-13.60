#include "../Source Code/include/port_plugin.h"
#include "../Source Code/include/port_game_plugin.hpp"
#include <assert.h>
#include <stdlib.h>
#include <fstream>
#include <vector>

static void u64(unsigned char *p, uint64_t n) { memcpy(p, &n, 8); }
int main(int argc,char **argv) {
    std::string title;
    assert(port_game_plugin_path("/data/etaHEN/game_plugins/CUSA12345/test.plugin",&title) && title=="CUSA12345");
    assert(port_game_plugin_path("/data/etaHEN/game_plugins/PPSA12345/test.plugin",&title));
    for(const char *path:{"/data/etaHEN/game_plugins/NPXS40047/test.plugin","/data/etaHEN/game_plugins/CUSA12345/../a.elf","/data/etaHEN/game_plugins/CUSA1234/test.plugin","/data/etaHEN/game_plugins/CUSA12345/a.elf.auto_start","/elsewhere/CUSA12345/a.elf"})assert(!port_game_plugin_path(path,&title));
    assert(port_game_plugin_path("/data/etaHEN/game_plugins/CUSA12345/menu.prx",&title));
    assert(port_game_plugin_path("/data/etaHEN/game_plugins/PPSA12345/menu.sprx",&title));
    unsigned char elf[128] = {0};
    memcpy(elf, "\177ELF\2\1\1", 7);
    elf[16]=3; elf[18]=62; elf[52]=64; elf[54]=56; elf[56]=1;
    u64(elf+32,64); elf[64]=1; elf[68]=5; u64(elf+96,128); u64(elf+104,128);
    PortPlugin info;
    assert(port_plugin_parse("/data/etaHEN/plugins/a.elf",elf,128,128,&info));
    assert(info.elf_offset==0 && strlen(info.identity)==15);
    char a[16],b[16];
    port_plugin_identity("/user/data/etaHEN/plugins/a.elf",a);
    port_plugin_identity("/data/etaHEN/plugins/a.elf",b); assert(!strcmp(a,b));
    port_plugin_identity("/usb2/etaHEN/plugins/a.elf",a);
    port_plugin_identity("/mnt/usb2/etaHEN/plugins/a.elf",b); assert(!strcmp(a,b));
    port_plugin_identity("/data/etaHEN/plugins/a.elf",b); assert(strcmp(a,b));
    assert(!port_plugin_parse("a.elf.auto_start",elf,128,128,&info));
    for (size_t n=0;n<128;n++) assert(!port_plugin_parse("a.elf",elf,n,n,&info));
    assert(!port_plugin_parse("a.elf",elf,128,PORT_PLUGIN_MAX_SIZE+1,&info));
    elf[18]=40; assert(!port_plugin_parse("a.elf",elf,128,128,&info)); elf[18]=62;
    u64(elf+32,UINT64_MAX); assert(!port_plugin_parse("a.elf",elf,128,128,&info));u64(elf+32,64);
    u64(elf+72,127); assert(!port_plugin_parse("a.elf",elf,128,128,&info));u64(elf+72,0);
    u64(elf+24,128);assert(!port_plugin_parse("a.elf",elf,128,128,&info));u64(elf+24,0);
    u64(elf+80,UINT64_MAX-10);assert(!port_plugin_parse("a.elf",elf,128,128,&info));u64(elf+80,0);
    u64(elf+112,3);assert(!port_plugin_parse("a.elf",elf,128,128,&info));u64(elf+112,0);
    unsigned char plugin[157]={0};
    memcpy(plugin,"etaHEN_PLUGIN",14); memcpy(plugin+14,"TEST12345",10);memcpy(plugin+24,"1.00",5);
    memcpy(plugin+29,elf,128);
    assert(port_plugin_parse("a.plugin",plugin,157,157,&info));
    assert(!strcmp(info.identity,"TEST12345") && !strcmp(info.version,"1.00") && info.elf_offset==29);
    plugin[14]='t';assert(port_plugin_parse("a.plugin",plugin,157,157,&info));plugin[14]='T';
    plugin[27]=0;assert(!port_plugin_parse("a.plugin",plugin,157,157,&info));plugin[27]='0';
    assert(!port_plugin_parse("renamed.plugin",elf,128,128,&info));
    plugin[23]='X'; assert(!port_plugin_parse("a.plugin",plugin,157,157,&info));plugin[23]=0;
    plugin[28]='X'; assert(!port_plugin_parse("a.plugin",plugin,157,157,&info));plugin[28]=0;
    plugin[14]='/'; assert(!port_plugin_parse("a.plugin",plugin,157,157,&info));plugin[14]='T';
    /* Prefix-only browsing still verifies that the declared table fits the file. */
    assert(port_plugin_parse("a.plugin",plugin,128,157,&info));
    plugin[29+56]=128;assert(!port_plugin_parse("a.plugin",plugin,128,157,&info));
    for(int i=1;i<argc;i++){
        std::ifstream f(argv[i],std::ios::binary);assert(f);
        std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(f)),{});
        assert(port_plugin_parse(argv[i],bytes.data(),bytes.size(),bytes.size(),&info));
    }
    return 0;
}

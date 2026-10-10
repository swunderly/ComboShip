"""Run extracted production cross-warp audio routing and MM dialect handlers.

No game archives are required. Native cue IDs and the MM instruction argument
contract are independent expectations; audio command queues are captured.
"""
from pathlib import Path
import argparse
import re
import subprocess
import struct
import zipfile
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
p.add_argument('--mm-archive', type=Path, help='Optional local archive: reproduce the native Soaring null-drum request')
a = p.parse_args()
source, work = a.source.resolve(), a.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
if a.mm_archive:
    with zipfile.ZipFile(a.mm_archive) as archive:
        raw = archive.read('audio/sequences/SongOfSoaring_55')
        length = struct.unpack_from('<I', raw, 64)[0]
        score = raw[68:68+length]
        # Follow the actual score's channel 9 and layer 1 pointers, rather than
        # searching arbitrary bytes for a coincidental drum opcode.
        channel9 = struct.unpack_from('>H', score, 17)[0]
        assert score[16] == 0x99 and channel9 == 0x147
        assert score[channel9:channel9+7] == bytes.fromhex('c4880379890381')
        layer1 = struct.unpack_from('>H', score, channel9+5)[0]
        assert bytes.fromhex('c17f') in score[channel9:channel9+24]
        assert score[layer1:layer1+7] == bytes.fromhex('c081386c80db5a')
        cursor = layer1
        def read_u8():
            global cursor
            value = score[cursor]
            cursor += 1
            return value
        def read_delay():
            value = read_u8()
            return ((value & 0x7f) << 8) | read_u8() if value & 0x80 else value
        assert read_u8() == 0xc0
        assert read_delay() == 312
        note = read_u8()
        assert note & 0xc0 == 0x40  # native large-note type 1
        assert note & 0x3f == 44
        assert read_delay() == 219 and read_u8() == 90
        # Both production engines report a null drum; this is archive evidence,
        # never a justification for clearing or suppressing the runtime flag.
        oot_playback = (source/'soh/src/code/audio_playback.c').read_text(encoding='utf-8')
        mm_playback = (source/'mm/src/audio/lib/playback.c').read_text(encoding='utf-8')
        assert '((fontId << 8) + drumId) + 0x5000000' in oot_playback
        assert 'AUDIO_ERROR(fontId, drumId, AUDIO_ERROR_NO_DRUM_SFX)' in mm_playback
        font = archive.read('audio/fonts/Soundfont_9')
        drums = struct.unpack_from('<I', font, 76)[0]
        cursor = 88
        for drum in range(drums):
            cursor += 3
            count = struct.unpack_from('<I', font, cursor)[0]
            cursor += 4 + count*4
            has_sample = font[cursor]
            size = struct.unpack_from('<I', font, cursor+1)[0]
            sample = font[cursor+5:cursor+5+size]
            cursor += 9 + size
            if drum == 44:
                assert has_sample == 0 and sample == b''
        (work/'native-mm-null-drum.txt').write_text(
            'Native score channel 9 (0x147), layer 1 (0x381): C0 81 38; 6C 80 DB 5A.\n'
            'Rest 312 ticks; large note drum 44, duration 219 ticks, velocity 90.\n'
            'Native Soundfont_9 drum 44 hasSample=0, empty sample reference.\n'
            'Native MM error 0x0500092c; imported font 38 error 0x0500262c.\n', encoding='utf-8')
text = (source/'soh/soh/Enhancements/CrossWarp/CrossWarpAudio.cpp').read_text(encoding='utf-8')

def function(signature):
    start = text.index(signature)
    at = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[at] == '{') - (text[at] == '}')
        at += 1
    return text[start:at] + '\n'

sfx = (source/'mm/include/sfx.h').read_text(encoding='utf-8')
cue_names = ['NA_SE_PL_WARP_WING_OPEN', 'NA_SE_PL_WARP_WING_CLOSE',
             'NA_SE_PL_WARP_WING_ROLL', 'NA_SE_PL_WARP_WING_VANISH']
cues = [int(re.search(r'#define\s+'+name+r'\s+(0x[\dA-Fa-f]+)', sfx)[1], 16) for name in cue_names]
code = r'''
#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <utility>
#include <vector>
using u8=uint8_t;using s8=int8_t;using u16=uint16_t;using s16=int16_t;using u32=uint32_t;using s32=int32_t;
enum {SEQ_PLAYER_BGM_MAIN=0,SEQ_PLAYER_FANFARE=1,SEQ_PLAYER_SFX=2,SEQ_PLAYER_BGM_SUB=3,
      NA_BGM_DISABLED=0xffff,NA_BGM_MINI_BOSS=38,NA_BGM_GENERAL_SFX=0,NA_BGM_OCA_LIGHT=55,SEQ_MODE_IGNORE=2,
      MEDIUM_RAM=0,MEDIUM_UNK=1,MEDIUM_CART=2};
struct SoundFontSample {int medium=MEDIUM_CART;u8* sampleAddr=nullptr;};
int dmaCalls=0;
u8* AudioLoad_DmaSampleData(uintptr_t,int,int,u8*,int){++dmaCalls;return nullptr;}
#define ALIGN16(value) (((value)+15)&~15)
#define CVAR_SETTING(x) "gSettings." x
#define SPDLOG_INFO(...) ((void)0)
int errors=0;
#define SPDLOG_ERROR(...) (++errors)
struct SeqScriptState {u8* pc=nullptr;s8 value=0;};
struct SequenceChannel;
struct SequencePlayer {u8* seqData=nullptr;SequenceChannel* channels[16]{};};
struct SequenceChannel {SequencePlayer* seqPlayer=nullptr;SeqScriptState scriptState;u16 unk_22=0;
    struct {u8 asByte=0;} stereo;float freqScale=0;struct {struct {bool freqScale=false;} s;} changes;};
struct SequenceData {char* seqData=nullptr;};
struct {int seqToPlay[4]{},seqReplaced[4]{},audioErrorFlags=0;} gAudioContext;
std::array<SequenceData*,3> sSequences{};
std::array<std::array<std::atomic<u8>,16>,16> sSfxState{};
std::array<bool,256> sDialectSeen{};
std::array<bool,3> sSequenceExecuting{};
std::atomic<u32> sDialectFailures{0};
int sSequenceBase=-1;bool sPlaying=false;
u32 sPreviousAudioErrors=0;
u16 sPreviousMain=NA_BGM_DISABLED,sPreviousSub=NA_BGM_DISABLED;
int active[4]={66,NA_BGM_DISABLED,0,NA_BGM_DISABLED};
std::vector<std::pair<u32,int>> commands;
std::vector<std::pair<int,int>> starts;
float volumes[4]{};int fanfare=-1,muted=0,mode=-1;
int func_800FA0B4(int player){return active[player];}
void Audio_SetSequenceMode(int value){mode=value;}
void Audio_StartSequence(int player,u8 label,int,int){starts.emplace_back(player,gAudioContext.seqToPlay[player]);active[player]=label;}
void Audio_SetGameVolume(int player,float value){volumes[player]=value;}
int CVarGetInteger(const char* name,int){return std::strstr(name,"SFX")?72:41;}
void Audio_QueueCmdS8(u32 cmd,int value){commands.emplace_back(cmd,static_cast<u8>(value));}
void Audio_QueueCmdF32(u32 cmd,float value){commands.emplace_back(cmd,static_cast<int>(value*100));}
void Audio_QueueSeqCmd(u32 cmd){commands.emplace_back(cmd,0);}
void Audio_PlayFanfare(int id){fanfare=id;}
void Audio_SetSfxBanksMute(int value){muted=value;}
u8 AudioSeq_ScriptReadU8(SeqScriptState* state){return *state->pc++;}
s16 AudioSeq_ScriptReadS16(SeqScriptState* state){u16 result=state->pc[0]*256+state->pc[1];state->pc+=2;return result;}
u32 Audio_NextRandom(){return 0x12345678;}
'''
for signature in ['void StoreMmSfxFloat(', 'float LoadMmSfxFrequency(', 'void UseResidentSampleStorage(', 'void StartForeignSequence(', 'extern "C" void Combo_SOHSoaringAudioBegin(',
                  'extern "C" void Combo_SOHSoaringSound(', 'extern "C" void Combo_SOHSoaringAudioEnd(',
                  'extern "C" void Combo_SOHSoaringFanfare(', 'extern "C" int Combo_SOHMmAudioChannelCommand(']:
    code += function(signature)
# Execute the production synthesis storage branch. A CART-tagged archive sample
# must be reclassified after the resource factory allocates its host bytes;
# otherwise the N64 DMA cache receives an arbitrary 64-bit host address.
assert 'std::make_shared<ResidentMmSampleFactory>()' in text
synthesis = (source/'soh/src/code/audio_synthesis.c').read_text(encoding='utf-8')
begin = synthesis.index('if (audioFontSample->medium == MEDIUM_RAM)')
end = synthesis.index('if (sampleData == NULL)', begin)
code += '''
u8* resolveSample(SoundFontSample* audioFontSample){
    uintptr_t sampleAddr=reinterpret_cast<uintptr_t>(audioFontSample->sampleAddr);
    int sampleDataStart=0,sampleDataOffset=9,nFramesToDecode=1,frameSize=9,flags=0;
    struct {u8 sampleDmaIndex=0;} synth;auto* synthState=&synth;
    u8* sampleData=nullptr;u8* cmd=nullptr;
'''+synthesis[begin:end]+'''
    return sampleData;
}
void testResidentSamples(){
    u8 bytes[32]{};SoundFontSample sample{MEDIUM_CART,bytes};
    assert(resolveSample(&sample)==nullptr&&dmaCalls==1);
    UseResidentSampleStorage(&sample);
    assert(sample.medium==MEDIUM_RAM&&sample.sampleAddr==bytes);
    assert(resolveSample(&sample)==bytes+9&&dmaCalls==1);
    UseResidentSampleStorage(nullptr);
}
'''
# Exercise the reverse adapter's production fanfare/cache functions independently.
text = (source/'mm/2s2h/Enhancements/Songs/CrossWarpAudio.cpp').read_text(encoding='utf-8')
code += r"""
namespace Reverse {
enum {SEQUENCE_TABLE=0,FONT_TABLE=1};
struct SoundFont {};
std::array<SequenceData*,6> sSequences{};
std::array<bool,6> sScoresExecuted{};
const char* kSequences[]={"minuet","bolero","serenade","requiem","nocturne","prelude"};
int traces=0;
#undef SPDLOG_INFO
#define SPDLOG_INFO(...) (++traces)
SoundFont* sFont=nullptr;
int sSequenceBase=-1,sFontId=-1;
void AudioSfx_MuteBanks(int value){muted=value;}
"""
for signature in ['extern "C" int Combo_MMIsOotWarpSequence(',
                  'extern "C" void* Combo_MMWarpAudioCache(',
                  'extern "C" void Combo_MMTraceWarpSequence(',
                  'extern "C" void Combo_MMPlayWarpFanfare(']:
    code += function(signature)
text = (source/'mm/src/audio/lib/seqplayer.c').read_text(encoding='utf-8')
# Extract the actual foreign-only command dispatch, retaining continue semantics.
code += 'int translate(int id,int cmd){struct {int seqId;} player{id};auto* seqPlayer=&player;do{\n'
code += function('if (Combo_MMIsOotWarpSequence(seqPlayer->seqId))')
code += 'return cmd;}while(false);return -1;}\n'
args_start = text.index('#define CMD_ARGS_0()')
args_end = text.index('};', text.index('u8 sSeqInstructionArgsTable[]')) + 2
code += text[args_start:args_end] + '\n'
text = (source/'soh/src/code/audio_seqplayer.c').read_text(encoding='utf-8')
args_start = text.index('u8 D_80130520[]')
code += text[args_start:text.index('};',args_start)+2] + '\n'
code += r"""
void test(){
    fanfare=-1;muted=0;
    Combo_MMPlayWarpFanfare(0);assert(fanfare==-1&&muted==0);
    sSequenceBase=200;sFontId=60;
    for(int song=0;song<6;++song){
        fanfare=-1;muted=0;Combo_MMPlayWarpFanfare(song);
        assert(fanfare==200+song&&muted==0x20);
    }
    fanfare=-1;Combo_MMPlayWarpFanfare(-1);Combo_MMPlayWarpFanfare(6);assert(fanfare==-1);
    char bytes[6][4]{};SequenceData data[6];SoundFont font;sFont=&font;
    for(int i=0;i<6;++i){
        data[i].seqData=bytes[i];sSequences[i]=&data[i];
        assert(Combo_MMWarpAudioCache(SEQUENCE_TABLE,200+i)==bytes[i]);
        assert(Combo_MMIsOotWarpSequence(200+i));
        Combo_MMTraceWarpSequence(200+i,nullptr);assert(traces==i);
        Combo_MMTraceWarpSequence(200+i,bytes[i]);assert(traces==i+1);
        Combo_MMTraceWarpSequence(200+i,bytes[i]);assert(traces==i+1);
        assert(translate(200+i,0xBD)==0xA8);
        assert(translate(200+i,0xBE)==-1);
        for(int op=0xA0;op<0xB0;++op)assert(translate(200+i,op)==-1);
        assert(translate(200+i,0xC6)==0xC6);
    }
    assert(!Combo_MMWarpAudioCache(SEQUENCE_TABLE,199));
    assert(!Combo_MMWarpAudioCache(SEQUENCE_TABLE,206));
    assert(!Combo_MMWarpAudioCache(FONT_TABLE,59));
    assert(Combo_MMWarpAudioCache(FONT_TABLE,60)==&font);
    for(int id:{0,199,206})for(int op=0;op<256;++op)assert(translate(id,op)==op);
    // Decode contract: OoT BD and MM A8 both take two signed 16-bit values.
    assert(D_80130520[0xBD-0xB0]==0xC2);
    assert(sSeqInstructionArgsTable[0xA8-0xA0]==D_80130520[0xBD-0xB0]);
    assert(sSeqInstructionArgsTable[0xBD-0xA0]==0x81);
    assert(sSeqInstructionArgsTable[0xBE - 0xA0]==0x01); // 0xBE-0xA0 lexes as one pp-number (E-) in GCC/Clang
    sSequenceBase=sFontId=-1;sFont=nullptr;
    assert(!Combo_MMIsOotWarpSequence(200));assert(!Combo_MMWarpAudioCache(FONT_TABLE,-1));
}
}
"""
code += 'const int nativeCues[] = {' + ','.join(map(str, cues)) + '};\n'
code += r'''
int main(){
    testResidentSamples();
    Combo_SOHSoaringAudioBegin();Combo_SOHSoaringSound(0);Combo_SOHSoaringFanfare();
    assert(starts.empty()&&commands.empty()&&fanfare==-1);
    sSequenceBase=200;Combo_SOHSoaringFanfare();
    assert(gAudioContext.seqToPlay[1]==202&&gAudioContext.seqReplaced[1]==1&&fanfare==NA_BGM_OCA_LIGHT&&muted==0x20);
    assert(commands.back().first==0x71070035); // MM human-form native playback IO7
    gAudioContext.audioErrorFlags=0x1234;
    commands.clear();Combo_SOHSoaringAudioBegin();Combo_SOHSoaringAudioBegin();
    assert(sPreviousAudioErrors==0x1234&&gAudioContext.audioErrorFlags==0x1234);
    assert(starts.size()==2&&starts[0]==std::make_pair(0,200)&&starts[1]==std::make_pair(3,201));
    assert(mode==SEQ_MODE_IGNORE&&volumes[3]>0.719f&&volumes[3]<0.721f);
    commands.clear();Combo_SOHSoaringSound(-1);Combo_SOHSoaringSound(4);assert(commands.empty());
    for(int phase=0;phase<4;++phase){
        commands.clear();Combo_SOHSoaringSound(phase);
        assert(commands.size()==6);
        const u32 target=(3<<16)|((phase%3)<<8);
        assert(commands[3]==std::make_pair(0x06000000|target,1));
        assert(commands[4]==std::make_pair(0x06000004|target,nativeCues[phase]&255));
        assert(commands[5]==std::make_pair(0x06000005|target,((nativeCues[phase]&0x300)>>7)+((nativeCues[phase]&255)>>7)));
    }
    commands.clear();Combo_SOHSoaringAudioEnd();Combo_SOHSoaringAudioEnd();
    assert(commands.size()==2&&commands[0].first==66&&commands[1].first==0x13000000);
    assert(volumes[3]>0.409f&&volumes[3]<0.411f);
    char bytes[32]{};SequenceData data{bytes};sSequences[1]=&data;
    SequencePlayer player;SequenceChannel channel;channel.seqPlayer=&player;player.channels[2]=&channel;
    u8 arguments[]={0,0,0x44,0x55};channel.scriptState.pc=arguments;
    // Ordinary OoT BD must remain entirely in its native decoder.
    assert(!Combo_SOHMmAudioChannelCommand(&channel,0xBD)&&channel.scriptState.pc==arguments);
    player.seqData=reinterpret_cast<u8*>(bytes);
    assert(Combo_SOHMmAudioChannelCommand(&channel,0xBD)&&channel.scriptState.pc==arguments+2);
    // MM SFX script IO state round-trip; values are signed in the interpreter.
    sSfxState[2][9]=0x40;arguments[1]=9;channel.scriptState.pc=arguments;
    Combo_SOHMmAudioChannelCommand(&channel,0xA0);assert(channel.scriptState.value==64);
    channel.scriptState.value=-1;channel.scriptState.pc=arguments;
    Combo_SOHMmAudioChannelCommand(&channel,0xA2);assert(sSfxState[2][9]==255);
    channel.scriptState.pc=arguments;Combo_SOHMmAudioChannelCommand(&channel,0xA0);assert(channel.scriptState.value==-1);
    u8 randomArgs[]={0x01,0x00,0x01,0x00};channel.scriptState.pc=randomArgs;
    Combo_SOHMmAudioChannelCommand(&channel,0xA8);assert(channel.unk_22==0x8178&&channel.scriptState.pc==randomArgs+4);
    arguments[0]=0;channel.scriptState.pc=arguments;channel.scriptState.value=0;channel.stereo.asByte=255;channel.freqScale=0;
    StoreMmSfxFloat(0,4,0.75f);sSfxState[0][10]=0x40;
    Combo_SOHMmAudioChannelCommand(&channel,0xBE);assert(channel.stereo.asByte==0x40&&channel.freqScale==0.75f&&channel.changes.s.freqScale);
    assert(!Combo_SOHMmAudioChannelCommand(&channel,0xC6));assert(errors==0);
    assert(sSequenceExecuting[1]&&!sSequenceExecuting[0]&&!sSequenceExecuting[2]);
    // Unsupported requests are explicit failures, latched even after subsequent
    // supported instructions, and only reported once per kind.
    arguments[0]=0;arguments[1]=1;channel.scriptState.pc=arguments;
    Combo_SOHMmAudioChannelCommand(&channel,0xBD);assert(sDialectFailures==2&&errors==1);
    channel.scriptState.pc=arguments;Combo_SOHMmAudioChannelCommand(&channel,0xBD);assert(errors==1);
    arguments[1]=0;channel.scriptState.pc=arguments;
    Combo_SOHMmAudioChannelCommand(&channel,0xBD);assert(sDialectFailures==2&&errors==1);
    channel.scriptState.pc=arguments;Combo_SOHMmAudioChannelCommand(&channel,0xA4);assert(sDialectFailures==3&&errors==2);
    Reverse::test();
    puts("Native wing cues, both-game fanfares, volume restoration, resident caches and isolated sequence dialects passed.");
}
'''
test=work/'warp-audio.cpp';test.write_text(code,encoding='utf-8')
exe=hostcxx.build(a.compiler,test)
subprocess.run([str(exe)],check=True, timeout=hostcxx.RUN_TIMEOUT)

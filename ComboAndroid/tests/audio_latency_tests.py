"""Compile actual mobile queue/conversion and OoT batching against real SDL.
The hardware queue is simulated; SDL's streaming resampler is real.
This does not measure a phone's acoustic output latency.
"""
from pathlib import Path
import argparse, subprocess
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--source',type=Path,required=True)
p.add_argument('--sdl',type=Path,required=True)
p.add_argument('--build-dir',type=Path,required=True)
a=p.parse_args();a.build_dir.mkdir(parents=True,exist_ok=True)
def function(text, signature):
    start=text.index(signature);brace=text.index('{',start);depth=1;end=brace+1
    while depth:
        if text[end]=='{':depth+=1
        elif text[end]=='}':depth-=1
        end+=1
    return text[start:end]
player=(a.source/'libultraship/src/ship/audio/SDLAudioPlayer.cpp').read_text(encoding='utf-8')
context=(a.source/'libultraship/src/ship/Context.cpp').read_text(encoding='utf-8')
policy=context[context.index('#if defined(__ANDROID__) && defined(COMBO_BUILD)',context.index('bool Context::InitAudio')):context.index('    mAudio = std::make_shared<Audio>(settings);')]
oot=(a.source/'soh/soh/OTRGlobals.cpp').read_text(encoding='utf-8')
batch=oot[oot.index('#define SAMPLES_HIGH',oot.index('void OTRAudio_Thread()')):oot.index('    // Self-pump cadence.',oot.index('void OTRAudio_Thread()'))]
mm=(a.source/'mm/2s2h/BenPort.cpp').read_text(encoding='utf-8')
mm_batch=mm[mm.index('#define SAMPLES_HIGH'):mm.index('        audio.processing = false;',mm.index('#define SAMPLES_HIGH'))]
cpp=r'''
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <vector>
using u32=uint32_t;using u8=uint8_t;using s16=int16_t;
static std::vector<uint8_t> queued;
static std::recursive_mutex sAndroidAudioMutex;
static uint32_t TestQueueSize(SDL_AudioDeviceID){return static_cast<uint32_t>(queued.size());}
static int TestQueue(SDL_AudioDeviceID,const void* data,uint32_t n){
    const auto* b=static_cast<const uint8_t*>(data);queued.insert(queued.end(),b,b+n);return 0;
}
#define SDL_GetQueuedAudioSize TestQueueSize
#define SDL_QueueAudio TestQueue
#define SPDLOG_ERROR(...) assert(false)
namespace Ship {
class SDLAudioPlayer {
public:
    SDL_AudioDeviceID mDevice=1;
    int mNumChannels=2, mOutputRate=48000;
    uint64_t mSourceFrames=0,mConvertedSourceFrames=0;
    SDL_AudioStream* mConversion=nullptr;
    int GetSampleRate(){return 32000;}
    int Buffered();void DoPlay(const uint8_t*,size_t);
};
'''+function(player,'int SDLAudioPlayer::Buffered()')+'\n'+function(player,'void SDLAudioPlayer::DoPlay(')+r'''
}
static uint64_t generated=0,ticks=0;static int maxBatch=0;
static Ship::SDLAudioPlayer* simulatedPlayer=nullptr;
static void AudioMgr_CreateNextAudioBuffer(s16* p,u32 n){std::fill(p,p+n*2,s16(0));generated+=n;++ticks;}
static void AudioPlayer_Play(const u8* p,uint32_t n){
    maxBatch=std::max(maxBatch,int(n/4));
    if(simulatedPlayer)simulatedPlayer->DoPlay(p,n);
}
static void CheckBatch(){
    const int R_UPDATE_RATE=3;
'''+batch+r'''
    for(int i=0;i<600;i++)produce_next_batch();
#if defined(__ANDROID__) && defined(COMBO_BUILD)
    assert(ticks==600 && generated==320000 && maxBatch==544);
#else
    assert(ticks==1800 && generated==960000 && maxBatch==1632);
#endif
    (void)R_UPDATE_RATE;
}
static void CheckSchedule(){
#if defined(__ANDROID__) && defined(COMBO_BUILD)
    Ship::SDLAudioPlayer player;
    player.mConversion=SDL_NewAudioStream(AUDIO_S16SYS,2,32000,AUDIO_S16SYS,2,48000);
    assert(player.mConversion);queued.clear();simulatedPlayer=&player;
    generated=ticks=0;int gaps=0,peak=0;
'''+batch+r'''
    // Two simulated minutes at the real 5 ms self-pump cadence, independent
    // of graphics frame wakes. Detect starvation, runaway queues or tempo drift.
    for(int ms=0;ms<120000;ms++){
        if(ms%5==0)while(player.Buffered()+544<=1536)produce_next_batch();
        peak=std::max(peak,player.Buffered());
        const size_t bytes=48*4;
        if(ms>250 && queued.size()<bytes)++gaps;
        queued.erase(queued.begin(),queued.begin()+std::min(bytes,queued.size()));
    }
    assert(gaps==0 && peak<=1536);
    assert(std::abs(double(generated)/120-32000)<20);
    printf("OoT schedule: peak=%d source frames, gaps=%d, rate=%.2f frames/sec\n",peak,gaps,double(generated)/120);
    SDL_FreeAudioStream(player.mConversion);simulatedPlayer=nullptr;
#endif
}
static int AudioPlayer_Buffered(){return simulatedPlayer->Buffered();}
static int AudioPlayer_GetDesiredBuffered(){return 1536;}
static void CheckMMSchedule(){
#if defined(__ANDROID__) && defined(COMBO_BUILD)
    Ship::SDLAudioPlayer player;
    player.mConversion=SDL_NewAudioStream(AUDIO_S16SYS,2,32000,AUDIO_S16SYS,2,48000);
    assert(player.mConversion);queued.clear();simulatedPlayer=&player;
    generated=ticks=0;int gaps=0,peak=0;
    const int R_UPDATE_RATE=3;
    auto produce_mm=[](){
#undef SAMPLES_HIGH
#undef SAMPLES_LOW
#undef AUDIO_FRAMES_PER_UPDATE
'''+mm_batch+r'''
    };
    for(int ms=0;ms<120000;ms++){
        if(ms%50==0)produce_mm();
        peak=std::max(peak,player.Buffered());
        const size_t bytes=48*4;
        if(ms>250 && queued.size()<bytes)++gaps;
        queued.erase(queued.begin(),queued.begin()+std::min(bytes,queued.size()));
    }
    assert(gaps==0 && peak<=3216);
    assert(std::abs(double(generated)/120-32000)<30);
    printf("MM schedule: peak=%d source frames, gaps=%d, rate=%.2f frames/sec\n",peak,gaps,double(generated)/120);
    SDL_FreeAudioStream(player.mConversion);simulatedPlayer=nullptr;
#endif
}
int main(){
    struct {int SampleLength=1024,DesiredBuffered=4096;} settings;
'''+policy+r'''
#if defined(__ANDROID__) && defined(COMBO_BUILD)
    assert(settings.SampleLength==256 && settings.DesiredBuffered==1536);
    // Account for conversion history as well as the largest producer tick.
    assert(settings.DesiredBuffered>=512+544);
    for(int rate: {32000,44100,48000,96000,192000}) for(int channels: {2,6}) {
        Ship::SDLAudioPlayer player;player.mOutputRate=rate;player.mNumChannels=channels;
        if(rate!=32000)player.mConversion=SDL_NewAudioStream(AUDIO_S16SYS,channels,32000,AUDIO_S16SYS,channels,rate);
        assert(rate==32000 || player.mConversion);
        queued.clear();std::vector<int16_t> signal;uint64_t source=0,played=0;
        int maxBuffered=0,maxPending=0;
        for(int i=0;i<600;i++){
            const int n=i%3==1?544:528;std::vector<int16_t> in(n*channels);
            for(int f=0;f<n;f++)for(int ch=0;ch<channels;ch++)
                in[f*channels+ch]=int16_t(12000*sin(2*3.141592653589793*440*(source+f)/32000));
            player.DoPlay(reinterpret_cast<uint8_t*>(in.data()),in.size()*2);source+=n;
            maxBuffered=std::max(maxBuffered,player.Buffered());
            const size_t old=signal.size();signal.resize(old+queued.size()/2);
            memcpy(signal.data()+old,queued.data(),queued.size());played+=queued.size()/(2*channels);queued.clear();
            const int pending=player.Buffered();maxPending=std::max(maxPending,pending);
            assert(pending>=0 && pending<1024);
            assert(pending<=513);
            assert(std::abs(double(played)*32000/rate+pending-source)<source*0.001+2);
        }
        assert(source==320000);
        int crossings=0,maxStep=0;for(size_t i=channels;i<signal.size();i+=channels){
            if(signal[i-channels]<=0 && signal[i]>0)++crossings;
            maxStep=std::max(maxStep,std::abs(int(signal[i])-int(signal[i-channels])));
        }
        const double seconds=double(signal.size())/channels/rate;
        assert(std::abs(crossings/seconds-440)<1.0);assert(maxStep<2000);
        if(player.mConversion)SDL_FreeAudioStream(player.mConversion);
        player.mConversion=nullptr;player.mSourceFrames=player.mConvertedSourceFrames=0;
        assert(player.Buffered()==0);
        printf("rate=%d channels=%d pitch=%.2f Hz max_pending=%d max_buffered=%d\n",rate,channels,crossings/seconds,maxPending,maxBuffered);
    }
#else
    assert(settings.SampleLength==1024 && settings.DesiredBuffered==4096);
#endif
    CheckBatch();CheckSchedule();CheckMMSchedule();puts("PASS: source-frame accounting, continuous conversion, pitch, both game schedules, desktop preservation");
}
'''
source=a.build_dir/'audio_latency.cpp';source.write_text(cpp)
libs=['SDL2-static.lib','winmm.lib','imm32.lib','version.lib','setupapi.lib','user32.lib','gdi32.lib','shell32.lib','ole32.lib','oleaut32.lib','uuid.lib','advapi32.lib']
for android in [True,False]:
    exe=a.build_dir/('mobile.exe' if android else 'desktop.exe')
    args=['cl','/nologo','/std:c++20','/EHsc','/MD','/W4','/I'+str(a.sdl/'include'),str(source),'/Fe:'+str(exe),'/Fo:'+str(a.build_dir/'audio_latency.obj')]
    if android:args+=['/D__ANDROID__','/DCOMBO_BUILD']
    subprocess.run(args+['/link','/LIBPATH:'+str(a.sdl/'lib')]+libs,check=True)
    subprocess.run([str(exe)],check=True)

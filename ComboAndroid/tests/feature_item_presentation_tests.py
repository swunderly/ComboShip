"""Compile production item recipes/draw bodies; assert pinned models, colors and aliases."""
from pathlib import Path
import argparse,re,subprocess
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--build-dir',type=Path,required=True);p.add_argument('--compiler',default='cl');a=p.parse_args()
s=a.source.resolve();w=a.build_dir.resolve();w.mkdir(parents=True,exist_ok=True)
def function(text,sig):
    begin=text.index(sig);at=text.index('{',begin)+1;depth=1
    while depth:
        depth+=(text[at]=='{')-(text[at]=='}');at+=1
    return text[begin:at]+'\n'
oot=(s/'combo/menu/ComboItemDrawOOT.h').read_text();mm=(s/'combo/menu/ComboItemDrawMM.h').read_text()
native=(s/'soh/soh/Enhancements/randomizer/silver_rupees/SilverRupees.cpp').read_text()
data=(s/'soh/soh/Enhancements/randomizer/silver_rupees/SilverRupeeData.inc').read_text()
rows=re.findall(r'SR_ITEM\((RG_\w+),\s*\d+,\s*(true|false),',data)
assert len(rows)==44
names=re.findall(r'RANDO_ENUM_ITEM\((RG_\w+)\)',(s/'soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h').read_text())
code='#include <cassert>\n#include <cstdint>\n#include <cstring>\n#include <vector>\n#include <string>\n#include <cstdio>\n#include <memory>\n#include <array>\n#include <algorithm>\n#include "'+(s/'combo/menu/ComboItemDrawABI.h').as_posix()+'"\n'
code+='using RandomizerGet=int;using RandoItemId=int;struct Color_RGB8 {uint8_t r,g,b;};\nenum {'+','.join(names)+'};\n'
code+='enum {RI_WOODFALL_KEY_RING=1,RI_SNOWHEAD_KEY_RING,RI_GREAT_BAY_KEY_RING,RI_STONE_TOWER_KEY_RING};\n'
assets=['gGiWalletDL','gGiWalletRupeeOuterDL','gGiAdultWalletStringColorDL','gGiWalletStringDL','gGiWalletRupeeInnerDL','gGiRupeeInnerDL','gGiRupeeOuterDL']
for name in assets:code+='const char* '+name+'="'+name+'";\n'
for sig in ['static void CwLayerPrim(','static void CwLayerEnv(','static int32_t OOT_FillSilverDrawInfo(']:code+=function(oot,sig)
for sig in ['static CwDrawOp* MM_Op(','static void MM_OpV(','static void MM_OpColor(','static void MM_OpDL(']:code+=function(mm,sig)
code+='bool keyReady=true;const char* MM_ColorlessKeyPath(){return keyReady ? "colorless" : nullptr;}\n'
code+=function(mm,'static int32_t MM_FillRingDrawInfo(')
code+='struct NativeItem {RandomizerGet item;bool pouch;};NativeItem nativeItems[]={'+','.join('{'+name+','+pouch+'}' for name,pouch in rows)+'};\n'
code+=function(native,'const NativeItem* FindItem(')
code+='namespace Rando::SilverRupees { using ::FindItem; }\n'
code+=r"""
struct PlayState {struct {int gfxCtx=0;}state;};struct GetItemEntry {int drawItemId;};using Gfx=char;
std::vector<std::string> drawn;int prim[3],env[3];
struct Layer {std::string dl;int p[3],e[3];};std::vector<Layer> layers;
void color(int* target,int r,int g,int b){target[0]=r;target[1]=g;target[2]=b;}
void draw(const Gfx* dl){Layer v;v.dl=dl;std::memcpy(v.p,prim,sizeof prim);std::memcpy(v.e,env,sizeof env);layers.push_back(v);}
#define OPEN_DISPS(x) {
#define CLOSE_DISPS(x) }
#define Gfx_SetupDL_25Opa(x)
#define Gfx_SetupDL_25Xlu(x)
#define gSPMatrix(...)
#define gDPSetPrimColor(stream,min,lod,r,g,b,a) color(prim,r,g,b)
#define gDPSetEnvColor(stream,r,g,b,a) color(env,r,g,b)
#define gSPDisplayList(stream,dl) draw(dl)
"""
code+=function(native,'void DrawSilver(')
inventory=(s/'mm/src/code/z_inventory.c').read_text()
# Keep the real aligned declarations used by the sparse initializer. Copying
# only the initializer would either fail to compile or miss its alignment ABI.
aligned_assets=dict(re.findall(r'(static const ALIGN_ASSET\(2\) char (sCombo\w+Icon)\[\] = "[^"\n]+";)',inventory))
aligned_names=set(aligned_assets.values())
assert len(aligned_names)==4
code+='#include "'+(s/'mm/include/align_asset_macro.h').as_posix()+'"\n'
code+='\n'.join(aligned_assets)+'\n'
icons=re.search(r'TexturePtr gItemIcons\[256\] = \{([\s\S]*?)\n\};',inventory).group(1)
# Select the real combined-build branch and preserve every sparse index.
icons=re.sub(r'#ifdef COMBO_BUILD\n([\s\S]*?)#else\n[\s\S]*?#endif',r'\1',icons)
icons=icons.replace('#ifdef COMBO_BUILD','').replace('#endif','')
icons=re.sub(r'//[^\n]*','',icons)
item_header=(s/'mm/include/z64item.h').read_text()
custom_ids={name:int(value,16) for name,value in re.findall(r'(ITEM_COMBO_\w+)\s*=\s*(0x[0-9A-Fa-f]+)',item_header)}
assignments=[];index=0
for entry in icons.split(','):
    entry=entry.strip()
    if not entry:continue
    explicit=re.match(r'\[(ITEM_COMBO_\w+)\]\s*=\s*\(TexturePtr\)(.*)',entry)
    if explicit:index=custom_ids[explicit[1]];value=explicit[2]
    else:value=entry if entry.startswith('"') or entry in aligned_names else '"'+entry+'"'
    assignments.append(f'v[{index}]={value};');index+=1
code+='using TexturePtr=const char*;auto gItemIcons=[](){std::array<TexturePtr,256> v{};'+''.join(assignments)+'return v;}();\n'
code+='void CheckNativeAssetAlignment(){'+''.join('static_assert(__alignof('+name+')>=2);assert((reinterpret_cast<uintptr_t>('+name+')&1)==0);' for name in sorted(aligned_names))+'}\n'
code+=r"""
TexturePtr gQuestIcons[]={"quest"},gBombersNotebookPhotos[]={"photo"};
const char* miscellaneousTextures[]={"misc"};const char* digitList[]={"digit"};
struct ImVec4{ImVec4(float,float,float,float){}};
std::vector<std::string> registeredIcons;
struct GuiBase{virtual ~GuiBase()=default;};
namespace Fast{struct Fast3dGui:GuiBase{void LoadGuiTexture(const std::string& id,const std::string& path,const char*,ImVec4){assert(!id.empty()&&id==path);registeredIcons.push_back(path);}};}
struct Window{std::shared_ptr<GuiBase> GetGui(){return std::make_shared<Fast::Fast3dGui>();}};
namespace Ship{struct Context{static Context* GetRawInstance(){static Context c;return &c;}std::shared_ptr<Window> GetWindow(){return std::make_shared<Window>();}};}
"""
code+=function((s/'mm/2s2h/ShipUtils.cpp').read_text(),'void LoadGuiTextures()')
code+=r"""
int main(){
    CheckNativeAssetAlignment();
    size_t populated=0;for(auto icon:gItemIcons)if(icon)++populated;
    assert(populated>100&&populated<gItemIcons.size());
    LoadGuiTextures();assert(registeredIcons.size()==populated+4);
    for(auto icon:gItemIcons)if(icon)assert(std::find(registeredIcons.begin(),registeredIcons.end(),icon)!=registeredIcons.end());
    PlayState play;
    for(auto row:nativeItems){
        CwItemDrawInfo info{};assert(OOT_FillSilverDrawInfo(row.item,&info)==1);
        assert(info.drawKind==CW_DRAW_KIND_COLOR_LAYERS);
        GetItemEntry entry{row.item};layers.clear();DrawSilver(&play,&entry);
        assert(layers.size()==size_t(info.dlistCount));
        for(int i=0;i<info.dlistCount;++i){
            assert(layers[i].dl==info.dlists[i]);
            if(info.layerPrimMask&(1<<i))for(int j=0;j<3;++j)assert(layers[i].p[j]==info.layerPrimColor[i][j]);
            if(info.layerEnvMask&(1<<i))for(int j=0;j<3;++j)assert(layers[i].e[j]==info.layerEnvColor[i][j]);
        }
        if(row.pouch){assert(info.dlistCount==5&&info.xluStartIndex==-1);assert(info.layerPrimColor[0][0]==50&&info.layerEnvColor[0][0]==15);assert(std::string(info.dlists[0])=="gGiWalletDL");}
        else {assert(info.dlistCount==2&&info.xluStartIndex==1);assert(info.layerPrimColor[0][0]==255&&info.layerEnvColor[0][0]==50);}
    }
    CwItemDrawInfo unsupported{};assert(!OOT_FillSilverDrawInfo(RG_NONE,&unsupported));
    int expected[4][3]={{236,120,186},{129,173,70},{99,90,183},{177,165,83}};
    for(int id=1;id<=4;++id){CwItemDrawInfo info{};assert(MM_FillRingDrawInfo(id,&info)==1);assert(info.drawKind==CW_DRAW_KIND_OPS&&info.dlistCount==1&&info.opCount==5);assert(info.ops[2].op==CW_OP_PRIM_COLOR&&info.ops[2].a==128);assert(info.ops[3].op==CW_OP_ENV_COLOR);for(int j=0;j<3;++j)assert(info.ops[3].rgba[j]==expected[id-1][j]);}
    keyReady=false;CwItemDrawInfo unavailable{};assert(MM_FillRingDrawInfo(1,&unavailable)==CW_DRAW_NOT_READY);
    puts("All 44 native/foreign silver recipes and four native-color MM ring recipes passed.");
}
"""
test=w/'feature-item-presentation.cpp';test.write_text(code)
exe=w/'feature-item-presentation.exe'
cmd=[a.compiler,'/nologo','/std:c++20','/EHsc','/UNDEBUG',str(test),'/Fe:'+str(exe),'/Fo:'+str(w/'feature-item-presentation.obj')]
r=subprocess.run(cmd,capture_output=True,text=True);(w/'compile.log').write_text(r.stdout+r.stderr)
if r.returncode:print(r.stdout+r.stderr)
r.check_returncode();subprocess.run([str(exe)],check=True)

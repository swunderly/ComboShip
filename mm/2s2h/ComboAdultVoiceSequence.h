#pragma once
// Ported from OoTMM 41a2b7c0 custom/index.ts (MIT); see ComboAdultUpstream.LICENSE.
#include "ComboAdultVoiceRecipe.h"
#include <map>
#include <string>
#include <stdexcept>
namespace ComboAdultMM {
class AdultAseqBuilder {
    size_t base;std::vector<uint8_t> bytes;std::map<std::string,size_t> labels;
    std::vector<std::pair<size_t,std::string>> fixups;
public:
    explicit AdultAseqBuilder(size_t start):base(start){}
    void Label(const std::string& name){labels[name]=base+bytes.size();}
    size_t Address(const std::string& name)const{return labels.at(name);}
    void U8(std::initializer_list<unsigned> values){for(auto x:values)bytes.push_back(uint8_t(x));}
    void Ptr(unsigned op,const std::string& target,int argument=-1){U8({op});if(argument>=0)U8({unsigned(argument)});fixups.push_back({bytes.size(),target});U8({0,0});}
    std::vector<uint8_t> Finish(){for(auto& f:fixups){size_t at=Address(f.second);if(at>65535)throw std::runtime_error("Adult voice sequence exceeds native16-bit targets");bytes[f.first]=at>>8;bytes[f.first+1]=at;}return bytes;}
};
inline unsigned AdultEffectSlot(unsigned effect){for(size_t i=0;i<AdultVoiceEffects.size();++i)if(AdultVoiceEffects[i]==effect)return i;throw std::runtime_error("Unknown pinned adult voice effect");}
inline bool PatchAdultVoiceSequence(const uint8_t* seq,size_t length,size_t adultBase,std::vector<uint8_t>& out) {
    out.clear();if(!seq||length<0xB34E + 0x40||length>65535||adultBase%64||adultBase/64>255)return false;
    try {
        AdultAseqBuilder b(length);const unsigned block=adultBase>>6;
        auto random=[&](const std::string& n,std::vector<unsigned> effects,std::vector<unsigned> velocities){
            b.Label(n);b.U8({0xB8,unsigned(effects.size()),0x77,0x56});b.Ptr(0xFA,n+"D");b.U8({0x67});b.Ptr(0xFB,n+"S");
            b.Label(n+"D");b.U8({0x67,0xC8,0xFF,0x77,0xC8,unsigned(effects.size())});b.Ptr(0xFA,n+"W");b.U8({0x67});b.Ptr(0xFB,n+"S");
            b.Label(n+"W");b.U8({0xCC,0,0x77});b.Label(n+"S");b.U8({0x76,0x3E,6,0x3F,6});b.Ptr(0xCB,n+"E");b.Ptr(0xC7,n+"N",0x40);b.U8({0x67});b.Ptr(0xCB,n+"V");b.Ptr(0xC7,n+"X",0);b.Ptr(0x88,n+"L");b.U8({0xFF});
            b.Label(n+"E");for(auto e:effects)b.U8({AdultEffectSlot(e)});b.Label(n+"V");for(auto v:velocities)b.U8({v});b.Label(n+"L");b.U8({0xC2,block});b.Label(n+"N");b.U8({0x40,0});b.Label(n+"X");b.U8({0x7F,0xFF});
        };
        random("adultSwordN",{0,1,2,3},{105,105,105,105});random("adultSwordL",{4,5},{110,110});random("adultLash",{21,22},{105,105});random("adultHang",{6,25},{95,105});random("adultClimbEnd",{7,8},{72,80});random("adultDamageS",{9,10,11},{117,117,117});random("adultFreeze",{12,13,14},{113,113,113});random("adultFallS",{17,18},{100,100});random("adultFallL",{15,16},{110,110});random("adultBreathRest",{19,23},{90,90});random("adultTakenAway",{15,16},{105,105});
        auto fixed=[&](const std::string& n,std::vector<std::vector<unsigned>> notes){b.Label(n);b.Ptr(0x88,n+"L");b.U8({0xFF});b.Label(n+"L");b.U8({0xC2,block});for(auto& note:notes){b.U8({0x40+AdultEffectSlot(note[0])});for(size_t i=1;i<note.size();++i)b.U8({note[i]});}b.U8({0xFF});};
        fixed("adultBreathDrink",{{56,0,96}});fixed("adultDown",{{77,0x57,0x64},{78,0x61,0x64},{79,0x47,0x64}});fixed("adultSneeze",{{80,0x7F,0x64},{81,0x81,0x18,0x64},{82,0x81,0x3E,0x64}});fixed("adultSweat",{{83,0x81,0x22,0x64},{84,0x80,0xA3,0x64},{85,0x35,0x64}});fixed("adultRelax",{{86,0x80,0xB9,0x64},{87,0x80,0x86,0x64},{88,0x74,0x64}});
        fixed("adultSwordPutaway",{{0,0,100}});fixed("adultGroan",{{67,0,50}});fixed("adultMagicNale",{{5,0,110}});fixed("adultSurprise",{{134,0,85}});fixed("adultMagicFrol",{{4,0,95}});fixed("adultPush",{{7,0,82}});fixed("adultHookshotHang",{{6,0,95}});fixed("adultLandDamage",{{24,0,110}});fixed("adultNull1B",{{60,0,100}});fixed("adultMagicAttack",{{61,0,110}});fixed("adultDemoDamage",{{13,0,113}});
        b.Label("adultDrink");b.Ptr(0x88,"adultDrinkL");b.U8({0xFF});b.Label("adultDrinkL");b.U8({0xC2,block,0x40+AdultEffectSlot(55),0x50,0x50,0xF4,0xFB});
        b.Label("adultAutoJump");b.U8({0x66,0xC8,0xFF,0xC9,1,0x77,0x76,0x3E,6,0x3F,6});b.Ptr(0xCB,"adultAutoJumpE");b.Ptr(0xC7,"adultAutoJumpN",0x40);b.U8({0xB8,2});b.Ptr(0xCB,"adultAutoJumpV");b.Ptr(0xC7,"adultAutoJumpW",0);b.Ptr(0x88,"adultAutoJumpL");b.U8({0xFF});b.Label("adultAutoJumpE");b.U8({AdultEffectSlot(26),AdultEffectSlot(27)});b.Label("adultAutoJumpV");b.U8({0x50,0x55});b.Label("adultAutoJumpL");b.U8({0xC2,block});b.Label("adultAutoJumpN");b.U8({0x40,0});b.Label("adultAutoJumpW");b.U8({0x50,0xFF});
        const char* targets[]={"adultSwordN","adultSwordL","adultLash","adultHang","adultClimbEnd","adultDamageS","adultFreeze","adultFallS","adultFallL","adultBreathRest","adultBreathDrink","adultDown","adultTakenAway","adultDamageS","adultSneeze","adultSweat","adultDrink","adultRelax","adultSwordPutaway","adultGroan","adultAutoJump","adultMagicNale","adultSurprise","adultMagicFrol","adultPush","adultHookshotHang","adultLandDamage","adultNull1B","adultMagicAttack","adultFallL","adultDemoDamage","adultSwordN"};
        auto body=b.Finish();out.assign(seq,seq+length);out.insert(out.end(),body.begin(),body.end());out.insert(out.end(),seq+0xB34E,seq+0xB34E + 0x40);
        for(auto t:targets){size_t at=b.Address(t);out.push_back(at>>8);out.push_back(at);}
        return true;
    }catch(const std::exception&){out.clear();return false;}
}
}

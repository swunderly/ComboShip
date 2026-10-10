#pragma once
#include "OotSoulPolicy.h"
#include <array>
#include <cstdint>
#include <stdexcept>
#include <nlohmann/json.hpp>
namespace Rando::Souls {
inline constexpr unsigned GeneralOotSoulCount=95;
using GeneralOotSoulsOwnership=std::array<uint8_t,12>;
inline constexpr std::array<OotSoul,GeneralOotSoulCount> generalOotSoulItems={
    OotSoul::ENEMY_POE,
    OotSoul::ENEMY_STALFOS,
    OotSoul::ENEMY_OCTOROK,
    OotSoul::ENEMY_WALLMASTER,
    OotSoul::ENEMY_DODONGO,
    OotSoul::ENEMY_KEESE,
    OotSoul::ENEMY_TEKTITE,
    OotSoul::ENEMY_PEAHAT,
    OotSoul::ENEMY_LIZALFOS_DINOLFOS,
    OotSoul::ENEMY_GOHMA_LARVA,
    OotSoul::ENEMY_SHABOM,
    OotSoul::ENEMY_BABY_DODONGO,
    OotSoul::ENEMY_DARK_LINK,
    OotSoul::ENEMY_BIRI_BARI,
    OotSoul::ENEMY_TAILPASARN,
    OotSoul::ENEMY_SKULLTULA,
    OotSoul::ENEMY_TORCH_SLUG,
    OotSoul::ENEMY_MOBLIN,
    OotSoul::ENEMY_ARMOS,
    OotSoul::ENEMY_DEKU_BABA,
    OotSoul::ENEMY_DEKU_SCRUB,
    OotSoul::ENEMY_BUBBLE,
    OotSoul::ENEMY_BEAMOS,
    OotSoul::ENEMY_FLOORMASTER,
    OotSoul::ENEMY_REDEAD_GIBDO,
    OotSoul::ENEMY_SKULLWALLTULA,
    OotSoul::ENEMY_FLARE_DANCER,
    OotSoul::ENEMY_DEAD_HAND,
    OotSoul::ENEMY_SHELL_BLADE,
    OotSoul::ENEMY_LIKE_LIKE,
    OotSoul::ENEMY_SPIKE,
    OotSoul::ENEMY_ANUBIS,
    OotSoul::ENEMY_IRON_KNUCKLE,
    OotSoul::ENEMY_SKULL_KID,
    OotSoul::ENEMY_FLYING_POT,
    OotSoul::ENEMY_FREEZARD,
    OotSoul::ENEMY_STINGER,
    OotSoul::ENEMY_WOLFOS,
    OotSoul::ENEMY_GUAY,
    OotSoul::ENEMY_PARASITE,
    OotSoul::ENEMY_LEEVER,
    OotSoul::ENEMY_STALCHILD,
    OotSoul::ENEMY_THIEVES,
    OotSoul::NPC_OLD_HAG,
    OotSoul::NPC_CARPET_MAN,
    OotSoul::NPC_GROG,
    OotSoul::NPC_GORMAN,
    OotSoul::NPC_SCIENTIST,
    OotSoul::NPC_BEAN_SALESMAN,
    OotSoul::NPC_HONEY_DARLING,
    OotSoul::NPC_CHEST_GAME_OWNER,
    OotSoul::NPC_COMPOSER_BROS,
    OotSoul::NPC_SARIA,
    OotSoul::NPC_DARUNIA,
    OotSoul::NPC_RUTO,
    OotSoul::NPC_KING_ZORA,
    OotSoul::NPC_ANJU,
    OotSoul::NPC_CARPENTERS,
    OotSoul::NPC_GURU_GURU,
    OotSoul::NPC_MIDO,
    OotSoul::NPC_KOKIRI,
    OotSoul::NPC_KOKIRI_SHOPKEEPER,
    OotSoul::NPC_POTION_SHOPKEEPER,
    OotSoul::NPC_BOMBCHU_SHOPKEEPER,
    OotSoul::NPC_BAZAAR_SHOPKEEPER,
    OotSoul::NPC_ZORA_SHOPKEEPER,
    OotSoul::NPC_GORON_SHOPKEEPER,
    OotSoul::NPC_HYLIAN_GUARD,
    OotSoul::NPC_ROOFTOP_MAN,
    OotSoul::NPC_CITIZEN,
    OotSoul::NPC_DOG_LADY,
    OotSoul::NPC_BANKER,
    OotSoul::NPC_ASTRONOMER,
    OotSoul::NPC_MALON,
    OotSoul::NPC_TALON,
    OotSoul::NPC_FISHING_POND_OWNER,
    OotSoul::NPC_GORON,
    OotSoul::NPC_GORON_CHILD,
    OotSoul::NPC_BIGGORON,
    OotSoul::NPC_MEDIGORON,
    OotSoul::NPC_ZORA,
    OotSoul::NPC_BOMBCHU_BOWLING_LADY,
    OotSoul::NPC_SHOOTING_GALLERY_OWNER,
    OotSoul::NPC_DAMPE,
    OotSoul::NPC_BOMBERS,
    OotSoul::NPC_POE_COLLECTOR,
    OotSoul::NPC_SHEIK,
    OotSoul::NPC_ZELDA,
    OotSoul::NPC_THIEVES,
    OotSoul::ANIMAL_CUCCO,
    OotSoul::ANIMAL_COW,
    OotSoul::ANIMAL_DOG,
    OotSoul::ANIMAL_BUTTERFLY,
    OotSoul::MISC_BUSINESS_SCRUB,
    OotSoul::MISC_GS,
};
inline int GeneralOotSoulIndex(OotSoul soul) {
 for(unsigned i=0;i<GeneralOotSoulCount;++i)if(generalOotSoulItems[i]==soul)return i;
 return -1;
}
inline bool GeneralOotSoulOwned(const GeneralOotSoulsOwnership& owned,unsigned index) {
 return index<GeneralOotSoulCount && (owned[index>>3]&(1u<<(index&7)));
}
inline bool ChangeGeneralOotSoul(GeneralOotSoulsOwnership& owned,unsigned index,bool grant) {
 if(index>=GeneralOotSoulCount)return false;
 if(grant)owned[index>>3]|=1u<<(index&7);else owned[index>>3]&=~(1u<<(index&7));
 return true;
}
inline GeneralOotSoulsOwnership RestoreGeneralOotSouls(const nlohmann::json& saved) {
 GeneralOotSoulsOwnership out{};
 if(!saved.contains("ootExpandedSoulsOwned"))return out;
 const auto& source=saved.at("ootExpandedSoulsOwned");
 if(!source.is_array() || source.size()!=out.size())throw std::runtime_error("Invalid OoT soul ownership length");
 for(unsigned i=0;i<out.size();++i){
  if(!source[i].is_number_integer())throw std::runtime_error("Invalid OoT soul ownership type");
  const int64_t value=source[i].get<int64_t>();
  if(value<0 || value>(i==11?127:255))throw std::runtime_error("Invalid OoT soul ownership bits");
  out[i]=static_cast<uint8_t>(value);
 }
 return out;
}
struct GeneralOotSoulOptions {bool enabled=false,enemy=false,npc=false,animal=false,misc=false;};
inline bool GeneralOotSoulShuffled(const GeneralOotSoulOptions& options,unsigned index) {
 if(!options.enabled || index>=95)return false;
 return index<43?options.enemy:index<89?options.npc:index<93?options.animal:options.misc;
}
inline bool HaveGeneralOotSoul(const GeneralOotSoulsOwnership& owned,const GeneralOotSoulOptions& options,OotSoul soul) {
 const int index=GeneralOotSoulIndex(soul);
 return index>=0 && (!GeneralOotSoulShuffled(options,index) || GeneralOotSoulOwned(owned,index));
}
}

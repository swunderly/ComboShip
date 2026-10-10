#pragma once
#include <nlohmann/json.hpp>
namespace ComboRando {
struct SpellRules {
 bool spellFireMm=false,spellLoveMm=false;
 nlohmann::json json()const{return {{"version",1},{"spellFireMm",spellFireMm},{"spellLoveMm",spellLoveMm}};}
};
inline bool ReadSpellRules(const nlohmann::json& j,SpellRules& out){
 out={};if(j.is_null())return true;
 if(!j.is_object()||!j.contains("version")||!j["version"].is_number_integer()||j["version"]!=1)return false;
 auto flag=[&](const char* key,bool& value){
  if(!j.contains(key))return false;const auto& v=j[key];
  if(v.is_boolean()){value=v.get<bool>();return true;}
  if(!v.is_number_integer()||v<0||v>1)return false;value=v==1;return true;
 };
 SpellRules r;if(!flag("spellFireMm",r.spellFireMm)||!flag("spellLoveMm",r.spellLoveMm))return false;
 out=r;return true;
}
inline SpellRules SpellRulesFromMmDump(const nlohmann::json& dump){
 const auto options=dump.value("options",nlohmann::json::object());
 auto yes=[&](const char* key){auto i=options.find(key);return i!=options.end()&&i->is_number_integer()&&*i==1;};
 return {yes("RO_MM_SPELL_FIRE"),yes("RO_MM_SPELL_LOVE")};
}
namespace SpellOracle {
// macros_common.yml can_use_din_raw / can_use_nayru_raw, MM options wrap the raw predicate.
inline bool CanUse(bool enabled,bool magic,bool shared,bool localOwned,bool sharedOwned){
 return enabled&&magic&&(shared?sharedOwned:localOwned);
}
}
}

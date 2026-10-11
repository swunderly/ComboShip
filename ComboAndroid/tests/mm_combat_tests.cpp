#include "rando/MmCombatRules.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
int main(int argc,char** argv) {
 if(argc!=2)return 2;
 nlohmann::json requests;std::ifstream(argv[1])>>requests;
 if(requests.at("commit")!="413573d22d1d53e8a81931a70927f625b73488b9")return 4;
 const auto& cases=requests.at("cases");
 if(cases.size()!=111)return 5;
 int compared=0,failures=0;
 for(const auto& q:cases) {
  if(!q.contains("combat"))return 6;
  auto caps=q.at("combatCapabilities");
  ComboRando::MmCombatCapabilities c{caps.at("sword").get<bool>(),caps.at("deku").get<bool>(),caps.at("goron").get<bool>(),caps.at("zora").get<bool>(),caps.at("deity").get<bool>(),caps.at("giant").get<bool>(),caps.at("magic").get<bool>(),caps.at("arrows").get<bool>(),caps.at("hookshot").get<bool>(),caps.at("explosives").get<bool>(),caps.at("nuts").get<bool>(),caps.at("stickFighting").get<bool>(),caps.at("fireArrows").get<bool>(),caps.at("iceArrows").get<bool>(),caps.at("lightArrows").get<bool>(),caps.at("mirrorShield").get<bool>(),caps.at("swim").get<bool>(),caps.at("day").get<bool>(),caps.at("twinmoldBow").get<bool>(),caps.at("twinmoldFireIce").get<bool>()};
  if(!q.at("combat").at("can_fight_pirates").is_boolean()) {std::cerr<<"Unverified reference predicate can_fight_pirates";return 3;}
  ++compared;
  if(ComboRando::can_fight_pirates(c)!=q.at("combat").at("can_fight_pirates").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_fight_pirates\n";}
  if(!q.at("combat").at("can_kill_aliens").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_aliens";return 3;}
  ++compared;
  if(ComboRando::can_kill_aliens(c)!=q.at("combat").at("can_kill_aliens").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_aliens\n";}
  if(!q.at("combat").at("can_kill_armos").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_armos";return 3;}
  ++compared;
  if(ComboRando::can_kill_armos(c)!=q.at("combat").at("can_kill_armos").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_armos\n";}
  if(!q.at("combat").at("can_kill_baba_nuts").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_baba_nuts";return 3;}
  ++compared;
  if(ComboRando::can_kill_baba_nuts(c)!=q.at("combat").at("can_kill_baba_nuts").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_baba_nuts\n";}
  if(!q.at("combat").at("can_kill_bad_bats_single").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_bad_bats_single";return 3;}
  ++compared;
  if(ComboRando::can_kill_bad_bats_single(c)!=q.at("combat").at("can_kill_bad_bats_single").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_bad_bats_single\n";}
  if(!q.at("combat").at("can_kill_beamos").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_beamos";return 3;}
  ++compared;
  if(ComboRando::can_kill_beamos(c)!=q.at("combat").at("can_kill_beamos").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_beamos\n";}
  if(!q.at("combat").at("can_kill_big_poe").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_big_poe";return 3;}
  ++compared;
  if(ComboRando::can_kill_big_poe(c)!=q.at("combat").at("can_kill_big_poe").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_big_poe\n";}
  if(!q.at("combat").at("can_kill_bio_babas").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_bio_babas";return 3;}
  ++compared;
  if(ComboRando::can_kill_bio_babas(c)!=q.at("combat").at("can_kill_bio_babas").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_bio_babas\n";}
  if(!q.at("combat").at("can_kill_boes").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_boes";return 3;}
  ++compared;
  if(ComboRando::can_kill_boes(c)!=q.at("combat").at("can_kill_boes").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_boes\n";}
  if(!q.at("combat").at("can_kill_bubbles").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_bubbles";return 3;}
  ++compared;
  if(ComboRando::can_kill_bubbles(c)!=q.at("combat").at("can_kill_bubbles").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_bubbles\n";}
  if(!q.at("combat").at("can_kill_chuchu").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_chuchu";return 3;}
  ++compared;
  if(ComboRando::can_kill_chuchu(c)!=q.at("combat").at("can_kill_chuchu").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_chuchu\n";}
  if(!q.at("combat").at("can_kill_deep_python").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_deep_python";return 3;}
  ++compared;
  if(ComboRando::can_kill_deep_python(c)!=q.at("combat").at("can_kill_deep_python").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_deep_python\n";}
  if(!q.at("combat").at("can_kill_dexihands").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_dexihands";return 3;}
  ++compared;
  if(ComboRando::can_kill_dexihands(c)!=q.at("combat").at("can_kill_dexihands").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_dexihands\n";}
  if(!q.at("combat").at("can_kill_dodongos").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_dodongos";return 3;}
  ++compared;
  if(ComboRando::can_kill_dodongos(c)!=q.at("combat").at("can_kill_dodongos").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_dodongos\n";}
  if(!q.at("combat").at("can_kill_dragonflies").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_dragonflies";return 3;}
  ++compared;
  if(ComboRando::can_kill_dragonflies(c)!=q.at("combat").at("can_kill_dragonflies").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_dragonflies\n";}
  if(!q.at("combat").at("can_kill_eyegore").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_eyegore";return 3;}
  ++compared;
  if(ComboRando::can_kill_eyegore(c)!=q.at("combat").at("can_kill_eyegore").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_eyegore\n";}
  if(!q.at("combat").at("can_kill_eyegore_distance").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_eyegore_distance";return 3;}
  ++compared;
  if(ComboRando::can_kill_eyegore_distance(c)!=q.at("combat").at("can_kill_eyegore_distance").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_eyegore_distance\n";}
  if(!q.at("combat").at("can_kill_floormasters").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_floormasters";return 3;}
  ++compared;
  if(ComboRando::can_kill_floormasters(c)!=q.at("combat").at("can_kill_floormasters").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_floormasters\n";}
  if(!q.at("combat").at("can_kill_freezard_short_range").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_freezard_short_range";return 3;}
  ++compared;
  if(ComboRando::can_kill_freezard_short_range(c)!=q.at("combat").at("can_kill_freezard_short_range").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_freezard_short_range\n";}
  if(!q.at("combat").at("can_kill_garo").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_garo";return 3;}
  ++compared;
  if(ComboRando::can_kill_garo(c)!=q.at("combat").at("can_kill_garo").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_garo\n";}
  if(!q.at("combat").at("can_kill_garo_master").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_garo_master";return 3;}
  ++compared;
  if(ComboRando::can_kill_garo_master(c)!=q.at("combat").at("can_kill_garo_master").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_garo_master\n";}
  if(!q.at("combat").at("can_kill_gekko_gbt").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_gekko_gbt";return 3;}
  ++compared;
  if(ComboRando::can_kill_gekko_gbt(c)!=q.at("combat").at("can_kill_gekko_gbt").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_gekko_gbt\n";}
  if(!q.at("combat").at("can_kill_gekko_wft").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_gekko_wft";return 3;}
  ++compared;
  if(ComboRando::can_kill_gekko_wft(c)!=q.at("combat").at("can_kill_gekko_wft").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_gekko_wft\n";}
  if(!q.at("combat").at("can_kill_goht").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_goht";return 3;}
  ++compared;
  if(ComboRando::can_kill_goht(c)!=q.at("combat").at("can_kill_goht").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_goht\n";}
  if(!q.at("combat").at("can_kill_gomess").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_gomess";return 3;}
  ++compared;
  if(ComboRando::can_kill_gomess(c)!=q.at("combat").at("can_kill_gomess").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_gomess\n";}
  if(!q.at("combat").at("can_kill_guays").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_guays";return 3;}
  ++compared;
  if(ComboRando::can_kill_guays(c)!=q.at("combat").at("can_kill_guays").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_guays\n";}
  if(!q.at("combat").at("can_kill_gyorg").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_gyorg";return 3;}
  ++compared;
  if(ComboRando::can_kill_gyorg(c)!=q.at("combat").at("can_kill_gyorg").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_gyorg\n";}
  if(!q.at("combat").at("can_kill_igos").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_igos";return 3;}
  ++compared;
  if(ComboRando::can_kill_igos(c)!=q.at("combat").at("can_kill_igos").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_igos\n";}
  if(!q.at("combat").at("can_kill_keese_close").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_keese_close";return 3;}
  ++compared;
  if(ComboRando::can_kill_keese_close(c)!=q.at("combat").at("can_kill_keese_close").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_keese_close\n";}
  if(!q.at("combat").at("can_kill_keeta").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_keeta";return 3;}
  ++compared;
  if(ComboRando::can_kill_keeta(c)!=q.at("combat").at("can_kill_keeta").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_keeta\n";}
  if(!q.at("combat").at("can_kill_leevers").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_leevers";return 3;}
  ++compared;
  if(ComboRando::can_kill_leevers(c)!=q.at("combat").at("can_kill_leevers").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_leevers\n";}
  if(!q.at("combat").at("can_kill_like_like").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_like_like";return 3;}
  ++compared;
  if(ComboRando::can_kill_like_like(c)!=q.at("combat").at("can_kill_like_like").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_like_like\n";}
  if(!q.at("combat").at("can_kill_lizalfos_dinolfos").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_lizalfos_dinolfos";return 3;}
  ++compared;
  if(ComboRando::can_kill_lizalfos_dinolfos(c)!=q.at("combat").at("can_kill_lizalfos_dinolfos").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_lizalfos_dinolfos\n";}
  if(!q.at("combat").at("can_kill_octoroks").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_octoroks";return 3;}
  ++compared;
  if(ComboRando::can_kill_octoroks(c)!=q.at("combat").at("can_kill_octoroks").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_octoroks\n";}
  if(!q.at("combat").at("can_kill_odolwa").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_odolwa";return 3;}
  ++compared;
  if(ComboRando::can_kill_odolwa(c)!=q.at("combat").at("can_kill_odolwa").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_odolwa\n";}
  if(!q.at("combat").at("can_kill_peahats").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_peahats";return 3;}
  ++compared;
  if(ComboRando::can_kill_peahats(c)!=q.at("combat").at("can_kill_peahats").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_peahats\n";}
  if(!q.at("combat").at("can_kill_poe_sisters").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_poe_sisters";return 3;}
  ++compared;
  if(ComboRando::can_kill_poe_sisters(c)!=q.at("combat").at("can_kill_poe_sisters").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_poe_sisters\n";}
  if(!q.at("combat").at("can_kill_scrubs_anywhere").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_scrubs_anywhere";return 3;}
  ++compared;
  if(ComboRando::can_kill_scrubs_anywhere(c)!=q.at("combat").at("can_kill_scrubs_anywhere").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_scrubs_anywhere\n";}
  if(!q.at("combat").at("can_kill_shell_blades_underwater").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_shell_blades_underwater";return 3;}
  ++compared;
  if(ComboRando::can_kill_shell_blades_underwater(c)!=q.at("combat").at("can_kill_shell_blades_underwater").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_shell_blades_underwater\n";}
  if(!q.at("combat").at("can_kill_skulltulas").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_skulltulas";return 3;}
  ++compared;
  if(ComboRando::can_kill_skulltulas(c)!=q.at("combat").at("can_kill_skulltulas").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_skulltulas\n";}
  if(!q.at("combat").at("can_kill_snappers_any").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_snappers_any";return 3;}
  ++compared;
  if(ComboRando::can_kill_snappers_any(c)!=q.at("combat").at("can_kill_snappers_any").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_snappers_any\n";}
  if(!q.at("combat").at("can_kill_snappers_flowers").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_snappers_flowers";return 3;}
  ++compared;
  if(ComboRando::can_kill_snappers_flowers(c)!=q.at("combat").at("can_kill_snappers_flowers").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_snappers_flowers\n";}
  if(!q.at("combat").at("can_kill_stalchildren").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_stalchildren";return 3;}
  ++compared;
  if(ComboRando::can_kill_stalchildren(c)!=q.at("combat").at("can_kill_stalchildren").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_stalchildren\n";}
  if(!q.at("combat").at("can_kill_tektites").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_tektites";return 3;}
  ++compared;
  if(ComboRando::can_kill_tektites(c)!=q.at("combat").at("can_kill_tektites").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_tektites\n";}
  if(!q.at("combat").at("can_kill_twinmold").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_twinmold";return 3;}
  ++compared;
  if(ComboRando::can_kill_twinmold(c)!=q.at("combat").at("can_kill_twinmold").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_twinmold\n";}
  if(!q.at("combat").at("can_kill_wallmasters").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_wallmasters";return 3;}
  ++compared;
  if(ComboRando::can_kill_wallmasters(c)!=q.at("combat").at("can_kill_wallmasters").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_wallmasters\n";}
  if(!q.at("combat").at("can_kill_warts").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_warts";return 3;}
  ++compared;
  if(ComboRando::can_kill_warts(c)!=q.at("combat").at("can_kill_warts").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_warts\n";}
  if(!q.at("combat").at("can_kill_wizzrobe_close").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_wizzrobe_close";return 3;}
  ++compared;
  if(ComboRando::can_kill_wizzrobe_close(c)!=q.at("combat").at("can_kill_wizzrobe_close").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_wizzrobe_close\n";}
  if(!q.at("combat").at("can_kill_wolfos").is_boolean()) {std::cerr<<"Unverified reference predicate can_kill_wolfos";return 3;}
  ++compared;
  if(ComboRando::can_kill_wolfos(c)!=q.at("combat").at("can_kill_wolfos").get<bool>()) {++failures;std::cerr<<q.at("id")<<" can_kill_wolfos\n";}
 }
 // Din's Fire (MM) and trick MM_IGOS_DINS, which the 111 fixture cases leave off: every combination
 // against macros_mm.yml:401 can_kill_igos = has_mirror_shield && (can_use_fire_arrows ||
 // (trick(MM_IGOS_DINS) && can_use_din)) && (can_fight || can_use_deku_bubble || can_use_slingshot), with
 // can_fight = has_weapon || has_mask_zora || has_mask_goron (:212) and no MM slingshot.
 int igosCases=0;
 for(int bits=0;bits<(1<<9);++bits) {
  auto on=[&](int i){return ((bits>>i)&1)!=0;};
  ComboRando::MmCombatCapabilities igos{};
  igos.mirrorShield=on(0);igos.fireArrows=on(1);igos.din=on(2);igos.igosDins=on(3);igos.sword=on(4);igos.zora=on(5);igos.goron=on(6);igos.deku=on(7);igos.magic=on(8);
  const bool expected=igos.mirrorShield&&(igos.fireArrows||(igos.igosDins&&igos.din))&&((igos.sword||igos.zora||igos.goron)||(igos.deku&&igos.magic));
  ++igosCases;
  if(ComboRando::can_kill_igos(igos)!=expected){++failures;std::cerr<<"igos case "<<bits<<"\n";}
 }
 // Din's Fire alone never burns the curtain without the trick.
 {ComboRando::MmCombatCapabilities din{};din.mirrorShield=din.din=din.sword=true;
  if(ComboRando::can_kill_igos(din)){++failures;std::cerr<<"igos without trick\n";}
  din.igosDins=true;if(!ComboRando::can_kill_igos(din)){++failures;std::cerr<<"igos with trick\n";}}
 std::cout<<"Compared "<<compared<<" imported predicates and "<<igosCases<<" Igos Din's Fire cases; differences "<<failures<<std::endl;
 return failures!=0;
}

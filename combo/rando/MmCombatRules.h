// Maintained by hand from the combat macros in data/macros/macros_mm.yml of OoTMM v32.3
// 413573d22d1d53e8a81931a70927f625b73488b9 (the combat macros are unchanged from 41a2b7c0).
/*
MIT License

Copyright (c) 2020-2022 OoTMM Team

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once
namespace ComboRando {
// Native caller retains actor-specific soul gates; unsupported equipment/tricks stay false.
struct MmCombatCapabilities {
    bool sword, deku, goron, zora, deity, giant, magic, arrows, hookshot;
    bool explosives, nuts, stickFighting, fireArrows, iceArrows, lightArrows;
    bool mirrorShield, swim, day, twinmoldBow, twinmoldFireIce;
    bool din = false;      // appended; legacy host fixtures retain false
    bool igosDins = false; // appended: trick(MM_IGOS_DINS)
};
inline bool can_fight_pirates(const MmCombatCapabilities& c) {
    return true && ((c.sword || c.zora || c.goron) || c.arrows || c.stickFighting);
}
inline bool can_kill_aliens(const MmCombatCapabilities& c) {
    return c.arrows;
}
inline bool can_kill_armos(const MmCombatCapabilities& c) {
    return true && ((c.sword || c.zora || c.goron) || c.arrows || c.din || c.explosives);
}
inline bool can_kill_baba_nuts(const MmCombatCapabilities& c) {
    return true && ((c.sword || c.zora || c.goron) || c.deku || c.hookshot || c.explosives || c.arrows);
}
inline bool can_kill_bad_bats_single(const MmCombatCapabilities& c) {
    return true &&
           ((c.sword || c.zora || c.goron) || c.deku || c.arrows || false || c.hookshot || c.explosives || c.din);
}
inline bool can_kill_beamos(const MmCombatCapabilities& c) {
    return true && (c.explosives);
}
inline bool can_kill_big_poe(const MmCombatCapabilities& c) {
    return c.arrows || c.zora || (c.deku && c.magic) || false || false;
}
inline bool can_kill_bio_babas(const MmCombatCapabilities& c) {
    return true && (c.arrows || false || c.hookshot || c.zora || false);
}
inline bool can_kill_boes(const MmCombatCapabilities& c) {
    return true &&
           (c.deku || (c.sword || c.zora || c.goron) || c.arrows || false || c.hookshot || c.nuts || c.explosives);
}
inline bool can_kill_bubbles(const MmCombatCapabilities& c) {
    return true && ((c.sword || c.zora || c.goron) || c.arrows || (c.deku && c.magic) || false || c.explosives ||
                    c.stickFighting);
}
inline bool can_kill_chuchu(const MmCombatCapabilities& c) {
    return true && (c.sword || c.arrows || c.zora || c.deku || c.explosives || c.stickFighting || false);
}
inline bool can_kill_deep_python(const MmCombatCapabilities& c) {
    return true && (c.zora || c.hookshot) && c.swim;
}
inline bool can_kill_dexihands(const MmCombatCapabilities& c) {
    return true && (c.zora || c.explosives || false);
}
inline bool can_kill_dodongos(const MmCombatCapabilities& c) {
    return true && (c.sword || c.explosives || c.goron || c.arrows || c.stickFighting);
}
inline bool can_kill_dragonflies(const MmCombatCapabilities& c) {
    return true && (c.deku || (c.sword || c.zora || c.goron) || c.nuts || c.arrows || false || c.din || c.explosives);
}
inline bool can_kill_eyegore(const MmCombatCapabilities& c) {
    return true && ((c.sword || c.zora || c.goron) || c.arrows || c.hookshot || c.din || c.deku || c.explosives);
}
inline bool can_kill_eyegore_distance(const MmCombatCapabilities& c) {
    return true && c.lightArrows;
}
inline bool can_kill_floormasters(const MmCombatCapabilities& c) {
    return true && (c.arrows || (c.sword || c.zora || c.goron) || c.din || c.explosives || c.stickFighting);
}
inline bool can_kill_freezard_short_range(const MmCombatCapabilities& c) {
    return (c.sword || c.zora || c.goron) || c.lightArrows || (c.fireArrows || c.din) || c.explosives || c.hookshot;
}
inline bool can_kill_garo(const MmCombatCapabilities& c) {
    return true && (c.sword || c.goron || c.arrows || c.explosives);
}
inline bool can_kill_garo_master(const MmCombatCapabilities& c) {
    return true && (c.sword || c.goron || c.arrows || c.explosives);
}
inline bool can_kill_gekko_gbt(const MmCombatCapabilities& c) {
    return true && c.iceArrows;
}
inline bool can_kill_gekko_wft(const MmCombatCapabilities& c) {
    return true && (c.arrows || false) && (c.deku || c.goron || false || c.explosives);
}
inline bool can_kill_goht(const MmCombatCapabilities& c) {
    return true && (c.fireArrows || c.din) && (c.arrows || c.goron || c.deity);
}
inline bool can_kill_gomess(const MmCombatCapabilities& c) {
    return true && c.lightArrows && (c.sword || c.zora || c.goron);
}
inline bool can_kill_guays(const MmCombatCapabilities& c) {
    return true && (c.deku || (c.sword || c.zora || c.goron) || c.arrows || c.hookshot || (c.deku && c.magic) ||
                    false || c.din || c.explosives || c.stickFighting);
}
inline bool can_kill_gyorg(const MmCombatCapabilities& c) {
    return true &&
           ((c.magic && ((c.zora && (c.arrows || false)) || c.deity)) || (false && false && c.hookshot && false));
}
inline bool can_kill_igos(const MmCombatCapabilities& c) {
    return true && c.mirrorShield && (c.fireArrows || (c.igosDins && c.din)) &&
           ((c.sword || c.zora || c.goron) || (c.deku && c.magic) || false);
}
inline bool can_kill_keese_close(const MmCombatCapabilities& c) {
    return true && (c.sword || c.arrows || c.hookshot || (c.deku && c.magic) || false || c.din);
}
inline bool can_kill_keeta(const MmCombatCapabilities& c) {
    return true && c.arrows && (c.sword || c.zora || c.goron);
}
inline bool can_kill_leevers(const MmCombatCapabilities& c) {
    return true && ((c.sword || c.zora || c.goron) || c.arrows || c.hookshot || (c.deku && c.magic) || false || c.din ||
                    c.explosives);
}
inline bool can_kill_like_like(const MmCombatCapabilities& c) {
    return true &&
           ((c.sword || c.zora || c.goron) || (c.deku && c.magic) || false || c.arrows || c.din || c.explosives);
}
inline bool can_kill_lizalfos_dinolfos(const MmCombatCapabilities& c) {
    return true && (c.sword || c.goron || c.arrows);
}
inline bool can_kill_octoroks(const MmCombatCapabilities& c) {
    return true && (c.arrows || c.hookshot || (c.deku && c.magic) || false || c.zora || false);
}
inline bool can_kill_odolwa(const MmCombatCapabilities& c) {
    return true && ((c.deity && c.magic) || ((c.arrows || false) && (c.sword || c.zora || c.goron)));
}
inline bool can_kill_peahats(const MmCombatCapabilities& c) {
    return true && ((c.sword || c.zora || c.goron) || c.arrows || c.stickFighting || c.deku) && c.day;
}
inline bool can_kill_poe_sisters(const MmCombatCapabilities& c) {
    return true && (c.arrows || c.hookshot || (c.deku && c.magic) || false);
}
inline bool can_kill_scrubs_anywhere(const MmCombatCapabilities& c) {
    return true && (c.deku || (c.sword || c.zora || c.goron) || c.arrows || c.hookshot || c.din ||
                    (c.deku && c.magic) || false || c.nuts || c.stickFighting);
}
inline bool can_kill_shell_blades_underwater(const MmCombatCapabilities& c) {
    return true && (c.zora || (false && c.hookshot));
}
inline bool can_kill_skulltulas(const MmCombatCapabilities& c) {
    return true && ((c.sword || c.zora || c.goron) || c.arrows || c.hookshot || c.din || c.explosives ||
                    (c.deku && c.magic) || false || c.stickFighting);
}
inline bool can_kill_snappers_any(const MmCombatCapabilities& c) {
    return true && (c.explosives || c.goron || false);
}
inline bool can_kill_snappers_flowers(const MmCombatCapabilities& c) {
    return true && (c.deku || c.explosives || c.goron || false);
}
inline bool can_kill_stalchildren(const MmCombatCapabilities& c) {
    return true && ((c.sword || c.zora || c.goron) || c.arrows || c.hookshot || (c.deku && c.magic) || false || c.din ||
                    c.explosives || c.stickFighting);
}
inline bool can_kill_tektites(const MmCombatCapabilities& c) {
    return true && (c.arrows || (c.sword || c.zora || c.goron) || c.explosives || c.stickFighting);
}
inline bool can_kill_twinmold(const MmCombatCapabilities& c) {
    return true && ((c.twinmoldBow && c.arrows) || (c.twinmoldFireIce && c.fireArrows && c.iceArrows) ||
                    (c.magic && ((c.giant && c.sword) || c.deity)));
}
inline bool can_kill_wallmasters(const MmCombatCapabilities& c) {
    return true && (c.arrows || (c.sword || c.zora || c.goron) || c.din || c.explosives || c.stickFighting);
}
inline bool can_kill_warts(const MmCombatCapabilities& c) {
    return true && c.sword && (c.zora || c.explosives || c.arrows || c.hookshot || c.din || c.nuts || false);
}
inline bool can_kill_wizzrobe_close(const MmCombatCapabilities& c) {
    return true && ((c.sword || c.zora || c.goron) || c.arrows || c.din || c.stickFighting);
}
inline bool can_kill_wolfos(const MmCombatCapabilities& c) {
    return true && ((c.sword || c.zora || c.goron) || c.arrows || (c.deku && c.magic) || false || c.explosives ||
                    c.stickFighting);
}
} // namespace ComboRando

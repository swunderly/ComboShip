#include "network/ComboAnchorProtocol.h"
#include <cassert>
#include <iostream>

using namespace ComboAnchorProtocol;
int main() {
    Json oot = {{"state", {{"healthCapacity", 48}}}, {"clientId", 3}, {"targetTeamId", "team"}};
    Json mm = {{"state", {{"shipSaveInfo", Json::object()}}}, {"targetTeamId", "team"}};
    auto packet = Combine(oot, mm, 42);
    auto parts = Expand(packet, 42);
    assert(parts.size() == 2 && parts[0]["originGame"] == "oot" && parts[1]["originGame"] == "mm");
    assert(parts[0]["state"] == oot["state"] && parts[1]["state"] == mm["state"]);
    assert(parts[0]["comboSnapshot"] == true && parts[1]["comboSnapshot"] == true);
    assert(!parts[0].contains("queue") && !parts[1].contains("queue"));
    // The actual server's cached reply preserves state, not originGame or clientId.
    Json cached = {{"type", "UPDATE_TEAM_STATE"}, {"state", packet["state"]}};
    assert(Expand(cached, 42).size() == 2);
    assert(Expand(cached, 43).empty());
    cached["state"]["comboProtocol"] = 2;
    assert(Expand(cached, 42).empty());
    cached = {{"type", "UPDATE_TEAM_STATE"}, {"state", oot["state"]}};
    assert(Expand(cached, 42).empty());
    cached = {{"type", "UPDATE_TEAM_STATE"}, {"queue", Json::array()}};
    assert(Expand(cached, 42).empty());
    Json item = {{"type", "GIVE_ITEM"}, {"originGame", "mm"}, {"comboProtocol", 1}, {"comboSeed", 42}};
    cached["queue"] = Json::array({item.dump(), "bad JSON", 17});
    assert(Expand(cached, 42) == std::vector<Json>{item});
    assert(Expand(cached, 43).empty());
    auto wrong = mm;
    wrong["targetTeamId"] = "other";
    bool rejected = false;
    try { Combine(oot, wrong, 42); } catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
    // Live packets retain routing and identity; shared roster messages are not progress.
    assert(Expand(item, 42) == std::vector<Json>{item});
    assert(IsProgress(item) && !IsProgress(Json{{"type", "UPDATE_CLIENT_STATE"}}));
    Json counters = Json::object();
    auto event = item;
    event["comboStream"] = "player-a-session-1";
    event["comboSequence"] = uint64_t{1};
    assert(AcceptEvent(counters, event));
    assert(!AcceptEvent(counters, event));
    // The saved journal and a teammate's snapshot preserve duplicate suppression
    // across process restarts and server queue replay.
    auto restored = Json::parse(counters.dump());
    assert(!AcceptEvent(restored, event));
    Json newer = {{"mm:player-a-session-1", uint64_t{4}}, {"oot:player-b-session-1", uint64_t{2}}};
    MergeCounters(restored, newer);
    event["comboSequence"] = uint64_t{3};
    assert(!AcceptEvent(restored, event));
    event["comboSequence"] = uint64_t{5};
    assert(AcceptEvent(restored, event));
    assert(!AcceptEvent(restored, item));
    Json saved = Json::object();
    CommitGameCounters(saved, restored, 0);
    assert(saved.contains("oot:player-b-session-1") && !saved.contains("mm:player-a-session-1"));
    // An MM reward that was never saved must remain replayable after loading.
    assert(AcceptEvent(saved, event));
    CommitGameCounters(saved, restored, 1);
    assert(!AcceptEvent(saved, event));
    auto cross = event;
    cross["type"] = "COMBO_CROSS_ITEM";
    cross["targetGame"] = 0;
    assert(AcceptEvent(saved, cross)); // origin is MM, reward belongs to OoT
    assert(!AcceptEvent(saved, cross));
    // A threaded save commits the journal captured with its inventory, even if
    // another event has arrived before the disk write runs.
    auto captured = restored;
    event["comboSequence"] = uint64_t{6};
    assert(AcceptEvent(restored, event));
    Json delayedSave = Json::object();
    CommitGameCounters(delayedSave, captured, 1);
    assert(delayedSave["mm:player-a-session-1"] == 5);
    assert(AcceptEvent(delayedSave, event));

    // Seed identity (C5). FNV-1a as the launcher's ComboHash.
    assert(Fnv1a("") == 2166136261u && Fnv1a("a") == 0xe40c292cu);
    // A stand-in layout: the identity reads any layout as data.
    Json layout = {{"settings", {{"erWallmasters", "ownGame"}}},
                   {"overrides", Json::array({Json::array({"wallmaster-source", "destination-a"})})}};
    // A seed the app made, and a headless layout seed, keep their displaySeed; a headless seed without a layout
    // keeps its master seed.
    Json app = {{"masterSeed", 7u}, {"displaySeed", 99u}, {"entranceLayout", layout}};
    assert(SeedIdentity(app) == 99u);
    app.erase("entranceLayout");
    assert(SeedIdentity(app) == 99u);
    assert(SeedIdentity(Json{{"masterSeed", 7u}, {"startingGame", "OOT"}}) == 7u);
    assert(SeedIdentity(Json()) == 0 && SeedIdentity(Json("seed")) == 0);
    // A layout seed from an older headless build has no displaySeed. Corpus rows share a master seed across start
    // contexts, and each layout seed is its own world.
    Json child = {{"masterSeed", 7u},
                  {"startingGame", "OOT"},
                  {"oot", {{"settings", {{"StartingAge", 0}}}, {"placements", {{"KF Chest", "Mirror Shield (OOT)"}}}}},
                  {"mm", {{"settings", Json::object()}, {"placements", {{"Bank", "Mirror Shield (MM)"}}}}},
                  {"foreign", Json::array({{{"checkGame", "oot"}, {"itemName", "Deku Mask"}}})},
                  {"hints", {{"version", 1}}},
                  {"entranceLayout", layout}};
    auto adult = child, mmStart = child, otherRoute = child, otherHints = child, noLayout = child;
    adult["oot"]["settings"]["StartingAge"] = 1;
    mmStart["startingGame"] = "MM";
    otherRoute["entranceLayout"]["overrides"][0][1] = "destination-b";
    otherHints["hints"]["version"] = 2;
    noLayout.erase("entranceLayout");
    auto content = child;
    content.erase("foreign");
    content["oot"].erase("placements");
    content["mm"].erase("placements");
    const uint32_t identity = SeedIdentity(child);
    assert(identity == Fnv1a(content.dump()) && identity != 7u && SeedIdentity(noLayout) == 7u);
    for (const auto* other : {&adult, &mmStart, &otherRoute, &otherHints})
        assert(SeedIdentity(*other) != identity);
    // Both players' copies of one file, and the copy a slot keeps, have one identity. That holds after the
    // Plandomizer's Save on either device too, which writes the placements and foreign[] again with item-name
    // suffixes from that device's own pools (ComboMenu.cpp PlandoSavePlay).
    assert(SeedIdentity(Json::parse(child.dump(2))) == identity);
    auto plando = child;
    plando["oot"]["placements"]["KF Chest"] = "Mirror Shield";
    plando["mm"]["placements"]["Bank"] = "Mirror Shield";
    plando["foreign"][0]["displayName"] = "Deku Mask (MM)";
    assert(plando != child && SeedIdentity(plando) == identity);
    // So the team state of one row is refused by a player of another (the launcher's PumpSync refuses
    // their progress packets by the same comboSeed).
    assert(Expand(Combine(oot, mm, identity), SeedIdentity(adult)).empty());
    assert(Expand(Combine(oot, mm, identity), identity).size() == 2);

    // Both games report that identity in their client state once a save is loaded.
    Json state = {{"isSaveLoaded", true}, {"seed", 5u}};
    StampClientState(state, identity);
    assert(state["seed"] == identity && state["comboProtocol"] == Version);
    Json fileSelect = {{"isSaveLoaded", false}, {"seed", 0u}};
    StampClientState(fileSelect, identity);
    assert(fileSelect["seed"] == 0u && fileSelect["comboProtocol"] == Version);

    // The room window's plain refusal, from the launcher roster's rows.
    Json me = {{"self", true},     {"online", true}, {"teamId", "team"},       {"isSaveLoaded", true},
               {"seed", identity}, {"game", "oot"},  {"clientVersion", "0412"}};
    auto mate = me;
    mate["self"] = false;
    mate["game"] = "mm";
    assert(TeammateWarning(me, mate).empty()); // same seed, the other game
    mate["seed"] = SeedIdentity(adult);
    assert(TeammateWarning(me, mate) == kTeammateOtherSeed); // compared across games too
    // An older build cannot load a layout seed: it reports another seed and another version.
    mate["clientVersion"] = "0411";
    assert(TeammateWarning(me, mate) == std::string(kTeammateOtherSeed) + "\n" + kTeammateOtherVersion);
    mate["seed"] = identity;
    assert(TeammateWarning(me, mate) == kTeammateOtherVersion);
    mate["clientVersion"] = "0412";
    mate["seed"] = SeedIdentity(adult);
    for (const char* key : {"isSaveLoaded", "online"}) {
        auto quiet = mate;
        quiet[key] = false;
        assert(TeammateWarning(me, quiet).empty()); // file select or offline: nothing to compare
    }
    auto atFileSelect = me, otherTeam = mate, ownRow = mate;
    atFileSelect["isSaveLoaded"] = false;
    otherTeam["teamId"] = "rivals";
    ownRow["self"] = true;
    assert(TeammateWarning(atFileSelect, mate).empty() && TeammateWarning(me, otherTeam).empty());
    assert(TeammateWarning(me, ownRow).empty() && TeammateWarning(Json(), mate).empty());
    for (const std::string sentence : {kTeammateOtherSeed, kTeammateOtherVersion})
        assert(sentence.find('!') == std::string::npos && sentence.find("mismatch") == std::string::npos &&
               sentence.find("break") == std::string::npos && sentence.back() == '.');
    std::cout << "PASS: combined state, server cache, queue routing, seed and protocol isolation, seed identity "
                 "of layout seeds, client state seeds and the room window's plain refusal\n";
}

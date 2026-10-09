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
    std::cout << "PASS: combined state, server cache, queue routing, seed and protocol isolation\n";
}

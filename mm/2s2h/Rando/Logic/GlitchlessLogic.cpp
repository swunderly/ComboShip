#include "2s2h/Rando/ActorBehavior/BeanSources.h"
#include "Logic.h"
#include "2s2h/Rando/Types.h"

#include <numeric>
#include <iterator>
#include <spdlog/spdlog.h>

extern "C" {
#include "variables.h"
#include "ShipUtils.h"
uint64_t GetUnixTimestamp();
}

namespace Rando {

namespace Logic {

void ApplyGlitchlessLogicToSaveContext(std::vector<RandoCheckId>& checkPool, std::vector<RandoItemId>& itemPool) {
    uint64_t tick = GetUnixTimestamp();
#ifdef COMBO_BUILD
    ComboRando::MmResourceScope resources;
    Rando::Logic::MmAccessScope access;
#endif

    SaveContext copiedSaveContext;
    memcpy(&copiedSaveContext, &gSaveContext, sizeof(SaveContext));

    std::set<RandoRegionId> regionsInLogic = { RR_MAX };
    std::set<RandoCheckId> checksInLogic;
    std::set<std::pair<RandoEvent, std::function<bool()>>*> eventsInLogic;

    // Initialize time states using shared function
    std::unordered_map<RandoRegionId, RegionTimeState> regionTimeStates = InitializeRegionTimeStates(RR_MAX);

    // Capped ring grants cannot be undone by subtracting the dungeon maximum:
    // a trial may have started with some collected or spent keys. Only fill owns
    // these snapshots; runtime grants never accumulate a global undo history.
    KeyRingTrialSnapshot trialKeys;
    bool trialBeanActive = false;
    u8 trialBeanSlot = ITEM_NONE;
    int trialBeanAmmo = 0;
    const auto grantsBeanLicense = [](RandoCheckId check) {
#ifdef COMBO_BUILD
        const auto& source = StaticData::Checks.at(check);
        return BeanSourceForCheck(check, source.randoCheckType).license;
#else
        return true;
#endif
    };
    bool trialOcarinaActive = false;
    u8 trialOcarinaItem = ITEM_NONE, trialOcarinaBits = 0;
    decltype(gSaveContext.save.saveInfo.equips) trialOcarinaEquips;
    decltype(gSaveContext.save.shipSaveInfo.dpadEquips) trialOcarinaDpad;
    const auto isOcarina = [](RandoItemId item) {
        return item == RI_OCARINA || item == RI_OCARINA_FAIRY || item == RI_PROGRESSIVE_OCARINA;
    };
    RandoCheckId checkWithJunk = RC_UNKNOWN;
    std::set<RandoItemId> nonJunkItemsThatWeHaveTried;
    std::vector<RandoCheckId> checksWithJunk;
    std::vector<int> checksWithJunkWeights;
    int weight = 1;

    // Inital shuffle of the item pool (Following shuffles done at the end of the loop)
    if (itemPool.size() > 1) {
        for (size_t i = 0; i < itemPool.size(); i++) {
            size_t j = Ship_Random(0, itemPool.size());
            std::swap(itemPool[i], itemPool[j]);
        }
    }

    auto handleError = [&](std::string message) {
        SPDLOG_ERROR("Items/Checks: {}/{}", itemPool.size(), checkPool.size());

        // Log out the checks that are still in the pool
        for (auto& randoCheckId : checkPool) {
            SPDLOG_ERROR("Check still in pool: {}", Rando::StaticData::Checks[randoCheckId].name);
        }
        // Log out the items that are still in the pool
        for (RandoItemId randoItemId : itemPool) {
            SPDLOG_ERROR("Item still in pool: {}", Rando::StaticData::Items[randoItemId].spoilerName);
        }

        memcpy(&gSaveContext, &copiedSaveContext, sizeof(SaveContext));
        throw std::runtime_error(message);
    };

    while (true) {
        // Break if we've been running for too long
        if (GetUnixTimestamp() - tick > 10000) {
            handleError("Generation took too long, aborting");
        }

        bool regionsInLogicChanged = false;
        bool eventsInLogicChanged = false;
        bool checksInLogicChanged = false;

        // Crawl through all reachable regions and add any new reachable regions
        auto prevRegionsInLogicSize = regionsInLogic.size();
        for (RandoRegionId regionId : regionsInLogic) {
            FindReachableRegions(regionId, regionsInLogic, regionTimeStates);
        }
        if (regionsInLogic.size() != prevRegionsInLogicSize) {
            regionsInLogicChanged = true;
        }

#ifdef COMBO_BUILD
        eventsInLogicChanged |= UpdateMmAccess();
        eventsInLogicChanged |= UpdateResourceSources(regionsInLogic, regionTimeStates);
#endif
        for (RandoRegionId regionId : regionsInLogic) {
            auto& randoRegion = Regions[regionId];

            // Set current region time for check evaluation
            SetCurrentRegionTime(regionTimeStates, regionId);

            // Apply any new events
            for (auto& randoEvent : randoRegion.events) {
                if (!eventsInLogic.contains(&randoEvent) &&
                    TimeLogic::EvaluateConditionTimes(randoEvent.second, gCurrentRegionTime)) {
                    RANDO_EVENTS[randoEvent.first]++;
                    eventsInLogic.insert(&randoEvent);
                    eventsInLogicChanged = true;
                }
            }

            // Apply any new checks
            for (auto& [randoCheckId, checkLogic] : randoRegion.checks) {
                if (!checksInLogic.contains(randoCheckId) &&
                    TimeLogic::EvaluateConditionTimes(checkLogic.first, gCurrentRegionTime)) {
                    // VALIDATION: Verify check is reachable with owned time
                    TimeLogic::ValidateRegionTimeOwnership(regionId, randoCheckId,
                                                           regionTimeStates[regionId].timeSlices, "Glitchless");

                    checksInLogic.insert(randoCheckId);

                    RandoItemId randoItemId = RANDO_SAVE_CHECKS[randoCheckId].randoItemId;

                    auto it = std::find(checkPool.begin(), checkPool.end(), randoCheckId);
                    bool inPool = it != checkPool.end();
                    if (inPool) {
                        checkPool.erase(it);

                        size_t pickIndex = SelectItemForCheck(itemPool, checkPool, randoCheckId);
                        if (pickIndex == itemPool.size()) {
                            handleError("No allowed item remains for reachable check: " +
                                        std::string(Rando::StaticData::Checks[randoCheckId].name));
                        }

                        randoItemId = RANDO_SAVE_CHECKS[randoCheckId].randoItemId = itemPool[pickIndex];
                        RANDO_SAVE_CHECKS[randoCheckId].shuffled = true;

                        itemPool.erase(itemPool.begin() + pickIndex);

                        if (Rando::StaticData::Items[randoItemId].randoItemType == RITYPE_JUNK ||
                            Rando::StaticData::Items[randoItemId].randoItemType == RITYPE_HEALTH) {
                            checksWithJunk.push_back(randoCheckId);
                            checksWithJunkWeights.push_back(weight);
                            SPDLOG_TRACE("Item Placed: {}:{}", Rando::StaticData::Checks[randoCheckId].name,
                                         Rando::StaticData::Items[randoItemId].spoilerName);
                        } else {
                            SPDLOG_DEBUG("Item Placed: {}:{}", Rando::StaticData::Checks[randoCheckId].name,
                                         Rando::StaticData::Items[randoItemId].spoilerName);
                        }
                    }

                    if (randoItemId != RI_MAGIC_BEAN || grantsBeanLicense(randoCheckId))
                        GiveItem(ConvertItem(randoItemId));

                    // Update time states for all regions when time items are obtained
                    if (randoItemId >= RI_TIME_DAY_1 && randoItemId <= RI_TIME_PROGRESSIVE) {
                        uint64_t newTimeSlices = TimeLogic::GetOwnedTimeSlices();
                        // Update RR_MAX time state first - this is the source for new region discoveries
                        if (regionTimeStates.find(RR_MAX) != regionTimeStates.end()) {
                            regionTimeStates[RR_MAX].timeSlices = newTimeSlices;
                        }
                        // New clocks expand root access. Recompute room times through their
                        // entrances; owning a clock must not bypass a locked or closed door.
                        for (auto& [regionId, timeState] : regionTimeStates) {
                            timeState.timeSlices = regionId == RR_MAX ? newTimeSlices : 0;
                        }
                        SetCurrentRegionTime(regionTimeStates, regionId);
                    }

                    checksInLogicChanged = true;
                }
            }
        }

        if (itemPool.empty()) {
            // Done!
            break;
        }

        // Choose a random check with junk, and attempt to place progressive items until we unlock something
        if (!regionsInLogicChanged && !checksInLogicChanged && !eventsInLogicChanged) {
            if (checkWithJunk == RC_UNKNOWN) {
                if (checksWithJunk.empty()) {
                    handleError("Out of checks to replace, cannot place remaining items");
                }

                if (checksWithJunk.size() == 1) {
                    checkWithJunk = checksWithJunk[0];
                    checksWithJunk.pop_back();
                    checksWithJunkWeights.pop_back();
                } else {
                    std::vector<double> cumulativeWeights(checksWithJunkWeights.size());
                    std::partial_sum(checksWithJunkWeights.begin(), checksWithJunkWeights.end(),
                                     cumulativeWeights.begin());
                    double random = Ship_Random(0, cumulativeWeights.back());
                    auto it = std::upper_bound(cumulativeWeights.begin(), cumulativeWeights.end(), random);
                    size_t index = std::distance(cumulativeWeights.begin(), it);

                    checkWithJunk = checksWithJunk[index];

                    // Remove the check from the list of checks with junk
                    checksWithJunk.erase(checksWithJunk.begin() + index);
                    checksWithJunkWeights.erase(checksWithJunkWeights.begin() + index);
                }

                SPDLOG_DEBUG("Nothing changed, attempting to replace check: {}",
                             Rando::StaticData::Checks[checkWithJunk].name);
            }

            std::vector<std::pair<RandoItemId, int>> nonJunkItemsThatWeHaveNotTried;
            bool anyNonJunkItemsLeft = false;
            for (size_t i = 0; i < itemPool.size(); i++) {
                if (Rando::StaticData::Items[itemPool[i]].randoItemType != RITYPE_JUNK &&
                    Rando::StaticData::Items[itemPool[i]].randoItemType != RITYPE_HEALTH) {
                    anyNonJunkItemsLeft = true;
                    if (nonJunkItemsThatWeHaveTried.find(itemPool[i]) == nonJunkItemsThatWeHaveTried.end() &&
                        IsItemAllowedAtCheck(itemPool[i], checkWithJunk)) {
                        nonJunkItemsThatWeHaveNotTried.push_back({ itemPool[i], i });
                    }
                }
            }

            if (!anyNonJunkItemsLeft) {
                handleError("No non-junk items left to place, progression is impossible");
            }

            if (nonJunkItemsThatWeHaveNotTried.empty()) {
                SPDLOG_TRACE("Already tried all non-junk items, leaving the last non-junk item in place: {}: {}",
                             Rando::StaticData::Checks[checkWithJunk].name,
                             Rando::StaticData::Items[RANDO_SAVE_CHECKS[checkWithJunk].randoItemId].spoilerName);
                checkWithJunk = RC_UNKNOWN;
                trialKeys.active = false;
                trialBeanActive = false;
                trialOcarinaActive = false;
                nonJunkItemsThatWeHaveTried.clear();
                continue;
            }

            // Remove item and place it back in the pool
            RandoItemId oldRandoItemId = RANDO_SAVE_CHECKS[checkWithJunk].randoItemId;
            auto& [newRandoItemId, indexInPool] = nonJunkItemsThatWeHaveNotTried[0];

            RANDO_SAVE_CHECKS[checkWithJunk].randoItemId = newRandoItemId;

            SPDLOG_DEBUG("Item Replaced Junk: {}:{}", Rando::StaticData::Checks[checkWithJunk].name,
                         Rando::StaticData::Items[newRandoItemId].spoilerName);

#ifdef COMBO_BUILD
            // A failed placement must not retain a source requiring the removed item.
            ComboRando::mmResourceAccess.sources = 0;
#endif
            if (oldRandoItemId == RI_MAGIC_BEAN && trialBeanActive) {
                INV_CONTENT(ITEM_MAGIC_BEANS) = trialBeanSlot;
                AMMO(ITEM_MAGIC_BEANS) = trialBeanAmmo;
            } else if (isOcarina(oldRandoItemId) && trialOcarinaActive) {
                INV_CONTENT(ITEM_OCARINA_OF_TIME) = trialOcarinaItem;
                gSaveContext.save.shipSaveInfo.rando.ocarinaOwnedBits = trialOcarinaBits;
                gSaveContext.save.saveInfo.equips = trialOcarinaEquips;
                gSaveContext.save.shipSaveInfo.dpadEquips = trialOcarinaDpad;
            } else if (FindKeyRing(oldRandoItemId) && trialKeys.active) {
                trialKeys.Restore(gSaveContext.save.saveInfo.inventory.dungeonKeys,
                                  gSaveContext.save.shipSaveInfo.rando.foundDungeonKeys);
            } else {
                RemoveItem(oldRandoItemId);
            }
            trialBeanActive = newRandoItemId == RI_MAGIC_BEAN;
            if (trialBeanActive) {
                trialBeanSlot = INV_CONTENT(ITEM_MAGIC_BEANS);
                trialBeanAmmo = AMMO(ITEM_MAGIC_BEANS);
            }
            trialKeys.active = FindKeyRing(newRandoItemId) != nullptr;
            if (trialKeys.active) {
                trialKeys.Capture(gSaveContext.save.saveInfo.inventory.dungeonKeys,
                                  gSaveContext.save.shipSaveInfo.rando.foundDungeonKeys);
            }
            trialOcarinaActive = isOcarina(newRandoItemId);
            if (trialOcarinaActive) {
                trialOcarinaItem = INV_CONTENT(ITEM_OCARINA_OF_TIME);
                trialOcarinaBits = gSaveContext.save.shipSaveInfo.rando.ocarinaOwnedBits;
                trialOcarinaEquips = gSaveContext.save.saveInfo.equips;
                trialOcarinaDpad = gSaveContext.save.shipSaveInfo.dpadEquips;
            }
            if (newRandoItemId != RI_MAGIC_BEAN || grantsBeanLicense(checkWithJunk))
                GiveItem(ConvertItem(newRandoItemId));

            itemPool.erase(itemPool.begin() + indexInPool);
            itemPool.push_back(oldRandoItemId);

            nonJunkItemsThatWeHaveTried.insert(newRandoItemId);
            SPDLOG_TRACE("Attempting to replaced junk item: {}:{}", Rando::StaticData::Checks[checkWithJunk].name,
                         Rando::StaticData::Items[newRandoItemId].spoilerName);
        } else {
            weight++;
            if (checkWithJunk != RC_UNKNOWN) {
                SPDLOG_DEBUG("Successfully Replaced junk item with: {}:{}",
                             Rando::StaticData::Checks[checkWithJunk].name,
                             Rando::StaticData::Items[RANDO_SAVE_CHECKS[checkWithJunk].randoItemId].spoilerName);
            }
            checkWithJunk = RC_UNKNOWN;
            trialKeys.active = false;
            trialOcarinaActive = false;
            nonJunkItemsThatWeHaveTried.clear();

            // Shuffle the item pool
            if (itemPool.size() > 1) {
                for (size_t i = 0; i < itemPool.size(); i++) {
                    size_t j = Ship_Random(0, itemPool.size());
                    std::swap(itemPool[i], itemPool[j]);
                }
            }
        }
    }

    for (auto& randoCheckId : checksInLogic) {
        copiedSaveContext.save.shipSaveInfo.rando.randoSaveChecks[randoCheckId].randoItemId =
            RANDO_SAVE_CHECKS[randoCheckId].randoItemId;
        copiedSaveContext.save.shipSaveInfo.rando.randoSaveChecks[randoCheckId].shuffled =
            RANDO_SAVE_CHECKS[randoCheckId].shuffled;
    }

    memcpy(&gSaveContext, &copiedSaveContext, sizeof(SaveContext));

    SPDLOG_INFO("Successfully placed all items with Glitchless logic");
}

} // namespace Logic

} // namespace Rando

#include "rando/KokiriSeedRevision.h"
#include "ComboRandomizerSavedSeed.h"
#include <cassert>
#include <iostream>

using nlohmann::json;
using namespace ComboRando;
static int rupees = 0, grass = 0;
static int GetRupees() { return rupees; }
static int GetGrass() { return grass; }
static int SetRupees(int value) { if (value < 0 || value > 1) return 0; rupees = value; return 1; }
static int SetGrass(int value) { if (value < 0 || value > 1) return 0; grass = value; return 1; }

int main() {
    const auto caps = Entrances::ActiveCapabilities();
    auto legacy = ReadKokiriCatalogRevisions(json::object());
    assert(legacy.invisibleRupees == 0 && legacy.grass == 0);
    assert(ReadKokiriSaveRevision(json::object()) == 0);
    for (int r = 0; r <= 1; ++r) for (int g = 0; g <= 1; ++g) {
        json fields = {{kKokiriSeedRevision, r}, {kKokiriGrassCatalogRevision, g}};
        auto values = ReadKokiriCatalogRevisions(fields);
        assert(values.invisibleRupees == r && values.grass == g);
        json save = {{"sections", {{"randomizer", {{"data", fields}}}}}};
        assert(ReadKokiriSaveRevision(save) == r);
        json seed = {{"fileType", "ComboShipRandomizer"}, {"oot", fields}};
        json container = {{"oot", save}, {"combo", {{"rando", seed}}}};
        ComboRando::Profiles::ValidateSavedCatalogs(container, caps);
        auto rules = ComboRando::Profiles::NativeSeedRulesFromContainer(container, caps);
        assert(rules["oot"][kKokiriSeedRevision] == r);
        assert(rules["oot"][kKokiriGrassCatalogRevision] == g);
        for (const char* marker : {kKokiriSeedRevision, kKokiriGrassCatalogRevision}) {
            auto mismatched = container;
            mismatched["combo"]["rando"]["oot"][marker] = 1 - fields[marker].get<int>();
            bool rejected = false;
            try { ComboRando::Profiles::ValidateSavedCatalogs(mismatched, caps); }
            catch (const UnsupportedKokiriSeedRevision&) { rejected = true; }
            assert(rejected);
        }
        seed["comboProfile"] = {{"version", 1}, {"settings", {{"oot", {
            {kKokiriSeedRevision, 1-r}, {kKokiriGrassCatalogRevision, 1-g}}}}}};
        auto setup = ComboRando::Profiles::SetupForSeed(seed, caps);
        assert(setup["oot"][kKokiriSeedRevision] == r);
        assert(setup["oot"][kKokiriGrassCatalogRevision] == g);
        rupees = grass = 0;
        try {
            KokiriSeedRevisionScope a(GetRupees, SetRupees, r);
            KokiriGrassCatalogRevisionScope b(GetGrass, SetGrass, g);
            assert(rupees == r && grass == g);
            throw std::runtime_error("failed import");
        } catch (const std::runtime_error&) {}
        assert(rupees == 0 && grass == 0);
        {
            KokiriSeedRevisionScope a(GetRupees, SetRupees, r);
            KokiriGrassCatalogRevisionScope b(GetGrass, SetGrass, g);
            a.Keep(); b.Keep();
        }
        assert(rupees == r && grass == g);
    }
    for (const char* field : {kKokiriSeedRevision, kKokiriGrassCatalogRevision}) {
        for (const json& invalid : {json(-1), json(2), json(true), json(1.0), json("1"),
                                   json(nullptr), json::array(), json::object()}) {
            json fields = {{kKokiriSeedRevision, 1}, {kKokiriGrassCatalogRevision, 1}};
            fields[field] = invalid;
            bool rejected = false;
            try { (void)ReadKokiriCatalogRevisions(fields); }
            catch (const UnsupportedKokiriSeedRevision&) { rejected = true; }
            assert(rejected);
            json save = {{"sections", {{"randomizer", {{"data", fields}}}}}};
            rejected = false;
            try { (void)ReadKokiriSaveRevision(save); }
            catch (const UnsupportedKokiriSeedRevision&) { rejected = true; }
            assert(rejected);
        }
    }
    // Loading another file must not inherit the modern catalog from a previous file.
    rupees = grass = 1;
    auto oldFile = ReadKokiriCatalogRevisions(json::object());
    ApplyKokiriSeedRevision(SetRupees, oldFile.invisibleRupees);
    ApplyKokiriSeedRevision(SetGrass, oldFile.grass);
    assert(rupees == 0 && grass == 0);
    ComboRando::Profiles::ValidateSavedCatalogs(json::object(), caps);
    std::cout << "PASS independent per-seed catalogs, legacy defaults, rejected markers and failed-import rollback\n";
}

// Included after the actual native reload implementation. This adapter owns no
// gameplay save writes; save creation calls the checked preparation callback.
static nlohmann::json ProfileNativeSnapshot() {
    auto read=[](FnDumpData fn) { if(!fn) throw std::runtime_error("Profile settings export unavailable");
        const char* raw=fn(); if(!raw) throw std::runtime_error("Profile settings snapshot unavailable");
        return nlohmann::json::parse(raw); };
    int required=0,total=0;
    ComboRando::CwGoal goal;
    if(SOH_ReadComboGoalTypeCVars)goal.type=static_cast<ComboRando::CwGoal::Type>(SOH_ReadComboGoalTypeCVars(&required,&total));
    goal.hunt=goal.type==ComboRando::CwGoal::HUNT;goal.required=required;goal.total=total;
    const int world=SOH_ReadComboStartingGameCVar?SOH_ReadComboStartingGameCVar():0;
    return {{"fileType","ComboShipRandomizer"},{"version",1},{"seed",sProfileLegacySeed?sProfileLegacySeed():""},
        {"oot",{{"settings",read(SOH_DumpRandoSettings)}}},
        {"mm",{{"settings",read(MM_DumpRandoSettings)}}},
        {"setupCVars",read(sProfileDumpCombo)}, {"gameplayRules",SnapshotGameplayRules()},
        {"songTravel",sSongTravel.json()},{"goal",ComboRando::CwGoalJson(goal)},
        {"startingGame",world==2?"Random":world==1?"MM":"OOT"},
        {"sharedItems",ComboRando::SharedKeysFromMask(SOH_ReadComboSharedCVars?SOH_ReadComboSharedCVars():0)}};
}
static nlohmann::json ProfileEditableSetup(nlohmann::json setup){
    if(!setup.contains("setupCVars")){
        auto combo=nlohmann::json::object();const auto goal=ComboRando::CwGoalFromJson(setup.value("goal",nlohmann::json::object()));
        combo["gCombo.Rando.Goal"]=ComboRando::CwGoalType(goal);combo["gCombo.Rando.TriforceHunt"]=goal.hunt?1:0;
        combo["gCombo.Rando.TriforceRequired"]=goal.hunt?goal.required:15;combo["gCombo.Rando.TriforceTotal"]=goal.total>0?goal.total:15;
        const auto world=setup.value("startingGame",std::string("OOT"));combo["gCombo.Rando.StartingGame"]=world=="Random"?2:world=="MM"?1:0;
        const auto mask=ComboRando::SharedMaskFromKeys(setup.value("sharedItems",nlohmann::json::array()));
        for(int i=0;i<ComboRando::SF_COUNT;++i)combo[ComboRando::SharedFamilyByIndex(i).cvar]=(mask&(1u<<i))?1:0;
        const auto oot=setup.value("oot",nlohmann::json::object()).value("settings",nlohmann::json::object());
        const auto access=setup.value("accessibility",nlohmann::json::object()).value("mode",std::string());
        combo[ComboRando::kCombinedGuarantee]=access=="completion"||(access.empty()&&oot.value(ComboRando::kNativeAllLocations,1)==0)?1:0;
        setup["setupCVars"]=std::move(combo);
    }
    auto& oot=setup["oot"]["settings"];
    if(!oot.contains("gRandoSettings.EnabledTricks")&&setup["oot"].contains("enabledTricks")&&setup["oot"]["enabledTricks"].is_array()){
        std::string csv;for(const auto& tag:setup["oot"]["enabledTricks"]){if(!tag.is_string())throw std::invalid_argument("Malformed saved trick tag");if(!csv.empty())csv+=',';csv+=tag.get<std::string>();}
        oot["gRandoSettings.EnabledTricks"]=csv;
    }
    if(setup.contains("gameplayRules")&&setup["gameplayRules"].contains("mmTricks")&&setup["gameplayRules"]["mmTricks"].is_object())
        for(const auto& trick:setup["gameplayRules"]["mmTricks"].items())if(!setup["mm"]["settings"].contains(trick.key()))setup["mm"]["settings"][trick.key()]=trick.value();
    if(!setup["mm"]["settings"].contains("gRando.SariaPriorityItems")&&setup["mm"].contains("sariaPriorityItems"))setup["mm"]["settings"]["gRando.SariaPriorityItems"]=setup["mm"]["sariaPriorityItems"];
    for(const char* game:{"oot","mm"})if(sProfileCatalog.contains(game)){
        auto& settings=setup[game]["settings"];for(const auto& field:sProfileCatalog[game]["defaults"].items())if(!settings.contains(field.key()))settings[field.key()]=field.value();
    }
    return setup;
}
static void ProfileValidateNative(const nlohmann::json& requested) {
    const auto setup=ProfileEditableSetup(requested);
    if(!sProfileValidateOot||!sProfileValidateMm||
       !sProfileValidateOot(setup.at("oot").at("settings").dump().c_str())||
       !sProfileValidateMm(setup.at("mm").at("settings").dump().c_str()))
        throw std::invalid_argument("Invalid native settings profile");
}
static void ProfileApplyNative(const nlohmann::json& requested) {
    ProfileValidateNative(requested);
    const auto setup=ProfileEditableSetup(requested);
    // ProfileEditableSetup derives setupCVars for legacy seeds from their own goal/start/shared choices.
    const auto& combo=setup.at("setupCVars");
    if(!sProfileApplyCombo(combo.dump().c_str()))throw std::invalid_argument("Invalid shared settings profile");
    RestoreGameplayRules(setup.value("gameplayRules",nlohmann::json::object()),sGameplayRulesSlot);
    InstallSongTravel(setup.value("songTravel",nlohmann::json()));
    // Native trick fields are authoritative after gameplayRules' legacy CSV.
    if(!sProfileApplyOot(setup.value("oot",nlohmann::json::object()).value("settings",nlohmann::json::object()).dump().c_str())||
       !sProfileApplyMm(setup.value("mm",nlohmann::json::object()).value("settings",nlohmann::json::object()).dump().c_str()))
        throw std::invalid_argument("Invalid native settings profile");
}
static void ProfileRestoreGenerationSetup() {
    if(!sProfileOriginalSetup.is_null()) {
        ProfileApplyNative(sProfileOriginalSetup);
        sGameplayRulesSlot=sProfileOriginalRuleSlot;
        sProfileOriginalSetup=nullptr;
    }
}
template<class F> static const char* ProfileResponse(F action) {
    static std::string result;
    try {result=action().dump();}catch(const std::exception& e){result=nlohmann::json{{"error",e.what()}}.dump();}
    catch(...){result="{\"error\":\"Settings operation failed\"}";}
    return result.c_str();
}
static uint64_t ProfileRevision() {
    return g_profileSavedSeedRevision.load() + (sProfiles ? sProfiles->revision() : 0);
}
static const char* ProfileInspect(int slot) {return ProfileResponse([&] {
    if(!sProfiles)throw std::runtime_error("Settings profiles unavailable");
    auto value=sProfiles->view().inspect(slot,Combo_ReadSavedSeedReadonly(slot));value["catalog"]=sProfileCatalog;
    value["draft"]=ProfileEditableSetup(value["draft"]);
    for(const char* key:{"savedSettings","generatedSettings"})if(value.contains(key)&&value[key].is_object())value[key]=ProfileEditableSetup(value[key]);
    value["nativeSavedRules"]=Combo_ReadNativeSeedRulesReadonly(slot);
    value["generation"]={{"running",g_ComboProgress.running.load()},{"phase",ComboRando::ComboGenProgress::PhaseLabel(g_ComboProgress.phase.load())},
        {"placed",g_ComboProgress.placed.load()},{"total",g_ComboProgress.total.load()},{"canCancel",g_ComboProgress.CanCancel()}};
    if(sProfileAttempt)value["generation"]["file"]=sProfileAttempt->slot;
    return value;
});}
static const char* ProfileSetDraft(int slot,const char* input){return ProfileResponse([&]{
    if(!sProfiles)throw std::runtime_error("Settings profiles unavailable");
    auto setup=nlohmann::json::parse(input?input:"null");
    if(setup.contains("setupCVars")&&setup["setupCVars"].is_object()){
        auto& combo=setup["setupCVars"];
        auto goal=ComboRando::CwGoalFromJson(setup.value("goal",nlohmann::json::object()));
        const int type=combo.value("gCombo.Rando.Goal",ComboRando::CwGoalType(goal));
        if(type<0||type>ComboRando::CwGoal::QUEST)throw std::invalid_argument("Unknown goal setting");
        goal.type=static_cast<ComboRando::CwGoal::Type>(type);goal.hunt=goal.type==ComboRando::CwGoal::HUNT;
        const int required=std::clamp(combo.value("gCombo.Rando.TriforceRequired",15),1,ComboRando::kMaxComboTriforcePieces);
        const int total=std::clamp(combo.value("gCombo.Rando.TriforceTotal",15),required,ComboRando::kMaxComboTriforcePieces);
        combo["gCombo.Rando.TriforceHunt"]=goal.hunt?1:0;
        if(combo.contains("gCombo.Rando.TriforceRequired"))combo["gCombo.Rando.TriforceRequired"]=required;
        if(combo.contains("gCombo.Rando.TriforceTotal"))combo["gCombo.Rando.TriforceTotal"]=total;
        goal.required=goal.hunt?required:0;goal.total=total;setup["goal"]=ComboRando::CwGoalJson(goal);
        if(combo.contains("gCombo.Rando.StartingGame")){
            const int world=combo["gCombo.Rando.StartingGame"].get<int>();if(world<0||world>2)throw std::invalid_argument("Unknown starting world");
            setup["startingGame"]=world==2?"Random":world==1?"MM":"OOT";
        }
        uint32_t mask=0;for(int i=0;i<ComboRando::SF_COUNT;++i){const auto& family=ComboRando::SharedFamilyByIndex(i);
            if(combo.value(family.cvar,0))mask|=1u<<i;}
        auto shared=nlohmann::json(ComboRando::SharedKeysFromMask(mask));
        for(const auto& key:setup.value("sharedItems",nlohmann::json::array()))
            if(ComboRando::SharedMaskFromKeys(nlohmann::json::array({key}))==0)shared.push_back(key);
        setup["sharedItems"]=std::move(shared);
    }
    const auto previous=sProfiles->view().draft(slot,Combo_ReadSavedSeedReadonly(slot));
    sProfiles->setDraft(slot,setup);
    // Drafts for another file must not change the run currently being played.
    if(!g_ComboProgress.running.load() && (sGameplayRulesSlot<0 || sGameplayRulesSlot==slot)) {
        const auto before=ComboRando::NormalizeGameplayRuleSnapshot(previous.value("gameplayRules",nlohmann::json::object())).at("values");
        const auto rules=ComboRando::NormalizeGameplayRuleSnapshot(setup.value("gameplayRules",nlohmann::json::object()));
        const auto values=rules.value("values",nlohmann::json::object());
        const auto logic=rules.value("logicValues",nlohmann::json::object());
        bool changed=false;
        for(const auto& rule:ComboRando::kGameplayRules) {
            if(rule.runtimeOnly || rule.trickGoverned)continue;
            // The loaded seed's own logic keeps every ability it assumes at its value (e.g. its frog
            // count); a draft for the next seed must never take one away from the run being played.
            const int running=sGameplayRulesSlot==slot&&sReadGameplayRule?sReadGameplayRule(ComboRando::LogicRuleKey(rule.name).c_str(),0):0;
            const int required=running>0 ? running : logic.value(rule.name,0);
            const int runtime=required>0 ? required : values.value(rule.name,rule.fallback);
            if(runtime==before.value(rule.name,rule.fallback) &&
               (!required || !sReadGameplayRule || sReadGameplayRule(rule.name,rule.fallback)==runtime))continue;
            if(sWriteGameplayRule)sWriteGameplayRule(rule.name,runtime);
            if(sApplyOotRule)sApplyOotRule(rule.name);
            if(sApplyMmRule)sApplyMmRule(rule.name);
            changed=true;
        }
        if(changed)RefreshGameplayRuntime();
    }
    return nlohmann::json{{"success",true}};
});}
static const char* ProfileCopySaved(int source,int destination){return ProfileResponse([&]{
    if(!sProfiles)throw std::runtime_error("Settings profiles unavailable");
    sProfiles->copySaved(source,destination,Combo_ReadSavedSeedReadonly(source));return nlohmann::json{{"success",true}};
});}
static const char* ProfileCopyDraft(int source,int destination){return ProfileResponse([&]{
    if(!sProfiles)throw std::runtime_error("Settings profiles unavailable");
    sProfiles->copyDraft(source,destination,Combo_ReadSavedSeedReadonly(source));return nlohmann::json{{"success",true}};
});}
// Repository finish persists before clearing its active token. Retain both the
// token and failed outcome until that persistence succeeds; never reinterpret
// a rejected preparation using an unrelated native generation result.
static bool sProfileFailurePending=false;
static std::string sProfileFailureError;
static void ProfileFinishFailedAttempt(const std::string& error) {
    sProfileFailurePending=true;sProfileFailureError=error;
    if(!sProfileAttempt)throw std::runtime_error("Missing failed generation attempt");
    sProfiles->finish(sProfileAttempt->token,false,nlohmann::json(),sProfileFailureError);
    sProfileAttempt.reset();sProfileFailurePending=false;sProfileFailureError.clear();
}
static void ProfileBeginGeneration(int slot,const char* seed) {
    if(!sProfiles||!sProfileCanGenerate||!sProfileCanGenerate())throw std::runtime_error("Generate from the empty-file randomizer screen");
    if(g_GenerateBusy.load()||g_ComboPendingFinalize.load())throw std::runtime_error("Generation already running");
    if(sProfileFailurePending)ProfileFinishFailedAttempt(sProfileFailureError);
    ProfileRestoreGenerationSetup(); // Retry any prior restoration failure before taking a new snapshot.
    auto previous=ProfileNativeSnapshot(); // A failed snapshot cannot strand a running attempt.
    sProfileAttempt=sProfiles->begin(slot,Combo_ReadSavedSeedReadonly(slot),seed?seed:"");
    sProfileOriginalSetup=std::move(previous);sProfileOriginalRuleSlot=sGameplayRulesSlot;
    bool applyStarted=false;
    try{
        ProfileValidateNative(sProfileOriginalSetup);
        ProfileValidateNative(sProfileAttempt->setup);
        applyStarted=true;ProfileApplyNative(sProfileAttempt->setup);
    }catch(...){
        const auto failure=std::current_exception();std::string restoreError;
        if(applyStarted)try{ProfileRestoreGenerationSetup();}
            catch(const std::exception& e){restoreError=std::string("; setup restore failed: ")+e.what();}
            catch(...){restoreError="; setup restore failed";}
        else sProfileOriginalSetup=nullptr;
        ProfileFinishFailedAttempt("Invalid settings profile"+restoreError);
        if(!restoreError.empty())throw std::runtime_error("Invalid settings profile"+restoreError);
        std::rethrow_exception(failure);
    }
}
static const char* ProfileGenerate(int slot,const char* seed){return ProfileResponse([&]{
    ProfileBeginGeneration(slot,seed);
    if(!sProfileTrigger(seed?seed:"")) {
        std::string restoreError;
        try{ProfileRestoreGenerationSetup();}
        catch(const std::exception& e){restoreError=std::string("; setup restore failed: ")+e.what();}
        catch(...){restoreError="; setup restore failed";}
        ProfileFinishFailedAttempt("Generation request rejected"+restoreError);
        throw std::runtime_error("Generation request rejected"+restoreError);
    }
    return nlohmann::json{{"success",true},{"file",slot}};
});}
static const char* ProfileCancel(){return ProfileResponse([]{
    if(!sProfileAttempt||!g_ComboProgress.running.load()||!g_ComboProgress.RequestCancel())throw std::runtime_error("Generation cannot be cancelled now");
    return nlohmann::json{{"success",true}};
});}
static void ProfileCompleteGeneration() {
    if(!sProfileAttempt)return;
    if(sProfileFailurePending){ProfileFinishFailedAttempt(sProfileFailureError);return;}
    ProfileRestoreGenerationSetup();
    const bool success=g_ComboProgress.success.load();
    // Persist before reporting success. A failed atomic replacement keeps the
    // immutable attempt available for retry; it never replaces the older seed.
    sProfiles->finish(sProfileAttempt->token,success,success?nlohmann::json::parse(g_ConsolidatedJson):nlohmann::json(),g_ComboProgress.error);
    sProfileAttempt.reset();
}
static nlohmann::json ProfileComboCatalog(){
    auto presets=nlohmann::json::array();
    for(const auto& preset:ComboRando::kCommunityPresets){auto writes=nlohmann::json::object();
        for(const auto& option:ComboRando::CommunityPresetOptions(preset.id))writes[option.key]=option.value;
        presets.push_back({{"id",int(preset.id)},{"label",preset.name},{"summary",preset.summary},{"help",preset.help},{"writes",writes}});
    }
    return {{"presets",presets}}; // The menu lays out its own combo options; only presets are read.
}
static int ProfilePrepareSave(int slot) {
    try {
        if(!sProfiles)return 1; // Callback not installed before initialization.
        ComboRando::Profiles::RequireSlot(slot);
        if(g_GenerateBusy.load()||g_ComboPendingFinalize.load()||sProfileAttempt)return 0;
        const auto seed=sProfiles->view().pendingSeed(slot);
        if(!ComboRando::Profiles::SeedDocument(seed))return 0;
        return Combo_OnReloadRequestImpl(nullptr,&seed);
    }catch(const std::exception& e){std::cerr<<"[ComboShip] Profile save preparation rejected: "<<e.what()<<'\n';return 0;}
}
static int ProfilePendingReady(int slot){
    try{return sProfiles&&!g_GenerateBusy.load()&&!g_ComboPendingFinalize.load()&&!sProfileAttempt&&
        ComboRando::Profiles::SeedDocument(sProfiles->view().pendingSeed(slot));}catch(...){return 0;}
}
static int Combo_OnReloadRequest(const char* path){
    if(!sProfiles)return Combo_OnReloadRequestImpl(path);
    const bool explicitImport=path&&path[0];
    try {
        if(g_GenerateBusy.load()||g_ComboPendingFinalize.load()||sProfileAttempt)return 0;
        const int slot=sProfileSelected?sProfileSelected():0;
        nlohmann::json seed;
        if(explicitImport){
            if(!sProfileCanGenerate||!sProfileCanGenerate())return 0;
            if(!TryLoadComboSeedFile(ResolveComboSeedPath(path),seed))return 0;
            // Commit the sidecar only after the actual native importer accepts it.
        }else{
            seed=sProfiles->view().pendingSeed(slot);
            if(seed.is_null()&&sProfileLegacyPendingMigration){
                const char* remembered=SOH_GetComboSpoilerPath?SOH_GetComboSpoilerPath():nullptr;
                if(!remembered||!remembered[0]||!TryLoadComboSeedFile(ResolveComboSeedPath(remembered),seed))FindNewestComboSeed(seed);
                // Before profiles, one remembered unstarted seed was usable in
                // any empty file. Retain that capability exactly once.
                if(!ComboRando::Profiles::SeedDocument(seed))seed=nullptr;
                sProfiles->migrateLegacyPending(seed);
                sProfileLegacyPendingMigration=false;
            }
        }
        if(!ComboRando::Profiles::SeedDocument(seed))return 0;
        const int accepted=Combo_OnReloadRequestImpl(nullptr,&seed);
        if(accepted&&explicitImport)sProfiles->importSeed(slot,seed,true);
        if(!accepted&&explicitImport&&SOH_SetSeedGenerated)SOH_SetSeedGenerated(0);
        return accepted;
    }catch(const std::exception& e){if(explicitImport&&SOH_SetSeedGenerated)SOH_SetSeedGenerated(0);std::cerr<<"[ComboShip] Profile import rejected: "<<e.what()<<'\n';return 0;}
}

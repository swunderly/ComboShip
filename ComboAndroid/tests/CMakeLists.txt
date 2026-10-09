cmake_minimum_required(VERSION 3.16)
project(ComboAndroidTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
find_package(Threads REQUIRED)
enable_testing()
if(COMBO_SOURCE_DIR)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(NAME oot_silver_native_routes_and_coverage COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/silver_route_tests.py"
        --source "${COMBO_SOURCE_DIR}/.." --build-dir "${CMAKE_BINARY_DIR}/silver-routes-native"
        --compiler "${CMAKE_CXX_COMPILER}")
    set(COMBO_PRESENTATION_TEST_COMPILER "${CMAKE_CXX_COMPILER}" CACHE FILEPATH
        "Host C++ compiler for extracted native cross-warp presentation tests")
    add_test(NAME cross_warp_native_presentation_and_skips COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/song_presentation_tests.py"
        --source "${COMBO_SOURCE_DIR}/.." --build-dir "${CMAKE_BINARY_DIR}/song-presentation-native"
        --compiler "${COMBO_PRESENTATION_TEST_COMPILER}")
    add_test(NAME cross_warp_native_audio_and_dialect COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/warp_audio_tests.py"
        --source "${COMBO_SOURCE_DIR}/.." --build-dir "${CMAKE_BINARY_DIR}/warp-audio-native"
        --compiler "${COMBO_PRESENTATION_TEST_COMPILER}")
endif()
if(MSVC AND COMBO_SOURCE_DIR AND COMBO_JSON_INCLUDE_DIR)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(NAME profile_gameplay_rules_sync_and_file_isolation COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/profile_gameplay_rule_tests.py"
        --source "${COMBO_SOURCE_DIR}/.." --build-dir "${CMAKE_BINARY_DIR}/profile-gameplay-rules"
        --json-include "${COMBO_JSON_INCLUDE_DIR}")
    get_filename_component(COMBO_PROFILE_TEST_IMGUI "${COMBO_JSON_INCLUDE_DIR}/../../imgui-src" ABSOLUTE)
    if(EXISTS "${COMBO_PROFILE_TEST_IMGUI}/imgui.cpp")
        add_test(NAME profile_menu_dropdown_refresh_memory_safety COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/profile_menu_dropdown_tests.py"
            --source "${COMBO_SOURCE_DIR}/.." --build-dir "${CMAKE_BINARY_DIR}/profile-menu-dropdown"
            --json-include "${COMBO_JSON_INCLUDE_DIR}" --imgui "${COMBO_PROFILE_TEST_IMGUI}")
    endif()
    add_test(NAME combo_dormant_mm_slot_isolation COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/combo_slot_residency_tests.py"
        --source "${COMBO_SOURCE_DIR}/.." --build-dir "${CMAKE_BINARY_DIR}/combo-slot-residency" --json-include "${COMBO_JSON_INCLUDE_DIR}"
        --compiler "${CMAKE_CXX_COMPILER}")
    add_test(NAME oot_early_frog_native_predicate_and_reward COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/early_frog_rules_tests.py"
        --source "${COMBO_SOURCE_DIR}/.." --build-dir "${CMAKE_BINARY_DIR}/early-frog-native")
    add_test(NAME oot_song_age_native_area_propagation COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/song_age_area_tests.py"
        --source "${COMBO_SOURCE_DIR}/.." --json-include "${COMBO_JSON_INCLUDE_DIR}"
        --build-dir "${CMAKE_BINARY_DIR}/song-age-native")
    add_test(NAME oot_tracker_transition_reachability_refresh COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tracker_transition_tests.py"
        --source "${COMBO_SOURCE_DIR}/.." --json-include "${COMBO_JSON_INCLUDE_DIR}"
        --build-dir "${CMAKE_BINARY_DIR}/tracker-transition-native")
    set(COMBO_BLOCK_TEST_CLANG "" CACHE FILEPATH "Clang with x86 FMA support for collision regression")
    if(COMBO_BLOCK_TEST_CLANG)
        add_test(NAME oot_push_block_permanent_collision COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/block_persistence_tests.py"
            --source "${COMBO_SOURCE_DIR}/.." --build-dir "${CMAKE_BINARY_DIR}/push-block-native"
            --compiler "${COMBO_BLOCK_TEST_CLANG}")
    endif()
    add_test(NAME oot_tracker_knowledge_save_lifecycle COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tracker_persistence_tests.py"
        --source "${COMBO_SOURCE_DIR}/.." --json-include "${COMBO_JSON_INCLUDE_DIR}"
        --build-dir "${CMAKE_BINARY_DIR}/tracker-persistence-native")
    add_test(NAME logic_parity_prices_scarecrow_and_live_rules COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/logic_parity_tests.py"
        --source "${COMBO_SOURCE_DIR}/.." --json-include "${COMBO_JSON_INCLUDE_DIR}"
        --build-dir "${CMAKE_BINARY_DIR}/logic-parity-native")
    add_test(NAME oot_native_renewable_resource_sources COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/resource_sources_tests.py"
        --source "${COMBO_SOURCE_DIR}/.." --build-dir "${CMAKE_BINARY_DIR}/resource-sources-native")
    add_test(NAME oot_headless_gameplay_rule_writer_abi COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/headless_rule_writer_tests.py"
        --source "${COMBO_SOURCE_DIR}/.." --build-dir "${CMAKE_BINARY_DIR}/headless-rule-writer-native")
    add_test(NAME song_native_search_mapping_and_oracle_flags COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/song_native_mapping_tests.py"
        --source "${COMBO_SOURCE_DIR}/.." --json-include "${COMBO_JSON_INCLUDE_DIR}"
        --build-dir "${CMAKE_BINARY_DIR}/song-native-mapping")
endif()
if(COMBO_SOURCE_DIR)
    add_executable(hud_layout_tests ComboHudLayoutTests.cpp)
    target_include_directories(hud_layout_tests PRIVATE "${COMBO_SOURCE_DIR}")
    if(MSVC)
        target_compile_options(hud_layout_tests PRIVATE /W4 /WX /UNDEBUG)
    else()
        target_compile_options(hud_layout_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    endif()
    add_test(NAME shared_hud_layout COMMAND hud_layout_tests)
    add_executable(screen_fit_tests screen_fit_tests.cpp)
    target_include_directories(screen_fit_tests PRIVATE "${COMBO_SOURCE_DIR}/../libultraship/include")
    if(MSVC)
        target_compile_options(screen_fit_tests PRIVATE /W4 /WX /UNDEBUG)
    else()
        target_compile_options(screen_fit_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    endif()
    add_test(NAME automatic_screen_fit COMMAND screen_fit_tests)
    add_executable(mm_key_ring_tests mm_key_ring_tests.cpp)
    target_include_directories(mm_key_ring_tests PRIVATE "${COMBO_SOURCE_DIR}/../mm/2s2h")
    if(MSVC)
        target_compile_options(mm_key_ring_tests PRIVATE /W4 /UNDEBUG /utf-8)
    else()
        target_compile_options(mm_key_ring_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    endif()
    add_test(NAME mm_key_ring_capped_grants_and_pool COMMAND mm_key_ring_tests)
    add_executable(silver_rupee_tests silver_rupee_tests.cpp)
    target_include_directories(silver_rupee_tests PRIVATE "${COMBO_SOURCE_DIR}/../soh/soh/Enhancements/randomizer/silver_rupees")
    if(MSVC)
        target_compile_options(silver_rupee_tests PRIVATE /W4 /WX /UNDEBUG /utf-8)
    else()
        target_compile_options(silver_rupee_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    endif()
    add_test(NAME silver_rupee_identity_counts_and_layouts COMMAND silver_rupee_tests)
    add_test(NAME silver_rupee_native_actor_grant_pool_and_persistence COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/silver_rupee_runtime_tests.py"
        --source "${COMBO_SOURCE_DIR}/.." --build-dir "${CMAKE_BINARY_DIR}/silver-rupee-native"
        --compiler "${COMBO_PRESENTATION_TEST_COMPILER}")
    add_test(NAME feature_item_native_and_foreign_presentation COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/feature_item_presentation_tests.py"
        --source "${COMBO_SOURCE_DIR}/.." --build-dir "${CMAKE_BINARY_DIR}/feature-item-presentation"
        --compiler "${COMBO_PRESENTATION_TEST_COMPILER}")
    add_test(NAME mm_key_ring_native_runtime_oracle_and_placement COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/mm_key_ring_native_tests.py"
        --source "${COMBO_SOURCE_DIR}/.." --build-dir "${CMAKE_BINARY_DIR}/mm-key-ring-native"
        --compiler "${COMBO_PRESENTATION_TEST_COMPILER}")
    if(COMBO_JSON_INCLUDE_DIR)
        add_executable(mm_save_compatibility_tests mm_save_compatibility_tests.cpp)
        target_compile_definitions(mm_save_compatibility_tests PRIVATE COMBO_BUILD _CRT_SECURE_NO_WARNINGS)
        target_include_directories(mm_save_compatibility_tests PRIVATE
            "${COMBO_SOURCE_DIR}"
            "${COMBO_SOURCE_DIR}/../mm/include" "${COMBO_SOURCE_DIR}/../mm/2s2h"
            "${COMBO_SOURCE_DIR}/../mm" "${COMBO_SOURCE_DIR}/../libultraship/include"
            "${COMBO_JSON_INCLUDE_DIR}" "${COMBO_JSON_INCLUDE_DIR}/../../spdlog-src/include")
        target_compile_features(mm_save_compatibility_tests PRIVATE cxx_std_20)
        if(MSVC)
            target_compile_options(mm_save_compatibility_tests PRIVATE /UNDEBUG /utf-8)
        else()
            target_compile_options(mm_save_compatibility_tests PRIVATE -UNDEBUG)
        endif()
        add_test(NAME mm_native_save_backward_compatibility COMMAND mm_save_compatibility_tests)
        add_executable(song_travel_tests song_travel_tests.cpp)
        target_include_directories(song_travel_tests PRIVATE "${COMBO_SOURCE_DIR}" "${COMBO_JSON_INCLUDE_DIR}")
        target_compile_features(song_travel_tests PRIVATE cxx_std_20)
        if(MSVC)
            target_compile_options(song_travel_tests PRIVATE /W4 /UNDEBUG)
        else()
            target_compile_options(song_travel_tests PRIVATE -Wall -Wextra -UNDEBUG)
        endif()
        add_test(NAME cross_game_song_routes COMMAND song_travel_tests)
        add_executable(gameplay_rules_tests gameplay_rules_tests.cpp)
        target_include_directories(gameplay_rules_tests PRIVATE "${COMBO_SOURCE_DIR}" "${COMBO_JSON_INCLUDE_DIR}")
        if(MSVC)
            target_compile_options(gameplay_rules_tests PRIVATE /W4 /WX /UNDEBUG)
        else()
            target_compile_options(gameplay_rules_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
        endif()
        add_test(NAME gameplay_rules COMMAND gameplay_rules_tests)
        add_executable(gameplay_rule_snapshot_tests gameplay_rule_snapshot_tests.cpp)
        target_include_directories(gameplay_rule_snapshot_tests PRIVATE "${COMBO_SOURCE_DIR}" "${COMBO_JSON_INCLUDE_DIR}")
        if(MSVC)
            target_compile_options(gameplay_rule_snapshot_tests PRIVATE /W4 /WX /UNDEBUG)
        else()
            target_compile_options(gameplay_rule_snapshot_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
        endif()
        add_test(NAME gameplay_rule_slot_snapshots COMMAND gameplay_rule_snapshot_tests)
        add_executable(shadow_door_logic_tests shadow_door_logic_tests.cpp)
        target_include_directories(shadow_door_logic_tests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/native_oot_route_probe" "${COMBO_SOURCE_DIR}" "${COMBO_JSON_INCLUDE_DIR}")
        if(MSVC)
            target_compile_options(shadow_door_logic_tests PRIVATE /W4 /WX /UNDEBUG)
        else()
            target_compile_options(shadow_door_logic_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
        endif()
        add_test(NAME shadow_door_native_logic_route COMMAND shadow_door_logic_tests)
        add_executable(multiplayer_protocol_tests multiplayer_protocol_tests.cpp)
        target_include_directories(multiplayer_protocol_tests PRIVATE "${COMBO_SOURCE_DIR}" "${COMBO_JSON_INCLUDE_DIR}")
        if(MSVC)
            target_compile_options(multiplayer_protocol_tests PRIVATE /W4 /WX /UNDEBUG)
        else()
            target_compile_options(multiplayer_protocol_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
        endif()
        add_test(NAME multiplayer_protocol COMMAND multiplayer_protocol_tests)
    endif()
    add_executable(generation_control_tests generation_control_tests.cpp)
    target_compile_definitions(generation_control_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_include_directories(generation_control_tests PRIVATE "${COMBO_SOURCE_DIR}")
    target_link_libraries(generation_control_tests PRIVATE Threads::Threads)
    if(MSVC)
        target_compile_options(generation_control_tests PRIVATE /W4 /WX /UNDEBUG)
    else()
        target_compile_options(generation_control_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    endif()
    add_test(NAME generation_cancel_and_timeout_prompt COMMAND generation_control_tests)
    add_executable(navigation_tests navigation_tests.cpp)
    target_include_directories(navigation_tests PRIVATE "${COMBO_SOURCE_DIR}/gui")
    if(MSVC)
        target_compile_options(navigation_tests PRIVATE /W4 /WX /UNDEBUG)
    else()
        target_compile_options(navigation_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    endif()
    add_test(NAME feature_navigation COMMAND navigation_tests)
    add_executable(check_tracker_tests check_tracker_tests.cpp)
    target_include_directories(check_tracker_tests PRIVATE "${COMBO_SOURCE_DIR}/gui")
    if(MSVC)
        target_compile_options(check_tracker_tests PRIVATE /W4 /WX /UNDEBUG)
    else()
        target_compile_options(check_tracker_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    endif()
    add_test(NAME shared_check_tracker COMMAND check_tracker_tests)
    add_executable(tracker_knowledge_tests tracker_knowledge_tests.cpp)
    target_include_directories(tracker_knowledge_tests PRIVATE "${COMBO_SOURCE_DIR}/gui")
    if(MSVC)
        target_compile_options(tracker_knowledge_tests PRIVATE /W4 /WX /UNDEBUG)
    else()
        target_compile_options(tracker_knowledge_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    endif()
    add_test(NAME tracker_knowledge_provenance_and_visibility COMMAND tracker_knowledge_tests)

    add_executable(shop_hours_tests shop_hours_tests.cpp)
    target_include_directories(shop_hours_tests PRIVATE "${COMBO_SOURCE_DIR}/gui")
    if(MSVC)
        target_compile_options(shop_hours_tests PRIVATE /W4 /WX /UNDEBUG)
    else()
        target_compile_options(shop_hours_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    endif()
    add_test(NAME oot_shop_hours_logic COMMAND shop_hours_tests)

    add_executable(oot_capability_tests oot_capability_tests.cpp)
    target_include_directories(oot_capability_tests PRIVATE "${COMBO_SOURCE_DIR}/gui")
    if(MSVC)
        target_compile_options(oot_capability_tests PRIVATE /W4 /WX /UNDEBUG)
    else()
        target_compile_options(oot_capability_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    endif()
    add_test(NAME oot_exact_capability_modes COMMAND oot_capability_tests)

    add_executable(mm_owl_editor_tests mm_owl_editor_tests.cpp)
    target_include_directories(mm_owl_editor_tests PRIVATE "${COMBO_SOURCE_DIR}/../mm/2s2h/DeveloperTools")
    if(MSVC)
        target_compile_options(mm_owl_editor_tests PRIVATE /W4 /WX /UNDEBUG)
    else()
        target_compile_options(mm_owl_editor_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    endif()
    add_test(NAME mm_owl_statue_editor COMMAND mm_owl_editor_tests)
endif()
add_executable(input_tests input_tests.cpp)
target_include_directories(input_tests PRIVATE ../app/src/main/cpp)
target_link_libraries(input_tests PRIVATE Threads::Threads)
if(MSVC)
    target_compile_options(input_tests PRIVATE /W4 /WX /UNDEBUG)
else()
    target_compile_options(input_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
endif()
add_test(NAME input_snapshot_coherence_and_release COMMAND input_tests)
if(UNIX)
    add_executable(native_tests native_tests.cpp ../app/src/main/cpp/RuntimePaths.cpp)
    target_include_directories(native_tests PRIVATE ../app/src/main/cpp)
    target_link_libraries(native_tests PRIVATE Threads::Threads)
    target_compile_options(native_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    add_test(NAME native_paths_input_and_session_lock COMMAND native_tests)
endif()

if(COMBO_SOURCE_DIR AND COMBO_JSON_INCLUDE_DIR)
    add_executable(combined_logic_tests combined_logic_tests.cpp)
    target_include_directories(combined_logic_tests PRIVATE "${COMBO_SOURCE_DIR}" "${COMBO_JSON_INCLUDE_DIR}")
    target_compile_features(combined_logic_tests PRIVATE cxx_std_20)
    target_compile_definitions(combined_logic_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
    if(MSVC)
        target_compile_options(combined_logic_tests PRIVATE /W4 /UNDEBUG)
    else()
        target_compile_options(combined_logic_tests PRIVATE -Wall -Wextra -UNDEBUG)
    endif()
    add_test(NAME combined_reachability_and_native_abilities COMMAND combined_logic_tests)
endif()

if(COMBO_SOURCE_DIR)
    add_executable(tricks_tests ComboTricksTests.cpp)
    target_include_directories(tricks_tests PRIVATE "${COMBO_SOURCE_DIR}")
    if(MSVC)
        target_compile_options(tricks_tests PRIVATE /W4 /WX /UNDEBUG)
    else()
        target_compile_options(tricks_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    endif()
    add_test(NAME unified_tricks_catalogue COMMAND tricks_tests)
endif()

if(COMBO_SOURCE_DIR)
    add_executable(mm_enhancement_logic_tests MmEnhancementLogicTests.cpp)
    target_include_directories(mm_enhancement_logic_tests PRIVATE "${COMBO_SOURCE_DIR}")
    if(MSVC)
        target_compile_options(mm_enhancement_logic_tests PRIVATE /W4 /WX /UNDEBUG)
    else()
        target_compile_options(mm_enhancement_logic_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    endif()
    add_test(NAME mm_enhancement_logic COMMAND mm_enhancement_logic_tests)
endif()

if(COMBO_SOURCE_DIR AND COMBO_JSON_INCLUDE_DIR)
    # Use the actual first (COMBO_BUILD) definitions from MM's production
    # header. Fail configuration if a renamed/missing macro stops coverage.
    set(COMBO_MM_LOGIC_DIR "${COMBO_SOURCE_DIR}/../mm/2s2h/Rando/Logic")
    get_filename_component(COMBO_MM_LOGIC_DIR "${COMBO_MM_LOGIC_DIR}" ABSOLUTE)
    file(TO_CMAKE_PATH "${COMBO_MM_LOGIC_DIR}" COMBO_MM_LOGIC_DIR)
    set(mm_trick_macros CAN_CROSS_PINNACLE_ROCK CAN_WIN_POST_OFFICE
        CAN_OPEN_OCEANSIDE_CODE_CHEST CAN_REACH_CLOCK_TOWN_LOWER_CHEST
        CAN_REACH_DOGGY_CHEST CAN_SKIP_PALACE_BEANS
        CAN_CLIMB_IKANA_RIVERSIDE CAN_CLIMB_STONE_TOWER)
    file(STRINGS "${COMBO_MM_LOGIC_DIR}/Logic.h" mm_logic_lines)
    set(mm_trick_header "#pragma once\n")
    foreach(macro IN LISTS mm_trick_macros)
        set(found_macro FALSE)
        foreach(line IN LISTS mm_logic_lines)
            if(line MATCHES "^#define ${macro} ")
                string(APPEND mm_trick_header "${line}\n")
                set(found_macro TRUE)
                break()
            endif()
        endforeach()
        if(NOT found_macro)
            message(FATAL_ERROR "Missing production MM trick macro: ${macro}")
        endif()
    endforeach()
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/MmTrickProductionMacros.h" "${mm_trick_header}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${COMBO_MM_LOGIC_DIR}/Logic.h")
    add_executable(mm_trick_route_tests mm_trick_route_tests.cpp)
    target_include_directories(mm_trick_route_tests PRIVATE "${COMBO_SOURCE_DIR}"
        "${COMBO_JSON_INCLUDE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
    target_compile_definitions(mm_trick_route_tests PRIVATE COMBO_MM_LOGIC_DIR="${COMBO_MM_LOGIC_DIR}")
    if(MSVC)
        target_compile_options(mm_trick_route_tests PRIVATE /W4 /WX /UNDEBUG)
    else()
        target_compile_options(mm_trick_route_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    endif()
    add_test(NAME mm_native_trick_routes_and_snapshots COMMAND mm_trick_route_tests)
endif()

if(COMBO_SOURCE_DIR AND COMBO_JSON_INCLUDE_DIR)
    add_executable(mm_zora_egg_logic_tests mm_zora_egg_logic_tests.cpp)
    target_include_directories(mm_zora_egg_logic_tests PRIVATE "${COMBO_SOURCE_DIR}" "${COMBO_JSON_INCLUDE_DIR}")
    if(MSVC)
        target_compile_options(mm_zora_egg_logic_tests PRIVATE /W4 /WX /UNDEBUG)
    else()
        target_compile_options(mm_zora_egg_logic_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    endif()
    add_test(NAME mm_zora_egg_rule_and_song_snapshot_defaults COMMAND mm_zora_egg_logic_tests)
endif()

if(COMBO_SOURCE_DIR AND COMBO_JSON_INCLUDE_DIR)
    add_executable(night_skulltula_logic_tests night_skulltula_logic_tests.cpp)
    target_include_directories(night_skulltula_logic_tests PRIVATE "${COMBO_SOURCE_DIR}" "${COMBO_JSON_INCLUDE_DIR}")
    if(MSVC)
        target_compile_options(night_skulltula_logic_tests PRIVATE /W4 /WX /UNDEBUG)
    else()
        target_compile_options(night_skulltula_logic_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    endif()
    add_test(NAME oot_night_skulltula_time_rule COMMAND night_skulltula_logic_tests)
endif()

if(COMBO_SOURCE_DIR AND COMBO_JSON_INCLUDE_DIR)
    add_executable(kokiri_seed_compatibility_tests kokiri_seed_compatibility_tests.cpp)
    target_include_directories(kokiri_seed_compatibility_tests PRIVATE "${COMBO_SOURCE_DIR}" "${COMBO_JSON_INCLUDE_DIR}")
    if(MSVC)
        target_compile_options(kokiri_seed_compatibility_tests PRIVATE /W4 /WX /UNDEBUG)
    else()
        target_compile_options(kokiri_seed_compatibility_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    endif()
    add_test(NAME kokiri_seed_catalog_compatibility COMMAND kokiri_seed_compatibility_tests)
    add_executable(menu_search_tests menu_search_tests.cpp)
    target_include_directories(menu_search_tests PRIVATE "${COMBO_SOURCE_DIR}" "${COMBO_JSON_INCLUDE_DIR}")
    if(MSVC)
        target_compile_options(menu_search_tests PRIVATE /W4 /WX /UNDEBUG)
    else()
        target_compile_options(menu_search_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    endif()
    add_test(NAME unified_menu_search_navigation COMMAND menu_search_tests)
    add_executable(menu_refresh_tests menu_refresh_tests.cpp)
    target_include_directories(menu_refresh_tests PRIVATE "${COMBO_SOURCE_DIR}" "${COMBO_JSON_INCLUDE_DIR}")
    if(MSVC)
        target_compile_options(menu_refresh_tests PRIVATE /W4 /WX /UNDEBUG)
    else()
        target_compile_options(menu_refresh_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    endif()
    add_test(NAME menu_refresh_counts_migration_and_presets COMMAND menu_refresh_tests)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(NAME community_preset_native_option_mapping COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/preset_native_mapping_tests.py" --source "${COMBO_SOURCE_DIR}/..")
endif()

if(COMBO_SOURCE_DIR)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_executable(mm_combat_tests mm_combat_tests.cpp)
    target_include_directories(mm_combat_tests PRIVATE "${COMBO_SOURCE_DIR}" "${COMBO_JSON_INCLUDE_DIR}")
    if(MSVC)
        target_compile_options(mm_combat_tests PRIVATE /W4 /WX /UNDEBUG)
    else()
        target_compile_options(mm_combat_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    endif()
    add_test(NAME mm_imported_ootmm_combat_predicates COMMAND mm_combat_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/ootmm-32.3-combat.json")
    add_test(NAME mm_native_resource_propagation COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/mm_resource_tests.py" --source "${COMBO_SOURCE_DIR}/.."
        --build-dir "${CMAKE_CURRENT_BINARY_DIR}/mm-resources")
    add_test(NAME mm_native_schedule_propagation COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/mm_schedule_tests.py" --source "${COMBO_SOURCE_DIR}/.."
        --build-dir "${CMAKE_CURRENT_BINARY_DIR}/mm-schedules")
    add_test(NAME mm_native_usable_bottles COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/mm_bottle_tests.py" --source "${COMBO_SOURCE_DIR}/.."
        --build-dir "${CMAKE_CURRENT_BINARY_DIR}/mm-bottles")
    add_test(NAME mm_native_scenery_requirements COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/mm_scenery_tests.py" --source "${COMBO_SOURCE_DIR}/.."
        --build-dir "${CMAKE_CURRENT_BINARY_DIR}/mm-scenery")
    add_test(NAME oot_enabled_tricks_survive_finalization COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/oot_trick_finalize_tests.py" --source "${COMBO_SOURCE_DIR}/.."
        --build-dir "${CMAKE_CURRENT_BINARY_DIR}/oot-tricks")
    add_test(NAME oot_masks_owned_and_generation_modes COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/oot_mask_quest_tests.py" --source "${COMBO_SOURCE_DIR}/.."
        --build-dir "${CMAKE_CURRENT_BINARY_DIR}/oot-masks" --json-include "${COMBO_JSON_INCLUDE_DIR}")
    add_executable(mm_oracle_stock_tests mm_oracle_stock_tests.cpp)
    target_compile_features(mm_oracle_stock_tests PRIVATE cxx_std_20)
    target_include_directories(mm_oracle_stock_tests PRIVATE "${COMBO_SOURCE_DIR}" "${COMBO_JSON_INCLUDE_DIR}")
    if(MSVC)
        target_compile_options(mm_oracle_stock_tests PRIVATE /UNDEBUG)
    else()
        target_compile_options(mm_oracle_stock_tests PRIVATE -UNDEBUG)
    endif()
    add_test(NAME mm_renewable_stock_survives_oracle_reset COMMAND mm_oracle_stock_tests)
endif()


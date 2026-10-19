# ComboShip: the Master Quest data bundled from OoTMM v32.3 (README.md in this folder) and its decoder.
# The pins are the SHA-256 of OoTMM's packages/generator/data/static files at tag v32.3; a changed copy stops
# the configuration (CMake/ComboEmbedBinary.cmake).
include("${CMAKE_CURRENT_LIST_DIR}/../../CMake/ComboEmbedBinary.cmake")

set(COMBO_MQ_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(COMBO_MQ_BINS
    kMqRooms  "${COMBO_MQ_DIR}/data/mq_rooms.bin"  3b3e1f836124f756560382e7ae033bfb7bd84570b2b1c5ea228655728cb6f636
    kMqScenes "${COMBO_MQ_DIR}/data/mq_scenes.bin" e1b77a41950ef656b7fcf411b119ea5b9617dc3dac3aa1560faf3567881c9a49
    kMqMaps   "${COMBO_MQ_DIR}/data/mq_maps.bin"   43f8f13fa1b8290f661dbc66d2e89cfcc6f59169e70ae5fa1ad629ac657bffaa
)

# combo_mq_add_sources(<target>): compile the pinned bins and the decoder (ComboMqData.cpp) into <target>, with the
# header-only runtime planner (MqRuntime.h), map-mark filler (MqMaps.h) and developer room log (MqRoomLog.h).
function(combo_mq_add_sources target)
    set(generated "${CMAKE_BINARY_DIR}/generated/combo_mq")
    combo_embed_binaries("${generated}" ComboMqBlobs ComboMq::Blobs ${COMBO_MQ_BINS})
    target_sources(${target} PRIVATE
        "${COMBO_MQ_DIR}/ComboMqData.h"
        "${COMBO_MQ_DIR}/ComboMqData.cpp"
        "${COMBO_MQ_DIR}/MqRuntime.h"
        "${COMBO_MQ_DIR}/MqMaps.h"
        "${COMBO_MQ_DIR}/MqRoomLog.h"
        "${generated}/ComboMqBlobs.h"
        "${generated}/ComboMqBlobs.cpp"
    )
    target_include_directories(${target} PRIVATE "${generated}")
endfunction()

#pragma once
// ComboShip: the developer Master Quest walker's PlayState glue (combo/mq/MqWalk.h). Compiled only with the CMake
// option COMBO_MQ_WALK_DEV, which is OFF by default and never on in a release.
#if defined(COMBO_BUILD) && defined(COMBO_MQ_WALK_DEV)
#include <ship/debug/Console.h>
#include <string>

// debugconsole.cpp's DebugConsole_Init calls it with its command registrar: adds mq_walk, mq_tp, mq_wait, mq_status,
// mq_log and mq_stop to the console and a frame hook that runs the walker and reads the adb command file.
void ComboMqWalk_Init(void (*addCommand)(const std::string& name, Ship::CommandEntry entry));
#endif

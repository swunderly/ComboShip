#pragma once
#include "ComboResolve.h"
#include <ship/debug/Console.h>
#include <utility>

namespace ComboConsole {
using Commands = std::map<std::string, Ship::CommandEntry>;
using Lookup = const Ship::CommandEntry* (*)(const char*);
using Resolve = void* (*)(const char*, const char*);

inline const Ship::CommandEntry* Find(const Commands& commands, const char* name) {
    const auto it = commands.find(name);
    return it == commands.end() ? nullptr : &it->second;
}

// Each game owns its handlers. Only the dispatcher enters the shared first-wins
// Console registry, so boot order cannot bind a command to a dormant game.
inline void Register(Commands& commands, const std::shared_ptr<Ship::Console>& console, const std::string& name,
                     Ship::CommandEntry entry, Resolve resolve = Combo_ResolveSym) {
    commands.insert_or_assign(name, entry);
    entry.Handler = [name, resolve](std::shared_ptr<Ship::Console> owner, std::vector<std::string> args,
                                    std::string* output) -> int32_t {
        const auto foreground = reinterpret_cast<int (*)(void)>(resolve("comboui", "ComboUI_GetForegroundGame"));
        const int game = foreground ? foreground() : -1;
        const auto lookup = game == 0   ? reinterpret_cast<Lookup>(resolve("soh", "SOH_GetConsoleCommandForCombo"))
                            : game == 1 ? reinterpret_cast<Lookup>(resolve("2ship", "MM_GetConsoleCommandForCombo"))
                                        : nullptr;
        const auto* command = lookup ? lookup(name.c_str()) : nullptr;
        if (!command || !command->Handler) {
            if (output)
                *output = "Command is unavailable in the active game.";
            return 1;
        }
        return command->Handler(std::move(owner), std::move(args), output);
    };
    console->AddCommand(name, std::move(entry));
}
} // namespace ComboConsole

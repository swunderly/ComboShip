// Single-world entrance spoiler text, adapted from OoTMM v32.3 (413573d)
// packages/generator/lib/combo/spoiler.ts:237-273 and util/log-writer.ts.
// See LICENSE.OoTMM. No seed decoding, layout installation or file I/O occurs here.
#pragma once
#include "EntranceLayout.h"
#include <algorithm>
#include <map>
#include <string>

namespace ComboRando::Entrances {
// Only a validated unified layout opts in. nullptr is the legacy/no-layout path and
// produces no section, including no default-link lines. The layout's frozen v1 labels
// are ASCII, so byte ordering and length equal JavaScript's UTF-16 sort/padEnd here.
inline std::string FormatEntranceSpoiler(const ValidatedLayout* layout) {
    if (!layout)
        return {};

    std::map<std::string, std::string> lines;
    size_t width = 0;
    const auto add = [&](const Record& source, const Record& destination) {
        auto key = std::string(source.from) + " to " + std::string(source.to) + " (" + std::string(source.name) + ")";
        auto value = std::string(destination.to) + " from " + std::string(destination.from) + " (" +
                     std::string(destination.name) + ")";
        width = std::max(width, key.size());
        lines.emplace(std::move(key), std::move(value));
    };
    // Full's cross-game routes are rows like any other: writeEntrances lists every override (spoiler.ts:254-266).
    for (const auto* routes : { &layout->oot, &layout->mm, &layout->ootToMm, &layout->mmToOot })
        for (const auto& route : *routes)
            add(*route.source, *route.destination);
    // connectGamesDefault adds these to OoTMM's world. They are implicit in our envelope.
    for (const auto& link : LayoutV1::kGameLinks)
        add(*LayoutV1::FindRow(link[0]), *LayoutV1::FindRow(link[1]));

    std::string text = "Entrances\n";
    for (const auto& [key, value] : lines)
        text += "  " + key + std::string(width - key.size(), ' ') + " -> " + value + "\n";
    // LogWriter.emit joins [header, ...rows, ""]: exactly one trailing newline.
    return text;
}
} // namespace ComboRando::Entrances

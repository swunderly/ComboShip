#pragma once
#include <nlohmann/json.hpp>
#include <set>
#include <vector>
#include <stdexcept>
namespace ComboRando::Profiles {
inline std::vector<int> ValidateTrialBinding(const nlohmann::json& input, int maximum) {
    if (!input.is_object() || input.value("version", 0) != 1 || !input.contains("requiredTrials") ||
        !input["requiredTrials"].is_array() || !input.contains("trialCount") ||
        !input["trialCount"].is_number_integer())
        throw std::invalid_argument("Malformed resolved trial rules");
    const auto& rows = input["requiredTrials"];
    if (rows.size() > size_t(maximum) || input["trialCount"] != rows.size())
        throw std::invalid_argument("Resolved trial count disagrees with selected trials");
    std::set<int> seen;
    std::vector<int> result;
    for (const auto& row : rows) {
        if (!row.is_number_integer() || row < 0 || row >= maximum)
            throw std::invalid_argument("Unknown resolved trial identity");
        const int id = row.get<int>();
        if (!seen.insert(id).second)
            throw std::invalid_argument("Duplicate resolved trial identity");
        result.push_back(id);
    }
    return result;
}
} // namespace ComboRando::Profiles

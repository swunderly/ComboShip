#pragma once
#include <filesystem>
#include <string>
namespace comboandroid {
std::filesystem::path requireDirectory(const std::string& value);
class SessionLock {
public:
    explicit SessionLock(const std::filesystem::path& root);
    ~SessionLock();
    SessionLock(const SessionLock&) = delete;
    SessionLock& operator=(const SessionLock&) = delete;
private:
    int fd_ = -1;
};
}

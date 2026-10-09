#pragma once
#include <filesystem>
#include <string>
namespace comboandroid {
std::filesystem::path requireDirectory(const std::string& value);
std::string libraryPath(const std::string& nativeDirectory, const std::string& name);
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

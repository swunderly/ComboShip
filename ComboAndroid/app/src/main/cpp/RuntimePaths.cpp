#include "RuntimePaths.h"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <unistd.h>
namespace comboandroid {
std::filesystem::path requireDirectory(const std::string& value) {
    if (value.empty() || !std::filesystem::path(value).is_absolute())
        throw std::invalid_argument("Expected an absolute application directory");
    const auto result = std::filesystem::canonical(value);
    if (!std::filesystem::is_directory(result)) throw std::invalid_argument("Application path is not a directory");
    return result;
}
std::string libraryPath(const std::string& nativeDirectory, const std::string& name) {
    if (name != "libsoh.so" && name != "lib2ship.so" && name != "libcomboui.so" && name != "libultraship.so")
        throw std::invalid_argument("Unexpected native module name: " + name);
    return (requireDirectory(nativeDirectory) / name).string();
}
SessionLock::SessionLock(const std::filesystem::path& root) {
    const auto lockPath = root / ".runtime.lock";
    fd_ = ::open(lockPath.c_str(), O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd_ < 0) throw std::runtime_error("Cannot open session lock: " + std::string(std::strerror(errno)));
    struct flock lock{};
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    // POSIX record lock, interoperable with Java FileChannel (not flock()).
    if (::fcntl(fd_, F_SETLK, &lock) < 0) {
        const int error = errno;
        ::close(fd_); fd_ = -1;
        throw std::runtime_error(error == EACCES || error == EAGAIN
            ? "A game session or import is already using this data folder"
            : "Cannot lock data folder: " + std::string(std::strerror(error)));
    }
}
SessionLock::~SessionLock() { if (fd_ >= 0) ::close(fd_); }
}

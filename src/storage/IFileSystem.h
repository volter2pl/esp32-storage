#ifndef STORAGE_IFILESYSTEM_H
#define STORAGE_IFILESYSTEM_H

#include <functional>
#include <memory>
#include <string>
#include "IFile.h"
#include "Debug.h"

namespace storage {

enum class OpenMode {
    Read,
    WriteTruncate,
    WriteAppend,
    ReadWrite,
};

// Interfejs abstrakcyjny systemu plików (SD, Flash, RAM)
class IFileSystem {
public:
    virtual ~IFileSystem() = default;

    virtual bool begin() = 0;
    virtual bool listDir(const char* path, std::function<void(const char*, size_t)> callback) = 0;
    virtual bool exists(const std::string& path) = 0;
    virtual bool remove(const std::string& path) = 0;
    // Rename/move a file or directory within the same filesystem.
    // Implementations should attempt to be atomic when possible (e.g. FAT rename).
    // If destination exists, it may be removed first.
    virtual bool rename(const std::string& from, const std::string& to) = 0;
    virtual bool mkdir(const std::string& path) = 0;
    virtual uint32_t getCreatedTimestamp(const std::string& path) = 0;
    virtual uint32_t getModifiedTimestamp(const std::string& path) = 0;
    // Capacity information (bytes). Return 0 if unsupported.
    virtual uint64_t totalBytes() = 0;
    virtual uint64_t freeBytes() = 0;

    virtual std::unique_ptr<IFile> open(const std::string& path, OpenMode mode) = 0;

    std::unique_ptr<IFile> openRead(const std::string& path) {
        DBG("IFileSystem::openRead(path=%s)", path.c_str());
        return open(path, OpenMode::Read);
    }

    std::unique_ptr<IFile> openWrite(const std::string& path, bool overwrite = true) {
        DBG("IFileSystem::openWrite(path=%s, overwrite=%d)", path.c_str(), overwrite);
        return open(path, overwrite ? OpenMode::WriteTruncate : OpenMode::WriteAppend);
    }

    std::unique_ptr<IFile> openAppend(const std::string& path) {
        DBG("IFileSystem::openAppend(path=%s)", path.c_str());
        return open(path, OpenMode::WriteAppend);
    }
};

} // namespace storage

#endif // STORAGE_IFILESYSTEM_H

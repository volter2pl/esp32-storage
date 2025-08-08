#ifndef STORAGE_LITTLEFS_LITTLEFSFILESYSTEM_H
#define STORAGE_LITTLEFS_LITTLEFSFILESYSTEM_H

#include <LittleFS.h>
#include <memory>
#include <string>
#include "storage/IFileSystem.h"
#include "LittleFsFileWrapper.h"

namespace storage {
namespace littlefs {

// Implementacja IFileSystem dla LittleFS
class LittleFsFileSystem : public IFileSystem {
public:
    LittleFsFileSystem() = default;
    bool begin() override;

    bool listDir(const char* path, std::function<void(const char*, size_t)> callback) override;
    bool exists(const std::string& path) override;
    bool remove(const std::string& path) override;
    bool mkdir(const std::string& path) override;
    uint32_t getCreatedTimestamp(const std::string& path) override;
    uint32_t getModifiedTimestamp(const std::string& path) override;

    std::unique_ptr<IFile> open(const std::string& path, OpenMode mode) override;
};

} // namespace littlefs
} // namespace storage

#endif // STORAGE_LITTLEFS_LITTLEFSFILESYSTEM_H

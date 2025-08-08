#ifndef STORAGE_SD_SDFATFILESYSTEM_H
#define STORAGE_SD_SDFATFILESYSTEM_H

#include <SdFat.h>
#include <memory>
#include <string>
#include "storage/IFileSystem.h"
#include "storage/ITimeProvider.h"
#include "SdFatFileWrapper.h"

namespace storage {
namespace sd {

// Implementacja IFileSystem z wykorzystaniem SdFat
class SdFatFileSystem : public IFileSystem {
private:
    SdFat sd;
    uint8_t csPin;
    ITimeProvider* timeProvider = nullptr;

    static ITimeProvider* staticTimeProvider;
    void getCreatedDateTime(const std::string& path, uint16_t* date, uint16_t* time);
    void getModifiedDateTime(const std::string& path, uint16_t* date, uint16_t* time);
public:
    explicit SdFatFileSystem(uint8_t cs);
    void setTimeProvider(ITimeProvider* provider);
    bool begin() override;

    static void getGlobalTime(uint16_t* date, uint16_t* time);
    bool listDir(const char* path, std::function<void(const char*, size_t)> callback) override;
    bool exists(const std::string& path) override;
    bool remove(const std::string& path) override;
    bool mkdir(const std::string& path) override;
    uint32_t getCreatedTimestamp(const std::string& path) override;
    uint32_t getModifiedTimestamp(const std::string& path) override;

    std::unique_ptr<IFile> open(const std::string& path, OpenMode mode) override;
};

} // namespace sd
} // namespace storage

#endif // STORAGE_SD_SDFATFILESYSTEM_H

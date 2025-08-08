#ifndef STORAGE_LITTLEFS_LITTLEFSFILEWRAPPER_H
#define STORAGE_LITTLEFS_LITTLEFSFILEWRAPPER_H

#include <LittleFS.h>
#include "storage/IFile.h"

namespace storage {
namespace littlefs {

// Wrapper IFile dla fs::File
class LittleFsFileWrapper : public IFile {
private:
    fs::File file;
public:
    explicit LittleFsFileWrapper(fs::File f);

    size_t read(void* buf, size_t size) override;
    size_t write(const void* buf, size_t size) override;
    void flush() override;
    bool seek(uint32_t pos) override;
    uint32_t position() override;
    uint32_t size() override;
    bool isOpen() const override;
    void close() override;
};

} // namespace littlefs
} // namespace storage

#endif // STORAGE_LITTLEFS_LITTLEFSFILEWRAPPER_H

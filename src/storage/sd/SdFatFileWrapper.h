#ifndef STORAGE_SD_SDFATFILEWRAPPER_H
#define STORAGE_SD_SDFATFILEWRAPPER_H

#include <SdFat.h>
#include "storage/IFile.h"

namespace storage {
namespace sd {

// Wrapper IFile dla FsFile
class SdFatFileWrapper : public IFile {
private:
    FsFile file;
public:
    explicit SdFatFileWrapper(FsFile f);

    size_t read(void* buf, size_t size) override;
    size_t write(const void* buf, size_t size) override;
    void flush() override;
    bool seek(uint32_t pos) override;
    uint32_t position() override;
    uint32_t size() override;
    bool isOpen() const override;
    void close() override;

    FsFile& getFile();
    bool getCreateDateTime(uint16_t* d, uint16_t* t) override;
};

} // namespace sd
} // namespace storage

#endif // STORAGE_SD_SDFATFILEWRAPPER_H

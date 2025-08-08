#ifndef STORAGE_IFILE_H
#define STORAGE_IFILE_H

#include <cstdint>
#include <cstddef>
#include "Debug.h"

namespace storage {

// Abstrakcja pojedynczego pliku
class IFile {
public:
    virtual ~IFile() = default;

    virtual size_t read(void* buf, size_t size) = 0;
    virtual size_t write(const void* buf, size_t size) = 0;
    virtual void flush() = 0;
    virtual bool seek(uint32_t pos) = 0;
    virtual uint32_t position() = 0;
    virtual uint32_t size() = 0;
    virtual bool isOpen() const = 0;
    virtual void close() = 0;

    virtual bool getCreateDateTime(uint16_t* d, uint16_t* t) {
        DBG("IFile::getCreateDateTime(d=%p, t=%p)", d, t);
        return false;
    }
};

} // namespace storage

#endif // STORAGE_IFILE_H

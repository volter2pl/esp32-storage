#include "SdFatFileWrapper.h"
#include "storage/Debug.h"

namespace storage {
namespace sd {

SdFatFileWrapper::SdFatFileWrapper(FsFile f) : file(std::move(f)) {
    DBG("SdFatFileWrapper::SdFatFileWrapper(%p)", &file);
}

size_t SdFatFileWrapper::read(void* buf, size_t size) {
    DBG("SdFatFileWrapper::read(size=%u)", size);
    size_t n = file.read(static_cast<uint8_t*>(buf), size);
    DBG("SdFatFileWrapper::read -> %u", n);
    return n;
}

size_t SdFatFileWrapper::write(const void* buf, size_t size) {
    DBG("SdFatFileWrapper::write(size=%u)", size);
    size_t n = file.write(static_cast<const uint8_t*>(buf), size);
    DBG("SdFatFileWrapper::write -> %u", n);
    return n;
}

void SdFatFileWrapper::flush() {
    DBG("SdFatFileWrapper::flush()");
    file.flush();
    DBG("SdFatFileWrapper::flush done");
}

bool SdFatFileWrapper::seek(uint32_t pos) {
    DBG("SdFatFileWrapper::seek(pos=%u)", pos);
    bool res = file.seek(pos);
    DBG("SdFatFileWrapper::seek -> %d", res);
    return res;
}

uint32_t SdFatFileWrapper::position() {
    DBG("SdFatFileWrapper::position()");
    uint32_t pos = file.position();
    DBG("SdFatFileWrapper::position -> %u", pos);
    return pos;
}

uint32_t SdFatFileWrapper::size() {
    DBG("SdFatFileWrapper::size()");
    uint32_t s = file.size();
    DBG("SdFatFileWrapper::size -> %u", s);
    return s;
}

bool SdFatFileWrapper::isOpen() const {
    DBG("SdFatFileWrapper::isOpen()");
    bool res = static_cast<bool>(file);
    DBG("SdFatFileWrapper::isOpen -> %d", res);
    return res;
}

void SdFatFileWrapper::close() {
    DBG("SdFatFileWrapper::close()");
    file.close();
    DBG("SdFatFileWrapper::close done");
}

FsFile& SdFatFileWrapper::getFile() {
    DBG("SdFatFileWrapper::getFile()");
    return file;
}

bool SdFatFileWrapper::getCreateDateTime(uint16_t* d, uint16_t* t) {
    DBG("SdFatFileWrapper::getCreateDateTime()");
    bool res = file.getCreateDateTime(d, t);
    DBG("SdFatFileWrapper::getCreateDateTime -> %d (date=%u time=%u)", res, d ? *d : 0, t ? *t : 0);
    return res;
}

} // namespace sd
} // namespace storage


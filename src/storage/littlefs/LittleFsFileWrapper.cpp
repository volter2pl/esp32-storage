#include "LittleFsFileWrapper.h"
#include "storage/Debug.h"

namespace storage {
namespace littlefs {

LittleFsFileWrapper::LittleFsFileWrapper(fs::File f) : file(std::move(f)) {
    DBG("LittleFsFileWrapper::LittleFsFileWrapper(%p)", &file);
}

size_t LittleFsFileWrapper::read(void* buf, size_t size) {
    DBG("LittleFsFileWrapper::read(size=%u)", size);
    size_t n = file.read(static_cast<uint8_t*>(buf), size);
    DBG("LittleFsFileWrapper::read -> %u", n);
    return n;
}

size_t LittleFsFileWrapper::write(const void* buf, size_t size) {
    DBG("LittleFsFileWrapper::write(size=%u)", size);
    size_t n = file.write(static_cast<const uint8_t*>(buf), size);
    DBG("LittleFsFileWrapper::write -> %u", n);
    return n;
}

void LittleFsFileWrapper::flush() {
    DBG("LittleFsFileWrapper::flush()");
    file.flush();
    DBG("LittleFsFileWrapper::flush done");
}

bool LittleFsFileWrapper::seek(uint32_t pos) {
    DBG("LittleFsFileWrapper::seek(pos=%u)", pos);
    bool res = file.seek(pos);
    DBG("LittleFsFileWrapper::seek -> %d", res);
    return res;
}

uint32_t LittleFsFileWrapper::position() {
    DBG("LittleFsFileWrapper::position()");
    uint32_t pos = file.position();
    DBG("LittleFsFileWrapper::position -> %u", pos);
    return pos;
}

uint32_t LittleFsFileWrapper::size() {
    DBG("LittleFsFileWrapper::size()");
    uint32_t s = file.size();
    DBG("LittleFsFileWrapper::size -> %u", s);
    return s;
}

bool LittleFsFileWrapper::isOpen() const {
    DBG("LittleFsFileWrapper::isOpen()");
    bool res = static_cast<bool>(file);
    DBG("LittleFsFileWrapper::isOpen -> %d", res);
    return res;
}

void LittleFsFileWrapper::close() {
    DBG("LittleFsFileWrapper::close()");
    file.close();
    DBG("LittleFsFileWrapper::close done");
}

} // namespace littlefs
} // namespace storage

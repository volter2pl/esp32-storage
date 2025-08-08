#include "SdFatFileSystem.h"
#include "storage/time/TimeUtils.h"
#include "storage/Debug.h"

namespace storage {
namespace sd {

static bool ensureParentDirs(SdFat& sd, const std::string& path) {
    if (path.empty()) return true;
    auto pos = path.find_last_of('/');
    if (pos == std::string::npos) return true;
    std::string dir = path.substr(0, pos);
    if (dir.empty()) return true;

    std::string cur;
    bool absolute = dir[0] == '/';
    size_t start = absolute ? 1 : 0;
    while (true) {
        size_t next = dir.find('/', start);
        std::string token = dir.substr(start, next == std::string::npos ? std::string::npos : next - start);
        if (!token.empty()) {
            if (cur.empty()) {
                cur = (absolute ? "/" : "") + token;
            } else {
                cur += "/" + token;
            }
            if (!sd.exists(cur.c_str())) {
                if (!sd.mkdir(cur.c_str())) {
                    return false;
                }
            }
        }
        if (next == std::string::npos) break;
        start = next + 1;
    }
    return true;
}

// statyczny wskaźnik do providera, używany przez FsDateTime
ITimeProvider* SdFatFileSystem::staticTimeProvider = nullptr;

SdFatFileSystem::SdFatFileSystem(uint8_t cs) : csPin(cs) {
    DBG("SdFatFileSystem::SdFatFileSystem(cs=%u)", cs);
}

void SdFatFileSystem::setTimeProvider(ITimeProvider* provider) {
    DBG("SdFatFileSystem::setTimeProvider(%p)", provider);
    timeProvider = provider;
    staticTimeProvider = provider;
}

void SdFatFileSystem::getGlobalTime(uint16_t* date, uint16_t* time) {
    DBG("SdFatFileSystem::getGlobalTime()");
    if (staticTimeProvider) {
        staticTimeProvider->getFatTime(date, time);
    } else {
        *date = 0;
        *time = 0;
    }
}

bool SdFatFileSystem::begin() {
    DBG("SdFatFileSystem::begin()");
    if (timeProvider) {
        FsDateTime::setCallback(SdFatFileSystem::getGlobalTime);
    }
    bool ok = sd.begin(csPin);
    DBG("SdFatFileSystem::begin result=%d", ok);
    return ok;
}

bool SdFatFileSystem::listDir(const char* path, std::function<void(const char*, size_t)> callback) {
    DBG("SdFatFileSystem::listDir(path=%s)", path);
    FsFile dir = sd.open(path);
    if (!dir || !dir.isDirectory()) {
        DBG("listDir: cannot open directory %s", path);
        return false;
    }

    FsFile entry;
    char nameBuf[64];

    while ((entry = dir.openNextFile())) {
        if (entry.getName(nameBuf, sizeof(nameBuf))) {
            DBG("listDir entry %s size=%u", nameBuf, entry.size());
            callback(nameBuf, entry.size());
        }
        entry.close();
    }
    DBG("SdFatFileSystem::listDir done");
    return true;
}

bool SdFatFileSystem::exists(const std::string& path) {
    DBG("SdFatFileSystem::exists(path=%s)", path.c_str());
    bool res = sd.exists(path.c_str());
    DBG("SdFatFileSystem::exists result=%d", res);
    return res;
}

bool SdFatFileSystem::remove(const std::string& path) {
    DBG("SdFatFileSystem::remove(path=%s)", path.c_str());
    bool res = sd.remove(path.c_str());
    DBG("SdFatFileSystem::remove result=%d", res);
    return res;
}

bool SdFatFileSystem::mkdir(const std::string& path) {
    DBG("SdFatFileSystem::mkdir(path=%s)", path.c_str());
    bool res = sd.mkdir(path.c_str());
    DBG("SdFatFileSystem::mkdir result=%d", res);
    return res;
}

void SdFatFileSystem::getCreatedDateTime(const std::string& path, uint16_t* date, uint16_t* time) {
    DBG("getCreatedDateTime(path=%s)", path.c_str());
    FsFile f = sd.open(path.c_str(), FILE_READ);
    if (!f) {
        DBG("getCreatedDateTime: open failed for %s", path.c_str());
        *date = 0;
        *time = 0;
        return;
    }

    if (!f.getCreateDateTime(date, time)) {
        DBG("getCreateDateTime: unable to read timestamps for %s", path.c_str());
        *date = 0;
        *time = 0;
    }

    f.close();
}

void SdFatFileSystem::getModifiedDateTime(const std::string& path, uint16_t* date, uint16_t* time) {
    DBG("getModifiedDateTime(path=%s)", path.c_str());
    FsFile f = sd.open(path.c_str(), FILE_READ);
    if (!f) {
        DBG("getModifiedDateTime: open failed for %s", path.c_str());
        *date = 0;
        *time = 0;
        return;
    }

    if (!f.getModifyDateTime(date, time)) {
        DBG("getModifiedDateTime: unable to read timestamps for %s", path.c_str());
        *date = 0;
        *time = 0;
    }

    f.close();
}

uint32_t SdFatFileSystem::getCreatedTimestamp(const std::string& path) {
    DBG("SdFatFileSystem::getCreatedTimestamp(path=%s)", path.c_str());
    uint16_t fatDate = 0, fatTime = 0;
    getCreatedDateTime(path, &fatDate, &fatTime);
    uint32_t ts = storage::time::fatDateTimeToUnix(fatDate, fatTime);
    DBG("getCreatedTimestamp(%s) -> %u", path.c_str(), ts);
    return ts;
}

uint32_t SdFatFileSystem::getModifiedTimestamp(const std::string& path) {
    DBG("SdFatFileSystem::getModifiedTimestamp(path=%s)", path.c_str());
    uint16_t fatDate = 0, fatTime = 0;
    getModifiedDateTime(path, &fatDate, &fatTime);
    uint32_t ts = storage::time::fatDateTimeToUnix(fatDate, fatTime);
    DBG("getModifiedTimestamp(%s) -> %u", path.c_str(), ts);
    return ts;
}

std::unique_ptr<IFile> SdFatFileSystem::open(const std::string& path, OpenMode mode) {
    DBG("SdFatFileSystem::open(path=%s, mode=%d)", path.c_str(), static_cast<int>(mode));
    oflag_t flags = O_RDONLY;
    switch (mode) {
        case OpenMode::Read:
            flags = O_RDONLY;
            break;
        case OpenMode::WriteTruncate:
            flags = O_RDWR | O_CREAT | O_TRUNC;
            break;
        case OpenMode::WriteAppend:
            flags = O_RDWR | O_CREAT | O_AT_END;
            break;
        case OpenMode::ReadWrite:
            flags = O_RDWR | O_CREAT;
            break;
    }
    if (mode != OpenMode::Read) {
        if (!ensureParentDirs(sd, path)) {
            DBG("ensureParentDirs failed for %s", path.c_str());
            return nullptr;
        }
    }
    FsFile raw = sd.open(path.c_str(), flags);
    DBG("open(%s) result=%d", path.c_str(), raw ? 1 : 0);
    if (!raw) {
        return nullptr;
    }
    return std::make_unique<SdFatFileWrapper>(std::move(raw));
}

} // namespace sd
} // namespace storage

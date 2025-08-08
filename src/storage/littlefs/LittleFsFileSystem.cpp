#include "LittleFsFileSystem.h"
#include "storage/Debug.h"

namespace storage {
namespace littlefs {

static bool ensureParentDirs(fs::FS& fs, const std::string& path) {
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
            if (!fs.exists(cur.c_str())) {
                if (!fs.mkdir(cur.c_str())) {
                    return false;
                }
            }
        }
        if (next == std::string::npos) break;
        start = next + 1;
    }
    return true;
}

namespace {
const char* MOUNT_PATH = "/littlefs";
const char* PARTITION_LABEL = "spiffs";
}

bool LittleFsFileSystem::begin() {
    DBG("LittleFsFileSystem::begin()");
    if (!LittleFS.begin(false, MOUNT_PATH, 5, PARTITION_LABEL)) {
        Serial.println("[LittleFS] Mount failed. Formatting...");
        if (!LittleFS.format()) {
            Serial.println("[LittleFS] Format failed.");
            return false;
        }
        if (!LittleFS.begin(false, MOUNT_PATH, 5, PARTITION_LABEL)) {
            Serial.println("[LittleFS] Mount failed after format.");
            return false;
        }
    }
    DBG("LittleFsFileSystem::begin success");
    return true;
}

bool LittleFsFileSystem::listDir(const char* path, std::function<void(const char*, size_t)> callback) {
    DBG("LittleFsFileSystem::listDir(path=%s)", path);
    fs::File dir = LittleFS.open(path);
    if (!dir || !dir.isDirectory()) {
        DBG("listDir: cannot open directory %s", path);
        return false;
    }

    fs::File entry;
    while ((entry = dir.openNextFile())) {
        DBG("listDir entry %s size=%u", entry.name(), entry.size());
        callback(entry.name(), entry.size());
        entry.close();
    }
    DBG("LittleFsFileSystem::listDir done");
    return true;
}

bool LittleFsFileSystem::exists(const std::string& path) {
    DBG("LittleFsFileSystem::exists(path=%s)", path.c_str());
    bool res = LittleFS.exists(path.c_str());
    DBG("LittleFsFileSystem::exists result=%d", res);
    return res;
}

bool LittleFsFileSystem::remove(const std::string& path) {
    DBG("LittleFsFileSystem::remove(path=%s)", path.c_str());
    bool res = LittleFS.remove(path.c_str());
    DBG("LittleFsFileSystem::remove result=%d", res);
    return res;
}

bool LittleFsFileSystem::mkdir(const std::string& path) {
    DBG("LittleFsFileSystem::mkdir(path=%s)", path.c_str());
    bool res = LittleFS.mkdir(path.c_str());
    DBG("LittleFsFileSystem::mkdir result=%d", res);
    return res;
}

uint32_t LittleFsFileSystem::getCreatedTimestamp(const std::string& /*path*/) {
    DBG("LittleFsFileSystem::getCreatedTimestamp() not supported");
    return 0;
}

uint32_t LittleFsFileSystem::getModifiedTimestamp(const std::string& /*path*/) {
    DBG("LittleFsFileSystem::getModifiedTimestamp() not supported");
    return 0;
}

std::unique_ptr<IFile> LittleFsFileSystem::open(const std::string& path, OpenMode mode) {
    DBG("LittleFsFileSystem::open(path=%s, mode=%d)", path.c_str(), static_cast<int>(mode));
    const char* flags = "r";

    switch (mode) {
        case OpenMode::Read:
            flags = "r";
            break;
        case OpenMode::WriteTruncate:
            flags = "w+";
            break;
        case OpenMode::WriteAppend:
            flags = "a+";
            break;
        case OpenMode::ReadWrite:
            flags = LittleFS.exists(path.c_str()) ? "r+" : "w+";
            break;
    }

    if (mode != OpenMode::Read) {
        if (!ensureParentDirs(LittleFS, path)) {
            DBG("ensureParentDirs failed for %s", path.c_str());
            return nullptr;
        }
    }

    fs::File f = LittleFS.open(path.c_str(), flags);
    DBG("open(%s) result=%d", path.c_str(), f ? 1 : 0);
    if (!f) {
        return nullptr;
    }
    return std::make_unique<LittleFsFileWrapper>(std::move(f));
}

} // namespace littlefs
} // namespace storage

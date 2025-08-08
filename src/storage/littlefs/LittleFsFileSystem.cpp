#include "LittleFsFileSystem.h"
#include "storage/Debug.h"

#include <vector>

namespace storage {
namespace littlefs {

namespace {
const char* MOUNT_PATH = "/littlefs";   // mount point używany przez VFS (wewnętrznie, żeby nie gryzł się z innymi FS, np. SD)
const char* PARTITION_LABEL = "spiffs"; // zgodnie z partitions.csv
}

// ------------------- helpers: path -------------------
static std::string normalizePath(const std::string& in) {
    if (in.empty()) return std::string();

    bool absolute = in[0] == '/';
    std::vector<std::string> stack;
    size_t i = 0, n = in.size();

    while (i < n) {
        while (i < n && in[i] == '/') i++; // pomiń duplikaty '/'
        if (i >= n) break;
        size_t j = i; while (j < n && in[j] != '/') j++;
        std::string seg = in.substr(i, j - i);
        i = j;

        if (seg == "." || seg.empty()) continue;
        if (seg == "..") {
            if (!stack.empty()) stack.pop_back();
        } else {
            stack.push_back(seg);
        }
    }

    std::string out; if (absolute) out.push_back('/');
    for (size_t k = 0; k < stack.size(); ++k) { if (k) out.push_back('/'); out += stack[k]; }
    if (out.empty()) return absolute ? std::string("/") : std::string();
    return out;
}

static bool isDirectory(fs::FS& fs, const char* path) {
    if (!fs.exists(path)) return false; // unikamy logów VFS przy nieistniejących ścieżkach
    fs::File f = fs.open(path, "r");
    bool dir = (bool)f && f.isDirectory();
    f.close();
    return dir;
}

static bool ensureParentDirs(fs::FS& fs, const std::string& rawPath) {
    if (rawPath.empty()) return true;
    std::string path = normalizePath(rawPath);
    auto pos = path.find_last_of('/');
    if (pos == std::string::npos) return true;

    std::string dir = path.substr(0, pos);
    if (dir.empty() || dir == "/") return true;

    bool absolute = dir[0] == '/';
    size_t start = absolute ? 1 : 0;
    std::string cur = absolute ? std::string("/") : std::string();

    while (start <= dir.size()) {
        size_t next = dir.find('/', start);
        std::string token = dir.substr(start, (next == std::string::npos) ? std::string::npos : next - start);
        if (!token.empty() && token != "." && token != "..") {
            if (cur.empty() || cur == "/") cur += token; else cur += "/" + token;
            (void)fs.mkdir(cur.c_str()); // idempotentnie; jeśli istnieje, zwróci false — ignorujemy
        }
        if (next == std::string::npos) break;
        start = next + 1;
    }
    return true;
}

static bool removeRecursive(fs::FS& fs, const std::string& rawPath) {
    std::string path = normalizePath(rawPath);
    if (path.empty()) return false;

    if (!fs.exists(path.c_str())) return false; // nic do usunięcia

    if (!isDirectory(fs, path.c_str())) {
        return fs.remove(path.c_str());
    }

    fs::File dir = fs.open(path.c_str(), "r");
    if (!dir) return false;

    while (true) {
        fs::File entry = dir.openNextFile();
        if (!entry) break;
        std::string child = (std::string(entry.path()));
        entry.close();
        if (child.rfind(MOUNT_PATH, 0) == 0) child.erase(0, std::string(MOUNT_PATH).size());
        if (child.empty()) child = "/";
        if (!removeRecursive(fs, child)) {
            DBG("removeRecursive: failed for %s", child.c_str());
        }
    }
    dir.close();

    #ifdef ESP_PLATFORM
    return fs.rmdir(path.c_str());
    #else
    return fs.remove(path.c_str());
    #endif
}

// ------------------- LittleFsFileSystem -------------------

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

bool LittleFsFileSystem::listDir(const char* rawPath, std::function<void(const char*, size_t)> callback) {
    std::string path = normalizePath(rawPath ? std::string(rawPath) : std::string("/"));
    if (path.empty()) path = "/";
    DBG("LittleFsFileSystem::listDir(path=%s)", path.c_str());

    fs::File dir = LittleFS.open(path.c_str(), "r");
    if (!dir || !dir.isDirectory()) {
        DBG("listDir: cannot open directory %s", path.c_str());
        return false;
    }

    while (true) {
        fs::File entry = dir.openNextFile();
        if (!entry) break;
        DBG("listDir entry %s size=%u", entry.name(), entry.size());
        callback(entry.name(), entry.size());
        entry.close();
    }
    DBG("LittleFsFileSystem::listDir done");
    return true;
}

bool LittleFsFileSystem::exists(const std::string& rawPath) {
    std::string path = normalizePath(rawPath);
    DBG("LittleFsFileSystem::exists(path=%s)", path.c_str());
    bool res = !path.empty() && LittleFS.exists(path.c_str());
    DBG("LittleFsFileSystem::exists result=%d", res);
    return res;
}

bool LittleFsFileSystem::remove(const std::string& rawPath) {
    std::string path = normalizePath(rawPath);
    DBG("LittleFsFileSystem::remove(path=%s)", path.c_str());
    if (path.empty()) return false;

    if (!LittleFS.exists(path.c_str())) { // unikamy open("r") na nieistniejących ścieżkach
        DBG("LittleFsFileSystem::remove target not exists: %s", path.c_str());
        return false;
    }

    bool res;
    if (isDirectory(LittleFS, path.c_str())) {
        res = removeRecursive(LittleFS, path);
    } else {
        res = LittleFS.remove(path.c_str());
    }
    DBG("LittleFsFileSystem::remove result=%d", res);
    return res;
}

bool LittleFsFileSystem::mkdir(const std::string& rawPath) {
    std::string path = normalizePath(rawPath);
    DBG("LittleFsFileSystem::mkdir(path=%s)", path.c_str());
    if (path.empty() || path == "/") return true;
    bool res = LittleFS.mkdir(path.c_str());
    DBG("LittleFsFileSystem::mkdir result=%d", res);
    return res;
}

uint32_t LittleFsFileSystem::getCreatedTimestamp(const std::string&) {
    DBG("LittleFsFileSystem::getCreatedTimestamp() not supported");
    return 0;
}

uint32_t LittleFsFileSystem::getModifiedTimestamp(const std::string&) {
    DBG("LittleFsFileSystem::getModifiedTimestamp() not supported");
    return 0;
}

std::unique_ptr<IFile> LittleFsFileSystem::open(const std::string& rawPath, OpenMode mode) {
    std::string path = normalizePath(rawPath);
    DBG("LittleFsFileSystem::open(path=%s, mode=%d)", path.c_str(), static_cast<int>(mode));

    const char* flags = "r"; // ciaśniejsze flagi
    switch (mode) {
        case OpenMode::Read:          flags = "r"; break;
        case OpenMode::WriteTruncate: flags = "w"; break;   // nie potrzebujemy czytania
        case OpenMode::WriteAppend:   flags = "a"; break;   // nie potrzebujemy czytania
        case OpenMode::ReadWrite:     flags = LittleFS.exists(path.c_str()) ? "r+" : "w+"; break;
    }

    if (mode != OpenMode::Read) {
        if (!ensureParentDirs(LittleFS, path)) {
            DBG("ensureParentDirs failed for %s", path.c_str());
            return nullptr;
        }
    }

    fs::File f = LittleFS.open(path.c_str(), flags);
    DBG("open(%s) result=%d", path.c_str(), f ? 1 : 0);
    if (!f) return nullptr;
    return std::make_unique<LittleFsFileWrapper>(std::move(f));
}

} // namespace littlefs
} // namespace storage

#include "SdFatFileSystem.h"
#include "storage/time/TimeUtils.h"
#include "storage/Debug.h"

#include <vector>

namespace storage {
namespace sd {

// ------------------- helpers: path -------------------
static std::string normalizePath(const std::string& in) {
    if (in.empty()) return std::string();

    bool absolute = in[0] == '/';
    std::vector<std::string> stack;
    size_t i = 0, n = in.size();

    while (i < n) {
        // pomiń wielokrotne '/'
        while (i < n && in[i] == '/') i++;
        if (i >= n) break;
        size_t j = i;
        while (j < n && in[j] != '/') j++;
        std::string seg = in.substr(i, j - i);
        i = j;

        if (seg == "." || seg.empty()) {
            continue; // ignoruj
        } else if (seg == "..") {
            if (!stack.empty()) stack.pop_back();
            // jeśli ścieżka relatywna i stack pusty, pozostaw tak (".." na początku)
        } else {
            stack.push_back(seg);
        }
    }

    std::string out;
    if (absolute) out.push_back('/');
    for (size_t k = 0; k < stack.size(); ++k) {
        if (k) out.push_back('/');
        out += stack[k];
    }
    if (out.empty()) return absolute ? std::string("/") : std::string();
    return out;
}

static bool ensureParentDirs(SdFat& sd, const std::string& rawPath) {
    if (rawPath.empty()) return true;
    std::string path = normalizePath(rawPath);
    auto pos = path.find_last_of('/');
    if (pos == std::string::npos) return true; // brak katalogu rodzica
    std::string dir = path.substr(0, pos);
    if (dir.empty() || dir == "/") return true;

    bool absolute = dir[0] == '/';
    size_t start = absolute ? 1 : 0;
    std::string cur = absolute ? std::string("/") : std::string();

    while (start <= dir.size()) {
        size_t next = dir.find('/', start);
        std::string token = dir.substr(start, (next == std::string::npos) ? std::string::npos : next - start);
        if (!token.empty() && token != ".") {
            if (token == "..") {
                // cofnięcie w górę dla relatywnych — dla absolutnych ignorujemy powyżej '/'
                if (!cur.empty() && cur != "/") {
                    auto slash = cur.find_last_of('/');
                    if (slash == std::string::npos) cur.clear();
                    else cur.erase(slash ? slash : 1);
                }
            } else {
                if (cur.empty() || cur == "/") cur += token;
                else cur += "/" + token;
                if (!sd.exists(cur.c_str())) {
                    if (!sd.mkdir(cur.c_str())) {
                        DBG("ensureParentDirs: mkdir failed for %s", cur.c_str());
                        return false;
                    }
                }
            }
        }
        if (next == std::string::npos) break;
        start = next + 1;
    }
    return true;
}

static bool isDirectory(SdFat& sd, const char* path) {
    FsFile f = sd.open(path);
    bool dir = (bool)f && f.isDirectory();
    f.close();
    return dir;
}

static bool removeRecursive(SdFat& sd, const std::string& rawPath) {
    std::string path = normalizePath(rawPath);
    if (path.empty()) return false;

    if (!isDirectory(sd, path.c_str())) {
        // plik
        return sd.remove(path.c_str());
    }

    // katalog — usuń zawartość
    FsFile dir = sd.open(path.c_str());
    if (!dir) return false;

    FsFile entry;
    char nameBuf[64];
    while ((entry = dir.openNextFile())) {
        if (entry.getName(nameBuf, sizeof(nameBuf))) {
            std::string child = (path == "/" ? std::string("/") + nameBuf : path + "/" + nameBuf);
            entry.close();
            if (!removeRecursive(sd, child)) {
                // spróbuj dalej, ale raportuj błąd
                DBG("removeRecursive: failed for %s", child.c_str());
                // nie przerywamy, by spróbować usunąć resztę
            }
        } else {
            entry.close();
        }
    }
    dir.close();

    // po opróżnieniu katalogu usuń sam katalog
    return sd.rmdir(path.c_str());
}

// ------------------- statyczny provider czasu -------------------
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
        *date = 0; *time = 0;
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

bool SdFatFileSystem::listDir(const char* rawPath, std::function<void(const char*, size_t)> callback) {
    std::string path = normalizePath(rawPath ? std::string(rawPath) : std::string("/"));
    if (path.empty()) path = "/";
    DBG("SdFatFileSystem::listDir(path=%s)", path.c_str());

    FsFile dir = sd.open(path.c_str());
    if (!dir || !dir.isDirectory()) {
        DBG("listDir: cannot open directory %s", path.c_str());
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

bool SdFatFileSystem::exists(const std::string& rawPath) {
    std::string path = normalizePath(rawPath);
    DBG("SdFatFileSystem::exists(path=%s)", path.c_str());
    bool res = !path.empty() && sd.exists(path.c_str());
    DBG("SdFatFileSystem::exists result=%d", res);
    return res;
}

bool SdFatFileSystem::remove(const std::string& rawPath) {
    std::string path = normalizePath(rawPath);
    DBG("SdFatFileSystem::remove(path=%s)", path.c_str());
    if (path.empty()) return false;

    bool res;
    if (isDirectory(sd, path.c_str())) {
        res = removeRecursive(sd, path);
    } else {
        res = sd.remove(path.c_str());
    }
    DBG("SdFatFileSystem::remove result=%d", res);
    return res;
}

bool SdFatFileSystem::mkdir(const std::string& rawPath) {
    std::string path = normalizePath(rawPath);
    DBG("SdFatFileSystem::mkdir(path=%s)", path.c_str());
    if (path.empty() || path == "/") return true;
    bool res = sd.mkdir(path.c_str());
    DBG("SdFatFileSystem::mkdir result=%d", res);
    return res;
}

void SdFatFileSystem::getCreatedDateTime(const std::string& rawPath, uint16_t* date, uint16_t* time) {
    std::string path = normalizePath(rawPath);
    DBG("getCreatedDateTime(path=%s)", path.c_str());
    FsFile f = sd.open(path.c_str(), FILE_READ);
    if (!f) { *date = 0; *time = 0; return; }
    if (!f.getCreateDateTime(date, time)) { *date = 0; *time = 0; }
    f.close();
}

void SdFatFileSystem::getModifiedDateTime(const std::string& rawPath, uint16_t* date, uint16_t* time) {
    std::string path = normalizePath(rawPath);
    DBG("getModifiedDateTime(path=%s)", path.c_str());
    FsFile f = sd.open(path.c_str(), FILE_READ);
    if (!f) { *date = 0; *time = 0; return; }
    if (!f.getModifyDateTime(date, time)) { *date = 0; *time = 0; }
    f.close();
}

uint32_t SdFatFileSystem::getCreatedTimestamp(const std::string& path) {
    uint16_t fatDate = 0, fatTime = 0;
    getCreatedDateTime(path, &fatDate, &fatTime);
    uint32_t ts = storage::time::fatDateTimeToUnix(fatDate, fatTime);
    DBG("getCreatedTimestamp(%s) -> %u", path.c_str(), ts);
    return ts;
}

uint32_t SdFatFileSystem::getModifiedTimestamp(const std::string& path) {
    uint16_t fatDate = 0, fatTime = 0;
    getModifiedDateTime(path, &fatDate, &fatTime);
    uint32_t ts = storage::time::fatDateTimeToUnix(fatDate, fatTime);
    DBG("getModifiedTimestamp(%s) -> %u", path.c_str(), ts);
    return ts;
}

std::unique_ptr<IFile> SdFatFileSystem::open(const std::string& rawPath, OpenMode mode) {
    std::string path = normalizePath(rawPath);
    DBG("SdFatFileSystem::open(path=%s, mode=%d)", path.c_str(), static_cast<int>(mode));

    oflag_t flags = O_RDONLY;
    switch (mode) {
        case OpenMode::Read:         flags = O_RDONLY; break;
        case OpenMode::WriteTruncate:flags = O_WRONLY | O_CREAT | O_TRUNC; break;
        case OpenMode::WriteAppend:  flags = O_WRONLY | O_CREAT | O_AT_END; break;
        case OpenMode::ReadWrite:    flags = O_RDWR   | O_CREAT; break;
    }

    if (mode != OpenMode::Read) {
        if (!ensureParentDirs(sd, path)) {
            DBG("ensureParentDirs failed for %s", path.c_str());
            return nullptr;
        }
    }

    FsFile raw = sd.open(path.c_str(), flags);
    DBG("open(%s) result=%d", path.c_str(), raw ? 1 : 0);
    if (!raw) return nullptr;
    return std::make_unique<SdFatFileWrapper>(std::move(raw));
}

} // namespace sd
} // namespace storage

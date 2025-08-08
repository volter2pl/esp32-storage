# Dokumentacja modułu: `storage::IFileSystem` oraz implementacji `SdFatFileSystem` i `LittleFsFileSystem`

## Przeznaczenie

Moduł `storage` dostarcza zunifikowany interfejs do systemów plików opartych o SD, flash lub RAM, z możliwością obsługi dat (czas utworzenia i modyfikacji) oraz strumieniowego dostępu do danych. Dostępne implementacje to:

* `SdFatFileSystem` — obsługa kart SD przez bibliotekę SdFat,
* `LittleFsFileSystem` — obsługa wbudowanej pamięci flash przez LittleFS (bez znaczników czasu).

## Przykładowe użycie (SdFat)

```cpp
#include "storage/sd/SdFatFileSystem.h"
#include "storage/time/NtpTimeProvider.h"

storage::sd::SdFatFileSystem sdFs(46); // CS = 46
storage::time::NtpTimeProvider ntp;
sdFs.setTimeProvider(&ntp);

if (sdFs.begin()) {
    auto file = sdFs.openAppend("/log.txt");
    if (file) {
        file->write((const uint8_t*)"Hello!\n", 7);
        file->close();
    }
}
```

## Przykładowe użycie (LittleFS)

```cpp
#include "storage/littlefs/LittleFsFileSystem.h"

storage::littlefs::LittleFsFileSystem flashFs;

if (flashFs.begin()) {
    auto file = flashFs.openAppend("/log.txt");
    if (file) {
        file->write((const uint8_t*)"Hello!\n", 7);
        file->close();
    }
}
```

---

## Interfejs: `IFileSystem`

```cpp
enum class OpenMode {
    Read,
    WriteTruncate,
    WriteAppend,
    ReadWrite,
};

class IFileSystem {
public:
    virtual ~IFileSystem() = default;
    virtual bool begin() = 0;
    virtual bool listDir(const char* path, std::function<void(const char*, size_t)> callback) = 0;
    virtual bool exists(const std::string& path) = 0;
    virtual bool remove(const std::string& path) = 0;
    virtual bool mkdir(const std::string& path) = 0;
    virtual uint32_t getCreatedTimestamp(const std::string& path) = 0;
    virtual uint32_t getModifiedTimestamp(const std::string& path) = 0;
    virtual std::unique_ptr<IFile> open(const std::string& path, OpenMode mode) = 0;

    std::unique_ptr<IFile> openRead(const std::string& path);
    std::unique_ptr<IFile> openWrite(const std::string& path, bool overwrite = true);
    std::unique_ptr<IFile> openAppend(const std::string& path);
};
```

## Implementacja: `SdFatFileSystem`

* Obsługuje kartę SD przez bibliotekę SdFat.
* Umożliwia rejestrowanie `ITimeProvider` (np. synchronizacja z NTP).
* Obsługuje daty FAT (rok od 1980, czas co 2 sekundy).

### Konstruktor

```cpp
explicit SdFatFileSystem(uint8_t csPin);
```

### Rejestracja providera czasu

```cpp
void setTimeProvider(ITimeProvider* provider);
```

### Inicjalizacja

```cpp
bool begin();
```

### Praca z plikami

```cpp
std::unique_ptr<IFile> open(const std::string& path, OpenMode mode);
```

* `FILE_WRITE` = dopisanie (append)
* `FILE_READ` = odczyt
* `O_WRITE | O_CREAT | O_TRUNC` = nadpisanie (overwrite)

### Zarządzanie plikami i katalogami

```cpp
bool exists(const std::string& path);
bool remove(const std::string& path);
bool mkdir(const std::string& path);
```

### Listowanie katalogu

```cpp
bool listDir(const char* path, std::function<void(const char*, size_t)> callback);
```

### Daty utworzenia i modyfikacji

```cpp
uint32_t getCreatedTimestamp(const std::string& path);
uint32_t getModifiedTimestamp(const std::string& path);
```

Zwracają czas w formacie `UNIX timestamp` (sekundy od 1970-01-01).

---

## Uwagi

* Brak zegara RTC nie przeszkadza w użyciu dat jeśli dostarczony zostanie `ITimeProvider` (np. z NTP).
* Pliki można trzymać otwarte (uchwyt `IFile`), co umożliwia wydajny zapis strumieniowy.
* System może działać bez dat, ale wtedy metody timestamp zwracają `0`.
* `LittleFsFileSystem` nie obsługuje znaczników czasu i zawsze zwraca `0`.

## Planowane rozszerzenia

* Wsparcie dla dodatkowych metadanych
* Opcjonalna walidacja spójności systemu plików

---

## Autorzy / Licencja

Projekt autorski. Można adaptować na własny użytek.

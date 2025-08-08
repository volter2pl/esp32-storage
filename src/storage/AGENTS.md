# ROBOTS.md

## Informacje dla agentów AI rozwijających moduł `storage`

Ten dokument jest przeznaczony dla autonomicznych agentów (AI dev tools, Codex, GPT, Claude, itd.) odpowiedzialnych za rozwój, testowanie i utrzymanie systemu plików zgodnego z `storage::IFileSystem` oraz implementacji `SdFatFileSystem`.

---

## Główne założenia projektu

* **Modularność**: interfejs `IFileSystem` definiuje podstawowy kontrakt dostępu do systemu plików.
* **Wydajność**: wsparcie dla trzymania uchwytów do plików (`IFile`) przez dłuższy czas (np. streaming).
* **Datowanie**: opcjonalne wsparcie dla znaczników czasu (czas utworzenia / modyfikacji) przez `ITimeProvider`.
* **Zgodność z FAT**: implementacja `SdFatFileSystem` korzysta z `SdFat.h` i obsługuje format FAT timestamp.
* **Zgodność z Arduino / ESP32**: biblioteka działa w PlatformIO + Arduino framework.

---

## Kluczowe pliki

```
include/
  storage/
    IFileSystem.h             <- kontrakt
    IFile.h                   <- interfejs pliku
    ITimeProvider.h           <- opcjonalne źródło czasu
    time/TimeUtils.h          <- FAT <-> UNIX timestamp
    time/NtpTimeProvider.h    <- implementacja ITimeProvider (NTP)
    sd/SdFatFileSystem.h      <- implementacja IFileSystem dla SdFat
    sd/SdFatFileWrapper.h     <- implementacja IFile
    littlefs/LittleFsFileSystem.h <- implementacja IFileSystem dla LittleFS
    littlefs/LittleFsFileWrapper.h <- implementacja IFile
src/
  storage/
    sd/SdFatFileSystem.cpp
    sd/SdFatFileWrapper.cpp
    littlefs/LittleFsFileSystem.cpp
    littlefs/LittleFsFileWrapper.cpp
    time/NtpTimeProvider.cpp
    time/TimeUtils.cpp
```

---

## Implementacje

### `IFileSystem`

Abstrakcyjny interfejs dla SD, RAMFS, FlashFS itp. Obowiązkowe metody:

* `begin()`
* `open(path, mode)`
* `listDir(path, callback)`
* `exists(path)` / `remove(path)` / `mkdir(path)`
* `getCreatedTimestamp(path)` / `getModifiedTimestamp(path)`

Timestamps są opcjonalne w implementacji, ale muszą zwracać 0 jeśli nieobsługiwane.

### `SdFatFileSystem`

Implementacja `IFileSystem` bazująca na SdFat. Obsługuje daty przez `FsDateTime::setCallback()`.

* `setTimeProvider()` pozwala zarejestrować instancję `ITimeProvider`
* pliki otwierane przez `open()` są opakowane przez `SdFatFileWrapper`
* metody `getCreatedTimestamp()` i `getModifiedTimestamp()` korzystają z `FsFile::getCreateDateTime()` itd.

### `LittleFsFileSystem`

Implementacja `IFileSystem` bazująca na LittleFS dla wbudowanej pamięci flash. Nie obsługuje
znaczników czasu — metody `getCreatedTimestamp()` i `getModifiedTimestamp()` zawsze zwracają `0`.

### `ITimeProvider`

Zwraca aktualną datę/czas jako `uint16_t date, time` zgodnie z FAT:

* data: rok (od 1980), miesiąc, dzień
* czas: godzina, minuta, sekundy/2

### `NtpTimeProvider`

Implementuje `ITimeProvider`. Oczekuje, że `configTime()` oraz `getLocalTime()` będą działać w `loop()` lub `setup()`.

---

## Konwencje i zalecenia

* Możliwe jest równoczesne otwarcie wielu uchwytów do plików (np. do zapisu i odczytu), ale należy unikać ich nadmiaru. Pliki należy zawsze zamykać po zakończeniu pracy.
* Tryb `FILE_WRITE` ustawia wskaźnik na początek pliku (domyślnie nadpisuje). Aby dopisać dane, należy po otwarciu użyć `file->seek(file->size())`. Jeśli wymagane jest nadpisanie od zera, należy jawnie użyć flagi `O_TRUNC` (czyli `FILE_WRITE | O_TRUNC`).
* Metody interfejsu `IFileSystem` powinny być atomowe, niskopoziomowe i wolne od efektów ubocznych — np. `exists()` nie tworzy pliku, `remove()` tylko usuwa, `open()` nie loguje ani nie modyfikuje nic poza żądanym otwarciem.

---

## Przykładowy agent task

> "Dodaj `MemoryFileSystem` działający wyłącznie w RAM na potrzeby testów."

---

## TODO dla agentów

* [x] `LittleFsFileSystem` (implementacja IFileSystem)
* [ ] `MemoryFileSystem` (RAM-only dla testów)
* [ ] Wsparcie dla odczytu atrybutów pliku (np. readonly)
* [ ] Funkcja `rename(oldPath, newPath)`
* [ ] `IFileSystem::stat(path)` zwracająca metadane

---

## Debug / testowanie

* Kod testowy znajduje się w `main.cpp` i symuluje zapis/odczyt, NTP, listowanie.
* Wersja produkcyjna powinna używać logiki z warstwy wyższej (np. Timeshift).

---

## Meta

* Autor: Człowiek
* Agent prowadzenie: GPT-4o
* Licencja: dowolny użytek lokalny
* Termin: lipiec 2025

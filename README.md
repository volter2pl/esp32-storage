# esp32-storage

Abstrakcja systemu plików dla ESP32. Udostępnia interfejs `storage::IFileSystem` wraz z
implementacjami dla kart SD (`SdFatFileSystem`) oraz wbudowanej pamięci flash
(`LittleFsFileSystem`). Zawiera także narzędzia czasu (`TimeUtils`, `NtpTimeProvider`).

## Dokumentacja

 * [README modułu](./src/storage/README.md)
 * [ROBOTS](./src/storage/AGENTS.md)

## Funkcje

 * `SdFatFileSystem` – obsługa kart SD przez bibliotekę SdFat
 * `LittleFsFileSystem` – obsługa flash przez LittleFS
 * `NtpTimeProvider` i `TimeUtils` – konwersje znaczników czasu FAT

## Użycie jako zależność w PlatformIO

Możesz dołączyć ten moduł do innego projektu PlatformIO poprzez wpis w `lib_deps`:

```ini
lib_deps =
  git+https://github.com/volter2pl/esp32-storage.git
```

Po dodaniu zależności PlatformIO pobierze kod z katalogu `src/storage`. Ten projekt nadal można
otworzyć w VS Code i kompilować przykładowy firmware znajdujący się w `examples/esp32-test/`.

## Debugowanie

Aby włączyć szczegółowe logi wykonywanych operacji, zdefiniuj flagę
`ESP32_STORAGE_DEBUG`. W PlatformIO można to zrobić dodając do `platformio.ini`:

```ini
build_flags = -DESP32_STORAGE_DEBUG
```

Przykładowy komunikat debugowy:

```
[esp32-storage] SdFatFileSystem::begin result=1
```

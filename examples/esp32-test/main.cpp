#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <time.h>
#include <Adafruit_NeoPixel.h>

#include "storage/sd/SdFatFileSystem.h"
#include "storage/littlefs/LittleFsFileSystem.h"
#include "storage/time/NtpTimeProvider.h"
#include "storage/IFile.h"

#include "wifi_credentials.h"

// --------------------- Piny / sprzęt ---------------------
#define SD_CS    46
#define BUTTON   0
#define LED_PIN  38

Adafruit_NeoPixel pixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);

// --------------------- Obiekty globalne ---------------------
static storage::time::NtpTimeProvider ntp;
storage::sd::SdFatFileSystem sdFs(SD_CS);
storage::littlefs::LittleFsFileSystem lfs;

// Pliki testowe (różne nazwy, ale te same scenariusze)
static const char* SD_TEST_FILE  = "/test_sd.txt";
static const char* LFS_TEST_FILE = "/test_littlefs.txt";
static const char* TEST_CONTENT  = "To jest test zapisu i odczytu.\n";

// --------------------- LED helper ---------------------
static void setColor(uint8_t g, uint8_t r, uint8_t b) {
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}

// --------------------- Mini framework logowania ---------------------
#define LOG_RUN(fsName, msg)   Serial.printf("[RUN][%s] %s\n",  fsName, msg)
#define LOG_INFO(fsName, msg)  Serial.printf("[INFO][%s] %s\n", fsName, msg)
#define LOG_OK(fsName, msg)    Serial.printf("[OK][%s] %s\n",   fsName, msg)
#define LOG_FAIL(fsName, msg)  Serial.printf("[FAIL][%s] %s\n", fsName, msg)

#define ASSERT_TRUE(fsName, cond, why)                 \
  do {                                                 \
    if (!(cond)) {                                     \
      LOG_FAIL(fsName, why);                           \
      return false;                                    \
    }                                                  \
  } while (0)

// --------------------- Narzędzia ogólne (templatki) ---------------------

// begin()
template <typename FS>
bool tc_begin(const char* fsName, FS& fs) {
  LOG_RUN(fsName, "Inicjalizacja systemu plików (begin)");
  bool ok = fs.begin();
  if (ok) LOG_OK(fsName, "begin() -> OK");
  else    LOG_FAIL(fsName, "begin() -> BŁĄD inicjalizacji");
  return ok;
}

// pre-clean na podstawie listy — bez exists(), minimalizacja hałasu LFS
template <typename FS>
void tc_precise_preclean(const char* fsName, FS& fs) {
  LOG_RUN(fsName, "Pre-clean (listDir + selective remove)");
  fs.listDir("/", [&, fsName](const char* name, size_t) {
    // Usuń tylko nasze artefakty testowe
    if (!strcmp(name, "test_sd.txt") || !strcmp(name, "test_littlefs.txt")) {
      (void)fs.remove(std::string("/") + name); // ignoruj wynik
    }
    if (!strcmp(name, "a")) {
      (void)fs.remove("/a"); // recursive remove w implementacji FS; ignoruj wynik
    }
  });
  LOG_OK(fsName, "Pre-clean done");
}

// openWrite + write + close
template <typename FS>
bool tc_write(const char* fsName, FS& fs, const char* path, const char* content) {
  LOG_RUN(fsName, "Zapis pliku (openWrite/write)");
  auto f = fs.openWrite(path);
  ASSERT_TRUE(fsName, (bool)f, "Nie udało się otworzyć pliku do zapisu");

  size_t len = strlen(content);
  size_t written = f->write((const uint8_t*)content, len);
  f->close();

  Serial.printf("[DEBUG][%s] write: %u/%u B\n", fsName, (unsigned)written, (unsigned)len);
  ASSERT_TRUE(fsName, written == len, "Zapisano mniej danych niż oczekiwano");
  LOG_OK(fsName, "Zapis zakończony poprawnie");
  return true;
}

// openRead + read + walidacja
template <typename FS>
bool tc_read_and_validate(const char* fsName, FS& fs, const char* path, const char* expected) {
  LOG_RUN(fsName, "Odczyt i walidacja pliku (openRead/read)");
  auto f = fs.openRead(path);
  ASSERT_TRUE(fsName, (bool)f, "Nie udało się otworzyć pliku do odczytu");

  size_t size = f->size();
  Serial.printf("[DEBUG][%s] file size: %u B\n", fsName, (unsigned)size);

  char buf[256] = {0};
  if (size >= sizeof(buf)) size = sizeof(buf) - 1;
  size_t n = f->read(buf, size);
  f->close();
  buf[n] = '\0';

  Serial.printf("[DEBUG][%s] read: %u B\n", fsName, (unsigned)n);
  Serial.printf("[DEBUG][%s] content: %s", fsName, buf);

  ASSERT_TRUE(fsName, strcmp(buf, expected) == 0, "Dane różnią się od oczekiwanych");
  LOG_OK(fsName, "Walidacja zawartości OK");
  return true;
}

// exists + rozmiar
template <typename FS>
bool tc_exists_and_size(const char* fsName, FS& fs, const char* path) {
  LOG_RUN(fsName, "Sprawdzenie istnienia pliku (exists)");
  bool ex = fs.exists(path);
  if (!ex) {
    LOG_FAIL(fsName, "Plik nie istnieje");
    return false;
  }
  auto f = fs.openRead(path);
  ASSERT_TRUE(fsName, (bool)f, "Nie udało się otworzyć pliku do odczytu (size)");
  Serial.printf("[INFO][%s] Rozmiar: %u B\n", fsName, (unsigned)f->size());
  f->close();
  LOG_OK(fsName, "exists/size OK");
  return true;
}

// openAppend + write
template <typename FS>
bool tc_append_line(const char* fsName, FS& fs, const char* path, const char* line) {
  LOG_RUN(fsName, "Dopisanie linii (openAppend/write)");
  auto f = fs.openAppend(path);
  ASSERT_TRUE(fsName, (bool)f, "Nie udało się otworzyć pliku do dopisania");
  size_t len = strlen(line);
  size_t w = f->write((const uint8_t*)line, len);
  f->close();
  Serial.printf("[DEBUG][%s] append: %u/%u B\n", fsName, (unsigned)w, (unsigned)len);
  ASSERT_TRUE(fsName, w == len, "Dopisano mniej danych niż oczekiwano");
  LOG_OK(fsName, "Dopisanie OK");
  return true;
}

// listDir("/") — bez znaczników czasu, spójnie dla obu FS
template <typename FS>
void tc_list_root(const char* fsName, FS& fs) {
  LOG_RUN(fsName, "Lista katalogu / (listDir)");
  fs.listDir("/", [fsName](const char* name, size_t size) {
    Serial.printf("[LIST][%s] %s (%u B)\n", fsName, name, (unsigned)size);
  });
  LOG_OK(fsName, "listDir zakończone");
}

// --- Timestamp checks ---
// fallback dla FS bez timestampów
template <typename FS>
bool tc_check_timestamps(const char* fsName, FS&, const char*) {
  LOG_INFO(fsName, "[SKIP] Timestamps not supported");
  return true;
}

// wersja dla SdFatFileSystem: sprawdzamy created/modified w oknie czasu
bool tc_check_timestamps(const char* fsName, storage::sd::SdFatFileSystem& fs, const char* path) {
  LOG_RUN(fsName, "Sprawdzenie znaczników czasu (created/modified)");
  time_t now = time(nullptr);
  uint32_t created  = fs.getCreatedTimestamp(path);
  uint32_t modified = fs.getModifiedTimestamp(path);
  Serial.printf("[DEBUG][%s] ts created=%u modified=%u now=%u\n",
                fsName, created, modified, (unsigned)now);

  if (created == 0 || modified == 0) {
    LOG_FAIL(fsName, "Brak znaczników czasu (0)");
    return false;
  }
  const uint32_t window = 300; // +/- 5 min
  if (!((created <= (uint32_t)now + window) && (created + window >= (uint32_t)now))) {
    LOG_FAIL(fsName, "Created poza oknem czasu");
    return false;
  }
  if (modified < created) {
    LOG_FAIL(fsName, "Modified < Created");
    return false;
  }
  LOG_OK(fsName, "Timestamps OK");
  return true;
}

// Tworzenie zagnieżdżonych katalogów przez zapis do ścieżki — bez pre-clean
template <typename FS>
bool tc_nested_write_and_cleanup(const char* fsName, FS& fs) {
  const char* nested = "/a/b/c/nested.txt";
  LOG_RUN(fsName, "Test zagnieżdżonych katalogów (openWrite)");

  // BEZ wcześniejszego remove — implementacje tworzą rodziców w openWrite
  auto f = fs.openWrite(nested);
  ASSERT_TRUE(fsName, (bool)f, "Nie udało się utworzyć pliku w zagnieżdżonych katalogach");
  const char* msg = "Nested directories test\n";
  (void)f->write((const uint8_t*)msg, strlen(msg));
  f->close();
  LOG_OK(fsName, "Utworzono plik w /a/b/c");

  // Sprzątanie bottom-up
  (void)fs.remove(nested);
  (void)fs.remove("/a/b/c");
  (void)fs.remove("/a/b");
  (void)fs.remove("/a");
  LOG_OK(fsName, "Usunięto plik i katalogi /a/b/c");
  return true;
}

// Usunięcie pliku
template <typename FS>
bool tc_remove_file(const char* fsName, FS& fs, const char* path) {
  LOG_RUN(fsName, "Usuwanie pliku (remove)");
  bool ok = fs.remove(path);
  if (ok) LOG_OK(fsName, "Plik usunięty");
  else    LOG_FAIL(fsName, "Nie udało się usunąć pliku");
  return ok;
}

// --------------------- Suite jednego FS ---------------------

template <typename FS>
bool run_suite_for_fs(const char* fsName, FS& fs, const char* testFile, const char* testContent) {
  uint8_t total = 0, passed = 0;

  LOG_INFO(fsName, "--- START TEST SUITE ---");

  total++; if (tc_begin(fsName, fs)) passed++; else goto SUMMARY;

  // Pre-clean oparte o listę — minimalizuje hałas LFS
  tc_precise_preclean(fsName, fs);

  total++; if (tc_write(fsName, fs, testFile, testContent)) passed++; else goto SUMMARY;
  total++; if (tc_check_timestamps(fsName, fs, testFile)) passed++; else goto SUMMARY;
  total++; if (tc_read_and_validate(fsName, fs, testFile, testContent)) passed++; else goto SUMMARY;
  total++; if (tc_exists_and_size(fsName, fs, testFile)) passed++; else goto SUMMARY;
  total++; if (tc_append_line(fsName, fs, testFile, "Dopisana linia.\n")) passed++; else goto SUMMARY;
  total++; if ([&]{
    LOG_RUN(fsName, "Weryfikacja timestamp po append");
    if (strcmp(fsName, "SD") != 0) { LOG_INFO(fsName, "[SKIP] Timestamps not supported"); return true; }
    uint32_t c1 = sdFs.getCreatedTimestamp(testFile);
    uint32_t m1 = sdFs.getModifiedTimestamp(testFile);
    delay(2100); // >= 2 s, żeby wyjść poza rozdzielczość FAT
    auto f = fs.openAppend(testFile); if (!f) { LOG_FAIL(fsName,"append reopen fail"); return false; }
    f->write((const uint8_t*)"x",1); f->close();
    uint32_t c2 = sdFs.getCreatedTimestamp(testFile);
    uint32_t m2 = sdFs.getModifiedTimestamp(testFile);
    ASSERT_TRUE(fsName, c2 == c1, "Created zmieniony po append");
    ASSERT_TRUE(fsName, m2 >= m1 + 2, "Modified nie wzrósł (FAT=2s)");
    LOG_OK(fsName, "Append zmienił modified, created bez zmian");
    return true;
  }()) passed++; else goto SUMMARY;
  total++; if (tc_list_root(fsName, fs), true) passed++; else goto SUMMARY; // listDir zawsze "true"
  total++; if (tc_nested_write_and_cleanup(fsName, fs)) passed++; else goto SUMMARY;
  total++; if (tc_remove_file(fsName, fs, testFile)) passed++; else goto SUMMARY;

SUMMARY:
  Serial.printf("[SUMMARY][%s] PASSED %u/%u\n", fsName, (unsigned)passed, (unsigned)total);
  LOG_INFO(fsName, "--- END TEST SUITE ---");
  return passed == total;
}

// --------------------- SETUP / LOOP ---------------------

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON, INPUT_PULLUP);
  pixel.begin();
  pixel.setBrightness(50);
  setColor(0, 0, 0);

  // Próba połączenia z ostatnią siecią (jeśli jest)
  Serial.print(F("Próba ponownego połączenia do sieci WiFi"));
  WiFi.begin();
  unsigned char timeout = 50; // ~5 s
  while (WiFi.status() != WL_CONNECTED && timeout--) { delay(100); }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf(" (ssid: %s): ok\n", WiFi.SSID().c_str());
  } else {
    Serial.println(F("nieudane"));
  }

  // Łączenie z określoną siecią
  WiFi.begin(ssid, password);
  Serial.printf("Łączenie z WiFi (%s, %s)...\n", ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println(" połączono.");

  // NTP
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Synchronizacja czasu...");
  time_t now = time(nullptr);
  while (now < 1600000000) { delay(500); Serial.print("."); now = time(nullptr); }
  Serial.println(" OK.");

  // Provider czasu dla SD (jeśli obsługuje znaczniki czasu)
  sdFs.setTimeProvider(&ntp);

  Serial.println("Czekam na wciśnięcie przycisku...");
}

void loop() {
  static bool prev = HIGH;
  bool now = digitalRead(BUTTON);

  if (prev == HIGH && now == LOW) {
    delay(50); // debounce
    Serial.println();
    Serial.println("================ RUN ALL TESTS ================");

    bool okSD  = run_suite_for_fs("SD",  sdFs,  SD_TEST_FILE,  TEST_CONTENT);
    bool okLFS = run_suite_for_fs("LittleFS", lfs, LFS_TEST_FILE, TEST_CONTENT);

    bool allOk = okSD && okLFS;
    Serial.printf("[RESULT] Całkowity wynik: %s\n", allOk ? "SUKCES" : "BŁĄD");

    // Kolor LED: zielony OK, czerwony FAIL
    setColor(allOk ? 0 : 255, allOk ? 255 : 0, 0);

    Serial.println("===============================================");
  }

  prev = now;
}

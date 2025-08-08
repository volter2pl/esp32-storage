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

#define SD_CS    46
#define BUTTON   0
#define LED_PIN  38

Adafruit_NeoPixel pixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);

static storage::time::NtpTimeProvider ntp;
storage::sd::SdFatFileSystem sdFs(SD_CS);
storage::littlefs::LittleFsFileSystem lfs;

const char* testFilename = "/test.txt";
const char* testContent = "To jest test zapisu i odczytu z karty SD.\n";
const char* lfsTestFilename = "/test_littlefs.txt";
const char* lfsTestContent = "To jest test zapisu i odczytu z LittleFS.\n";

// --------------------- LED ---------------------
void setColor(uint8_t g, uint8_t r, uint8_t b) {
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}

// --------------------- TEST SD ---------------------
bool initializeCard() {
  if (!sdFs.begin()) {
    Serial.println("Błąd inicjalizacji karty SD!");
    return false;
  }
  Serial.println("Karta SD wykryta (przez SdFatFileSystem).");
  return true;
}

bool writeTestFile() {
  sdFs.remove(testFilename);
  Serial.println("[DEBUG] Plik usunięty przed zapisem.");

  auto fileWrite = sdFs.openWrite(testFilename);
  if (!fileWrite) {
    Serial.println("Błąd otwarcia pliku do zapisu.");
    return false;
  }

  size_t len = strlen(testContent);
  size_t written = fileWrite->write((const uint8_t*)testContent, len);
  fileWrite->close();
  Serial.printf("[DEBUG] Zapisano %d bajtów.\n", (int)written);

  return written == len;
}

bool readAndValidateTestFile() {
  auto fileRead = sdFs.openRead(testFilename);
  if (!fileRead) {
    Serial.println("Błąd otwarcia pliku do odczytu.");
    return false;
  }

  size_t size = fileRead->size();
  Serial.printf("[DEBUG] Rozmiar pliku: %d bajtów.\n", (int)size);

  char buffer[128] = {0};
  if (size >= sizeof(buffer)) size = sizeof(buffer) - 1;

  size_t readBytes = fileRead->read(buffer, size);
  fileRead->close();
  buffer[readBytes] = '\0';

  Serial.println("Zawartość odczytana:");
  Serial.println(buffer);
  Serial.printf("[DEBUG] Przeczytano %d bajtów.\n", (int)readBytes);

  if (strcmp(buffer, testContent) != 0) {
    Serial.println("Dane nie zgadzają się!");
    return false;
  }

  Serial.println("Test zakończony sukcesem.");
  return true;
}

bool testSDCard() {
  return initializeCard() &&
         writeTestFile() &&
         readAndValidateTestFile();
}

void listLittleFsRootDirectory() {
  Serial.println("[INFO] Zawartość katalogu Littlefs /:");

  lfs.listDir("/", [](const char* name, size_t size) {
    uint32_t ts = sdFs.getCreatedTimestamp(name);

    if (ts == 0) {
      Serial.printf(" - %s (%u B)\n", name, (unsigned int)size);
      return;
    }

    time_t t = ts;
    struct tm* tm = localtime(&t);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);

    Serial.printf(" - %s (%u B) utworzony: %s\n", name, (unsigned int)size, buf);
  });
}

bool testLittleFs() {
  if (!lfs.begin()) {
    Serial.println("[LittleFS] Błąd inicjalizacji!");
    return false;
  }

  lfs.remove(lfsTestFilename);
  Serial.printf("[LittleFS] Otwieram plik: %s\n", lfsTestFilename);
  auto fileWrite = lfs.openWrite(lfsTestFilename);
  if (!fileWrite) {
    Serial.println("[LittleFS] Błąd otwarcia pliku do zapisu.");
    return false;
  }
  size_t len = strlen(lfsTestContent);
  size_t written = fileWrite->write((const uint8_t*)lfsTestContent, len);
  fileWrite->close();
  if (written != len) {
    Serial.println("[LittleFS] Nie udało się zapisać do LittleFS.");
    return false;
  } else {
    Serial.printf("[LittleFS] Zapisano do LittleFS %d z %d bajtów\n", written, len);
  }

  auto fileRead = lfs.openRead(lfsTestFilename);
  if (!fileRead) {
    Serial.println("[LittleFS] Błąd otwarcia pliku do odczytu.");
    return false;
  }
  char buffer[128] = {0};
  size_t readBytes = fileRead->read(buffer, sizeof(buffer) - 1);
  fileRead->close();
  buffer[readBytes] = '\0';
  if (strcmp(buffer, lfsTestContent) != 0) {
    Serial.println("[LittleFS] Dane się nie zgadzają!");
    Serial.printf("[LittleFS] EXPECTED: %s\n", lfsTestContent);
    Serial.printf("[LittleFS] GOT:      %s\n", buffer);
    return false;
  }

  listLittleFsRootDirectory();

  Serial.println("[LittleFS] Test zakończony sukcesem.");
  return true;
}

// --------------------- FUNKCJE SYSTEMU PLIKÓW ---------------------
bool reportIfExists(const std::string& path) {
  if (!sdFs.exists(path)) {
    Serial.println("[INFO] Plik nie istnieje.");
    return false;
  }

  Serial.println("[INFO] Plik istnieje.");
  auto f = sdFs.openRead(path);
  if (f) {
    Serial.printf("[INFO] Rozmiar: %d B\n", (int)f->size());
    f->close();
  }
  return true;
}

bool appendLineToFile(const std::string& path, const char* line) {
  auto f = sdFs.openAppend(path);
  if (!f) {
    Serial.println("[WARN] Nie udało się otworzyć pliku do dopisania.");
    return false;
  }

  f->write((const uint8_t*)line, strlen(line));
  f->close();
  Serial.println("[INFO] Dopisano dane.");
  return true;
}

bool printFileContents(const std::string& path) {
  auto f = sdFs.openRead(path);
  if (!f) {
    Serial.println("[WARN] Nie udało się otworzyć pliku do odczytu.");
    return false;
  }

  size_t size = f->size();
  char buffer[256] = {0};
  if (size >= sizeof(buffer)) size = sizeof(buffer) - 1;

  size_t readBytes = f->read(buffer, size);
  f->close();
  buffer[readBytes] = '\0';

  Serial.println("[INFO] Zawartość pliku:");
  Serial.println(buffer);
  return true;
}

void listRootDirectory() {
  Serial.println("[INFO] Zawartość katalogu /:");

  sdFs.listDir("/", [](const char* name, size_t size) {
    uint32_t ts = sdFs.getCreatedTimestamp(name);

    if (ts == 0) {
      Serial.printf(" - %s (%u B)\n", name, (unsigned int)size);
      return;
    }

    time_t t = ts;
    struct tm* tm = localtime(&t);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);

    Serial.printf(" - %s (%u B) utworzony: %s\n", name, (unsigned int)size, buf);
  });
}

void removeFile(const std::string& path) {
  if (sdFs.remove(path)) {
    Serial.println("[INFO] Plik został usunięty.");
  } else {
    Serial.println("[WARN] Nie udało się usunąć pliku.");
  }
}

void createAndRemoveNestedFile() {
  const char* nestedPath = "/a/b/c/nested.txt";

  sdFs.remove(nestedPath);
  sdFs.remove("/a/b/c");
  sdFs.remove("/a/b");
  sdFs.remove("/a");

  auto f = sdFs.openWrite(nestedPath);
  if (!f) {
    Serial.println("[WARN] Nie udało się utworzyć pliku w zagnieżdżonych katalogach.");
    return;
  }

  const char* msg = "Nested directories test\n";
  f->write((const uint8_t*)msg, strlen(msg));
  f->close();
  Serial.println("[INFO] Utworzono plik w zagnieżdżonych katalogach.");

  sdFs.remove(nestedPath);
  sdFs.remove("/a/b/c");
  sdFs.remove("/a/b");
  sdFs.remove("/a");
  Serial.println("[INFO] Plik i katalogi zostały usunięte.");
}

void testFilesystemFeatures() {
  const std::string path = "/test.txt";
  if (!reportIfExists(path)) return;
  if (!appendLineToFile(path, "Dopisana linia.\n")) return;
  if (!printFileContents(path)) return;
  listRootDirectory();
  removeFile(path);
  createAndRemoveNestedFile();
}

// --------------------- SETUP / LOOP ---------------------
void setup() {
  Serial.begin(115200);
  pinMode(BUTTON, INPUT_PULLUP);
  pixel.begin();
  pixel.setBrightness(50);
  setColor(0, 0, 0);

  Serial.print(F("Próba ponownego połączenia do sieci WiFi"));
  WiFi.begin();
  unsigned char timeout = 50; // 5 sekund

  do {
    delay(100);
  } while (WiFi.status() != WL_CONNECTED && timeout--);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf(" (ssid: %s): ok\n", WiFi.SSID().c_str());
  } else {
    Serial.println(F("nieudane"));
  }

  // Połączenie WiFi
  WiFi.begin(ssid, password);
  Serial.printf("Łączenie z WiFi (%s, %s)...\n", ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" połączono.");

  // Konfiguracja czasu przez NTP
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Synchronizacja czasu...");
  time_t now = time(nullptr);
  while (now < 1600000000) { // jakiś sensowny próg (2020+)
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println(" OK.");

  // Podpięcie providera czasu
  sdFs.setTimeProvider(&ntp);

  Serial.println("Czekam na wciśnięcie przycisku...");
}

void loop() {
  static bool buttonPrev = HIGH;
  bool buttonNow = digitalRead(BUTTON);

  if (buttonPrev == HIGH && buttonNow == LOW) {
    delay(50); // debounce
    Serial.println("Rozpoczynam test karty SD...");

    bool sdResult = testSDCard();

    Serial.println("Rozpoczynam test LittleFS...");
    bool lfsResult = testLittleFs();

    if (sdResult) testFilesystemFeatures();

    bool result = sdResult && lfsResult;

    setColor(result ? 0 : 255, result ? 255 : 0, 0); // zielony lub czerwony
  }

  buttonPrev = buttonNow;
}

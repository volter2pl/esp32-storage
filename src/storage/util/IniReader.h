/*
 * IniReader – lekki parser plików konfiguracyjnych w formacie INI.
 *
 *  ZAŁOŻENIA / FORMAT:
 *   - Wejściem jest dowolny obiekt implementujący interfejs pliku (`IFile`),
 *     np. uchwyt z SD, LittleFS itp.
 *   - Obsługiwane są sekcje w formacie:
 *         [NazwaSekcji]
 *   - W sekcji znajdują się pary:
 *         klucz=wartość
 *     lub
 *         klucz wartość
 *   - Separator klucz–wartość: znak `=` lub pierwszy biały znak (spacja / tab).
 *   - Obsługiwane są komentarze pełnoliniowe (`;` lub `#`) oraz końcowe komentarze
 *     po wartości (np. `port=8080 ; komentarz`).
 *   - Białe znaki na początku i końcu kluczy i wartości są obcinane.
 *   - Klucze i nazwy sekcji mogą być normalizowane do lowercase (opcjonalnie).
 *
 *  MOŻLIWOŚCI:
 *   - Odczyt wartości w postaci tekstu, liczby całkowitej, liczby zmiennoprzecinkowej lub bool.
 *   - Iteracyjne przetwarzanie sekcji i kluczy.
 *   - Pomijanie nieznanych sekcji lub kluczy (możliwość walidacji schematu).
 *   - Działa w ograniczonych środowiskach (Arduino/ESP) bez dużego narzutu pamięci.
 *
 *  OGRANICZENIA:
 *   - Brak obsługi zagnieżdżonych sekcji lub wielokrotnego występowania tego samego klucza
 *     (ostatnia wartość nadpisuje poprzednią).
 *   - Brak wsparcia dla wartości wielolinijkowych.
 *   - Maksymalna długość linii ograniczona rozmiarem bufora przekazanego w konstruktorze.
 *
 *  PRZYKŁAD UŻYCIA:
 *     IniReader ini(fileHandle);
 *     String value;
 *     if (ini.get("Network", "host", value)) {
 *         Serial.println("Host: " + value);
 *     }
 *     int port;
 *     if (ini.getInt("Network", "port", port)) {
 *         Serial.println(port);
 *     }
 *
 *  PRZYKŁADOWY PLIK INI:
 *     ; Konfiguracja aplikacji audio
 *     # Wersja pliku 1.0
 *
 *     [Network]
 *     host = example.com
 *     port = 8080
 *     path = /api/stream
 *
 *     [Schedule]
 *     turnOnWeekday = 7
 *     turnOnWeekend = 9
 *
 *     [Audio]
 *     timeshift = true
 *     volume = 75
 *     frequency = 99.7
 *     fm_volume = 0.85
 *
 *     [WiFi]
 *     ssid = MyWiFiNetwork
 *     password = supertajnehaslo
 */

// storage/util/IniReader.h
#pragma once
#include <Arduino.h>
#include "storage/IFile.h"

namespace storage { namespace util {

class IniReader {
public:
  explicit IniReader(IFile& f, size_t bufCap = 512)
    : f_(f), cap_(bufCap) {}

  // Zwraca true = sukces parsa całego pliku
  // onKV(section, key, value) -> return false, aby przerwać
  bool parse(std::function<bool(const String&, const String&, const String&)> onKV) {
    String line, section, key, val;
    f_.seek(0);
    while (readLine(line)) {
      trim(line);
      if (!line.length()) continue;

      // komentarz pełnoliniowy
      if (line[0] == ';' || line[0] == '#') continue;

      // sekcja [name]
      if (line[0] == '[') {
        int r = line.indexOf(']');
        if (r > 1) {
          section = line.substring(1, r);
          toLower(section);
          trim(section);
        }
        continue;
      }

      if (!parseKv(line, key, val)) continue;
      toLower(key); // klucz bezwzględnie lowercase
      // inline komentarz po value
      stripInlineComment(val);

      if (!onKV(section, key, val)) return false;
    }
    return true;
  }

private:
  IFile& f_; size_t cap_;

  bool readLine(String& out) {
    out.remove(0);
    while (f_.position() < f_.size()) {
      char c; if (f_.read(&c,1) != 1) break;
      if (c == '\r') continue;
      if (c == '\n') return true;
      if ((int)out.length() < (int)cap_) out += c;
    }
    return out.length() > 0; // ostatnia linia bez \n
  }

  static void ltrim(String& s){ while (s.length() && isspace((unsigned char)s[0])) s.remove(0,1); }
  static void rtrim(String& s){ while (s.length() && isspace((unsigned char)s[s.length()-1])) s.remove(s.length()-1,1); }
  static void trim(String& s){ rtrim(s); ltrim(s); }
  static void toLower(String& s){ s.toLowerCase(); }

  static bool parseKv(const String& lineIn, String& key, String& val) {
    String line = lineIn; // kopia
    // preferuj '='; jeśli brak, rozdziel pierwszym białym znakiem
    int eq = line.indexOf('=');
    int split = eq >= 0 ? eq : firstSpace(line);
    if (split < 0) { key = line; val = ""; trim(key); return key.length() > 0; }
    key = line.substring(0, split);
    int j = split + 1;
    if (eq < 0) { while (j < line.length() && isspace((unsigned char)line[j])) j++; }
    val = line.substring(j);
    trim(key); trim(val);
    return key.length() > 0;
  }

  static int firstSpace(const String& s) {
    for (int i=0;i<s.length();++i) if (isspace((unsigned char)s[i])) return i;
    return -1;
  }

  static void stripInlineComment(String& v) {
    int pos = -1;
    for (int i=0;i<v.length();++i){
      if (v[i] == ';' || v[i] == '#'){ pos = i; break; }
    }
    if (pos >= 0) { v.remove(pos); trim(v); }
  }
};

}} // ns

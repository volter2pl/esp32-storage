/*
 * LineParser – prosty parser plików tekstowych do odczytu linia-po-linii.
 *
 *  ZAŁOŻENIA / FORMAT:
 *   - Wejściem jest dowolny obiekt implementujący interfejs pliku (`IFile`),
 *     np. uchwyt z SD, LittleFS itp.
 *   - Odczyt odbywa się linia po linii (separator LF lub CRLF).
 *   - Każda linia jest zwracana jako String (bez znaków końca linii).
 *   - Obsługiwane są komentarze pełnoliniowe zaczynające się od `;` lub `#`
 *     – są automatycznie pomijane.
 *   - Linie puste oraz zawierające wyłącznie białe znaki są pomijane.
 *   - Białe znaki na początku i końcu linii są obcinane.
 *
 *  MOŻLIWOŚCI:
 *   - Prosty interfejs do iteracyjnego przetwarzania pliku tekstowego,
 *     np. do implementacji parserów konfiguracji.
 *   - Działa w ograniczonych środowiskach (Arduino/ESP) bez dużego narzutu pamięci.
 *
 *  OGRANICZENIA:
 *   - Brak buforowania całego pliku – odczyt następuje sekwencyjnie od początku.
 *   - Brak wsparcia dla cudzysłowów, escape’ów czy kontynuacji linii.
 *   - Maksymalna długość linii ograniczona rozmiarem bufora przekazanego w konstruktorze.
 *
 *  PRZYKŁAD UŻYCIA:
 *     LineParser parser(fileHandle);
 *     String line;
 *     while (parser.readNextLine(line)) {
 *       // przetwarzanie linii
 *     }
 *
 *  PRZYKŁADOWY PLIK KONFIGURACYJNY:
 *     ; Konfiguracja aplikacji audio
 *     # Wersja pliku konfiguracyjnego 1.0
 *
 *     host example.com
 *     port 8080
 *     path /api/stream
 *
 *     ; Parametry czasowe
 *     turnOnWeekday 7
 *     turnOnWeekend 9
 *
 *     # Parametry logiczne (bool)
 *     timeshift true
 *     backlight 0
 *     squelch yes
 *     squelch_threshold 45
 *
 *     ; Wartości zmiennoprzecinkowe
 *     frequency 99.7
 *     fm_volume 0.85
 *
 *     ; Dane logowania
 *     ssid MyWiFiNetwork
 *     password supertajnehaslo
 *
 *     ; Komentarz w środku
 *     volume 75
 *
 *     # Puste linie i komentarze są ignorowane
 */

#pragma once
#include <Arduino.h>
#include "storage/IFile.h"

namespace storage { namespace util {

// Czyta linie tekstu z IFile bez zmiany interfejsu IFile.
class LineReader {
public:
  explicit LineReader(IFile& f, size_t bufCap = 256)
  : file_(f), bufCap_(bufCap) {}

  // Zwraca true gdy zwrócono linię. false = EOF.
  bool readLine(String& out, bool keepNewline = false) {
    out.remove(0);
    while (file_.position() < file_.size()) {
      char c;
      if (file_.read(&c, 1) != 1) break;
      if (c == '\r') { // CRLF -> zignoruj CR, sprawdź następny
        size_t pos = file_.position();
        char n;
        if (pos < file_.size() && file_.read(&n,1) == 1) {
          if (n != '\n') file_.seek(pos); // nie było LF, cofnij
        }
        if (!keepNewline) return true;
        out += '\n'; return true;
      }
      if (c == '\n') {
        if (!keepNewline) return true;
        out += '\n'; return true;
      }
      if (out.length() < (int)bufCap_) out += c; // twardy limit
    }
    // EOF: jeśli coś zebraliśmy, zwróć ostatnią linię
    return out.length() > 0;
  }

private:
  IFile& file_;
  size_t bufCap_;
};

// Trim helpers
inline void ltrim(String& s) { while (s.length() && isspace((unsigned char)s[0])) s.remove(0,1); }
inline void rtrim(String& s) { while (s.length() && isspace((unsigned char)s[s.length()-1])) s.remove(s.length()-1,1); }
inline void trim(String& s) { rtrim(s); ltrim(s); }

// Parsuje „name sep value”; sep domyślnie spacja/tab lub '='.
// Zwraca false dla pustych/komentarzy.
inline bool parseKv(String line, String& name, String& value,
                    const char* commentPrefixes = ";#",
                    const char* seps = " \t=",
                    bool lowercaseKey = true) {
  trim(line);
  if (!line.length()) return false;
  // komentarz (pełnoliniowy)
  for (const char* p = commentPrefixes; *p; ++p)
    if (line.startsWith(String(*p))) return false;

  // znajdź pierwszy separator
  int idx = -1;
  for (const char* s = seps; *s && idx < 0; ++s) {
    int k = line.indexOf(*s);
    if (k >= 0) idx = (idx < 0) ? k : min(idx, k);
  }
  if (idx < 0) { // tylko klucz bez wartości
    name = line; value = "";
  } else {
    name = line.substring(0, idx);
    // pominij wszystkie kolejne separatory
    int j = idx;
    while (j < (int)line.length() && strchr(seps, line[j])) j++;
    value = line.substring(j);
  }

  trim(name); trim(value);
  if (lowercaseKey) name.toLowerCase();
  // komentarz na końcu wartości (np. value ; comment)
  for (const char* p = commentPrefixes; *p; ++p) {
    int cpos = value.indexOf(*p);
    if (cpos >= 0) { value.remove(cpos); trim(value); break; }
  }
  return name.length() > 0;
}

}} // ns

#include "TimeUtils.h"
#include <time.h>

namespace storage {
namespace time {

uint32_t fatDateTimeToUnix(uint16_t fatDate, uint16_t fatTime) {
    if (fatDate == 0 && fatTime == 0) return 0;

    struct tm t = {};
    t.tm_year = ((fatDate >> 9) & 0x7F) + 80;  // rok od 1980 → +80 dla struct tm
    t.tm_mon  = ((fatDate >> 5) & 0x0F) - 1;   // miesiąc 1–12 → 0–11
    t.tm_mday = (fatDate & 0x1F);

    t.tm_hour = (fatTime >> 11) & 0x1F;
    t.tm_min  = (fatTime >> 5)  & 0x3F;
    t.tm_sec  = (fatTime & 0x1F) * 2;

    time_t ts = mktime(&t);
    return ts > 0 ? ts : 0;
}

void unixToFatDateTime(uint32_t timestamp, uint16_t* fatDate, uint16_t* fatTime) {
    if (!fatDate || !fatTime) return;

    time_t ts = timestamp;
    struct tm* t = localtime(&ts);
    if (!t) {
        *fatDate = 0;
        *fatTime = 0;
        return;
    }

    *fatDate = ((t->tm_year - 80) << 9) |
               ((t->tm_mon + 1) << 5) |
               (t->tm_mday);

    *fatTime = (t->tm_hour << 11) |
               (t->tm_min << 5) |
               (t->tm_sec / 2);
}

} // namespace time
} // namespace storage

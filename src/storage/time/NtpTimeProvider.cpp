#include "storage/time/NtpTimeProvider.h"
#include "storage/Debug.h"

namespace storage {
namespace time {

void NtpTimeProvider::getFatTime(uint16_t* date, uint16_t* time) {
    DBG("NtpTimeProvider::getFatTime()");
    time_t now = ::time(nullptr);
    struct tm* tmStruct = localtime(&now);

    if (!tmStruct) {
        DBG("getFatTime: localtime failed");
        *date = 0;
        *time = 0;
        return;
    }

    // FAT date: bits: YYYYYYYMMMMDDDDD (7 bits year from 1980, 4 bits month, 5 bits day)
    *date = ((tmStruct->tm_year - 80) << 9) |
            ((tmStruct->tm_mon + 1) << 5) |
            (tmStruct->tm_mday);

    // FAT time: bits: HHHHHMMMMMMSSSSS (5 bits hour, 6 bits min, 5 bits sec/2)
    *time = (tmStruct->tm_hour << 11) |
            (tmStruct->tm_min << 5) |
            (tmStruct->tm_sec / 2);
    DBG("getFatTime -> date=%u time=%u", *date, *time);
}

} // namespace time
} // namespace storage

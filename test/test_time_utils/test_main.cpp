#include <cstdlib>
#include <ctime>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "../../src/storage/time/TimeUtils.h"
#include "../../src/storage/time/TimeUtils.cpp"

using storage::time::fatDateTimeToUnix;
using storage::time::unixToFatDateTime;

TEST_CASE("FAT date/time converts to Unix timestamp and back") {
    setenv("TZ", "UTC", 1);
    tzset();

    // FAT representation of 2023-03-17 12:34:56
    const uint16_t fatDate = (static_cast<uint16_t>(2023 - 1980) << 9) |
                             (static_cast<uint16_t>(3) << 5) |
                             static_cast<uint16_t>(17);
    const uint16_t fatTime = (static_cast<uint16_t>(12) << 11) |
                             (static_cast<uint16_t>(34) << 5) |
                             static_cast<uint16_t>(56 / 2);

    std::tm tm{};
    tm.tm_year = 2023 - 1900;
    tm.tm_mon = 3 - 1;
    tm.tm_mday = 17;
    tm.tm_hour = 12;
    tm.tm_min = 34;
    tm.tm_sec = 56;
    time_t expected = std::mktime(&tm);

    CHECK(fatDateTimeToUnix(fatDate, fatTime) == expected);

    uint16_t outDate = 0, outTime = 0;
    unixToFatDateTime(static_cast<uint32_t>(expected), &outDate, &outTime);

    CHECK(outDate == fatDate);
    CHECK(outTime == fatTime);
}

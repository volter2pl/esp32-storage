#ifndef STORAGE_TIME_NTPTIMEPROVIDER_H
#define STORAGE_TIME_NTPTIMEPROVIDER_H

#include "storage/ITimeProvider.h"
#include <time.h>

namespace storage {
namespace time {

/**
 * @brief Implementacja ITimeProvider oparta o funkcje systemowe time.h
 *        (po synchronizacji z NTP przez configTime).
 */
class NtpTimeProvider : public ITimeProvider {
public:
    /**
     * @brief Zwraca aktualny czas systemowy jako FAT date/time.
     *
     * Wymaga wcześniejszego wywołania `configTime()` i udanej synchronizacji.
     */
    void getFatTime(uint16_t* date, uint16_t* time) override;
};

} // namespace time
} // namespace storage

#endif // STORAGE_TIME_NTPTIMEPROVIDER_H

#ifndef STORAGE_TIME_TIMEUTILS_H
#define STORAGE_TIME_TIMEUTILS_H

#include <stdint.h>

namespace storage {
namespace time {

/**
 * @brief Konwertuje datę i czas w formacie FAT na znacznik czasu Unix (sekundy od 1970-01-01).
 *
 * @param fatDate Data FAT (16-bit)
 * @param fatTime Czas FAT (16-bit)
 * @return uint32_t Timestamp Unix (lub 0 jeśli niepoprawna data)
 */
uint32_t fatDateTimeToUnix(uint16_t fatDate, uint16_t fatTime);

/**
 * @brief Konwertuje znacznik czasu Unix na datę i czas FAT.
 *
 * @param timestamp Znacznik czasu Unix
 * @param fatDate Wskaźnik na 16-bitowy wynik daty FAT
 * @param fatTime Wskaźnik na 16-bitowy wynik czasu FAT
 */
void unixToFatDateTime(uint32_t timestamp, uint16_t* fatDate, uint16_t* fatTime);

} // namespace time
} // namespace storage

#endif // STORAGE_TIME_TIMEUTILS_H

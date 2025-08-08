#ifndef STORAGE_ITIMEPROVIDER_H
#define STORAGE_ITIMEPROVIDER_H

#include <stdint.h>

namespace storage {

/**
 * @brief Interfejs dostawcy czasu dla systemu plików.
 *
 * Umożliwia dostarczenie znacznika czasu (data i godzina)
 * w formacie zgodnym z FAT (16-bitowa data, 16-bitowy czas).
 * Może być używany przez SdFat do ustawiania daty utworzenia
 * i modyfikacji plików.
 */
class ITimeProvider {
public:
    virtual ~ITimeProvider() = default;

    /**
     * @brief Pobiera aktualny czas w formacie FAT.
     *
     * @param date  Wskaźnik, gdzie zapisany zostanie 16-bitowy kod daty.
     * @param time  Wskaźnik, gdzie zapisany zostanie 16-bitowy kod czasu.
     *
     * Format daty FAT:
     * - bity 0–4: dzień (1–31)
     * - bity 5–8: miesiąc (1–12)
     * - bity 9–15: rok od 1980 (0 = 1980)
     *
     * Format czasu FAT:
     * - bity 0–4: sekundy / 2 (0–29, reprezentuje 0–58 sekund)
     * - bity 5–10: minuty (0–59)
     * - bity 11–15: godziny (0–23)
     */
    virtual void getFatTime(uint16_t* date, uint16_t* time) = 0;
};

} // namespace storage

#endif // STORAGE_ITIMEPROVIDER_H

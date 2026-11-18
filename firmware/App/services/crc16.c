/**
 * @file crc16.c
 * @brief bitwise crc16 / CCITT-FALSE (small code, ~50 bytes per frame -> fast enough)
 */

#include "crc16.h"

uint16_t crc16_update(uint16_t crc, const uint8_t *data, size_t len)
{
    for(size_t i = 0u; i < len; i++){
        crc ^= (uint16_t)((uint16_t)data[i] << 8);
        for(uint32_t b = 0u; b < 8u; b++){
            crc = ((crc & 0x8000u) != 0u) ? (uint16_t)((crc << 1) ^ 0x1021u) : (uint16_t)(crc << 1);
        }
    }
    return crc;
}
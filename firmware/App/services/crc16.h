/**
 * @file crc16.h
 */

#ifndef CRC16_H
#define CRC16_H

#include <stddef.h>
#include <stdint.h>

#define CRC16_INIT (0xFFFFu)

uint16_t crc16_update(uint16_t crc, const uint8_t *data, size_t len);

#endif

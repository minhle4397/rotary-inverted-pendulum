/**
 * @file ringbuf.h
 * @brief Byte ring buffer, single producer / single consumer , lock-free
 */

#ifndef RINGBUF_H
#define RINGBUF_H


#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t             *mem;
    uint16_t             mask; //size - 1
    volatile uint16_t   head;
    volatile uint16_t   tail;
} ringbuf_t;

//size_pow2 must be a power of 2, <=32768
void        ringbuf_init(ringbuf_t *rb, uint8_t *mem, uint16_t size_pow2);
uint16_t    ringbuf_used(const ringbuf_t *rb);
uint16_t    ringbuf_free(const ringbuf_t *rb);
//All or nothing write. return false if not enough space
bool        ringbuf_write(ringbuf_t *rb, const uint8_t *data, uint16_t len);
//Read up to max bytes, return the count
uint16_t    ringbuf_read(ringbuf_t *rb, uint8_t *dst, uint16_t max);

#endif
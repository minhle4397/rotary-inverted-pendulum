/**
 * @file ringbuf.c
 */

#include "ringbuf.h"
#include <stdint.h>

void ringbuf_init(ringbuf_t *rb, uint8_t *mem, uint16_t size_pow2){
    rb->mem     = mem;
    rb->mask    = (uint16_t)(size_pow2-1u); 
    rb->head    = 0u;
    rb->tail    = 0u;
}

uint16_t ringbuf_used(const ringbuf_t *rb){
    return (uint16_t)(rb->head - rb->tail);
}

uint16_t ringbuf_free(const ringbuf_t *rb){
    return (uint16_t)((uint16_t)(rb->mask+1u) - ringbuf_used(rb));
}

bool ringbuf_write(ringbuf_t *rb, const uint8_t *data, uint16_t len){
    if(len > ringbuf_free(rb))
        return false;
    uint16_t h = rb->head;
    for(uint16_t i = 0u; i < len; i++){
        rb->mem[h & rb->mask] = data[i];
        h++;
    }
    rb->head = h;
    return true;
}

uint16_t ringbuf_read(ringbuf_t *rb, uint8_t *dst, uint16_t max)
{
    uint16_t t = rb->tail;
    uint16_t n = 0u;
    while((t != rb->head) && (n < max)){
        dst[n++] = rb->mem[t & rb->mask];
        t++;
    }
    rb->tail = t;
    return n;
}
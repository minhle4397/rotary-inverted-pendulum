/**
 * @file strfmt.h
 * @brief tiny string builder + number formatting/parsing without printf/strtof
 */

#ifndef STRFMT_H
#define STRFMT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct{
    char *buf;
    size_t cap; //including the terminating 
    size_t len;
} sbuf_t;

void sb_init(sbuf_t *sb, char *buf, size_t cap);
void sb_char(sbuf_t *sb, char c);
void sb_str(sbuf_t *sb, const char *s);

//Signed integer, right aligned in 'width' characters (0 = no padding)
void sb_i32(sbuf_t *sb, int32_t v, unsigned width);
//fixed-point float with 'decimals' digits, right aligned in 'width'
void sb_f32(sbuf_t *sb, float v, unsigned decimals, unsigned width);
void sb_hex(sbuf_t *sb, uint32_t v, unsigned digits);
//Pad with spaces up to total length n
void sb_pad(sbuf_t *sb, size_t n);

//Parse a decimal float ("-1.25", "3", "2e-3"). Returns false on syntax error.
bool str_to_f32(const char *s, float *out);

#endif
/**
 * @file strfmt.c
 * @brief string builder and number conversion
 */

#include "strfmt.h"

void sb_init(sbuf_t *sb, char *buf, size_t cap){
    sb->buf = buf;
    sb->cap = cap;
    sb->len = 0u;
    if(cap > 0u){
        buf[0] = '\0';
    }
}

void sb_char(sbuf_t *sb, char c){
    if((sb->len + 1u) < sb->cap){
        sb->buf[sb->len++] = c;
        sb->buf[sb->len]   = '\0';
    }
}

void sb_str(sbuf_t *sb, const char *s){
    while(*s != '\0'){
        sb_char(sb, *s++);
    }
}

//digits of |V| into tmp (reversed), returns the count
static unsigned utoa_rev(uint32_t v, char *tmp){
    unsigned n = 0u;
    do{
        tmp[n++] = (char)('0' + (char)(v % 10u));
        v /= 10u;
    }while(v!=0u);
    return n;
}

void sb_i32(sbuf_t *sb, int32_t v, unsigned int width){
    char           tmp[12];
    const bool     neg = v < 0;
    const uint32_t mag = neg ? (uint32_t)(-(v + 1)) + 1u : (uint32_t)v;
    const unsigned n   = utoa_rev(mag, tmp);
    for (unsigned w = n + (neg ? 1u : 0u); w < width; w++) {
        sb_char(sb, ' ');
    }
    if (neg) {
        sb_char(sb, '-');
    }
    for (unsigned i = n; i > 0u; i--) {
        sb_char(sb, tmp[i - 1u]);
    }
}

void sb_f32(sbuf_t *sb, float v, unsigned decimals, unsigned width)
{
    if (decimals > 6u) {
        decimals = 6u;
    }
    if ((v != v) || (v > 2.0e9f) || (v < -2.0e9f)) { /* NaN / out of range */
        for (unsigned w = 3u; w < width; w++) {
            sb_char(sb, ' ');
        }
        sb_str(sb, "nan");
        return;
    }
    uint32_t scale = 1u;
    for (unsigned i = 0u; i < decimals; i++) {
        scale *= 10u;
    }
    const bool neg = v < 0.0f;
    const float a  = neg ? -v : v;
    /* round to the requested number of decimals */
    const uint32_t total  = (uint32_t)((a * (float)scale) + 0.5f);
    const uint32_t ipart  = total / scale;
    uint32_t       fpart  = total % scale;

    char     itmp[12];
    unsigned in  = utoa_rev(ipart, itmp);
    unsigned len = in + (neg ? 1u : 0u) + ((decimals > 0u) ? (decimals + 1u) : 0u);
    for (unsigned w = len; w < width; w++) {
        sb_char(sb, ' ');
    }
    if (neg && (total != 0u)) {
        sb_char(sb, '-');
    } else if (neg) {
        sb_char(sb, ' ');
    } else {
        /* positive */
    }
    for (unsigned i = in; i > 0u; i--) {
        sb_char(sb, itmp[i - 1u]);
    }
    if (decimals > 0u) {
        sb_char(sb, '.');
        char ftmp[8];
        for (unsigned i = 0u; i < decimals; i++) {
            ftmp[decimals - 1u - i] = (char)('0' + (char)(fpart % 10u));
            fpart /= 10u;
        }
        for (unsigned i = 0u; i < decimals; i++) {
            sb_char(sb, ftmp[i]);
        }
    }
}

void sb_hex(sbuf_t *sb, uint32_t v, unsigned digits)
{
    static const char hex[] = "0123456789ABCDEF";
    for (unsigned i = digits; i > 0u; i--) {
        sb_char(sb, hex[(v >> (4u * (i - 1u))) & 0x0Fu]);
    }
}

void sb_pad(sbuf_t *sb, size_t n)
{
    while (sb->len < n) {
        sb_char(sb, ' ');
    }
}

bool str_to_f32(const char *s, float *out)
{
    float sign = 1.0f;
    float val  = 0.0f;
    bool  digits = false;

    if ((*s == '+') || (*s == '-')) {
        sign = (*s == '-') ? -1.0f : 1.0f;
        s++;
    }
    while ((*s >= '0') && (*s <= '9')) {
        val    = val * 10.0f + (float)(*s - '0');
        digits = true;
        s++;
    }
    if (*s == '.') {
        s++;
        float scale = 0.1f;
        while ((*s >= '0') && (*s <= '9')) {
            val += (float)(*s - '0') * scale;
            scale *= 0.1f;
            digits = true;
            s++;
        }
    }
    if (!digits) {
        return false;
    }
    if ((*s == 'e') || (*s == 'E')) {
        s++;
        int  esign = 1;
        int  e     = 0;
        bool edig  = false;
        if ((*s == '+') || (*s == '-')) {
            esign = (*s == '-') ? -1 : 1;
            s++;
        }
        while ((*s >= '0') && (*s <= '9') && (e < 38)) {
            e    = e * 10 + (*s - '0');
            edig = true;
            s++;
        }
        if (!edig) {
            return false;
        }
        for (int i = 0; i < e; i++) {
            val = (esign > 0) ? (val * 10.0f) : (val / 10.0f);
        }
    }
    if (*s != '\0') {
        return false;
    }
    *out = sign * val;
    return true;
}

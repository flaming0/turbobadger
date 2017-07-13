#include <cstdio>
#include "utf8/utf8.h"
#include "tb_str.h"
#include <limits>
#include <cstdint>

namespace utf8 {

/** is c the start of a UTF-8 sequence? */
#define isutf(c) (((c)&0xC0)!=0x80)

UCS4 decode(const char *&src, uintptr_t src_end)
{
    const char* start = src;

    if ((uintptr_t)src == src_end)
        return 0;

    unsigned char ch1 = *(src++);
    if (ch1 <= 0x7F)
        return ch1;

    // should not have continuation character here
    bool tmp1 = (uintptr_t)src < src_end;
    if (((ch1 & 0xC0) != 0x80) && tmp1)
    {
        unsigned char ch2 = *(src++);
        // should be continuation
        if ((ch2 & 0xC0) != 0x80)
            goto invalid;
        if ((ch1 & 0xE0) == 0xC0)
        {
            if ((ch2 & 0xC0) == 0x80)
            {
                unsigned int rv = ((ch1 & 0x1F) << 6) + (ch2 & 0x3F);
                if (rv >= 0x80)
                    return rv;
            }
            goto invalid;
        }
        if ((uintptr_t)src < src_end)
        {
            unsigned char ch3 = *(src++);
            // should be continuation
            if ((ch3 & 0xC0) != 0x80)
                goto invalid;
            if ((ch1 & 0xF0) == 0xE0)
            {
                unsigned rv = ((ch1 & 0x0F) << 12)
                    + ((ch2 & 0x3F) << 6)
                    + (ch3 & 0x3F);
                if (rv <= 0x800)
                    goto invalid;
                return rv;
            }
            if ((uintptr_t)src < src_end)
            {
                unsigned char ch4 = *(src++);
                if ((ch4 & 0xC0) != 0x80)
                    goto invalid;
                UCS4 rv = ((ch1 & 0x07) << 18)
                    + ((ch2 & 0x3F) << 12)
                    + ((ch3 & 0x3F) << 6)
                    + (ch4 & 0x3F);
                if (rv > 0xFFFF)
                    return rv;
            }
        }
    }
invalid:
    src = start;
    return 0xFFFF;
}

int encode(UCS4 ch, char *dst)
{
    if (ch < 0x80) {
        dst[0] = (char)ch;
        return 1;
    }
    else if (ch < 0x800) {
        dst[0] = (char)(0xC0 + (ch >> 6));
        dst[1] = (char)(0x80 + (ch & 0x3F));
        return 2;
    }
    else if (ch < 0x10000) {
        dst[0] = (char)(0xE0 + (ch >> 12));
        dst[1] = (char)(0x80 + ((ch >> 6) & 0x3F));
        dst[2] = (char)(0x80 + (ch & 0x3F));
        return 3;
    }
    else if (ch <= 0x10FFFF) {
        dst[0] = (char)(0xF0 + (ch >> 18));
        dst[1] = (char)(0x80 + ((ch >> 12) & 0x3F));
        dst[2] = (char)(0x80 + ((ch >> 6) & 0x3F));
        dst[3] = (char)(0x80 + (ch & 0x3F));
        return 4;
    }
    else {
        //  output UTF-8 encoding of 0xFFFF
        dst[0] = (char)0xEF;
        dst[1] = (char)0xBF;
        dst[2] = (char)0xBF;
        return 3;
    }
}

UCS4 decode_next(const char *str, int *i, int i_max)
{
    str += *i;
    i_max -= *i;
    const char *old_str = str;

    // Handle wrapping that could happen if the caller use
    // something really large for i_max if src is known to
    // be null terminated (like TB_ALL_TO_TERMINATION)
    uintptr_t str_end = (uintptr_t)str + i_max;
    if (str_end < (uintptr_t)str)
        str_end = std::numeric_limits<uintptr_t>::max();

    UCS4 ch = decode(str, str_end);
    if (ch == 0xFFFF) // Invalid character!
        (*i)++;
    else
        *i += str - old_str;
    return ch;
}

void move_inc(const char *str, int *i, int i_max)
{
    (void)((*i < i_max && isutf(str[++(*i)])) ||
        (*i < i_max && isutf(str[++(*i)])) ||
        (*i < i_max && isutf(str[++(*i)])) ||
        (*i < i_max && ++(*i)));
}

void move_dec(const char *str, int *i)
{
    (void)((*i > 0 && isutf(str[--(*i)])) ||
        (*i > 0 && isutf(str[--(*i)])) ||
        (*i > 0 && isutf(str[--(*i)])) ||
        (*i > 0 && --(*i)));
}

int count_characters(const char *str, int i_max)
{
    int count = 0;
    int i = 0;
    while (i < i_max && decode_next(str, &i, i_max))
        count++;
    return count;
}

} // namespace utf8
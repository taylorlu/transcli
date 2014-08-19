#ifndef __WRITE_BITS_H__
#define __WRITE_BITS_H__
#include <stdint.h>
#include <stdio.h>

#define av_assert2(con) 

struct PutBitContext {
    uint32_t bit_buf;
    int bit_left;
    uint8_t *buf, *buf_ptr, *buf_end;
    int size_in_bits;
};

static inline void AV_WL32(void *p, uint32_t v)
{
	(*(uint32_t*)p) = v;
}

#ifndef AV_WB32
#   define AV_WB32(p, darg) do {                \
	unsigned d = (darg);                    \
	((uint8_t*)(p))[3] = (d)&0xFF;               \
	((uint8_t*)(p))[2] = (d)>>8;            \
	((uint8_t*)(p))[1] = (d)>>16;           \
	((uint8_t*)(p))[0] = (d)>>24;           \
} while(0)
#endif

/**
 * Initialize the PutBitContext s.
 *
 * @param buffer the buffer where to put bits
 * @param buffer_size the size in bytes of buffer
 */
static inline void init_put_bits(PutBitContext *s, uint8_t *buffer,
                                 int buffer_size)
{
    if (buffer_size < 0) {
        buffer_size = 0;
        buffer      = NULL;
    }

    s->size_in_bits = 8 * buffer_size;
    s->buf          = buffer;
    s->buf_end      = s->buf + buffer_size;
    s->buf_ptr      = s->buf;
    s->bit_left     = 32;
    s->bit_buf      = 0;
}

/**
 * @return the total number of bits written to the bitstream.
 */
static inline int put_bits_count(PutBitContext *s)
{
    return (s->buf_ptr - s->buf) * 8 + 32 - s->bit_left;
}

/**
 * @return the number of bits available in the bitstream.
 */
static inline int put_bits_left(PutBitContext* s)
{
    return (s->buf_end - s->buf_ptr) * 8 - 32 + s->bit_left;
}

/**
 * Pad the end of the output stream with zeros.
 */
static inline void flush_put_bits(PutBitContext *s)
{
#ifndef BITSTREAM_WRITER_LE
    if (s->bit_left < 32)
        s->bit_buf <<= s->bit_left;
#endif
    while (s->bit_left < 32) {
        /* XXX: should test end of buffer */
#ifdef BITSTREAM_WRITER_LE
        *s->buf_ptr++ = s->bit_buf;
        s->bit_buf  >>= 8;
#else
        *s->buf_ptr++ = s->bit_buf >> 24;
        s->bit_buf  <<= 8;
#endif
        s->bit_left  += 8;
    }
    s->bit_left = 32;
    s->bit_buf  = 0;
}

/**
 * Write up to 31 bits into a bitstream.
 * Use put_bits32 to write 32 bits.
 */
static inline void put_bits(PutBitContext *s, int n, unsigned int value)
{
    unsigned int bit_buf;
    int bit_left;

    av_assert2(n <= 31 && value < (1U << n));

    bit_buf  = s->bit_buf;
    bit_left = s->bit_left;

    /* XXX: optimize */
#ifdef BITSTREAM_WRITER_LE
    bit_buf |= value << (32 - bit_left);
    if (n >= bit_left) {
        av_assert2(s->buf_ptr+3<s->buf_end);
        AV_WL32(s->buf_ptr, bit_buf);
        s->buf_ptr += 4;
        bit_buf     = (bit_left == 32) ? 0 : value >> bit_left;
        bit_left   += 32;
    }
    bit_left -= n;
#else
    if (n < bit_left) {
        bit_buf     = (bit_buf << n) | value;
        bit_left   -= n;
    } else {
        bit_buf   <<= bit_left;
        bit_buf    |= value >> (n - bit_left);
        av_assert2(s->buf_ptr+3<s->buf_end);
        AV_WB32(s->buf_ptr, bit_buf);
        s->buf_ptr += 4;
        bit_left   += 32 - n;
        bit_buf     = value;
    }
#endif

    s->bit_buf  = bit_buf;
    s->bit_left = bit_left;
}

static inline void put_sbits(PutBitContext *pb, int n, int32_t value)
{
    av_assert2(n >= 0 && n <= 31);

    put_bits(pb, n, value & ((1 << n) - 1));
}

/**
 * Write exactly 32 bits into a bitstream.
 */
static void put_bits32(PutBitContext *s, uint32_t value)
{
    int lo = value & 0xffff;
    int hi = value >> 16;
#ifdef BITSTREAM_WRITER_LE
    put_bits(s, 16, lo);
    put_bits(s, 16, hi);
#else
    put_bits(s, 16, hi);
    put_bits(s, 16, lo);
#endif
}

/**
 * Return the pointer to the byte where the bitstream writer will put
 * the next bit.
 */
static inline uint8_t *put_bits_ptr(PutBitContext *s)
{
    return s->buf_ptr;
}

/**
 * Skip the given number of bytes.
 * PutBitContext must be flushed & aligned to a byte boundary before calling this.
 */
static inline void skip_put_bytes(PutBitContext *s, int n)
{
    av_assert2((put_bits_count(s) & 7) == 0);
    av_assert2(s->bit_left == 32);
    s->buf_ptr += n;
}

/**
 * Skip the given number of bits.
 * Must only be used if the actual values in the bitstream do not matter.
 * If n is 0 the behavior is undefined.
 */
static inline void skip_put_bits(PutBitContext *s, int n)
{
    s->bit_left -= n;
    s->buf_ptr  -= 4 * (s->bit_left >> 5);
    s->bit_left &= 31;
}

/**
 * Change the end of the buffer.
 *
 * @param size the new size in bytes of the buffer where to put bits
 */
static inline void set_put_bits_buffer_size(PutBitContext *s, int size)
{
    s->buf_end = s->buf + size;
}

#endif
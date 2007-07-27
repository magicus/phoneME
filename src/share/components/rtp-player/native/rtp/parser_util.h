/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
 
/** 
 * @file parser_util.h
 *
 * Utility functions for RTP and RTCP parsers.
 */

#include <string.h>

/**
 * @var RTP_WORD parser_offset.
 *
 * The offset into the byte array to be parsed.
 */

static RTP_WORD parser_offset;


/** 
 *  Reads a byte from the input byte array.
 *
 * @param buf          Pointer to the input byte array.
 * @return             Returns a value of type RTP_BYTE.
 */

static RTP_BYTE read_byte(RTP_BYTE *buf) {
    RTP_BYTE val = buf[ parser_offset];

    parser_offset++;

    return val;
}


/** 
 *  Copies bytes from the input byte array to the output byte array.
 *
 * @param from         Pointer to the input byte array.
 * @param to           Pointer to the output byte array.
 * @param length       The number of bytes to be copied.
 */

static void read_bytes(RTP_BYTE *from, RTP_BYTE *to, RTP_WORD length) {
    memcpy(to, from + parser_offset, length);

    parser_offset += length;
}


/** 
 *  Skips the specified number of bytes.
 *
 * @param length       The number of bytes to be skipped.
 */

static void skip_bytes( RTP_WORD length) {
    parser_offset += length;
}


/** 
 *  Reads 16 bits from the input byte array.
 *
 * @param buf          Pointer to the input byte array.
 * @return             Returns a value of type RTP_SHORT.
 */

static RTP_SHORT read_short(RTP_BYTE *buf) {
    RTP_SHORT val = ((buf[ parser_offset] << 8) + buf[ parser_offset + 1]);

    parser_offset += 2;

    return val;
}


/** 
 *  Reads 32 bits from the input byte array.
 *
 * @param buf          Pointer to the input byte array.
 * @return             Returns a value of type RTP_WORD.
 */

static RTP_WORD read_word(RTP_BYTE *buf) {
    RTP_WORD val = ((buf[ parser_offset] << 24) + (buf[ parser_offset + 1] << 16) 
                 + (buf[ parser_offset + 2] << 8) + buf[ parser_offset + 3]);

    parser_offset += 4;

    return val;
}


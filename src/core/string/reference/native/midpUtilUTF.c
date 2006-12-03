/*
 *   
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation. 
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt). 
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA 
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions. 
 */

/**
 * @file
 *
 * Useful utilities to convert between UTF8 and UCS.
 */

/**
 * Converts UCS-2 (2 byte Unicode) to UTF-8. Note that values D800 to DFFF are
 * considered UCS-4 and should not be given to this function since it will not
 * perform any special UCS-4 character pair handling. The constant
 * UTF_8_PER_UCS_2_CHARS can be used to size the result buffer.
 *
 * @param pUcs2 UCS-2 input buffer
 * @param ucs2Len number of UCS-2 chars to convert
 * @param pUtf8 UTF-8 result buffer
 * @param utf8Len length of the UTF-8 result buffer
 *
 * @return the number of UTF-8 characters generated from the UCS-2 characters
 *  or less than 0 the result buffer is to small
 */
int midpUTF8Encode(const unsigned short* pUcs2, int ucs2Len, unsigned char* pUtf8,
               int utf8Len) {
    int i;
    int j;
    unsigned short current;

    for (i = 0, j = 0; i < ucs2Len; i++) {
        if (j == utf8Len) {
            /* Can't continue, result buffer too small */
            return -1;
        }

        current = pUcs2[i];

        if (current < 0x80) {
            /* binary 0xxxxxxx where x is a char bit */
            pUtf8[j++] = (unsigned char)current;
            continue;
        }

        if (current < 0x800) {
            /* binary 110xxxxx 10xxxxxx where x is a char bit */
            pUtf8[j++] = (unsigned char)(0xC0 | ((current >> 6) & 0x1F));
            if (j == utf8Len) {
                /* Can't continue, result buffer too small */
                return -1;
            }

            pUtf8[j++] = (unsigned char)(0x80 | (current & 0x3F));
            continue;
        }

        /* binary 1110xxxx 10xxxxxx 10xxxxxx where x is a char bit */
        pUtf8[j++] = (unsigned char)(0xE0 | ((current >> 12) & 0x0F));
        if (j == utf8Len) {
            /* Can't continue, result buffer too small */
            return -1;
        }

        pUtf8[j++] = (unsigned char)(0x80 | ((current >> 6) & 0x3F));
        if (j == utf8Len) {
            /* Can't continue, result buffer too small */
            return -1;
        }

        pUtf8[j++] = (unsigned char)(0x80 | (current & 0x3F));
    }

    return j;
}

/**
 * Converts UTF-8 to UCS-2 (2 byte Unicode). If a UTF-8 sequence is more
 * than 3 bytes it will be considered an error. When allocating the result
 * buffer assume the each UTF-8 char will become a UCS-2 char.
 *
 * @param pUtf8 UTF-8 result buffer
 * @param utf8Len length of the UTF-8 result buffer
 * @param pUcs2 UCS-2 input buffer
 * @param ucs2Len number of UCS-2 chars to convert
 *
 * @return the number of UCS-2 characters generated from the UTF-8 characters
 *  or less than 0 if the result buffer is to small or a 4 byte sequence is
 *  encountered or partial sequence is encountered
 */
int midpUTF8Decode(unsigned char* pUtf8, int utf8Len, unsigned short* pUcs2,
               int ucs2Len) {
    int i;
    int j;
    unsigned char current;

    for (i = 0, j = 0; i < ucs2Len && j < utf8Len; i++) {
        current = pUtf8[j++];

        if (current < 0x80) {
            /* binary 0xxxxxxx where x is a char bit */
            pUcs2[i] = (unsigned short)current;
            continue;
        }

        if ((current & 0xE0) == 0xC0) {
            /* binary 110xxxxx 10xxxxxx where x is a char bit */
            pUcs2[i] = (unsigned short)(current & 0x1F) << 6;
            if (j == utf8Len) {
                /* Can't continue, input buffer too small */
                return -1;
            }

            pUcs2[i] += (unsigned short)(pUtf8[j++] & 0x3F);
            continue;
        }

        if ((current & 0xF0) == 0xE0) {
            /* binary 1110xxxx 10xxxxxx 10xxxxxx where x is a char bit */
            pUcs2[i] = (unsigned short)(current & 0x0F) << 12;
            if (j == utf8Len) {
                /* Can't continue, input buffer too small */
                return -2;
            }

            pUcs2[i] += (unsigned short)(pUtf8[j++] & 0x3F) << 6;
            if (j == utf8Len) {
                /* Can't continue, input buffer too small */
                return -3;
            }

            pUcs2[i] += (unsigned short)(pUtf8[j++] & 0x3F);
            continue;
        }

        /* Not a proper UCS-2 char sequence */
        return -4;
    }

    if (j < utf8Len) {
        /* Can't continue, result buffer too small */
        return -5;
    }

    return i;
}

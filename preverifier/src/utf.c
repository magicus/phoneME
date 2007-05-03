/*
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

/*=========================================================================
 * SYSTEM:    Verifier
 * SUBSYSTEM: Unicode translators.
 * FILE:      utf.c
 * OVERVIEW:  Routines for Unicode -> UTF and UTF -> unicode translators.
 *       
 * This file implements the unicode -> UTF and UTF -> unicode translators
 * needed by the various parts of the compiler and interpreter.
 *
 * UTF strings are streams of bytes, in which unicode characters are encoded
 * as follows:
 *       Unicode                  UTF
 *       00000000 0jklmnop       0jklmnop 
 *       00000fgh ijklmnop       110fghij 10klmnop
 *       abcdefgh ijklmnop       1110abcd 10efghij 10klmnop
 *
 * unicode bytes with 7 or fewer significant bits MUST be converted using the
 * first format.  bytes with 11 or fewer bits MUST be converted using the
 * second format.
 *
 * In JAVA/JAVAC, we deviate slightly from the above.  
 *    1) The null unicode character is represented using the 2-byte format
 *    2)  All UTF strings are null-terminated.
 * In this way, we do not need to separately maintain a length field for the
 * UTF string.
 *
 * Given a unicode string and its length, convert it to a utf string.  But 
 * the result into the given buffer, whose length is buflength.  The utf
 * string should include a null terminator.
 *
 * If both buffer and buflength are 0, then malloc an appropriately sized
 * buffer for the result.
 *
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <oobj.h>
#include <utf.h>
#include <sys_api.h>

char *unicode2utf(unicode *unistring, int length, char *buffer, int buflength)
{
    int i;
    unicode *uniptr;
    char *bufptr;
    unsigned bufleft;

    if ((buffer == 0) && (buflength == 0)) {
    buflength = unicode2utfstrlen(unistring, length);
    if ((buffer = (char *) sysMalloc(buflength)) == 0) 
        return 0;
    }

    bufleft = buflength - 1; /* take note of null now! */

    for(i = length, uniptr = unistring, bufptr = buffer; --i >= 0; uniptr++) {
    unicode ch = *uniptr;
    if ((ch != 0) && (ch <=0x7f)) {
        if ((int)(--bufleft) < 0)    /* no space for character */
            break;
        *bufptr++ = (char)ch;
    } else if (ch <= 0x7FF) { 
        /* 11 bits or less. */
        unsigned char high_five = ch >> 6;
        unsigned char low_six = ch & 0x3F;
        if ((int)(bufleft -= 2) < 0) /* no space for character */
            break;
        *bufptr++ = high_five | 0xC0; /* 110xxxxx */
        *bufptr++ = low_six | 0x80;      /* 10xxxxxx */
    } else {
        /* possibly full 16 bits. */
        char high_four = ch >> 12;
        char mid_six = (ch >> 6) & 0x3F;
        char low_six = ch & 0x3f;
        if ((int)(bufleft -= 3) < 0) /* no space for character */
            break;
        *bufptr++ = high_four | 0xE0; /* 1110xxxx */
        *bufptr++ = mid_six | 0x80;   /* 10xxxxxx */
        *bufptr++ = low_six | 0x80;   /* 10xxxxxx*/
    }
    }
    *bufptr = 0;
    return buffer;
}

/* Return the number of characters that would be needed to hold the unicode
 * string in utf.  This INCLUDES the NULL!
 */
int unicode2utfstrlen(unicode *unistring, int unilength)
{
    int result_length = 1;
    
    for (; unilength > 0; unistring++, unilength--) {
    unicode ch = *unistring;
    if ((ch != 0) && (ch <= 0x7f)) /* 1 byte */
        result_length++;
    else if (ch <= 0x7FF)
        result_length += 2;    /* 2 byte character */
    else 
        result_length += 3;    /* 3 byte character */
    }
    return result_length;
}

/* Give the number of unicode characters in a utf string */
int utfstrlen(char *utfstring) 
{
    int length;
    for (length = 0; *utfstring != 0; length++) 
        next_utf2unicode(&utfstring);
    return length;
}

/* Convert a utfstring to unicode in the buffer provided.  Put at most
 * max_length characters into the buffer.  Whether or not we actually overflow
 * the space, indicate the actual unicode length.
 *
 * Whether or not we overflow the space, return the actual number of
 * characters that we used.
 */

void
utf2unicode(char *utfstring, unicode *unistring, 
        int max_length, int *lengthp)
{
    int length_remaining = max_length;
    
    while (length_remaining > 0 && *utfstring != 0) {
    *unistring++ = next_utf2unicode(&utfstring);
    length_remaining--;
    }

    if (length_remaining == 0) {
    *lengthp = max_length + utfstrlen(utfstring);
    } else {
    *lengthp = max_length - length_remaining;
    }
}

bool_t is_simple_utf(char *utfstring) 
{
    unsigned char *ptr;
    for (ptr = (unsigned char *)utfstring; *ptr != 0; ptr++) {
    if (*ptr > 0x80) return FALSE;
    }
    return TRUE;
}

unicode next_utf2unicode(char **utfstring_ptr) {
    unsigned char *ptr = (unsigned char *)(*utfstring_ptr);
    unsigned char ch, ch2, ch3;
    int length = 1;        /* default length */
    unicode result = 0x80;    /* default bad result; */
    switch ((ch = ptr[0]) >> 4) {
        default:
        result = ch;
        break;

    case 0x8: case 0x9: case 0xA: case 0xB: case 0xF:
        /* Shouldn't happen. */
        break;

    case 0xC: case 0xD:    
        /* 110xxxxx  10xxxxxx */
        if (((ch2 = ptr[1]) & 0xC0) == 0x80) {
        unsigned char high_five = ch & 0x1F;
        unsigned char low_six = ch2 & 0x3F;
        result = (high_five << 6) + low_six;
        length = 2;
        } 
        break;

    case 0xE:
        /* 1110xxxx 10xxxxxx 10xxxxxx */
        if (((ch2 = ptr[1]) & 0xC0) == 0x80) {
        if (((ch3 = ptr[2]) & 0xC0) == 0x80) {
            unsigned char high_four = ch & 0x0f;
            unsigned char mid_six = ch2 & 0x3f;
            unsigned char low_six = ch3 & 0x3f;
            result = (((high_four << 6) + mid_six) << 6) + low_six;
            length = 3;
        } else {
            length = 2;
        }
        }
        break;
    } /* end of switch */

    *utfstring_ptr = (char *)(ptr + length);
    return result;
}

/*
 *   
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
 * @defgroup wma2.0 JSR120 Wireless Messaging API (WMA) 1.1
 * @ingroup subsystems
 */

/**
 * @file
 * @defgroup sms SMS Packet Manipulation Interface
 * @ingroup wma2.0
 * @brief These functions can be used to create SMS, CBS and MMS packets
 *
 *
 * @{
 */

#include <bytePackUnpack.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef ENABLE_PCSL
  #include <pcsl_memory.h>
#else
  #define pcsl_mem_malloc malloc
#endif

/**
 * Read an integer value from the stream. The index is advanced beyond the
 * integer.
 *
 * @param buffer The buffer that holds the integer.
 * @param index The reading index.
 *
 * @return the integer that was read.
 */
int getInt(char* buf, int* index) {
    int x1, x2, x3, x4;
    x1 = ((int)(buf[(*index)++] & 0xff));
    x2 = ((int)(buf[(*index)++] & 0xff) << 8);
    x3 = ((int)(buf[(*index)++] & 0xff) << 16);
    x4 = ((int)(buf[(*index)++] & 0xff) << 24);
    return (x1 + x2 + x3 + x4);
}

/**
 * Read a short value from the stream. The index is advanced beyond the short.
 *
 * @param buffer The buffer that holds the short.
 * @param index The reading index.
 *
 * @return the short that was read.
 */
short getShort(char* buf, int* index) {
    short x1, x2;
    x1 = ((short)(buf[(*index)++] & 0xff));
    x2 = ((short)(buf[(*index)++] & 0xff) << 8);
    return (x1 + x2);
}

/**
 * Read a long value from the stream. The index is advanced beyond the long.
 *
 * @param buffer The buffer that holds the long.
 * @param index The reading index.
 *
 * @return the long that was read.
 */
jlong getLongLong(char* buffer, int* index) {
    jlong x1, x2, x3, x4, x5, x6, x7, x8;
    x1 =   (jlong)(buffer[(*index)++] & 0xff);
    x2 =  ((jlong)(buffer[(*index)++] & 0xff) << 8);
    x3 =  ((jlong)(buffer[(*index)++] & 0xff) << 16);
    x4 =  ((jlong)(buffer[(*index)++] & 0xff) << 24);
    x5 =  ((jlong)(buffer[(*index)++] & 0xff) << 32);
    x6 =  ((jlong)(buffer[(*index)++] & 0xff) << 40);
    x7 =  ((jlong)(buffer[(*index)++] & 0xff) << 48);
    x8 =  ((jlong)(buffer[(*index)++] & 0xff) << 56);
    return (x1 + x2 + x3 + x4 + x5 + x6 + x7 +x8);
}

/**
 * Read a zero-terminated string from the stream.
 *
 * @param buffer The buffer that holds the string.
 * @param index The reading index. 
 */
char* getString(char* buffer, int* index) {
    int len = 0;
    char* s = NULL;

    /* Special case of leading 0xff for null strings. */
    if (*(buffer + (*index)) != (char)0xff) {
        len = strlen(buffer + (*index));
        s = (char*)pcsl_mem_malloc(len + 1);
        strcpy(s, buffer + (*index));
    }

    /* Advance the index past the string. */
    *index = *index + len + 1;
    return s;
}

/**
 * Read an array of bytes from the stream into a buffer. The memory for the
 * buffer is automatically allocated and must be freed later by the calling
 * code.
 *
 * @param buffer The buffer that holds the array of bytes.
 * @param index The reading index.
 * @param length The number of bytes to be read.
 *
 * @return a pointer to the the array of bytes. The memory for this array must
 *     be freed by the calling code.
 */
char* getBytes(char* buffer, int* index, int length) {
    char* dst = (char*)pcsl_mem_malloc(length);
    int j = 0;
    putBytes(dst, &j, buffer + *index, length);
    *index = *index + length;
    return dst;
}

/**
 * Write an integer value to the stream.
 *
 * @param buffer The buffer that will hold the integer.
 * @param index The writing index.
 * @param x The integer value to be written.
 */ 
void putInt(char *buffer, int *index, int x) {
    buffer[(*index)++] = (char) (x & 0xff);
    buffer[(*index)++] = (char)((x >> 8) & 0xff);
    buffer[(*index)++] = (char)((x >> 16) & 0xff);
    buffer[(*index)++] = (char)((x >> 24) & 0xff);
}

/**
 * Write a short value to the stream.
 * 
 * @param buffer The buffer that will hold the integer.
 * @param index The writing index.
 * @param x The short value to be written.
 */
void putShort(char *buffer, int *index, short x) {
    buffer[(*index)++] = (char) (x & 0xff);
    buffer[(*index)++] = (char)((x >> 8) & 0xff);
}

/**
 * Write a long value to the stream.
 *
 * @param buffer The buffer that will hold the integer.
 * @param index The writing index.
 * @param x The long value to be written.
 */
void putLongLong(char *buffer, int *index, jlong x) {
    buffer[(*index)++] = (char) (x & 0xff);
    buffer[(*index)++] = (char)((x >> 8) & 0xff);
    buffer[(*index)++] = (char)((x >> 16) & 0xff);
    buffer[(*index)++] = (char)((x >> 24) & 0xff);
    buffer[(*index)++] = (char)((x >> 32) & 0xff);
    buffer[(*index)++] = (char)((x >> 40) & 0xff);
    buffer[(*index)++] = (char)((x >> 48) & 0xff);
    buffer[(*index)++] = (char)((x >> 56) & 0xff);
}

/**
 * Write a group of bytes to the stream.
 *
 * @param dst The stream. 
 * @param index The writing index into the stream.
 * @param src The source of the bytes to be written.
 * @param length The number of bytes to be written.
 */
void putBytes(char *dst, int *index, char *src, int length) {
    int i;

    for (i = 0; i < length; i++) {
        dst[(*index)++] = src[i];
    }
}

/**
 * Put a string or NULL into the data stream.
 *
 * @param buffer The destination buffer.
 * @param index The index into <code>buffer</code> at which to start
 *     writing.
 * @param s The zero-terminated string to write, or <code>NULL</code> if the
 *     string does not exist. When the string doesn't exist, <code>0xff</code>
 *     is written, instead, which is a special marker for
 *     <code>getString()</code> to use to identify <code>NULL</code> strings.
 */
void putString(char* buffer, int* index, char* s) {
    if (s == NULL) {
        /* Store 0xff for a NULL string. Note: No zero-terminator. */
        putBytes(buffer, index, "\377", 1);
    } else {
        putBytes(buffer, index, s, strlen(s) + 1);
    }
}

/* @} */

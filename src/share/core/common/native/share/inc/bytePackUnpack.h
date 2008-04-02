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

#ifndef _BYTE_PACK_UNPACK_H_
#define _BYTE_PACK_UNPACK_H_

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

#include <kni.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns an int from a byte buffer, starting at specified index.
 *
 * @param buf byte buffer.
 * @param index address of index into buffer.
 *
 * @return The integer at the specified index
 *         The index is incremented by 4
 *
 */
int getInt(char* buf, int* index);

/**
 * Returns a short from a byte buffer, starting at specified index.
 *
 * @param buf byte buffer.
 * @param index address of index into buffer.
 *
 * @return The short at the specified index
 *         The index is incremented by 2
 *
 */
short getShort(char* buf, int* index);

/**
 * Returns an long long from a byte buffer, starting at specified index.
 *
 * @param buf byte buffer.
 * @param index address of index into buffer.
 *
 * @return The long long at the specified index
 *         The index is incremented by 8
 *
 */
jlong getLongLong(char* buf, int* index);

/**
 * Returns a string from a byte buffer, starting at specified index.
 *
 * @param buf byte buffer.
 * @param index address of index into buffer.
 *
 * @return The string at the specified index
 *         The index is incremented by length of string + 1
 *
 */
char* getString(char* buf, int* index);

/**
 * Puts an int into a byte buffer, starting at specified index.
 *
 * @param buf byte buffer.
 * @param index address of index into buffer.
 *
 * @return The index is incremented by 4 on return
 *
 */
void putInt(char *buf, int *index, int x);

/**
 * Puts a short into a byte buffer, starting at specified index.
 *
 * @param buf byte buffer.
 * @param index address of index into buffer.
 *
 * @return The index is incremented by 2 on return
 *
 */
void putShort(char *buf, int *index, short x);

/**
 * Puts a long long into a byte buffer, starting at specified index.
 *
 * @param buf byte buffer.
 * @param index address of index into buffer.
 *
 * @return The index is incremented by 8 on return
 *
 */
void putLongLong(char *buf, int *index, jlong x);

/**
 * Puts a byte array of specified length into a byte buffer, starting at specified index.
 *
 * @param buffer byte buffer.
 * @param index address of index into buffer.
 * @param buf byte buffer to copy from
 * @param length number of bytes to copy
 *
 * @return The index is incremented by length on return
 *
 */
void putBytes(char *buffer, int *index, char *buf, int length);

/**
 * Gets an array of bytes from a buffer of bytes. Memory is allocated for the
 * copied bytes.
 *
 * @param buffer The buffer of bytes.
 * @param index The index into the buffer of the first byte to copy.
 * @param length The number of bytes to copy.
 *
 * @return A new buffer of <code>length</code> bytes, copied from the
 *     <code>buffer</code> The <code>index</code> is incremented by
 *     <code>length</code>.
 *
 */
char* getBytes(char* buffer, int* index, int length);

/**
 * Puts a null-terminated string into the byte stream, starting at specified
 * index.
 *
 * @param buffer byte buffer.
 * @param index address of index into buffer.
 * @param buf byte buffer to copy from
 *
 * @return The index is incremented by (length + 1) on return
 *
 */
void putString(char *buffer, int *index, char *buf);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* #ifdef _BYTE_PACK_UNPACK_H_ */

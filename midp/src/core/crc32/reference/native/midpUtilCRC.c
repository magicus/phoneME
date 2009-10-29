/*
 *   
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

/**
 * @file
 *
 * @brief CRC32 checksum algorithm implementation for small device
 *
 * A CRC32 checksum algorithm implementation
 * allows a checksum to be computed in small chunks.  This lets
 * the caller limit the amount of space used to store the data
 * being hashed.
 */
#include <midpUtilCRC.h>

unsigned long
midpCRC32Init(unsigned char *data, int length){
    return (midpCRC32Update(data, length, 0xFFFFFFFF));
}

unsigned long
midpCRC32Update(unsigned char *data, int length, unsigned long iv) { 
    unsigned long crc = iv;
    unsigned int j;
    for ( ; length > 0; length--, data++) { 
        crc ^= *data;
        for (j = 8; j > 0; --j) { 
            crc = (crc & 1) ? ((crc >> 1) ^ 0xedb88320) : (crc >> 1);
        }
    }
    return crc;
}

unsigned long
midpCRC32Finalize(unsigned long crc) {
    return ~crc;
}

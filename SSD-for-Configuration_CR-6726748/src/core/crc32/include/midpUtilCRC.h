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

#ifndef _MIDP_UTIL_CRC_H_
#define _MIDP_UTIL_CRC_H_

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @defgroup core_crc32 CRC32 Checksum External Interface
 * @ingroup core
 */

/**
 * @file
 * @ingroup core_crc32
 *
 * @brief Interface to a CRC32 checksum algorithm
 *
 * <p>This interface computes a checksum in small chunks, which
 * enables the caller limit the amount of space used to store the data
 * being hashed. The following example computes the checksum of 180
 * bytes of data in three 60-byte chunks.
 *
 * <pre>
 *    unsigned long crc;
 *    crc = midpCRC32Init(data_chunk_1, 60);
 *    crc = midpCRC32Update(data_chunk_2, 60, crc);
 *    crc = midpCRC32Update(data_chunk_3, 60, crc);
 *    crc = midpCRC32Finalize(crc);
 * </pre>
 *
 * <p><tt>crc</tt> now contains a CRC32 checksum of the 180 data
 * bytes.
 *
 * <p><b>Note:</b> You can call <tt>midpCRC32Init</tt> followed
 * directly by <tt>midpCRC32Finalize</tt> if the data buffer being
 * checked is complete instead of partial. The 
 * <tt>midpCRC32Finalize</tt> function is not re-entry safe, however.
 *
 * ##include &lt;&gt;
 * @{
 */


/**
 * This function starts a CRC32 checksum calculation.
 * 
 * After the checksum has been computed over all the data, call
 * midpCRC32Finalize on the checksum.  It is OK to call
 * midpCRC32Finalize immediately after midpCRC32Init if all of the
 * data to be checksummed is contained in the data argument of the
 * midpCRC32Init call.
 *
 * @param data The bytes for updating the checksum
 * @param length The length in bytes of 'data'
 *
 * @return An in progress CRC calculation.
 */
unsigned long midpCRC32Init(unsigned char *data, int length);

/**
 * This function updates an in-progress CRC32 checksum calculation.
 * 
 * After the checksum has been computed over all the data, call
 * midpCRC32Finalize on the checksum.  
 *
 * @param data The bytes for updating the checksum
 * @param length The length in bytes of 'data'
 * @param iv The in-progress checksum returned by a previous call
 * to CRC32.  
 *
 * @return An in-progress CRC calculation.
 */
unsigned long midpCRC32Update(unsigned char *data, int length, 
			   unsigned long iv);

/**
 * This function finalizes a CRC32 checksum calculation.
 *
 * @param crc The in progress checksum created by
 *            a call to midpCRC32Init and zero or more 
 *            calls to midpCRC32Update.
 * @return A final CRC32 checksum
 */
unsigned long midpCRC32Finalize(unsigned long crc); 

/* @} */

#ifdef __cplusplus
}
#endif

#endif /* _MIDP_UTIL_CRC_H_ */


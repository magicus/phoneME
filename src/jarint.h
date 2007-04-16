/*
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
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: JAR file reader.
 * FILE:      jarint.h
 * OVERVIEW:  Internal declarations for JAR file reader. This file should
 *            be included only by files in the core KVM.
 *=======================================================================*/

#ifndef _JAR_INT_H_
#define _JAR_INT_H_

/*
 * indicates whether JAR inflater uses standard i/o during decompression
 */
#define JAR_INFLATER_USES_STDIO 1

/*
 * The HuffmanCodeTable structure contains the dynamic huffman codes for
 * the Code Length Codes or the Distance Codes. The structure is
 * dynamically allocated. We just allocate enough space to contain all
 * possible codes.
 */

typedef struct HuffmanCodeTableHeader {
    unsigned short quickBits;    /* quick bit size */
    unsigned short maxCodeLen;    /* Max number of bits in any code */
} HuffmanCodeTableHeader;

/* If this bit is set in a huffman entry, it means that this is not
 * really an entry, but a pointer into the long codes table.
 * The remaining 15 bits is the offset (in bytes) from the table header
 * to first "long" entry representing this item.
 */

#define HUFFINFO_LONG_MASK 0x8000 /*  high bit set */

#define MAX_QUICK_CXD  6
#define MAX_QUICK_LXL  9

/* For debugging, the following can be set to the name of a file (in quotes).
 * If we are decompressing something that is the exact same length as that
 * file, this will check to make sure that the decompressed bytes look
 * exactly the same as the bytes in the specified file.
 * java/lang/String.class is a particularly useful file to try.
 *
 */

#ifndef inflateVerbose
#define inflateVerbose 0
#endif

#if INCLUDEDEBUGCODE
    static void ziperr(const char * msg);
#   define ASSERT(x) assert((x))
#else
#   define ziperr(msg)
#   define ASSERT(x) (void)0
#endif

/* A normal sized huffman code table with a 9-bit quick bit */
typedef struct HuffmanCodeTable { 
    struct HuffmanCodeTableHeader h;
    /* There are 1 << quickBit entries.  512 is just an example. 
     * For each entry:
     *     If the high bit is 0:
     *        Next 11 bits give the character
     *        Low   4 bits give the actual number of bits used
     *     If the high bit is 1:
     *        Next 15 bits give the offset (in bytes) from the header to 
     *        the first long entry dealing with this long code.
     */
    unsigned short entries[512];
} HuffmanCodeTable;

/* A small sized huffman code table with a 9-bit quick bit.  We have
 * this so that we can initialize fixedHuffmanDistanceTable in jartables.h
 */
typedef struct shortHuffmanCodeTable { 
    struct HuffmanCodeTableHeader h;
    unsigned short entries[32];
} shortHuffmanCodeTable;

typedef struct inflaterState {
    /* The input stream */
    JarCompressedType inFile;

    int inRemaining;        /* Number of bytes left that we can read */
    unsigned int inDataSize;    /* Number of good bits in inData */
    unsigned long inData;    /* Low inDataSize bits are from stream.
                 * High unused bits must be zero 
                 */
    /* The output stream */
    unsigned char *out;        /* Current output position */
    unsigned char *outStart;    /* Start and end of output buffer */
    unsigned char *outEnd;
#ifdef JAR_DEBUG_FILE  
    unsigned char *jarDebugBytes;
#endif
} inflaterState;

/*=========================================================================
 * Macros used internally
 *=======================================================================*/

/* Call this macro to make sure that we have at least "j" bits of
 * input available
 */

#define NEEDBITS(j) {  \
      while (inDataSize < (j)) {                                \
           inData |= ((unsigned long)NEXTBYTE) << inDataSize; \
       inRemaining--; inDataSize += 8;                      \
      }                                     \
      ASSERT(inDataSize <= 32);                     \
}

/* Return (without consuming) the next "j" bits of the input */
#define NEXTBITS(j) \
       (ASSERT((j) <= inDataSize), inData & ((1 << (j)) - 1))

/* Consume (quietly) "j" bits of input, and make them no longer available
 * to the user
 */
#define DUMPBITS(j) {                                          \
       ASSERT((j) <= inDataSize);                       \
       inData >>= (j);                                    \
       inDataSize -= (j);                                 \
    }  

/* Read bits from the input stream and decode it using the specified
 * table.  The resulting decoded character is placed into "result".
 * If there is a problem, we goto "errorLabel"
 *
 * For speed, we assume that quickBits = table->h.quickBits and that
 * it has been cached into a variable.
 */

#define GET_HUFFMAN_ENTRY(table, quickBits, result, errorLabel) { \
    unsigned int huff = table->entries[NEXTBITS(quickBits)];      \
    if (huff & HUFFINFO_LONG_MASK) {                   \
        long delta = (huff & ~HUFFINFO_LONG_MASK);                \
        unsigned short *table2 = (unsigned short *)((char *)table + delta); \
    huff = table2[NEXTBITS(table->h.maxCodeLen) >> quickBits]; \
    }                                                              \
    if (huff == 0) { goto errorLabel; }                   \
    DUMPBITS(huff & 0xF);                          \
    result = huff >> 4;                                            \
    }

#if JAR_INFLATER_USES_STDIO
#   define LOAD_INFILE_IF_NECESSARY
#   define STORE_INFILE_IF_NECESSARY
#   define INITIALIZE_INFILE_IF_NECESSARY  = state->inFile
#   define NEXTBYTE                        fgetc(inFile)
#   define COPY_N_BYTES(buffer, n)         (fread(buffer, 1, n, inFile) == n)

#else 
#   define LOAD_INFILE_IF_NECESSARY        inFile = state->inFile;
#   define STORE_INFILE_IF_NECESSARY       state->inFile = inFile;
#   define INITIALIZE_INFILE_IF_NECESSARY  
#   define NEXTBYTE                        (*inFile++)
#   define COPY_N_BYTES(buffer, n)         \
                           (memcpy(buffer, inFile, n), inFile += n, TRUE)
#endif

/* Copy values from the inflaterState structure to local variables */
#define LOAD_IN                       \
    LOAD_INFILE_IF_NECESSARY          \
    inData = state->inData;           \
    inDataSize = state->inDataSize;   \
    inRemaining = state->inRemaining; 

/* Copy values from local variables back to the inflaterState structure */
#define STORE_IN                      \
    STORE_INFILE_IF_NECESSARY         \
    state->inData = inData;           \
    state->inDataSize = inDataSize;   \
    state->inRemaining = inRemaining; 

#define DECLARE_IN_VARIABLES \
    register JarCompressedType inFile INITIALIZE_INFILE_IF_NECESSARY; \
    register unsigned long inData;         \
    register unsigned int inDataSize;      \
    register long inRemaining;             \

#define LOAD_OUT out = state->out;

#define STORE_OUT state->out = out;

#define DECLARE_OUT_VARIABLES \
    register unsigned char *out;

#endif /* JAR_INT_H_ */

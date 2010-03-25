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

#ifndef _INFLATE_INT_H_
#define _INFLATE_INT_H_

/**
 * @file
 *
 * Utility functions for inflating file data.
 */

typedef long (*GetBytesFunctionType)(void* state, unsigned char* buffer,
                                long numberOfChars);

/* returns a memory handle, call addrFromHandle to use */
typedef void* (*MallocBytesFunctionType)(void* state, int size);

/* handle, is a memory handle */
typedef void (*FreeBytesFunctionType)(void* state, void* handle);

/* This function is to support heaps that compact memory. */
typedef void* (*AddrFromHandleFunctionType)(void* state, void* handle);

/*
 * The HuffmanCodeTable structure contains the dynamic huffman codes for
 * the Code Length Codes or the Distance Codes. The structure is
 * dynamically allocated. We just allocate enough space to contain all
 * possible codes.
 */
typedef struct HuffmanCodeTableHeader {
    unsigned short quickBits;   /* quick bit size */
    unsigned short maxCodeLen;  /* Max number of bits in any code */
} HuffmanCodeTableHeader;

/* If this bit is set in a huffman entry, it means that this is not
 * really an entry, but a pointer into the long codes table.
 * The remaining 15 bits is the offset (in bytes) from the table header
 * to first "long" entry representing this item.
 */
#define HUFFINFO_LONG_MASK 0x8000 /*  high bit set */

#define MAX_QUICK_CXD  6
#define MAX_QUICK_LXL  9

#if !defined(ASSERT)
#    if 0
#        define ASSERT(x) assert((x))
#    else
#        define ASSERT(x) (void)0
#    endif
#endif

/*=========================================================================
 * JAR file reader defines and macros
 *=======================================================================*/

#define BTYPE_NO_COMPRESSION 0x00  
#define BTYPE_FIXED_HUFFMAN  0x01  /* Fixed Huffman Code */
#define BTYPE_DYNA_HUFFMAN   0x02  /* Dynamic Huffman code */
#define BTYPE_INVALID        0x03  /* Invalid code */

#define MAX_BITS 15   /* Maximum number of codes in Huffman Code Table */

#define LITXLEN_BASE 257

#define INFLATEBUFFERSIZE 256

/* A normal sized huffman code table with a 9-bit quick bit */
typedef struct _HuffmanCodeTable {
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

typedef struct _InflaterState {
    /* The input stream */
    void* fileState;               /* The state information */
    GetBytesFunctionType getBytes;

    void* heapState;
    MallocBytesFunctionType mallocBytes;
    FreeBytesFunctionType freeBytes;
    AddrFromHandleFunction addrFromHandle;

    int inRemaining;            /* Number of bytes left that we can read */
    unsigned int inDataSize;    /* Number of good bits in inData */
    unsigned long inData;       /* Low inDataSize bits are from stream. */
                                /* High unused bits must be zero */
    /* The output buffer */
    unsigned char* outBuffer;
    unsigned long outOffset;
    unsigned long outLength;
    int outBufferIsAHandle; /* non-zero if decompBuffer is mem handle that
                       must be given to heapObj.addrFromHandle before using */

    int inflateBufferIndex;
    int inflateBufferCount;
    unsigned char inflateBuffer[INFLATEBUFFERSIZE];
} InflaterState;

/*=========================================================================
 * Macros used internally
 *=======================================================================*/

#define NEXTBYTE ((state->inflateBufferCount)-- > 0 ? (unsigned long)(state->inflateBuffer[state->inflateBufferIndex++]) : \
    ((state->inflateBufferCount = getBytes(fileState, state->inflateBuffer, INFLATEBUFFERSIZE)) > 0 ? \
    (state->inflateBufferIndex = 1, state->inflateBufferCount--, (unsigned long)(state->inflateBuffer[0])) : 0xff))

/* Call this macro to make sure that we have at least "j" bits of
 * input available
 */
#define NEEDBITS(j) {                                         \
      while (inDataSize < (j)) {                              \
           inData |= ((unsigned long)NEXTBYTE) << inDataSize; \
           inRemaining--; inDataSize += 8;                    \
      }                                                       \
      ASSERT(inDataSize <= 32);                               \
}

/* Return (without consuming) the next "j" bits of the input */
#define NEXTBITS(j) \
       (ASSERT((j) <= inDataSize), inData & ((1 << (j)) - 1))

/* Consume (quietly) "j" bits of input, and make them no longer available
 * to the user
 */
#define DUMPBITS(j) {                                         \
       ASSERT((j) <= inDataSize);                             \
       inData >>= (j);                                        \
       inDataSize -= (j);                                     \
    }  

/* Read bits from the input stream and decode it using the specified
 * table.  The resulting decoded character is placed into "result".
 * If there is a problem, we set error and break or of the loop.
 *
 * For speed, we assume that quickBits = table->h.quickBits and that
 * it has been cached into a variable.
 */
#define GET_HUFFMAN_ENTRY(table, quickBits, result) {  \
    unsigned int huff = table->entries[NEXTBITS(quickBits)];       \
    if (huff & HUFFINFO_LONG_MASK) {                               \
        long delta = (huff & ~HUFFINFO_LONG_MASK);                 \
        unsigned short *table2 = (unsigned short *)((char *)table + delta); \
        huff = table2[NEXTBITS(table->h.maxCodeLen) >> quickBits]; \
    }                                                              \
    if (huff == 0) {                                               \
        error = INFLATE_HUFFMAN_ENTRY_ERROR;                       \
        break;                                                     \
    }                                                              \
    DUMPBITS(huff & 0xF);                                          \
    result = huff >> 4;                                            \
}

#define NEXTBYTE ((state->inflateBufferCount)-- > 0 ? (unsigned long)(state->inflateBuffer[state->inflateBufferIndex++]) : \
    ((state->inflateBufferCount = getBytes(fileState, state->inflateBuffer, INFLATEBUFFERSIZE)) > 0 ? \
    (state->inflateBufferIndex = 1, state->inflateBufferCount--, (unsigned long)(state->inflateBuffer[0])) : 0xff))

#define DECLARE_IN_VARIABLES                         \
    register void* fileState = state->fileState;     \
    GetBytesFunctionType getBytes = state->getBytes; \
    register unsigned long inData;                   \
    register unsigned int inDataSize;                \
    register long inRemaining;

/* Copy values from the inflaterState structure to local variables */
#define LOAD_IN                       \
    inData = state->inData;           \
    inDataSize = state->inDataSize;   \
    inRemaining = state->inRemaining; \

/* Copy values from local variables back to the inflaterState structure */
#define STORE_IN                      \
    state->inData = inData;           \
    state->inDataSize = inDataSize;   \
    state->inRemaining = inRemaining; 

#define DECLARE_OUT_VARIABLES                              \
    register unsigned char *outBuffer = state->outBuffer; \
    register unsigned long outLength = state->outLength;   \
    register unsigned long outOffset;

#define LOAD_OUT outOffset = state->outOffset;

#define STORE_OUT state->outOffset = outOffset;

#endif /* INFLATE_INT_H_ */



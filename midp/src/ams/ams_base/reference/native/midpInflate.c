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

#include <string.h>

#include <jar.h>
#include <midpInflateint.h>
#include <midpInflatetables.h>

static int decodeDynamicHuffmanTables(InflaterState *state,
                                      void** lcodesPtr,
                                      void** dcodesPtr);

static int makeCodeTable(InflaterState *state,
                         unsigned char *codelen,
                         unsigned numElems,
                         unsigned maxQuickBits,
                         void** result);

static int inflateHuffman(InflaterState *state, int fixedHuffman);
static int inflateStored(InflaterState *state);

#define INFLATER_EXTRA_BYTES 4

/**
 * Inflates the data in a file.
 * <p>
 * NOTE:
 *    The caller of this method must insure that this function can safely
 *    up to INFLATER_EXTRA_BYTES beyond compData + compLen without causing
 *    any problems.
 *    The inflater algorithm occasionally reads one more byte than it needs
 *    to.  But it double checks that it doesn't actually care what's in that
 *    extra byte.</p>
 *
 * @param fileObj File object for reading the compressed data with the
 *                current file position set to the beginning of the data
 * @param heapManObj Heap manager object for temp data
 * @param method Compression method
 * @param compLen Length of the compressed data
 * @param decompBuffer where to store the uncompressed data
 * @param decompLen Expected length of the uncompressed data
 * @param bufferIsAHandle non-zero if decompBuffer is mem handle that must be
 *        given to heapObj.addrFromHandle before using
 *
 * @return TRUE if the data was encoded in a supported <method> and the
 *                  size of the decoded data is exactly the same as <decompLen>
 *               FALSE if an error occurs
 */
int inflateData(FileObj* fileObj, HeapManObj* heapManObj, int compLen,
                unsigned char* decompBuffer, int decompLen,
                int bufferIsAHandle) {
    /* The macros LOAD_IN, LOAD_OUT,etc. use a variable called "state" */
    InflaterState stateStruct;
    InflaterState* state = &stateStruct;
    int result = 0;

    state->outBuffer = decompBuffer;
    state->outOffset = 0;
    state->outLength = decompLen;
    state->outBufferIsAHandle = bufferIsAHandle;

    state->fileState = fileObj->state;
    state->getBytes = fileObj->read;

    state->heapState = heapManObj->state;
    state->mallocBytes = heapManObj->alloc;
    state->freeBytes = heapManObj->free;
    state->addrFromHandle = heapManObj->addrFromHandle;

    state->inData = 0;
    state->inDataSize = 0;
    state->inRemaining = compLen + INFLATER_EXTRA_BYTES;

    state->inflateBufferIndex = 0;
    state->inflateBufferCount = 0;

    for (; ; ) {
        int type;
        DECLARE_IN_VARIABLES

        LOAD_IN;
        NEEDBITS(3);
        type = NEXTBITS(3);
        DUMPBITS(3);
        STORE_IN;

        switch (type >> 1) {
        default:
        case BTYPE_INVALID:
            result = INFLATE_INVALID_BTYPE;
            break;

        case BTYPE_NO_COMPRESSION:
            result = inflateStored(state);
            break;

        case BTYPE_FIXED_HUFFMAN:
            result = inflateHuffman(state, 1);
            break;

        case BTYPE_DYNA_HUFFMAN:
            result = inflateHuffman(state, 0);
            break;
        }

        if (result != 0) {
            break;
        }

        if (type & 1) {
            if (state->inRemaining + (state->inDataSize >> 3) != 
                    INFLATER_EXTRA_BYTES) {
                result = INFLATE_INPUT_BIT_ERROR;
                break;
            }

            if (state->outOffset != state->outLength) {
                result = INFLATE_OUTPUT_BIT_ERROR;
                break;
            }

            /* Success */
            break;
        }
    }

    return result;
}

static int inflateStored(InflaterState *state) {
    DECLARE_IN_VARIABLES
    DECLARE_OUT_VARIABLES
    long len, nlen;

    LOAD_IN; LOAD_OUT;

    DUMPBITS(inDataSize & 7);   /* move to byte boundary */
    NEEDBITS(32)
    len = NEXTBITS(16);
    DUMPBITS(16);
    nlen = NEXTBITS(16);
    DUMPBITS(16);

    ASSERT(inDataSize == 0);

    if (len + nlen != 0xFFFF) {
        return INFLATE_BAD_LENGTH_FIELD;
    } else if (inRemaining < len) {
        return INFLATE_INPUT_OVERFLOW;
    } else if (outOffset + len > outLength) {
        return INFLATE_OUTPUT_OVERFLOW;
    } else {
        int count;

        if (state->outBufferIsAHandle) {
            /* This is to support heaps with memory compaction. */
            outBuffer = state->addrFromHandle(state->heapState,
                                              state->outBuffer);
        }

        while (len > 0) {
            if (state->inflateBufferCount > 0) {
                /* we have data buffered, copy it first */
                memcpy(&outBuffer[outOffset],
                       &(state->inflateBuffer[state->inflateBufferIndex]),
                       (count = (state->inflateBufferCount <= len ?
                                 state->inflateBufferCount : len)));
                len -= count;
                (state->inflateBufferCount) -= count;
                (state->inflateBufferIndex) += count;
                outOffset += count;
                inRemaining -= count;
            }

            if (len > 0) {
                /* need more, refill the buffer */
                outBuffer[outOffset++] = (unsigned char)(NEXTBYTE);
                len--;
                inRemaining--;
            }
        }
    }

    STORE_IN;
    STORE_OUT;
    return 0;
}

static int inflateHuffman(InflaterState *state, int fixedHuffman) {
    int error = 0;
    DECLARE_IN_VARIABLES
    DECLARE_OUT_VARIABLES

    unsigned int quickDataSize = 0;
    unsigned int quickDistanceSize = 0;
    unsigned int code;
    unsigned int litxlen;
    void* lcodesMemHandle = NULL;
    void* dcodesMemHandle = NULL;
    HuffmanCodeTable* lcodes = NULL;
    HuffmanCodeTable* dcodes = NULL;

    if (!fixedHuffman) {
        error = decodeDynamicHuffmanTables(state, &lcodesMemHandle,
                                           &dcodesMemHandle);
        if (error != 0) {
            return error;
        }

        /* This is to support heaps with memory compaction. */
        lcodes = state->addrFromHandle(state->heapState,
                                       lcodesMemHandle);
        dcodes = state->addrFromHandle(state->heapState,
                                       dcodesMemHandle);

        quickDataSize = lcodes->h.quickBits;
        quickDistanceSize = dcodes->h.quickBits;
    }

    LOAD_IN;
    LOAD_OUT;

    if (state->outBufferIsAHandle) {
        /* This is to support heaps with memory compaction. */
        outBuffer = state->addrFromHandle(state->heapState,
                                          state->outBuffer);
    }

    for (; ; ) {
        if (inRemaining < 0) {
            error = INFLATE_EARLY_END_OF_INPUT;
            break;
        }

        NEEDBITS(MAX_BITS + MAX_ZIP_EXTRA_LENGTH_BITS);

        if (fixedHuffman) {
            /*   literal (hex)
             * 0x100 - 0x117   7   0.0000.00   -  0.0101.11
             *     0 -    8f   8   0.0110.000  -  1.0111.111
             *   118 -   11f   8   1.1000.000  -  1.1000.111
             *    90 -    ff   9   1.1001.0000 -  1.1111.1111
             */

            /* Get 9 bits, and reverse them. */
            code = NEXTBITS(9);
            code = REVERSE_9BITS(code);

            if (code <  0x060) {
                /* A 7-bit code  */
                DUMPBITS(7);
                litxlen = 0x100 + (code >> 2);
            } else if (code < 0x190) {
                DUMPBITS(8);
                litxlen = (code >> 1) + ((code < 0x180) ? (0x000 - 0x030)
                                         : (0x118 - 0x0c0));
            } else {
                DUMPBITS(9);
                litxlen = 0x90 + code - 0x190;
            }
        } else {
            GET_HUFFMAN_ENTRY(lcodes, quickDataSize, litxlen);
        }

        if (litxlen <= 255) {
            if (outOffset < outLength) {
                outBuffer[outOffset] = litxlen;
                outOffset++;
            } else {
                /* success */
                break;
            }
        } else if (litxlen == 256) {               /* end of block */
            /* success */
            break;
        } else if (litxlen > 285) {
            error = INFLATE_INVALID_LITERAL_OR_LENGTH;
            break;
        } else {
            unsigned int n = litxlen - LITXLEN_BASE;
            unsigned int length = ll_length_base[n];
            unsigned int moreBits = ll_extra_bits[n];
            unsigned int d0, distance;

            /* The NEEDBITS(..) above took care of this */
            length += NEXTBITS(moreBits);
            DUMPBITS(moreBits);

            NEEDBITS(MAX_BITS);
            if (fixedHuffman) {
                d0 = REVERSE_5BITS(NEXTBITS(5));
                DUMPBITS(5);
            } else {
                GET_HUFFMAN_ENTRY(dcodes, quickDistanceSize, d0);
            }

            if (d0 > MAX_ZIP_DISTANCE_CODE) {
                error = INFLATE_BAD_DISTANCE_CODE;
                break;
            }

            NEEDBITS(MAX_ZIP_EXTRA_DISTANCE_BITS)
            distance = dist_base[d0];
            moreBits = dist_extra_bits[d0];
            distance += NEXTBITS(moreBits);
            DUMPBITS(moreBits);

            if (outOffset < distance) {
                error = INFLATE_COPY_UNDERFLOW;
                break;
            } else if (outOffset + length > outLength) {
                error = INFLATE_OUTPUT_OVERFLOW;
                break;
            } else {
                unsigned long prev = outOffset - distance;
                unsigned long end = outOffset + length;
                unsigned char value;
                while (outOffset != end) {
                    value = outBuffer[prev];
                    outBuffer[outOffset] = value;
                    outOffset++;
                    prev++;
                }
            }
        }
    }

    STORE_IN;
    STORE_OUT;

    if (!fixedHuffman) {
        state->freeBytes(state->heapState, lcodesMemHandle);
        state->freeBytes(state->heapState, dcodesMemHandle);
    }

    return error;
}

/*=========================================================================
 * FUNCTION:  decodeDynamicHuffmanTables
 * TYPE:      Huffman code Decoding
 * INTERFACE:
 *   parameters: inData Pointer, Mask Pointer, Max InLimit,
 *               literal/length codes Pointer, Decodes Pointer
 *   returns:    TRUE if successful in decoding or
 *               FALSE if an error occurs
 *=======================================================================*/

static int decodeDynamicHuffmanTables(InflaterState *state,
                                      void** lcodesMemHandle,
                                      void** dcodesMemHandle) {
    DECLARE_IN_VARIABLES

    void* ccodesMemHandle;
    HuffmanCodeTable *ccodes = NULL; 

    int hlit, hdist, hclen;
    int i;
    unsigned int quickBits;
    unsigned char codelen[286 + 32];
    unsigned char *codePtr, *endCodePtr;
    int error = 0;
    LOAD_IN;

    /* 5 Bits: hlit,  # of Literal/Length codes  (257 - 286) 
     * 5 Bits: hdist, # of Distance codes        (1 - 32) 
     * 4 Bits: hclen, # of Code Length codes     (4 - 19) 
     */
    NEEDBITS(14);
    hlit = 257 + NEXTBITS(5);
    DUMPBITS(5);
    hdist = 1 + NEXTBITS(5);
    DUMPBITS(5);
    hclen = 4 + NEXTBITS(4);
    DUMPBITS(4);

    /*
     *  hclen x 3 bits: code lengths for the code length
     *  alphabet given just above, in the order: 16, 17, 18,
     *  0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
     *
     *  These code lengths are interpreted as 3-bit integers
     *  (0-7); as above, a code length of 0 means the
     *  corresponding symbol (literal/length or distance code
     *  length) is not used.
     */
    memset(codelen, 0x0, 19);
    for (i=0; i<hclen; i++) {
        NEEDBITS(3);    
        if (inRemaining < 0) {
            return INFLATE_CODE_TABLE_LENGTH_ERROR;
        }

        codelen[(int)ccode_idx[i]] = (unsigned char)(NEXTBITS(3));
        DUMPBITS(3);
    }

    error = makeCodeTable(state, codelen, 19, MAX_QUICK_CXD, &ccodesMemHandle);

    /* DANGER:  ccodes is a heap object.   It can become
     * unusable anytime we allocate from the heap.
     */

    if (error != 0) {
        return error;
    }

    /* This is to support heaps with memory compaction. */
    ccodes = state->addrFromHandle(state->heapState, ccodesMemHandle);

    quickBits = ccodes->h.quickBits;

    /*
     * hlit code lengths for the literal/length alphabet,
     * encoded using the code length Huffman code.
     *
     * hdist code lengths for the distance alphabet,
     * encoded using the code length Huffman code.
     *
     * The code length repeat codes can cross from the literal/length
     * to the code length alphabets.  In other words, all code lengths form
     * a single sequence of hlit + hdist
     */

    memset(codelen, 0x0, sizeof(codelen));
    for (   codePtr = codelen, endCodePtr = codePtr + hlit + hdist;
        codePtr < endCodePtr; ) {

        int val;

        if (inRemaining < 0) {
            error = INFLATE_EARLY_END_OF_CCTABLE_INPUT;
            break;
        }

        NEEDBITS(MAX_BITS + 7); /* 7 is max repeat bits below */
        GET_HUFFMAN_ENTRY(ccodes, quickBits, val);

            /*
             *  0 - 15: Represent code lengths of 0 - 15
             *      16: Copy the previous code length 3 - 6 times.
             *          3 + (2 bits of length)
             *      17: Repeat a code length of 0 for 3 - 10 times.
             *          3 + (3 bits of length)
             *      18: Repeat a code length of 0 for 11 - 138 times
             *          11 + (7 bits of length)
             */
        if (val <= 15) {
            *codePtr++ = val;
        } else if (val <= 18) {
            unsigned repeat  = (val == 18) ? 11 : 3;
            unsigned bits    = (val == 18) ? 7 : (val - 14);

            repeat += NEXTBITS(bits); /* The NEEDBITS is above */
            DUMPBITS(bits);

            if (codePtr + repeat > endCodePtr) {
                error  = INFLATE_BAD_REPEAT_CODE;
                break;
            }

            if (val == 16) {
                if (codePtr == codelen) {
                    error = INFLATE_BAD_REPEAT_CODE;
                    break;
                }
                memset(codePtr, codePtr[-1], repeat);
            } else {
                    /* The values have already been set to zero, above, so
                     * we don't have to do anything */
            }
            codePtr += repeat;
        } else {
            error = INFLATE_BAD_CODELENGTH_CODE;
            break;
        }
    }

    /* ccodes, at this point, is unusable */
    state->freeBytes(state->heapState, ccodesMemHandle);

    while (error == 0) {

        error = makeCodeTable(state, codelen, hlit, MAX_QUICK_LXL,
                lcodesMemHandle);
        if (error != 0) {
            break;
        }

        error = makeCodeTable(state, codelen + hlit, hdist, MAX_QUICK_CXD,
                              dcodesMemHandle);
        if (error != 0) {
            break;
        }

        STORE_IN;

        /* success */
        return 0;
    }

    /* error */
    state->freeBytes(state->heapState, *dcodesMemHandle);
    *dcodesMemHandle = NULL;
    state->freeBytes(state->heapState, *lcodesMemHandle);
    *lcodesMemHandle = NULL;

    return error;
}

/**
 * Huffman code table creation
 *
 * @param codeLen code length, number of elements, type of the alphabet,
 * @param maxQuickBits
 * @param result mem handle to Huffman code table created if successful or
 *               NULL if an error occurs
 *
 * @return 0 for success or an error
 */

static int makeCodeTable(InflaterState *state,
              unsigned char *codelen, /* Code lengths */
              unsigned numElems,      /* Number of elements of the alphabet */
              unsigned maxQuickBits,  /* If the length of a code is longer than
                                       * <maxQuickBits> number of bits, the
                                       * code is stored in the sequential
                                       * lookup table instead of the quick 
                                       * lookup array. */
              void** result           /* handle to heap memory*/)
{
    unsigned int bitLengthCount[MAX_BITS + 1];
    unsigned int codes[MAX_BITS + 1];
    unsigned bits, minCodeLen = 0, maxCodeLen = 0;
    const unsigned char *endCodeLen = codelen + numElems;
    unsigned int code, quickMask;
    unsigned char *p;
    void* tableMemHandle;
    HuffmanCodeTable* table;
    int mainTableLength, longTableLength, numLongTables;
    int tableSize;
    int j;
    unsigned short *nextLongTable;

    *result = NULL;

    /* Count the number of codes for each code length */
    memset(bitLengthCount, 0, sizeof(bitLengthCount));
    for (p = codelen; p < endCodeLen; p++) {
        bitLengthCount[*p]++;
    }

    if (bitLengthCount[0] == numElems) {
        // Empty code table is allowed!
    }

    /* Find the minimum and maximum.  It's faster to do it in a separate
     * loop that goes 1..MAX_BITS, than in the above loop that looks at
     * every code element */
    code = minCodeLen = maxCodeLen = 0;
    for (bits = 1; bits <= MAX_BITS; bits++) {
        codes[bits] = code;
        if (bitLengthCount[bits] != 0) {
            if (minCodeLen == 0) {
                minCodeLen = bits;
            }
            maxCodeLen = bits;
            code += bitLengthCount[bits] << (MAX_BITS - bits);
        }
    }

    /* Calculate the size of the code table and allocate it. */
    if (maxCodeLen <= maxQuickBits) {
        /* We don't need any subtables.  We may even be able to get
         * away with a table smaller than maxCodeLen
         */
        maxQuickBits = maxCodeLen;
        mainTableLength = (1 << maxCodeLen);
        numLongTables = longTableLength = 0;
    } else {
        mainTableLength = (1 << maxQuickBits);
        numLongTables = (1 << MAX_BITS) - codes[maxQuickBits + 1];
        numLongTables = numLongTables >> (MAX_BITS - maxQuickBits);
        longTableLength = 1 << (maxCodeLen - maxQuickBits);
    }

    ASSERT(mainTableLength == 1 << maxQuickBits);
    tableSize = sizeof(HuffmanCodeTableHeader)
                + (mainTableLength + numLongTables * longTableLength)
                * sizeof(table->entries[0]);

    tableMemHandle = state->mallocBytes(state->heapState, tableSize);

    if (tableMemHandle == NULL) {
        return OUT_OF_MEMORY_ERROR;
    }

    /* This is to support heaps with memory compaction. */
    table = state->addrFromHandle(state->heapState, tableMemHandle);

    nextLongTable = &table->entries[mainTableLength];

    memset(table, 0, tableSize);

    table->h.maxCodeLen   = maxCodeLen;
    table->h.quickBits    = maxQuickBits;

    quickMask = (1 << maxQuickBits) - 1;

    for (p = codelen; p < endCodeLen; p++) {
        unsigned short huff;
        bits = *p;
        if (bits == 0) {
            continue;
        }

        /* Get the next code of the current length */
        code = codes[bits];
        codes[bits] += 1 << (MAX_BITS - bits);
        code = REVERSE_15BITS(code);
        huff = ((p - codelen) << 4) + bits;

        if (bits <= maxQuickBits) {
            unsigned stride = 1 << bits;
            for (j = code; j < mainTableLength; j += stride) {
                table->entries[j] = huff;
            }
        } else {
            unsigned short *thisLongTable;
            unsigned stride = 1 << (bits - maxQuickBits);
            unsigned int prefixCode = code & quickMask;
            unsigned int suffixCode = code >> maxQuickBits;

            if (table->entries[prefixCode] == 0) {
                /* This in the first long code with the indicated prefix.
                 * Create a pointer to the subtable */
                long delta = (char *)nextLongTable - (char *)table;
                table->entries[prefixCode] = 
                (unsigned short)(HUFFINFO_LONG_MASK | delta);
                thisLongTable = nextLongTable;
                nextLongTable += longTableLength;
            } else {
                long delta = table->entries[prefixCode] & ~HUFFINFO_LONG_MASK;
                thisLongTable = (unsigned short *)((char *)table + delta);
            }

            for (j = suffixCode; j < longTableLength; j += stride) {
                thisLongTable[j] = huff;
            }
        }
    }

    ASSERT(nextLongTable == 
           &table->entries[mainTableLength + numLongTables * longTableLength]);

    *result = tableMemHandle;
    return 0;
}



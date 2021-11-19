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
 * SYSTEM:    Verifier 
 * SUBSYSTEM: JAR class loader.
 * FILE:      jar.c
 * OVERVIEW:  Routines for reading contents of a JAR file. The routines
 *            are optimized to reduce code size and run-time memory
 *            requirement so that they can run happily on small devices.
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <stdio.h>
#include <sys/types.h>
#include <stddef.h>
#include <assert.h>

#include <oobj.h>
#include <typedefs.h>
#include <jar.h>
#include <jarint.h>
#include <jartables.h>

/*=========================================================================
 * Globals and extern declarations
 *=======================================================================*/

static bool_t decodeDynamicHuffmanTables(inflaterState *state,
                     HuffmanCodeTable **lcodesPtr,
                     HuffmanCodeTable **dcodesPtr);

static HuffmanCodeTable *makeCodeTable(inflaterState *state,
                       unsigned char *codelen,
                       unsigned numElems,
                       unsigned maxQuickBits);

static bool_t inflateHuffman(inflaterState *state, bool_t fixedHuffman);
static bool_t inflateStored(inflaterState *state);

/*===========================================================================
 * FUNCTION:  inflate
 * TYPE:      jar file decoding
 * OVERVIEW   Used for decoding JAR files given the compressed data source,
 *            compressed data length, buffer to store decompressed data and 
 *            length of decompression buffer. 
 *
 * INTERFACE:
 *   parameters: compressed data source, compressed data length,
 *               buffer to store decompressed data, decompression buffer size
 *   returns:    TRUE if the data was encoded in a supported <method> and the
 *               size of the decoded data is exactly the same as <decompLen>
 *               FALSE if an error occurs
 * NOTE:
 *    The caller of this method must insure that this function can safely get
 *    up to INFLATER_EXTRA_BYTES beyond compData + compLen without causing
 *    any problems.
 *    The inflater algorithm occasionally reads one more byte than it needs
 *    to.  But it double checks that it doesn't actually care what's in that
 *    extra byte.
 *===========================================================================*/

/* Change some definitions so that this compiles nicely, even if it is
 * compiled as part of something that requires real malloc() and free()
 */
#if !JAR_INFLATER_INSIDE_KVM
#  undef START_TEMPORARY_ROOTS
#  undef END_TEMPORARY_ROOTS
#  undef MAKE_TEMPORARY_ROOT
#  define START_TEMPORARY_ROOTS
#  define END_TEMPORARY_ROOTS
#  define MAKE_TEMPORARY_ROOT(x)
#  define mallocBytes(x) malloc(x)
#  define freeBytes(x)   if (x == NULL) {} else free(x)
#else
#  define freeBytes(x)
#endif

bool_t 
inflate(JarCompressedType compData, /* compressed data source */
    int compLen,        /* length of compressed data */
    unsigned char *decompData, /* buffer to store decompressed data */
    int decompLen)        /* length of decompression buffer */
{
    inflaterState stateStruct;
    bool_t result;
/* Temporarily define state, so that LOAD_IN, LOAD_OUT, etc. macros work */
#define state (&stateStruct)
    stateStruct.out = decompData;
    stateStruct.outStart = decompData;
    stateStruct.outEnd = decompData + decompLen;

    stateStruct.inFile = compData;
    stateStruct.inData = 0;
    stateStruct.inDataSize = 0;
    stateStruct.inRemaining = compLen + INFLATER_EXTRA_BYTES;

#ifdef JAR_DEBUG_FILE 
    { 
    static int length = 0;
    if (length == 0) {
        struct stat stat_buffer;
        stat(JAR_DEBUG_FILE, &stat_buffer);
        length = stat_buffer.st_size;;
    }
    if (length == decompLen) {
        FILE *f = fopen(JAR_DEBUG_FILE, "rb");
        state->jarDebugBytes = malloc(length);
        fseek(f, 0, SEEK_SET);
        fread(state->jarDebugBytes, sizeof(char), length, f);
        fclose(f);
    } else { 
        state->jarDebugBytes = NULL;
    }
    }
#endif

    for(;;) {
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
        ziperr("Invalid BTYPE");
        result = FALSE;
        break;

        case BTYPE_NO_COMPRESSION:
        result = inflateStored(state);
        break;

        case BTYPE_FIXED_HUFFMAN:
        result = inflateHuffman(state, TRUE);
        break;

        case BTYPE_DYNA_HUFFMAN:
        START_TEMPORARY_ROOTS
           result = inflateHuffman(state, FALSE);
        END_TEMPORARY_ROOTS
        break;
    }

    if (!result || (type & 1)) { 
        break;
    }
    }

    if (state->inRemaining + (state->inDataSize >> 3) != INFLATER_EXTRA_BYTES) { 
    ziperr("Error on the input bits");
    result = FALSE;
    } else if (state->out != state->outEnd) {
    ziperr("Error on the output bits");
    result = FALSE;
    }

#ifdef JAR_DEBUG_FILE
    if (state->jarDebugBytes != NULL) { 
    free(state->jarDebugBytes);
    }
#endif

    /* Remove temporary definition of state defined above */
#undef state

    return result;
}

/*=========================================================================
 * FUNCTION:  inflateStored 
 * TYPE:      Huffman code Decoding helper function
 * OVERVIEW:  Used by inflate() for BTYPE_NO_COMPRESSION.
 * INTERFACE:
 *   parameters: inflaterState: *state 
 *              
 *   returns:    boolean type 
 *=======================================================================*/
static bool_t
inflateStored(inflaterState *state)
{
    DECLARE_IN_VARIABLES
    DECLARE_OUT_VARIABLES
    unsigned len, nlen;

    LOAD_IN; LOAD_OUT;

    DUMPBITS(inDataSize & 7);    /* move to byte boundary */
    NEEDBITS(32)
    len = NEXTBITS(16);
    DUMPBITS(16);
    nlen = NEXTBITS(16);
    DUMPBITS(16);

    ASSERT(inDataSize == 0);

    if (len + nlen != 0xFFFF) {
    ziperr("Bad length field");
    return FALSE;
    } else if ((unsigned) inRemaining < len) {
    ziperr("Input overflow");
    return FALSE;
    } else if (out + len > state->outEnd) {
    ziperr("Output overflow");
    return FALSE;
    } else {
    if (!COPY_N_BYTES(out, len)) { 
        ziperr("Bad Read");
        return FALSE;
    }
    inRemaining -= len;
    out += len;
    }
    STORE_IN;
    STORE_OUT;
    return TRUE;
}

/*=========================================================================
 * FUNCTION:  inflateHuffman
 * TYPE:      Huffman code Decoding helper function
 * OVERVIEW:  Used by inflate() for BTYPE_FIXED_HUFFMAN and BTYPE_DYNA_HUFFMAN.
 * INTERFACE:
 *   parameters: inflaterState: *state 
 *               bool_t: fixedHuffman
 *              
 *   returns:    boolean type 
 *=======================================================================*/
static bool_t
inflateHuffman(inflaterState *state, bool_t fixedHuffman)
{
    DECLARE_IN_VARIABLES
    DECLARE_OUT_VARIABLES
    unsigned char *outStart = state->outStart;
    unsigned char *outEnd = state->outEnd;   

    bool_t noerror = FALSE;
    unsigned int quickDataSize = 0, quickDistanceSize = 0;
    unsigned int code, litxlen;
    HuffmanCodeTable *lcodes, *dcodes;

    if (!fixedHuffman) {
    if (!decodeDynamicHuffmanTables(state, &lcodes, &dcodes)) {
        return FALSE;
    }
    quickDataSize = lcodes->h.quickBits;
    quickDistanceSize = dcodes->h.quickBits;
    }

    LOAD_IN;
    LOAD_OUT;

    for (;;) {
    if (inRemaining < 0) { 
        goto done_loop;
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
        GET_HUFFMAN_ENTRY(lcodes, quickDataSize, litxlen, done_loop);
    }

    if (litxlen <= 255) {
        if (out < outEnd) {
#ifdef JAR_DEBUG_FILE
        if (state->jarDebugBytes && state->jarDebugBytes[out - outStart] != litxlen) {
            ziperr("Dragon single byte");
        }
#endif
        *out++ = litxlen;
        } else {
        goto done_loop;
        }
    } else if (litxlen == 256) {               /* end of block */
        noerror = TRUE;
        goto done_loop;
    } else if (litxlen > 285) {
        ziperr("Invalid literal/length");
        goto done_loop;
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
        GET_HUFFMAN_ENTRY(dcodes, quickDistanceSize, d0, done_loop);
        }

        if (d0 > MAX_ZIP_DISTANCE_CODE) {
        ziperr("Bad distance code");
        goto done_loop;
        }

        NEEDBITS(MAX_ZIP_EXTRA_DISTANCE_BITS)
        distance = dist_base[d0];
        moreBits = dist_extra_bits[d0];
        distance += NEXTBITS(moreBits);
        DUMPBITS(moreBits);

        if (out - distance < outStart) {
        ziperr("copy underflow");
        goto done_loop;
        } else if (out + length > outEnd) {
        ziperr("Output overflow");
        goto done_loop;
        } else {
        unsigned char *prev = out - distance;
        unsigned char *end  = out + length;
        while (out != end) {
#ifdef JAR_DEBUG_FILE            
            if (state->jarDebugBytes 
            && state->jarDebugBytes[out - outStart] != *prev) {
            ziperr("Dragon copy error");
            }
#endif
            *out++ = *prev++;
        }
        }
    }
    }

 done_loop:
    STORE_IN;
    STORE_OUT;

    if (!JAR_INFLATER_INSIDE_KVM && !fixedHuffman) { 
    freeBytes(lcodes);
    freeBytes(dcodes);
    }
    return noerror;
}

/*=========================================================================
 * FUNCTION:  decodeDynamicHuffmanTables
 * TYPE:      Huffman code Decoding
 * OVERVIEW:  Used by inflateHuffman() for decoding dynamic Huffman tables. 
 * INTERFACE:
 *   parameters: inflaterState: *state
 *               HuffmanCodeTable: **lcodesPtr
 *               HuffmanCodeTable: **dcodesPtr
 * 
 *   returns:    TRUE if successful in decoding or
 *               FALSE if an error occurs
 *=======================================================================*/

static bool_t
decodeDynamicHuffmanTables(inflaterState *state,
               HuffmanCodeTable **lcodesPtr,
               HuffmanCodeTable **dcodesPtr) {
    DECLARE_IN_VARIABLES

    HuffmanCodeTable *ccodes = NULL;
    HuffmanCodeTable *lcodes = NULL;
    HuffmanCodeTable *dcodes = NULL;

    int hlit, hdist, hclen;
    int i;
    unsigned int quickBits;
    unsigned char codelen[286 + 32];
    unsigned char *codePtr, *endCodePtr;

    LOAD_IN;

    /* 5 Bits: HLIT, # of Literal/Length codes - 257 (257 - 286) */
    /* 5 Bits: HDIST, # of Distance codes - 1        (1 - 32) */
    /* 4 Bits: HCLEN, # of Code Length codes - 4     (4 - 19) */
    NEEDBITS(14);
    hlit = 257 + NEXTBITS(5);
    DUMPBITS(5);
    hdist = 1 + NEXTBITS(5);
    DUMPBITS(5);
    hclen = 4 + NEXTBITS(4);
    DUMPBITS(4);

    /*
     * (HCLEN + 4) x 3 bits: code lengths for the code length
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
    NEEDBITS(3);        /* 3, plus 7 below */
    if (inRemaining < 0) { 
        goto error;
    }
    codelen[(int)ccode_idx[i]] = (unsigned char) NEXTBITS(3);
    DUMPBITS(3);
    }

    ccodes = makeCodeTable(state, codelen, 19, MAX_QUICK_CXD);
    if (ccodes == NULL) {
    goto error;
    }
    quickBits = ccodes->h.quickBits;

    /*
     * HLIT + 257 code lengths for the literal/length alphabet,
     * encoded using the code length Huffman code.
     *
     * HDIST + 1 code lengths for the distance alphabet,
     * encoded using the code length Huffman code.
     *
     * The code length repeat codes can cross from HLIT + 257 to the
     * HDIST + 1 code lengths.  In other words, all code lengths form
     * a single sequence of HLIT + HDIST + 258 values.
     */

    memset(codelen, 0x0, sizeof(codelen));
    for (   codePtr = codelen, endCodePtr = codePtr + hlit + hdist;
        codePtr < endCodePtr; ) {
    int val;

    if (inRemaining < 0) { 
        goto error;
    }

    NEEDBITS(MAX_BITS + 7);    /* 7 is max repeat bits below */
    GET_HUFFMAN_ENTRY(ccodes, quickBits, val, error);

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
        ziperr("Bad repeat code");
        }

        if (val == 16) {
        if (codePtr == codelen) {
            ziperr("Bad repeat code");
            goto error;
        }
        memset(codePtr, codePtr[-1], repeat);
        } else {
        /* The values have already been set to zero, above, so
         * we don't have to do anything */
        }
        codePtr += repeat;
    } else {
        ziperr("Bad code-length code");
        goto error;
    }
    }

    lcodes = makeCodeTable(state, codelen, hlit, MAX_QUICK_LXL);
    if (lcodes == NULL) {
        goto error;
    }

    dcodes = makeCodeTable(state, codelen + hlit, hdist, MAX_QUICK_CXD);
    if (dcodes == NULL) {
        goto error;
    }

    *lcodesPtr = lcodes;
    *dcodesPtr = dcodes;
    STORE_IN;
    if (!JAR_INFLATER_INSIDE_KVM) { 
    freeBytes(ccodes);
    }
    return TRUE;

error:
    if (!JAR_INFLATER_INSIDE_KVM) { 
    freeBytes(ccodes);
    freeBytes(dcodes);
    freeBytes(lcodes);
    }
    return FALSE;
}

/*=========================================================================
 * FUNCTION:  makeCodeTable
 * TYPE:      Huffman code table creation
 * INTERFACE:
 *   parameters: inflaterState 
 *               code length 
 *               number of elements of the alphabet 
 *               maxQuickBits 
 *   returns:    Huffman code table created if successful or
 *               NULL if an error occurs
 *=======================================================================*/

HuffmanCodeTable * makeCodeTable(
    inflaterState *state,
        unsigned char *codelen,    /* Code lengths */
        unsigned numElems,      /* Number of elements of the alphabet */
        unsigned maxQuickBits)  /* If the length of a code is longer than
                                 * <maxQuickBits> number of bits, the code is
                                 * stored in the sequential lookup table
                                 * instead of the quick lookup array. */
{
    unsigned int bitLengthCount[MAX_BITS + 1];
    unsigned int codes[MAX_BITS + 1];
    unsigned bits, minCodeLen = 0, maxCodeLen = 0;
    const unsigned char *endCodeLen = codelen + numElems;
    unsigned int code, quickMask;
    unsigned char *p;

    HuffmanCodeTable * table;
    int mainTableLength, longTableLength, numLongTables;
    int tableSize;
    int j;

    unsigned short *nextLongTable;

    /* Count the number of codes for each code length */
    memset(bitLengthCount, 0, sizeof(bitLengthCount));
    for (p = codelen; p < endCodeLen; p++) { 
    bitLengthCount[*p]++;
    }

    if (bitLengthCount[0] == numElems) {
    ziperr("Bad code table -- empty");
        return NULL;
    }

    /* Find the minimum and maximum.  It's faster to do it in a separate
     * loop that goes 1..MAX_BITS, than in the above loop that looks at
     * every code element */
    code = minCodeLen = maxCodeLen = 0;
    for (bits = 1; bits <= MAX_BITS; bits++) {
    codes[bits] = code;
    if (bitLengthCount[bits] != 0) {
        if (minCodeLen == 0) minCodeLen = bits;
        maxCodeLen = bits;
        code += bitLengthCount[bits] << (MAX_BITS - bits);
    }
    }

    if (INCLUDEDEBUGCODE) { 
    if (code != (1 << MAX_BITS)) {
        code += (1 << (MAX_BITS - maxCodeLen));
        if (code != (1 << MAX_BITS)) {
        ziperr("Unexpected bit codes");
        }
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
          + (mainTableLength + numLongTables * longTableLength) *
               sizeof(table->entries[0]);
    table = (HuffmanCodeTable*)mallocBytes(tableSize);
    nextLongTable = &table->entries[mainTableLength];
    MAKE_TEMPORARY_ROOT(table);

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
        table->entries[prefixCode] = (unsigned short)(HUFFINFO_LONG_MASK | delta);
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
    if (INCLUDEDEBUGCODE && inflateVerbose > 0) {
        putchar(' ');
        
        for (j = 15; j >= 0; j--) {
        char c = (j >= (signed)bits) ? ' '
            : (code & (1 << j)) ? '1' : '0';
        putchar(c);
        }
        fprintf(stdout, 
            "   Char = %02x, Code = %4x\n", (p - codelen), code);
    }

    }
    ASSERT(nextLongTable == &table->entries[mainTableLength + numLongTables * longTableLength]);

    return table;
}

#if INCLUDEDEBUGCODE

/*=========================================================================
 * FUNCTION:  ziperr 
 * TYPE:      zip error processing function
 * OVERVIEW:  Used by inflate() and other functions for zip error processing. 
 * INTERFACE:
 *   parameters: const char *message 
 *              
 *   returns:    nothing 
 *=======================================================================*/
static void
ziperr(const char *message) {
    fprintf(stderr, "Zip Error: %s\n", message);
}

#endif

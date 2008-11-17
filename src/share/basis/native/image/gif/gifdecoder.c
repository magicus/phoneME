/*
 * @(#)gifdecoder.c	1.30 06/10/10
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
 *
 */

/*
 * NOTE: Currently we use a java version of GifImageDecoder.parseImage()
 * rather than this this JNI version, because with a JIT present, the java
 * version gives better performance.
 */
#if 0

#include <stdio.h>
#include "jni.h"

#define OUTCODELENGTH 4097

/* We use Get/ReleasePrimitiveArrayCritical functions to avoid
 * the need to copy buffer elements.
 *
 * MAKE SURE TO:
 *
 * - carefully insert pairs of RELEASE_ARRAYS and GET_ARRAYS around
 *   callbacks to Java.
 * - call RELEASE_ARRAYS before returning to Java.
 *
 * Otherwise things will go horribly wrong. There may be memory leaks,
 * excessive pinning, or even VM crashes!
 *
 * Note that GetPrimitiveArrayCritical may fail!
 */
#define GET_ARRAYS() \
    prefix  = (short *) \
        (*env)->GetShortArrayElements(env, prefixh, 0); \
    if (prefix == 0) \
        goto out_of_memory; \
    suffix  = (unsigned char *) \
        (*env)->GetByteArrayElements(env, suffixh, 0); \
    if (suffix == 0) \
        goto out_of_memory; \
    outCode = (unsigned char *) \
        (*env)->GetByteArrayElements(env, outCodeh, 0); \
    if (outCode == 0) \
        goto out_of_memory; \
    rasline = (unsigned char *) \
        (*env)->GetByteArrayElements(env, raslineh, 0); \
    if (rasline == 0) \
        goto out_of_memory; \
    block = (unsigned char *) \
        (*env)->GetByteArrayElements(env, blockh, 0); \
    if (block == 0) \
        goto out_of_memory

/* 
 * Note that it is important to check whether the arrays are NULL,
 * because GetPrimitiveArrayCritical might have failed.
 */
#define RELEASE_ARRAYS() \
if (prefix) \
    (*env)->ReleaseShortArrayElements(env, prefixh, prefix, 0); \
if (suffix) \
    (*env)->ReleaseByteArrayElements(env, suffixh, (jbyte*)suffix, 0); \
if (outCode) \
    (*env)->ReleaseByteArrayElements(env, outCodeh, (jbyte*)outCode, 0); \
if (rasline) \
    (*env)->ReleaseByteArrayElements(env, raslineh, (jbyte*)rasline, 0); \
if (block) \
    (*env)->ReleaseByteArrayElements(env, blockh, (jbyte*)block, 0)

#define SAWEXCEPTION() (*env)->ExceptionOccurred(env)

static jmethodID readID = 0;
static jmethodID sendID = 0;
static jfieldID prefixID = 0;
static jfieldID suffixID = 0;
static jfieldID outCodeID = 0;

JNIEXPORT void JNICALL
Java_sun_awt_image_GifImageDecoder_initIDs(JNIEnv *env, jclass this)
{
    if (outCodeID != 0) return;

    readID = (*env)->GetMethodID(env, this, "readBytes", "([BII)I");
    sendID = (*env)->GetMethodID(env, this, "sendPixels",
				 "(IIII[BLjava/awt/image/ColorModel;)I");
    prefixID = (*env)->GetFieldID(env, this, "prefix", "[S");
    suffixID = (*env)->GetFieldID(env, this, "suffix", "[B");
    outCodeID = (*env)->GetFieldID(env, this, "outCode", "[B");
}


JNIEXPORT jboolean JNICALL
Java_sun_awt_image_GifImageDecoder_parseImage(JNIEnv *env, jobject this,
                                              jint relx, jint rely,
                                              jint width, jint height,
                                              jint interlace,
                                              jint initCodeSize,
                                              jbyteArray blockh,
                                              jbyteArray raslineh,
                                              jobject cmh)
{
    /* Patrick Naughton:
     * Note that I ignore the possible existence of a local color map.
     * I'm told there aren't many files around that use them, and the
     * spec says it's defined for future use.  This could lead to an
     * error reading some files.
     *
     * Start reading the image data. First we get the intial code size
     * and compute decompressor constant values, based on this code
     * size.
     *
     * The GIF spec has it that the code size is the code size used to
     * compute the above values is the code size given in the file,
     * but the code size used in compression/decompression is the code
     * size given in the file plus one. (thus the ++).
     *
     * Arthur van Hoff:
     * The following narly code reads LZW compressed data blocks and
     * dumps it into the image data. The input stream is broken up into
     * blocks of 1-255 characters, each preceded by a length byte.
     * 3-12 bit codes are read from these blocks. The codes correspond to
     * entry is the hashtable (the prefix, suffix stuff), and the appropriate
     * pixels are written to the image.
     */

    int clearCode = (1 << initCodeSize);
    int eofCode = clearCode + 1;
    int bitMask;
    int curCode;
    int outCount;

    /* Variables used to form reading data */
    int blockEnd = 0;
    int remain = 0;
    int byteoff = 0;
    int accumbits = 0;
    int accumdata = 0;

    /* Variables used to decompress the data */
    int codeSize = initCodeSize + 1;
    int maxCode = 1 << codeSize;
    int codeMask = maxCode - 1;
    int freeCode = clearCode + 2;
    int code = 0;
    int oldCode = 0;
    unsigned char prevChar = 0;

    /* Temproray storage for decompression */
    short *prefix;
    unsigned char *suffix = NULL;
    unsigned char *outCode = NULL;
    unsigned char *rasline = NULL;
    unsigned char *block = NULL;

    int blockLength = 0;

    /* Variables used for writing pixels */
    int x = width;
    int y = 0;
    int off = 0;
    int passinc = interlace ? 8 : 1;
    int passht = passinc;
    int len;

    jshortArray prefixh = (*env)->GetObjectField(env, this, prefixID);
    jbyteArray suffixh = (*env)->GetObjectField(env, this, suffixID);
    jbyteArray outCodeh = (*env)->GetObjectField(env, this, outCodeID);

    if (blockh == 0 || raslineh == 0 
	|| prefixh == 0 || suffixh == 0 || outCodeh == 0) {
	/* JNU_ThrowNullPointerException(env, 0); */
	return 0;
    }

    if (((*env)->GetArrayLength(env, prefixh) != 4096) ||
	((*env)->GetArrayLength(env, suffixh) != 4096) ||
	((*env)->GetArrayLength(env, outCodeh) != OUTCODELENGTH)) {
	/* JNU_ThrowArrayIndexOutOfBoundsException(env, 0); */
	return 0;
    }

    {
        jclass cmClass = (*env)->GetObjectClass(env, cmh);
        jfieldID map_sizeID = (*env)->GetFieldID(env, cmClass, "map_size", "I");
	bitMask = (*env)->GetIntField(env, cmh, map_sizeID) - 1;
    }

    GET_ARRAYS();

    /* Read codes until the eofCode is encountered */
    for (;;) {
	if (accumbits < codeSize) {
	    /* fill the buffer if needed */
	    while (remain < 2) {
		if (blockEnd) {
		    /* Sometimes we have one last byte to process... */
		    if (remain == 1 && accumbits + 8 >= codeSize) {
			remain--;
			goto last_byte;
		    }

		    RELEASE_ARRAYS();

		    if (off > 0) {
			(*env)->CallIntMethod(env, this, sendID,
					      relx, rely + y,
					      width, passht,
					      raslineh, cmh);
		    }
		    /* quietly accept truncated GIF images */
		    return 1;
		}
		/* move remaining bytes to the beginning of the buffer */
		block[0] = block[byteoff];
		byteoff = 0;

		RELEASE_ARRAYS();
		/* fill the block */

		len = (*env)->CallIntMethod(env, this, readID,
					    blockh, remain, blockLength + 1);
		if (SAWEXCEPTION()) {
		    return 0;
		}

		GET_ARRAYS();

		remain += blockLength;
		if (len > 0) {
		    remain -= (len - 1);
		    blockLength = 0;
		} else {
		    blockLength = block[remain];
		}
		if (blockLength == 0) {
		    blockEnd = 1;
		}
	    }
	    remain -= 2;

	    /* 2 bytes at a time saves checking for accumbits < codeSize.
	     * We know we'll get enough and also that we can't overflow
	     * since codeSize <= 12.
	     */
	    accumdata += (block[byteoff++] & 0xff) << accumbits;
	    accumbits += 8;
	last_byte:
	    accumdata += (block[byteoff++] & 0xff) << accumbits;
	    accumbits += 8;
	}

	/* Compute the code */
	code = accumdata & codeMask;
	accumdata >>= codeSize;
	accumbits -= codeSize;

	/*
	 * Interpret the code
	 */
	if (code == clearCode) {
	    /* Clear code sets everything back to its initial value, then
	     * reads the immediately subsequent code as uncompressed data.
	     */

	    /* Note that freeCode is one less than it is supposed to be,
	     * this is because it will be incremented next time round the loop
	     */
	    freeCode = clearCode + 1;
	    codeSize = initCodeSize + 1;
	    maxCode = 1 << codeSize;
	    codeMask = maxCode - 1;

	    /* Continue if we've NOT reached the end, some Gif images
	     * contain bogus codes after the last clear code.
	     */
	    if (y < height) {
		continue;
	    }

	    /* pretend we've reached the end of the data */
	    code = eofCode;
	}

	if (code == eofCode) {
	    /* make sure we read the whole block of pixels. */
	flushit:
	    while (!blockEnd) {

		RELEASE_ARRAYS();

		if ((*env)->CallIntMethod(env, this, readID,
					  blockh, 0, blockLength + 1) != 0
		    || (*env)->ExceptionOccurred(env)) {
		    /* quietly accept truncated GIF images */
		    return (!(*env)->ExceptionOccurred(env));
		}

		GET_ARRAYS();

		blockLength = block[blockLength];
		blockEnd = (blockLength == 0);
	    }

	    RELEASE_ARRAYS();
	    return 1;
	} 

	/* It must be data: save code in CurCode */
	curCode = code;
	outCount = OUTCODELENGTH;

	/* If greater or equal to freeCode, not in the hash table
	 * yet; repeat the last character decoded
	 */
	if (curCode >= freeCode) {
	    if (curCode > freeCode) {
		/*
		 * if we get a code too far outside our range, it
		 * could case the parser to start traversing parts
		 * of our data structure that are out of range...
		 */
		goto flushit;
	    }
	    curCode = oldCode;
	    outCode[--outCount] = prevChar;
	}

	/* Unless this code is raw data, pursue the chain pointed
	 * to by curCode through the hash table to its end; each
	 * code in the chain puts its associated output code on
	 * the output queue.
	 */
	 while (curCode > bitMask) {
	     outCode[--outCount] = suffix[curCode];
	     if (outCount == 0) {
		 /*
		  * In theory this should never happen since our
		  * prefix and suffix arrays are monotonically
		  * decreasing and so outCode will only be filled
		  * as much as those arrays, but I don't want to
		  * take that chance and the test is probably
		  * cheap compared to the read and write operations.
		  * If we ever do overflow the array, we will just
		  * flush the rest of the data and quietly accept
		  * the GIF as truncated here.
		  */
		 goto flushit;
	     }
	     curCode = prefix[curCode];
	 }

	/* The last code in the chain is treated as raw data. */
	prevChar = (unsigned char)curCode;
	outCode[--outCount] = prevChar;

	/* Now we put the data out to the Output routine. It's
	 * been stacked LIFO, so deal with it that way...
	 */
	len = OUTCODELENGTH - outCount;
	while (--len >= 0) {
	    rasline[off++] = outCode[outCount++];

	    /* Update the X-coordinate, and if it overflows, update the
	     * Y-coordinate
	     */
	    if (--x == 0) {
		int count;
		RELEASE_ARRAYS();

		/* If a non-interlaced picture, just increment y to the next
		 * scan line.  If it's interlaced, deal with the interlace as
		 * described in the GIF spec.  Put the decoded scan line out
		 * to the screen if we haven't gone past the bottom of it
		 */
		count = (*env)->CallIntMethod(env, this, sendID,
					      relx, rely + y,
					      width, passht,
					      raslineh, cmh);

		if (count <= 0 || SAWEXCEPTION()) {
		    /* Nobody is listening any more. */
		    return 0;
		}

		GET_ARRAYS();

		x = width;
		off = 0;
		/*  pass	inc	ht	ystart */
		/*   0           8      8          0   */
		/*   1           8      4          4   */
		/*   2           4      2          2   */
		/*   3           2      1          1   */
		y += passinc;
		while (y >= height) {
		    passinc = passht;
		    passht >>= 1;
		    y = passht;
		    if (passht == 0) {
			goto flushit;
		    }
		}
	    }
	}

	/* Build the hash table on-the-fly. No table is stored in the file. */
	prefix[freeCode] = (short)oldCode;
	suffix[freeCode] = prevChar;
	oldCode = code;

	/* Point to the next slot in the table.  If we exceed the
	 * maxCode, increment the code size unless
	 * it's already 12.  If it is, do nothing: the next code
	 * decompressed better be CLEAR
	 */
	if (++freeCode >= maxCode) {
	    if (codeSize < 12) {
		codeSize++;
		maxCode <<= 1;
		codeMask = maxCode - 1;
	    } else {
		/* Just in case */
		freeCode = maxCode - 1;
	    }
	}
    }

out_of_memory:
    RELEASE_ARRAYS();
    return 0;
}
#endif

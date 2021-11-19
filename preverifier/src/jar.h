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
 * FILE:      jar.h
 * OVERVIEW:  Public header file for the JAR file reader module. 
 *            The JAR DataStream API is used by the Pre-verifier for loading
 *          JAR files.
 *=======================================================================*/

#ifndef _JAR_H_
#define _JAR_H_

#include <typedefs.h>
#include <stdio.h>
#include <sys/types.h>
#include <stddef.h>

/* 
 * Debug flag for JAR support
 */
#define JAR_DEBUG 0 

/*=========================================================================
 * JAR file reader defines and macros
 *=======================================================================*/

/* 
 * Supported compression types
 */
#define STORED        0
#define DEFLATED    8

#define MAX_BITS 15    /* Maximum number of codes in Huffman Code Table */

/*
 * Header sizes including signatures
 */
#define LOCHDRSIZ 30
#define CENHDRSIZ 46
#define ENDHDRSIZ 22

/*
 * Header field access macros
 */
#define CH(b, n) ((long)(((unsigned char *)(b))[n]))
#define SH(b, n) ((long)(CH(b, n) | (CH(b, n+1) << 8)))
#define LG(b, n) ((long)(SH(b, n) | (SH(b, n+2) << 16)))

#define GETSIG(b) LG(b, 0)        /* signature */

#define LOCSIG (('P' << 0) + ('K' << 8) + (3 << 16) + (4 << 24))
#define CENSIG (('P' << 0) + ('K' << 8) + (1 << 16) + (2 << 24))
#define ENDSIG (('P' << 0) + ('K' << 8) + (5 << 16) + (6 << 24))

#define freeBytes(x) if (x == NULL) {} else free(x)

/*
 * Macros for getting local file header (LOC) fields
 */
#define LOCVER(b) SH(b, 4)        /* version needed to extract */
#define LOCFLG(b) SH(b, 6)        /* encrypt flags */
#define LOCHOW(b) SH(b, 8)        /* compression method */
#define LOCTIM(b) LG(b, 10)        /* modification time */
#define LOCCRC(b) LG(b, 14)        /* uncompressed file crc-32 value */
#define LOCSIZ(b) LG(b, 18)        /* compressed size */
#define LOCLEN(b) LG(b, 22)        /* uncompressed size */
#define LOCNAM(b) SH(b, 26)        /* filename size */
#define LOCEXT(b) SH(b, 28)        /* extra field size */

/*
 * Macros for getting central directory header (CEN) fields
 */
#define CENVEM(b) SH(b, 4)        /* version made by */
#define CENVER(b) SH(b, 6)        /* version needed to extract */
#define CENFLG(b) SH(b, 8)        /* general purpose bit flags */
#define CENHOW(b) SH(b, 10)        /* compression method */
#define CENTIM(b) LG(b, 12)        /* file modification time (DOS format) */
#define CENCRC(b) LG(b, 16)        /* crc of uncompressed data */
#define CENSIZ(b) LG(b, 20)        /* compressed size */
#define CENLEN(b) LG(b, 24)        /* uncompressed size */
#define CENNAM(b) SH(b, 28)        /* length of filename */
#define CENEXT(b) SH(b, 30)        /* length of extra field */
#define CENCOM(b) SH(b, 32)        /* file comment length */
#define CENDSK(b) SH(b, 34)        /* disk number start */
#define CENATT(b) SH(b, 36)        /* internal file attributes */
#define CENATX(b) LG(b, 38)        /* external file attributes */
#define CENOFF(b) LG(b, 42)        /* offset of local header */

/*
 * Macros for getting end of central directory header (END) fields
 */
#define ENDSUB(b) SH(b, 8)        /* number of entries on this disk */
#define ENDTOT(b) SH(b, 10)        /* total number of entries */
#define ENDSIZ(b) LG(b, 12)        /* central directory size */
#define ENDOFF(b) LG(b, 16)        /* central directory offset */
#define ENDCOM(b) SH(b, 20)        /* size of zip file comment */

/*=========================================================================
 * Macros for Huffman Codes used by the JAR file reader
 *=======================================================================*/

/*
 * This is the algorithm for decoding Huffman-encoded
 * data.
 *
 *     loop (until end of block code recognized)
 *        decode literal/length value from input stream
 *        if value < 256
 *           copy value (literal byte) to output stream
 *        otherwise
 *           if value = end of block (256)
 *              break from loop
 *           otherwise (value = 257..285)
 *              decode distance from input stream
 *
 *              move backwards distance bytes in the output
 *              stream, and copy length bytes from this
 *              position to the output stream.
 *     end loop
 */
#define BTYPE_NO_COMPRESSION 0x00  
#define BTYPE_FIXED_HUFFMAN  0x01  /* Fixed Huffman Code */
#define BTYPE_DYNA_HUFFMAN   0x02  /* Dynamic Huffman code */
#define BTYPE_INVALID        0x03  /* Invalid code */

#define LITXLEN_BASE 257

/*=========================================================================
 * JAR DataStream API for reading and writing to/from JAR files
 *=======================================================================*/

#define JAR_READ 1        /* Mode for reading from a JAR data stream */
#define JAR_WRITE 2        /* Mode for writing from a JAR data stream */
#define JAR_RESOURCE 1        /* type of resource (see JAR_DataStream) */

/*=========================================================================
 * JAR Data Stream structure
 *=======================================================================*/
typedef struct JAR_DataStream {
    unsigned char *data;    /* data stream for reading/writing */
    int type;        /* indicates type of resource */
    int dataLen;        /* length of data stream */
    int dataIndex;        /* current position for reading */
    int mode;        /* mode for reading or writing */
                /* mode must be either JAR_READ or JAR_WRITE */
} JAR_DataStream;

typedef struct JAR_DataStream* JAR_DataStreamPtr;

/*=========================================================================
 * Forward declarations for JAR DataStream API
 *=======================================================================*/
int JAR_ReadBytes(JAR_DataStream *ds, char *buf, int len);
int JAR_WriteBytes(JAR_DataStream *ds, char *buf, int len);
int JAR_SkipBytes(JAR_DataStream *ds, int len);

/*=========================================================================
 * Forward declarations for JAR file reader
 *=======================================================================*/

typedef FILE *JarCompressedType;
/*
typedef const unsigned char *JarCompressedType;
*/

bool_t inflate(JarCompressedType data, int compLen,
           unsigned char *decompData, int decompLen);

/* Any caller to inflate must ensure that it is safe to read at least
 * this many bytes beyond compData + compLen
 */
#define INFLATER_EXTRA_BYTES 4

/* 
 * Indicates whether JAR inflater is executed from KVM or a stand-alone 
 * program.
 */
#define JAR_INFLATER_INSIDE_KVM 0

#endif /* _JAR_H_ */

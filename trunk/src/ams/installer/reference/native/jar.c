/*
 *   
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

#include <stdio.h>
#include <string.h>

#include <jar.h>

/**
 * @file
 * JAR file reading operations.
 */

/*=========================================================================
 * JAR Data Stream structure
 *=======================================================================*/

#define LOCSIG (('P' << 0) + ('K' << 8) + (3 << 16) + (4 << 24))
#define CENSIG (('P' << 0) + ('K' << 8) + (1 << 16) + (2 << 24))
#define ENDSIG (('P' << 0) + ('K' << 8) + (5 << 16) + (6 << 24))

/*
 * Supported compression types
 */
#define STORED      0
#define DEFLATED    8

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

#define GETSIG(b) LG(b, 0)      /* signature */

/** Get local file header's version needed to extract */
#define LOCVER(b) SH(b, 4)
/** Get local file header's encrypt flags */
#define LOCFLG(b) SH(b, 6)
/** Get local file header's compression method */
#define LOCHOW(b) SH(b, 8)
/** Get local file header's modification time */
#define LOCTIM(b) LG(b, 10)
/** Get local file header's uncompressed file crc-32 value */
#define LOCCRC(b) LG(b, 14)
/** Get local file header's compressed size */
#define LOCSIZ(b) LG(b, 18)
/** Get local file header's uncompressed size */
#define LOCLEN(b) LG(b, 22)
/** Get local file header's filename size */
#define LOCNAM(b) SH(b, 26)
/** Get local file header's extra field size */
#define LOCEXT(b) SH(b, 28)

/** Get central directory header's version made by */
#define CENVEM(b) SH(b, 4)
/** Get central directory header's version needed to extract */
#define CENVER(b) SH(b, 6)
/** Get central directory header's general purpose bit flags */
#define CENFLG(b) SH(b, 8)
/** Get central directory header's compression method */
#define CENHOW(b) SH(b, 10)
/** Get central directory header's file modification time (DOS format) */
#define CENTIM(b) LG(b, 12)
/** Get central directory header's crc of uncompressed data */
#define CENCRC(b) LG(b, 16)
/** Get central directory header's compressed size */
#define CENSIZ(b) LG(b, 20)
/** Get central directory header's uncompressed size */
#define CENLEN(b) LG(b, 24)
/** Get central directory header's length of filename */
#define CENNAM(b) SH(b, 28)
/** Get central directory header's length of extra field */
#define CENEXT(b) SH(b, 30)
/** Get central directory header's file comment length */
#define CENCOM(b) SH(b, 32)
/** Get central directory header's disk number start */
#define CENDSK(b) SH(b, 34)
/** Get central directory header's internal file attributes */
#define CENATT(b) SH(b, 36)
/** Get central directory header's external file attributes */
#define CENATX(b) LG(b, 38)
/** Get central directory header's offset of local header */
#define CENOFF(b) LG(b, 42)

/** Get end header's number of entries on this disk */
#define ENDSUB(b) SH(b, 8)
/** Get end header's total number of entries */
#define ENDTOT(b) SH(b, 10)
/** Get end header's  central directory size */
#define ENDSIZ(b) LG(b, 12)
/** Get end header's  central directory offset */
#define ENDOFF(b) LG(b, 16)
/** Get end header's size of zip file comment */
#define ENDCOM(b) SH(b, 20)


/**
 * Returns the CRC of an array of bytes, using the same
 * algorithm as used by the JAR reader.
 *
 * @param data pointer to the array of bytes
 * @param length length of data, in bytes
 *
 * @return CRC
 */
static unsigned long
jarCRC32(unsigned char *data, unsigned long length) {
    unsigned long crc = 0xFFFFFFFF;
    unsigned int j;
    for ( ; length > 0; length--, data++) {
        crc ^= *data;
        for (j = 8; j > 0; --j) {
            crc = (crc & 1) ? ((crc >> 1) ^ 0xedb88320) : (crc >> 1);
        }
    }
    return ~crc;
}

/**
 * Reads the entry at at given file offset. Does not perform
 * memory allocation.
 *
 * @param fileObj file object
 * @param jarInfo JAR info object
 * @param offset offset of a central directory entry.
 *
 * @return entry info with a status of zero for success or a non-zero
 * error code.
 */
static JarEntryInfo
readJarEntryInfo(FileObj* fileObj, JarInfo* jarInfo,
        unsigned long offset) {
    JarEntryInfo entry;
    unsigned char header[CENHDRSIZ];

    memset(&entry, 0, sizeof (entry));

    /*
     * Offset contains the offset of the next central header. Read the
     * header into the temporary buffer
     */

    /* Go to the header and Read the bytes */
    if ((fileObj->seek(fileObj->state, offset, SEEK_SET) < 0) 
        || (fileObj->read(fileObj->state, header, CENHDRSIZ) != CENHDRSIZ)) {
        entry.status = JAR_CORRUPT;
        return entry;
    }

    /* header should contain the current central header */
    if (GETSIG(header) != CENSIG) {
        /* We've reached the end of the headers */
        entry.status = JAR_ENTRY_NOT_FOUND;
        return entry;
    }

    entry.nameLen = CENNAM(header);
    entry.nameOffset = offset + CENHDRSIZ;
    entry.decompLen = CENLEN(header); /* the decompressed length */
    entry.compLen   = CENSIZ(header); /* the compressed length */
    entry.method    = CENHOW(header); /* how it is stored */
    entry.expectedCRC = CENCRC(header); /* expected CRC */
    entry.encrypted = (CENFLG(header) & 1) == 1;
    entry.offset = jarInfo->locOffset + CENOFF(header);
    entry.nextCenEntryOffset = entry.nameOffset + entry.nameLen +
                               CENEXT(header) + CENCOM(header);
    return entry;
}

/**
 * Fills a Jar info struct so that entries can be read using getJarEntryInfo.
 * Does not perform memory allocation.
 *
 * @param fileObj file object
 * 
 * @return Jar info status of 0 if success, non-zero otherwise.
 */
JarInfo
getJarInfo(FileObj* fileObj) {
    unsigned char buffer[ENDHDRSIZ * 2];
    JarInfo jarInfo;
    long length;
    long minOffset;
    long currentOffset;
    unsigned const char *bp;

    memset(&jarInfo, 0, sizeof (jarInfo));

    /* Get the length of the JAR */
    length = fileObj->size(fileObj->state);

    /*
     * Calculate the smallest possible offset for the end header.  It
     * can be at most 0xFFFF + ENDHDRSIZ bytes from the end of the file, but
     * the JAR must also have a local header and a central header
     */
    minOffset = length - (0xFFFF + ENDHDRSIZ);
    if (minOffset < LOCHDRSIZ + CENHDRSIZ) {
        minOffset = LOCHDRSIZ + CENHDRSIZ;
    }

    /*
     * Read in the last ENDHDRSIZ bytes into the buffer.  99% of the time,
     * the JAR won't have a comment, and this is the only read we'll need
     */
    if ((fileObj->seek(fileObj->state, -ENDHDRSIZ, SEEK_END) < 0) ||
        (fileObj->read(fileObj->state, buffer, ENDHDRSIZ) != ENDHDRSIZ)) {
        jarInfo.status = -1;
        return jarInfo;
    }

    /* Set currentOffset to be the offset of buffer[0] */
    currentOffset = length - ENDHDRSIZ; 

    /* Set bp to be the location at which to start looking */
    bp = buffer;

    for (; ; ) {
        /*
         * "buffer" contains a block of data from the JAR, starting at
         * currentOffset "position" in the JAR.
         * We investigate whether   currentOffset + (bp - buffer)  is the start
         * of the end header in the JAR.  
         *
         * We use a simplified version of Knuth-Morris-Pratt search algorithm.
         * The header we're looking for is 'P' 'K' 5  6
         */
        switch (bp[0]) {
        case '\006':
            /* The header must start at least 3 bytes back */
            bp -= 3;
            break;

        case '\005':
            /* The header must start at least 2 bytes back  */
            bp -= 2;
            break;

        case 'K':
            /* The header must start at least 1 byte back  */
            bp -= 1;
            break;

        case 'P':
            /*
             * Either this is the header, or the header must
             * start at least 4  back
             */
            if (bp[1] == 'K' && bp[2] == 5 && bp[3] == 6) {
                /* We have what may be a header.  Let's make sure the
                 * implied length of the jar file matches the actual
                 * length.
                 */
                int endpos = currentOffset + (bp - buffer);
                if (endpos + ENDHDRSIZ + ENDCOM(bp) == length) {
                    unsigned long cenOffset = endpos - ENDSIZ(bp);
                    unsigned long locOffset = cenOffset - ENDOFF(bp);

                    if (fileObj->seek(fileObj->state, locOffset,
                            SEEK_SET)
                            >= 0 && fileObj->readChar(fileObj->state) == 'P'
                            && fileObj->readChar(fileObj->state) == 'K'
                            && fileObj->readChar(fileObj->state) == 3
                            && fileObj->readChar(fileObj->state) == 4) {
                        jarInfo.cenOffset = cenOffset;
                        jarInfo.locOffset = locOffset;
                        return jarInfo;
                    }

                    jarInfo.status = -2;
                    return jarInfo;
                }
            }

            /* FALL THROUGH */
        default:    
            /*
             * The header must start at least four characters back, since
             * the current character isn't in the header
             */
            bp -= 4;
        }

        if (bp < buffer) {
            /*
             * We've moved outside our window into the file.  We must
             * move the window backwards
             */
            int count = currentOffset - minOffset; /* Bytes left in file */
            if (count <= 0) {
                /* Nothing left to read.  Time to give up */
                jarInfo.status = -3;
                return jarInfo;
            } else {
                /*
                 * Up to ((bp - buffer) + ENDHDRSIZ) bytes in the buffer might
                 * still be part of the end header, so the most bytes we can
                 * actually read are
                 *      sizeof (buffer) - ((bp - buffer) + ENDHDRSIZE).
                 */
                int available = (sizeof (buffer) - ENDHDRSIZ) +
                    (buffer - bp);
                if (count > available) {
                    count = available;
                }
            }

            /* Back up, while keeping our virtual currentOffset the same */
            currentOffset -= count;
            bp += count;
            memmove(buffer + count, buffer, sizeof (buffer) - count);
            if ((fileObj->seek(fileObj->state, currentOffset, SEEK_SET) < 0) ||
                (fileObj->read(fileObj->state, buffer, count) !=
                   (long)count)) {
                jarInfo.status = -4;
                return jarInfo;
            }
        }
    } /* end of forever loop */

    /* Can get here */
}

/**
 * Reads an information for an named entry in a JAR. Does not perform
 * memory allocation.
 *
 * @param fileObj file object
 * @param jarInfo structure filled by fillJarInfo
 * @param name name of entry to find, must be name len long
 * @param nameLen length of the name
 * @param compBuffer buffer for reading in entry name for comparison,
 *                   must be nameLen long
 *
 * @return entry info with a status of zero for success or a non-zero
 * error code.
 */
JarEntryInfo
findJarEntryInfo(FileObj* fileObj, JarInfo* jarInfo, const unsigned char *name,
                 unsigned int nameLen, unsigned char* compBuffer) {
    JarEntryInfo entry;

    entry = getFirstJarEntryInfo(fileObj, jarInfo);

    while (entry.status == 0) {
        if (entry.nameLen == nameLen) {
            entry.status = getJarEntryName(fileObj, &entry, compBuffer);
            if (entry.status != 0) {
                break;
            }

            if (memcmp(compBuffer, name, nameLen) == 0) {
                /* found  it */
                break;
            }
        }

        entry = getNextJarEntryInfo(fileObj, jarInfo, &entry);
    }

    return entry;
}

/**
 * Reads an information for the first entry in a JAR. Does not perform
 * memory allocation.
 *
 * @param fileObj file object
 * @param jarInfo structure filled by fillJarInfo
 *
 * @return entry info with a status of zero for success or a non-zero
 * error code.
 */
JarEntryInfo
getFirstJarEntryInfo(FileObj* fileObj, JarInfo* jarInfo) {
    return readJarEntryInfo(fileObj, jarInfo, jarInfo->cenOffset);
}

/**
 * Reads an information for the entry after the given entry in a JAR.
 * Does not perform memory allocation.
 *
 * @param fileObj file object
 * @param jarInfo JAR info object
 * @param current current JAR entry info
 *
 * @return entry info with a status of zero for success or a non-zero
 * error code including JAR_ENTRY_NOT_FOUND at the end of the entries
 */
JarEntryInfo
getNextJarEntryInfo(FileObj* fileObj, JarInfo* jarInfo, JarEntryInfo* current){
    return readJarEntryInfo(fileObj, jarInfo, current->nextCenEntryOffset);
}

/**
 * Get the name of a JAR entry. Does not perform memory allocation.
 *
 * parameters:  
 * @param fileObj file object
 * @param entryInfo entry info structure returned by getJarEntryInfo
 * @param nameBuffer buffer for the name must be at least entry.nameLen long
 *
 * *return 0 for success else an error status
 */
int
getJarEntryName(FileObj* fileObj, JarEntryInfo* entry,
                unsigned char *nameBuffer) {
    /* Go to the name in the central header for this entry. */
    if (fileObj->seek(fileObj->state,  entry->nameOffset, SEEK_SET) < 0) {
        return JAR_EARLY_END_OF_INPUT;
    }

    /* Read the local entry header to get the size of the name and ext. */
    if (fileObj->read(fileObj->state, nameBuffer, entry->nameLen) !=
            (long)entry->nameLen) {
        return JAR_EARLY_END_OF_INPUT;
    }

    return 0;
}

/**
 * Inflates a JAR entry.
 *
 * @param fileObj file object
 * @param heapManObj heap object, only used for temp tables
 * @param entryInfo entry info structure returned by getJarEntryInfo
 * @param decompBuffer buffer for uncompressed data must be at
 *                     least entry.decompLen long
 * @param bufferIsAHandle non-zero if decompBuffer is mem handle that must be
 *        given to heapManObj->addrFromHandle before using
 *
 * @return 0 for success else an error status
 */
int
inflateJarEntry(FileObj* fileObj, HeapManObj* heapManObj, JarEntryInfo* entry,
                unsigned char *decompBuffer, int bufferIsAHandle) {
    unsigned long actualCRC;
    int status;
    unsigned char temp[LOCHDRSIZ];
    long bytesRead;

    /* Make sure entry is not encrypted */
    if (entry->encrypted) {
        return JAR_ENCRYPTION_NOT_SUPPORTED;
    }

    /* Seek to data for this entry */

    /* Go to the beginning of the LOC header for this entry. */
    if (fileObj->seek(fileObj->state,  entry->offset, SEEK_SET) < 0) {
        return JAR_EARLY_END_OF_INPUT;
    }

    /* Read the local entry header to get the size of the name and ext. */
    if (fileObj->read(fileObj->state, temp, LOCHDRSIZ) != LOCHDRSIZ) {
        return JAR_EARLY_END_OF_INPUT;
    }

    /* Skip over the name and ext to the beginning of entry data */
    if (fileObj->seek(fileObj->state, LOCNAM(temp) + LOCEXT(temp),
                     SEEK_CUR) < 0) {
        return JAR_EARLY_END_OF_INPUT;
    }        

    switch (entry->method) {
    case STORED:
            /* The actual bits are right there in the file */
        if (entry->compLen != entry->decompLen) {
            return JAR_ENTRY_SIZE_MISMATCH;
        }

        if (bufferIsAHandle) {
            /* This is to support heaps with memory compaction. */
            decompBuffer = heapManObj->addrFromHandle(heapManObj->state,
                                                   decompBuffer);
        }

        bytesRead = fileObj->read(fileObj->state, decompBuffer,
                                  entry->decompLen);
        if (bytesRead != (long)entry->decompLen) {
            return JAR_EARLY_END_OF_INPUT;
        }

        break;

    case DEFLATED:
        status = inflateData(fileObj, heapManObj, entry->compLen,
                             decompBuffer, entry->decompLen, bufferIsAHandle);
        if (status != 0) {
            return status;
        }

        break;

    default:
        return JAR_UNKNOWN_COMP_METHOD;
    }

    if (bufferIsAHandle) {
        /* This is to support heaps with memory compaction. */
        decompBuffer = heapManObj->addrFromHandle(heapManObj->state,
                                                  decompBuffer);
    }

    actualCRC = jarCRC32(decompBuffer, entry->decompLen);
    if (actualCRC != entry->expectedCRC) {
        return JAR_ENTRY_CORRUPT;
    }

    return 0;
}


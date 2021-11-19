/*
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

/*=========================================================================
 * SYSTEM:    Verifier
 * SUBSYSTEM: JAR support routines for the verifier.
 * FILE:      jar_support.c
 * OVERVIEW:  JAR support routines for verifying class files from a ZIP or
 *            JAR file.
 *            Note that the JAR file reader used is based on the KVM
 *            implementation with some modifications.
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#include <sys_api.h>
#include <path_md.h>
#include <path.h>
#include <oobj.h>
#include <jar.h>
#include <convert_md.h>

#include <string.h>

#ifdef WIN32
#include <process.h>
#endif
/*=========================================================================
 * Globals and extern declarations
 *=======================================================================*/

extern int errno;
char str_buffer[STRINGBUFFERSIZE];   /*  shared string buffer */
bool_t JARfile = FALSE;    /* if true, indicates that output is in a
                                                  JAR file */
extern bool_t tmpDirExists;
                           /* if true, indicates that a temp dir exists
                              with classes to be verified */
char *zipFileName = NULL;  /* stores name of the zip file */
extern char tmp_dir[32];   /* temporary directory for storing
                                                  verified classes */
extern char *output_dir;   /* output directory */

char manifestfile[1024];   /* used for saving the JAR manifest file name */

extern void VerifyFile(register char *fn);

/*=========================================================================
 * FUNCTION:      isJARfile
 * OVERVIEW:      Determines if the given file is a JAR or ZIP file.
 *                Returns true if the suffix ends with ".jar" or ".zip".
 * INTERFACE:
 *   parameters:  fn:      name of the JAR file
 *                length:  length of data, in bytes
 *   returns:     boolean type
 *=======================================================================*/
bool_t
isJARfile (char *fn, int length)
{
    char *suffix;

    if (length >= 4 &&
       (( suffix = fn + length - 4)[0] == '.') &&
         ((( _toupper(suffix[1]) == 'Z') &&
          ( _toupper(suffix[2]) == 'I') &&
          ( _toupper(suffix[3]) == 'P')) ||
         (( _toupper(suffix[1]) == 'J') &&
          ( _toupper(suffix[2]) == 'A') &&
          ( _toupper(suffix[3]) == 'R')))) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*=========================================================================
 * FUNCTION:      isManifestfile
 * OVERVIEW:      Determines if the given file is a JAR Manifest file.
 *                Returns true if the file ends with "MANIFEST.MF".
 * INTERFACE:
 *   parameters:  fn:      name of the JAR manifest file
 *                length:  length of data, in bytes
 *   returns:     boolean type
 *=======================================================================*/
bool_t
isManifestfile (char *fn, int length)
{
    if ((length >= 11) &&
       (strcmp(fn + length - 11, "MANIFEST.MF") == 0)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*=========================================================================
 * FUNCTION:      ensure_tmpdir_exists
 * OVERVIEW:      Validates to ensure that the tmpdir exists using the
 *                system-specific directory delimiters.
 *
 * INTERFACE:
 *   parameters:  char* dir name
 *   returns:     nothing
 *=======================================================================*/
void ensure_tmpdir_exists(char *dir)
{
    struct stat stat_buf;
    char *parent;
    char *q;
    if (dir[0] == 0) {
        return;
    }
    parent = strdup(dir);
    q = strrchr(parent, (char)LOCAL_DIR_SEPARATOR);
    if (q) {
        *q = 0;
        ensure_tmpdir_exists(parent);
    }
    if (stat(dir, &stat_buf) < 0) {
        if (JAR_DEBUG && verbose) {
            jio_fprintf(stderr, "Creating output directory [%s]\n", dir);
        }
#ifdef WIN32
        mkdir(dir);
#endif
#ifdef UNIX
        mkdir(dir, 0755);
#endif
    }
    free(parent);
}

/*=========================================================================
 * FUNCTION:      JARname2fname
 * OVERVIEW:      Converts JAR name to the system-specific file name with
 *                the correct directory delimiters.
 *
 * INTERFACE:
 *   parameters:  char* source JAR name
 *                char* dest file name
 *                int size
 *   returns:     char*
 *=======================================================================*/
char*
JARname2fname(char *src, char *dst, int size) {
    char *buf = dst;
    for (; (--size > 0) && (*src != '\0') ; src++, dst++) {
    if (*src == '/') {
        *dst = (char)LOCAL_DIR_SEPARATOR;
    }  else {
        *dst = *src;
    }
    }
    dst++;
    *dst = '\0';
    return buf;
}

/*=========================================================================
 * FUNCTION:      getZipEntry
 * OVERVIEW:      Converts a zip file to a Zip entry type.
 * INTERFACE:
 *   parameters:  zipFile:      name of the JAR file
 *                len:          length of data, in bytes
 *   returns:     zip entry type
 *=======================================================================*/
zip_t *
getZipEntry (char *zipFile, int len) {
    zip_t * zipEntry = NULL;  /* for processing errors */

    if (JAR_DEBUG && verbose)
        jio_fprintf(stderr,
            "getZipEntry: JAR file [%s] Size [%d]\n", zipFile, len);

    /* create the zip entry for loading the ZIP file */
    zipEntry = (zip_t *) sysMalloc(sizeof(zip_t) + len);
    if (zipEntry == NULL) {
        fprintf(stderr, "getZipEntry: Out of memory\n");
        exit(1);
    }

    memcpy(zipEntry->name, zipFile, len);

    zipEntry->name[len] = '\0';

    if (JAR_DEBUG && verbose)
        jio_fprintf(stderr, "getZipEntry: Zip Entry Name [%s]\n", zipEntry->name);

    zipEntry->type = '\0';
    return zipEntry;
}

/*=========================================================================
 * FUNCTION:      findJARDirectories
 * OVERVIEW:      Helper function used for JAR loading for locating JAR
 *                directories.
 *                It returns TRUE if it is successful in locating the
 *                JAR directory headers, false otherwise.
 *                If successful,
 *                entry->jar.locpos is set to the position of the first
 *                local header.
 *                entry->jar.cenpos is set to the position of the first
 *                central header.
 *
 *                Note that *locpos pointer is the logical "0" of the file.
 *                All offsets extracted need to have this value added to them.
 *
 * INTERFACE:
 *   parameters:  entry:     zipFileEntry
 *                statbuf:   pointer to the stat buffer
 *   returns:     boolean type
 *=======================================================================*/

bool_t
findJARDirectories(zip_t *entry, struct stat *statbuf)
{
    bool_t result = FALSE;
    long length = statbuf->st_size;
    long position, minPosition;
    char *bp;
    FILE *file;

    char *buffer = str_buffer;
    unsigned const int bufferSize = STRINGBUFFERSIZE;

    /* Calculate the smallest possible position for the end header.  It
     * can be at most 0xFFFF + ENDHDRSIZ bytes from the end of the file, but
     * the file must also have a local header and a central header
     */
    minPosition = length - (0xFFFF + ENDHDRSIZ);
    if (minPosition < LOCHDRSIZ + CENHDRSIZ) {
        minPosition = LOCHDRSIZ + CENHDRSIZ;
    }

    file = fopen(entry->name, "rb");
    if (file == NULL) {
        goto done;
    }

    /* Read in the last ENDHDRSIZ bytes into the buffer.  99% of the time,
     * the file won't have a comment, and this is the only read we'll need */
    if (   (fseek(file, -ENDHDRSIZ, SEEK_END) < 0)
        || (fread(buffer, sizeof(char), ENDHDRSIZ, file) != ENDHDRSIZ)) {
        goto done;
    }
    /* Get the position in the file stored into buffer[0] */
    position = length - ENDHDRSIZ;  /* Position in file of buffer[0] */
    bp = buffer;            /* Where to start looking */
    for (;;) {
        /* "buffer" contains a block of data from the file, starting at
         * position "position" in the file.
         * We investigate whether   position + (bp - buffer)  is the start
         * of the end header in the zip file.  This file position is at
         * position bp in the buffer.
         */
        /* Use simplified version of Knuth Morris Pratt search algorithm. */
        switch(bp[0]) {
            case '\006':   /* The header must start at least 3 bytes back */
                bp -= 3; break;
            case '\005':   /* The header must start at least 2 bytes back  */
                bp -= 2; break;
            case 'K':      /* The header must start at least 1 byte back  */
                bp -= 1; break;
            case 'P':      /* Either this is the header, or the header must
                            * start at least 4  back
                            */
                if (bp[1] == 'K' && bp[2] == 5 && bp[3] == 6) {
                    int endpos = position + (bp - buffer);
                    if (endpos + ENDHDRSIZ + ENDCOM(bp) == length) {
                        unsigned long cenpos = endpos - ENDSIZ(bp);
                        unsigned long locpos = cenpos - ENDOFF(bp);
                        entry->jar.cenpos = cenpos;
                        entry->jar.locpos = locpos;
                        result = TRUE;
                        goto done;
                    }
                }
            /* FALL THROUGH */
            default:      /* This char isn't in the header signature, so
                           * the header must start at least four chars back */
                bp -= 4;
        }

        if (bp < buffer) {
            /* We've moved outside our window into the file.  We must
             * move the window backwards */
            int count = position - minPosition; /* Bytes left in file */
            if (count == 0) {
                /* Nothing left to read.  Time to give up */
                goto done;
            } else {
                /* up to ((bp - buffer) + ENDHDRSIZ) bytes in the buffer might
                 * still be part of the end header, so the most bytes we can
                 * actually read are
                 *      bufferSize - ((bp - buffer) + ENDHDRSIZE).
                 */
                int available = (bufferSize - ENDHDRSIZ) + (buffer - bp);
                if (count > available) {
                    count = available;
                }
            }
            /* Back up, while keeping our virtual position the same */
            position -= count;
            bp += count;
            memmove(buffer + count, buffer, bufferSize - count);
            if (   (fseek(file, position, SEEK_SET) < 0)
              || (fread(buffer, sizeof(char), count, file) != (unsigned)count)) {
                goto done;
            }
        }
    } /* end of for loop */

 done:
    if (file != NULL) {
        fclose(file);
    }
    return result;
}

/*=========================================================================
 * FUNCTION:      jarCRC32
 * OVERVIEW:      Returns the CRC of an array of bytes, using the same
 *                algorithm as used by the JAR reader.
 * INTERFACE:
 *   parameters:  data:     pointer to the array of bytes
 *                length:   length of data, in bytes
 *   returns:     CRC
 *=======================================================================*/

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

/*=========================================================================
 * FUNCTION:      loadJARfile()
 * TYPE:          load JAR file
 * OVERVIEW:      Internal function used by openClassfileInternal().
 *
 *  This function reads the specified class file from the JAR file.  The
 *  result is returned as a JAR_DataStream*.  NULL is returned if it
 *  cannot find the file, or there is some error.
 *
 * INTERFACE:
 *   parameters:  entry:    zip file entry for the JAR file
 *                filename: class file name to search for
 *   returns:     JAR_DataStream* for saving the JAR file info, or NULL.
 *=======================================================================*/

JAR_DataStreamPtr
loadJARfile(zip_t *entry, const char* filename)
{
    JAR_DataStreamPtr jdstream = NULL; /* result on error */
    unsigned int filenameLength;
    unsigned int nameLength;
    char buff[BUFSIZ];
    char *UTFfilename = &buff[0];
    char *p = str_buffer;   /* temporary storage */
    int offset;
    char *fname = NULL;
    FILE *file = fopen(entry->name, "rb");
    if (file == NULL) {
        goto done;
    }

    if (JAR_DEBUG && verbose)
        jio_fprintf(stderr,
               "loadJARfile: Opening zip file %s to search for [%s]\n",
                entry->name, filename);

    /* add the .class to the filename */

    if (JAR_DEBUG && verbose)
        jio_fprintf(stderr,
        "loadJARfile: Adding '.class' to %s size [%d]\n", filename, strlen(filename));

    /* Conversion for Japanese filenames */
    native2utf8(filename, UTFfilename, BUFSIZ);

    /* allocate fname large enough to hold .class + '\0' terminator */
    fname = (char *)malloc(strlen(UTFfilename) + 6 + 1);
    sprintf(fname, "%s.class", UTFfilename);
    filenameLength=strlen(fname);
    fname[filenameLength]='\0';

    if (JAR_DEBUG && verbose)
        jio_fprintf(stderr,
        "loadJARfile: Searching for filename [%s]\n", fname);

    /* Go to the start of the central headers */
    offset = entry->jar.cenpos;
    for (;;) {

        if (/* Go to the next central header */
            (fseek(file, offset, SEEK_SET) < 0)
            /* Read the bytes */
        || (fread(p, sizeof(char), CENHDRSIZ, file) != CENHDRSIZ)
            /* Make sure it is a header */
        || (GETSIG(p) != CENSIG)) {
            goto done;
        }

        /* Get the nameLength */
        nameLength = CENNAM(p);

        if (nameLength == filenameLength) {
            if (fread(p + CENHDRSIZ, sizeof(char), nameLength, file)
                     != nameLength) {
                goto done;
            }
            if (memcmp(p + CENHDRSIZ, fname, nameLength) == 0) {

                if (JAR_DEBUG && verbose)
                    jio_fprintf(stderr, "loadJARfile: Class name found [%s]...\n", fname);
                break;
            }
        }

        /* Set offset to the next central header */
        offset += CENHDRSIZ + nameLength + CENEXT(p) + CENCOM(p);

    } /* end loop */

    /* p points at the central header for the file */
    {
    unsigned long decompLen = CENLEN(p); /* the decompressed length */
    unsigned long compLen   = CENSIZ(p); /* the compressed length */
    unsigned long method    = CENHOW(p); /* how it is stored */
    unsigned long expectedCRC = CENCRC(p); /* expected CRC */
    unsigned long actualCRC;

    unsigned char *decompData;

    /* Make sure file is not encrypted */
    if ((CENFLG(p) & 1) == 1) {
        fprintf(stderr, "Entry is encrypted");
        goto done;
    }

    jdstream =
        (JAR_DataStreamPtr)sysMalloc(sizeof(JAR_DataStream) + decompLen);
    decompData = (unsigned char *)(jdstream + 1);

    if (/* Go to the beginning of the LOC header */
           (fseek(file, entry->jar.locpos + CENOFF(p), SEEK_SET) < 0)
        /* Read it */
        || (fread(p, sizeof(char), LOCHDRSIZ, file) != LOCHDRSIZ)
        /* Skip over name and extension, if any */
        || (fseek(file, LOCNAM(p) + LOCEXT(p), SEEK_CUR) < 0)) {
        goto done;
    }

    switch (method) {
        case STORED:
            if (compLen != decompLen) {
                return NULL;
            }
            fread(decompData, sizeof(char), decompLen, file);
            break;

        case DEFLATED: {
            bool_t inflateOK;
            inflateOK = inflate(file, compLen, decompData, decompLen);

            if (!inflateOK) {
                sysFree(jdstream);
                jdstream = NULL;
            }
            break;
        }

        default:
            sysFree(jdstream);
            jdstream = NULL;
        break;
    }

    actualCRC = jarCRC32(decompData, decompLen);
    if (actualCRC != expectedCRC) {
        printf("Unexpected CRC value");
    }

    done:
    if (file != NULL) {
        fclose(file);
    }

    if (fname != NULL) {
        sysFree(fname);
    }

    if (jdstream != NULL) {

        jdstream->type = JAR_RESOURCE;
        jdstream->data = decompData;
        jdstream->dataLen = decompLen;
        jdstream->dataIndex = 0;
        jdstream->mode = JAR_READ;
    }
    return jdstream;

    }
}

/*=========================================================================
 * FUNCTION:      ReadFromZip()
 * TYPE:          Reads and verifies ZIP file entries
 * OVERVIEW:      Internal function used by ProcessInputs() and recurse_dir.
 *
 *  This function reads all the Zip entries, and stores them temporarily in
 *  tmpdir. If the ZIP entry read is a class file, then VerifyFile is invoked
 *  to verify the class file. Otherwise, the file read is simply copied over
 *  temporarily to the tmpdir. These are later used for generating a new JAR
 *  file.
 *
 * INTERFACE:
 *   parameters:  ZipEntry:  name of the zip file entry.
 *   returns:     nothing
 *=======================================================================*/

void
ReadFromZip(zip_t *entry)
{
    unsigned int nameLength;
    char *filename = NULL;
    char *p = str_buffer;               /* temporary storage */
    int offset, nextOffset;
    int fd;
    int status;
    JAR_DataStreamPtr jdstream = NULL;
    struct stat stat_buf;
    unsigned char *decompData;
    unsigned long decompLen;     /* the decompressed length */

    FILE *file = fopen(entry->name, "rb");
    if (file == NULL) {
        goto done;
    }

    if (JAR_DEBUG && verbose)
        jio_fprintf(stderr,
               "ReadFromZip: Opened zip file to read classes \n");

    /* initialize */
    memset(manifestfile, 0, 1024);

    /* Go to the start of the central headers */
    offset = entry->jar.cenpos;
    for (;;) {

        if (/* Go to the next central header */
           (fseek(file, offset, SEEK_SET) < 0)
        /* Read the bytes */
        || (fread(p, sizeof(char), CENHDRSIZ, file) != CENHDRSIZ)
        /* Make sure it is a header */
        || (GETSIG(p) != CENSIG)) {
            goto done;
        }
        /* Get the nameLength */
        nameLength = CENNAM(p);

        if (fread(p + CENHDRSIZ, sizeof(char), nameLength, file)
            != nameLength) {
            goto done;
        }

        /* initialize the filename with nulls every time */

        filename = (char *) sysCalloc(STRINGBUFFERSIZE, nameLength);

        if (filename == NULL) {
            fprintf(stderr, "ReadFromZip: Out of memory \n");
            exit(1);
        }

        memcpy(filename, p + CENHDRSIZ, nameLength);

        /* We have to calculate nextOffset now, because VerifyFile bashes
         * str_buffer
         */
        nextOffset = offset + CENHDRSIZ + nameLength + CENEXT(p) + CENCOM(p);

        if (JAR_DEBUG && verbose)
            jio_fprintf(stderr,
               "ReadFromZip: filename read %s\n", filename);

        /* extract the .class from the filename */
        if (nameLength > 6 &&
            strcmp(filename + nameLength - 6, ".class") == 0) {
            /* Verify the class file */

            if (JAR_DEBUG && verbose)
                jio_fprintf(stderr,
                 "ReadFromZip: Extracted '.class' from %s\n", filename);

            filename[nameLength-6] = 0;

            if (JAR_DEBUG && verbose)
                jio_fprintf(stderr,
                 "ReadFromZip: Verifying classfile %s\n", filename);

            /* call VerifyFile to verify the class */
            VerifyFile(filename);
        } else {
            /* Read and copy over the file to tmpdir */
            /* p points at the central header for the file */
            unsigned long compLen   = CENSIZ(p); /* the compressed length */
            unsigned long method    = CENHOW(p); /* how it is stored */
            unsigned long expectedCRC = CENCRC(p); /* expected CRC */
            unsigned long actualCRC;

            decompLen = CENLEN(p);

            if (JAR_DEBUG && verbose)
                jio_fprintf(stderr,
                  "READFROMZIP: Reading file [%s]...\n", filename);

            /* Make sure file is not encrypted */
            if ((CENFLG(p) & 1) == 1) {
                jio_fprintf(stderr, "Entry is encrypted\n");
                goto done;
            }

            jdstream =
                (JAR_DataStreamPtr)sysMalloc(sizeof(JAR_DataStream) + decompLen);
            decompData = (unsigned char *)(jdstream + 1);

            if (/* Go to the beginning of the LOC header */
                   (fseek(file, entry->jar.locpos + CENOFF(p), SEEK_SET) < 0)
                /* Read it */
                || (fread(p, sizeof(char), LOCHDRSIZ, file) != LOCHDRSIZ)
                /* Skip over name and extension, if any */
                || (fseek(file, LOCNAM(p) + LOCEXT(p), SEEK_CUR) < 0)) {
                goto done;
            }

            switch (method) {
                case STORED:
                    if (compLen != decompLen) {
                        sysFree(jdstream);
                        jdstream = NULL;
                        goto done;
                    }
                    fread(decompData, sizeof(char), decompLen, file);
                    break;

                case DEFLATED: {
                    bool_t inflateOK;
                    inflateOK = inflate(file, compLen, decompData, decompLen);

                    if (!inflateOK) {
                        sysFree(jdstream);
                        jdstream = NULL;
                    }
                    break;
                }

                default:
                    sysFree(jdstream);
                    jdstream = NULL;
                break;
            }

            actualCRC = jarCRC32(decompData, decompLen);
            if (actualCRC != expectedCRC) {
                jio_fprintf(stderr, "Unexpected CRC value\n");
            }

            /* create the tempdir if it doesn't already exist */
            {
                char fname[1024];     /* file name restored from JAR */
                char *dir;
                char *q;
                char *sfname = fname; /* system-specific file name */
                char *dname = fname;  /* destination file name */

                if (JAR_DEBUG && verbose)
                    jio_fprintf(stderr,
                        "Reading filename [%s] from JAR \n", filename);

                sprintf(fname, "%s%c%s", tmp_dir,
                                         (char) LOCAL_DIR_SEPARATOR,
                                         filename);

                if (JAR_DEBUG && verbose)
                    jio_fprintf(stderr,
                               "Before conversion: fname [%s]  sfname [%s]\n",
                                fname, sfname);

        /*
                 * convert JAR name to the system-specific file name
                 */

                sfname = JARname2fname(fname, dname, strlen(fname)+1);

                if (JAR_DEBUG && verbose)
                    jio_fprintf(stderr,
                                "After conversion: Converted [%s] to [%s]\n",
                                 fname, sfname);

                if (JAR_DEBUG && verbose)
                    jio_fprintf(stderr,
                      "Preparing to write file [%s]\n", sfname);

                dir = strdup(sfname);
                q = strrchr(dir, (char)LOCAL_DIR_SEPARATOR);
                if (q) {
                    *q = 0;
                    ensure_tmpdir_exists(dir);
                }
                free(dir);

                /* Attempt to stat or open only if this is NOT a directory */

                if (!(sfname[strlen(sfname)-1] == (char)LOCAL_DIR_SEPARATOR)) {
                    if (JAR_DEBUG && verbose)
                        jio_fprintf(stderr,
                        "Attempting stat on dir [%s]\n", sfname);

                    status = stat(sfname, &stat_buf);

                    if (JAR_DEBUG && verbose)
                        jio_fprintf(stderr,
                        "Status from stat of [%s] is %d\n", sfname, status);

                    /* Attempt to open only if the file does not already exist.
                     * This is indicated by the stat command returning -1.
                     * And this is not a directory.
                     */

                    if ((status < 0) || !(stat_buf.st_mode & S_IFDIR)) {

                        if (JAR_DEBUG && verbose)
                            jio_fprintf(stderr,
                                "Opening file [%s]\n", sfname);
#ifdef UNIX
                        fd = open(sfname, O_WRONLY | O_CREAT | O_TRUNC , 0644);
#endif

#ifdef WIN32
                        fd = open(sfname, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644);
#endif

                        if (fd < 0) {
                            panic("failed to open %s", sfname);
                        }

                        if (JAR_DEBUG && verbose)
                            jio_fprintf(stderr,
                            "Writing file [%s]...\n", sfname);

                        /* write the file to the tmpdir just created */
                        write(fd, decompData, decompLen);

                        /* check for the JAR Manifest.mf file */

                        if (isManifestfile(sfname, strlen(sfname))) {
                /* save it for using it later to create
                             * the JAR file
                             */
                            memcpy(manifestfile, sfname + strlen(tmp_dir), strlen(sfname));

                            if (JAR_DEBUG && verbose)
                                jio_fprintf(stderr,
                                            "Saving JAR manifest file [%s]\n",
                                            manifestfile);
                        }

                        close(fd);
                    }
                }
            }
            if (jdstream != NULL) {
                sysFree(jdstream);
                jdstream = NULL;
            }
        }

        /* Set offset to the next central header */
        offset = nextOffset;
        sysFree(filename);
        filename = NULL;
    }
done:

    if (file != NULL) {
        fclose(file);
    }

    if (jdstream != NULL) {

        jdstream->type = JAR_RESOURCE;
        jdstream->data = decompData;
        jdstream->dataLen = decompLen;
        jdstream->dataIndex = 0;
        jdstream->mode = JAR_READ;
    }

    if (filename != NULL)
        sysFree(filename);
}

/*=========================================================================
 * FUNCTION:      remove_dir()
 * TYPE:          Handles removing files from recursive directories
 * OVERVIEW:      Internal function called by ProcessJARfile().
 *
 *  This function reads a directory, searching for either another directory,
 *  or an individual class name that is to be removed using the remove()
 *  API call.
 *
 * INTERFACE:
 *   parameters:  dirname   name of the directory entry.
 *                pkgname   name of the package
 *   returns:     nothing
 *=======================================================================*/
static void remove_dir(char *dirname, char *pkgname)
{
    struct dirent *ent;
    char buf[MAXPACKAGENAME];
    char pkgbuf[MAXPACKAGENAME];
    char tmpbuf[MAXPACKAGENAME];
    char tmppkg[MAXPACKAGENAME];
    char tmppkgbuf[MAXPACKAGENAME];
    char *name = NULL;
    DIR *dir = opendir(dirname);
    int err = 0;
    int status;

    /* Initialize the buffers to 0 */
    memset(buf, 0, sizeof(buf));
    memset(pkgbuf, 0, sizeof(pkgbuf));
    memset(tmpbuf, 0, sizeof(tmpbuf));
    memset(tmppkg, 0, sizeof(tmppkg));
    memset(tmppkgbuf, 0, sizeof(tmppkgbuf));

    if (dir == NULL) {
        fprintf(stderr, "Can't open dir %s\n", dirname);
        exit(1);
    }
    for (ent = readdir(dir); ent; ent = readdir(dir)) {
        struct stat stat_buf;
        int len;
        name = ent->d_name;

        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        strcpy(pkgbuf, pkgname);
        if (pkgname[0] != 0) {
            /* concatenate '/' to the package name */
            sprintf(tmppkgbuf, "%s%c", pkgbuf, (char)LOCAL_DIR_SEPARATOR);
        }
        if (JAR_DEBUG && verbose)
            jio_fprintf(stderr,
               "remove_dir: Reading filename [%s] from directory [%s]\n",
                        name, dirname);

        /* we just have a class file that needs to be removed */

        len = strlen(name);

        /* append the dirname and name */
        strcpy(buf, dirname);
        strcat(buf, name);

        status = stat(buf, &stat_buf);

        if ((status == 0) && !(stat_buf.st_mode & S_IFDIR)) {
            /* remove if this is a file and not a directory */

            if (JAR_DEBUG && verbose)
                    jio_fprintf(stderr,
                "remove_dir: Removing file [%s] \n", buf);

            err = remove(buf);

            if (JAR_DEBUG && verbose) {
                jio_fprintf(stderr,
                           "remove_dir: remove() returned error %d\n", err);
            }

        } else {
            strcat(tmppkgbuf, name);
            stat(buf, &stat_buf);
            len = strlen(buf);

            if (stat_buf.st_mode & S_IFDIR) {
                /* handle the recursive directory found */

                sprintf(tmpbuf, "%s%c", buf, (char)LOCAL_DIR_SEPARATOR);

                if (JAR_DEBUG && verbose)
                    jio_fprintf(stderr,
                     "remove_dir: Recursive dir, calling remove_dir [%s,%s]\n",
                      tmpbuf, tmppkgbuf);
                remove_dir(tmpbuf, tmppkgbuf);
                continue;
            }
        }

    }

    /* close the directory to free the dirp first */
    closedir(dir);

#ifdef WIN32
    /* remove the trailing '\' from the directory */
    {
        int tmppkglen = strlen(dirname);
        dirname[tmppkglen-1] = '\0';
    }
#endif

    /* Remove the directory by calling rmdir() or remove() API as appropriate */

    sprintf(tmppkg,"%s", dirname);

    if (JAR_DEBUG && verbose)
        jio_fprintf(stderr,
                  "remove_dir: Removing [%s]\n", tmppkg);

#ifdef WIN32
    err = rmdir(tmppkg);

    if (JAR_DEBUG && verbose) {
        if (err != 0) {
            jio_fprintf(stderr,
                     "remove_dir: rmdir(%s) failed with error %d\n",
                       tmppkg, err);
        }
    }
#endif

#ifdef UNIX
    err = remove (tmppkg);

    if (JAR_DEBUG && verbose) {
        if (err != 0) {
            jio_fprintf(stderr,
                     "remove_dir: remove(%s) failed with error %d\n",
                       tmppkg, err);
        }
    }
#endif

}
/*=========================================================================
 * FUNCTION:      ProcessJARfile()
 * TYPE:          Processes ZIP file entries
 * OVERVIEW:      Internal function called by ProcessInputs().
 *
 *  This function processes a JAR file by first creating a zip entry for
 *  reading JAR directories, then calls ReadFromZip() to read the Zip
 *  class names and verifies them. It finally creates a new JAR file and
 *  places all the verified classes into it.
 *  It returns a boolean type to indicate if a valid JAR file was found
 *  and the contents of the JAR file were processed without errors.
 *
 * INTERFACE:
 *   parameters:  buf:  JAR file name.
 *                len:  size of the file
 *   returns:     boolean type
 *=======================================================================*/
bool_t ProcessJARfile(char *buf, int len)  {

    zip_t *zipEntry;
    struct stat stat_buf;
    char *fname = NULL;
    char *buffer = NULL;
    char *jarName = NULL;
    char dirdelim = '\0';        /* directory delimiter */
    int err = 0;
    int tmpdir_len = 0;
    int statcode;
    char tmpdir_buf[MAXPACKAGENAME];

    if (JAR_DEBUG && verbose)
        jio_fprintf(stderr,
               "ProcessJARfile: JAR file [%s] Size [%d]\n", buf, len);

    statcode = stat(buf, &stat_buf);

    /* Create the zip entry for searching the JAR directories.
     * If the zip entry is NULL, it would indicate that we ran
     * out of memory and would have exited already.
     */

    zipEntry = getZipEntry (buf, len);

    if (JAR_DEBUG && verbose)
        jio_fprintf(stderr, "Searching for JAR directories...\n");

    /* search for the JAR directories */
    if (findJARDirectories(zipEntry, &stat_buf)) {
              /* the JAR directories were found */

        JARfile = TRUE;
        zipFileName = buf;
        if (JAR_DEBUG && verbose) {
            jio_fprintf(stderr,
                        "ProcessJARfile: JAR directory [%s] found \n",
                         zipEntry->name);
        }

        zipEntry->type = 'j';

        /* Read and Verify the classes from the ZIP file */

       if (JAR_DEBUG && verbose)
           jio_fprintf(stderr, "ProcessJARfile: Verifying Zip class names...\n");

       pushJarFileOntoClassPath(zipEntry);

       ReadFromZip(zipEntry);

       popClassPath();

       /* Ensure that the output_dir also exists or create it if it
        * does not already exist
        */

       if (output_dir != NULL) {
           char *dir = strdup(output_dir);

           if (JAR_DEBUG && verbose)
               jio_fprintf(stderr, "ProcessJARfile: Checking if output [%s] exists\n",
                   output_dir);
           ensure_dir_exists(dir);
           ensure_dir_writable(dir);
           free(dir);
       }

       /* Create a JAR file only if the input parameter was a JAR file,
        * the tmp_dir was created with classes verified and an output
        * dir also exists.
        */

       if (JARfile && tmpDirExists && tmp_dir && output_dir) {
           const char *p;

           /* Allocate enough space to hold the JAR name */
           jarName = (char *)sysCalloc(len+32, len);

           if (jarName == NULL) {
               fprintf(stderr, "ProcessJARfile: Out of memory");
               exit(1);
           }

           if (JAR_DEBUG && verbose)
               jio_fprintf(stderr, "ProcessJARfile: Creating JAR file of verified classes...\n");
           /* search for the last '/' to get the actual JAR file name */
           for (p = buf+len; ;) {
               --p;

               if (*p == '/' || *p == '\\') {
                   dirdelim = *p;
                   memcpy(jarName, p+1, (len-1)-(p-buf));
                   if (JAR_DEBUG && verbose)
                       jio_fprintf(stderr, "ProcessJARfile: JAR file [%s]\n", jarName);
                   break;
               }
               if (p == buf) {
                   /* no directories in path, get the individual JAR name */
                   strncpy(jarName, buf, len);
                   if (JAR_DEBUG && verbose)
                       jio_fprintf(stderr, "ProcessJARfile: JAR filename [%s]\n", jarName);
                   break;
               }
           }

           /* move the verified classes into a JAR file */

           /* Be sure to allocate enough space to hold the sprintfs below */
           fname = (char *)malloc(strlen(output_dir)+ len+32);

           if (fname == NULL) {
               fprintf(stderr, "ProcessJARfile: Out of memory");
               exit(1);
           }

           if (dirdelim != '\0') {
               sprintf(fname, "%s%c%s", output_dir,
               dirdelim, jarName);
           } else {
               sprintf(fname, "%s%c%s", output_dir,
                       (char)LOCAL_DIR_SEPARATOR, jarName);
           }

           /* Be sure to allocate enough space to hold the sprintfs below */
           /* The size here must be adjusted anytime the buffer used in the
            * sprintfs is extended.
            */

           buffer = (char *)malloc(strlen(output_dir)+strlen(fname) +
                    strlen(tmp_dir) + strlen(manifestfile) +
                    strlen(tmp_dir) + 51);
           if (buffer == NULL) {
               fprintf(stderr, "ProcessJARfile: Out of memory");
               exit(1);
           }

           if (verbose) {
                   if (isManifestfile(manifestfile, strlen(manifestfile))) {
                       /* use existing manifest if one exists */
                       sprintf(buffer, "jar -cvfm \"%s\" %s%c%s -C %s .",
                                       fname, tmp_dir,
                                       (char)LOCAL_DIR_SEPARATOR,
                                       manifestfile, tmp_dir);

                   } else {

               sprintf(buffer, "jar -cvfM \"%s\" -C %s .",
                                        fname, tmp_dir);

                   }
           } else {
               /* Run jar in non-verbose mode, and log errors in a file */

                   if (isManifestfile(manifestfile, strlen(manifestfile))) {
                       /* use existing manifest if one exists */

#ifdef UNIX
                       /* create JAR with existing manifest file */
                       /* Redirect errors and stdout to jarlog.txt */
                        sprintf(buffer,
                          "sh -c \"jar -cfm \\\"%s\\\" %s%c%s -C %s .\" > \"%s%c\"%s",
                          fname, tmp_dir, (char)LOCAL_DIR_SEPARATOR,
                          manifestfile, tmp_dir, output_dir,
                          (char)LOCAL_DIR_SEPARATOR,"jarlog.txt 2>&1");

#else
                       sprintf(buffer,
                          "jar -cfm \"%s\" %s%c%s -C %s . ",
                          fname, tmp_dir, (char)LOCAL_DIR_SEPARATOR,
                          manifestfile, tmp_dir);
#endif
                   } else {
#ifdef UNIX
                       /* create JAR with no manifest since none exists */
                       /* Redirect errors and stdout to jarlog.txt */
                       sprintf(buffer,
                           "sh -c \"jar -cfM \\\"%s\\\" -C %s .\" > \"%s%c\"%s",
                           fname, tmp_dir, output_dir,
                           (char)LOCAL_DIR_SEPARATOR,"jarlog.txt 2>&1");
#else
                       sprintf(buffer,
                           "jar -cfM \"%s\" -C %s . ",
                           fname, tmp_dir);
#endif

                   }
           }
           if (verbose) {
               jio_fprintf(stderr, "Executing command [%s]\n", buffer);
           }

#ifdef WIN32
           /* system() function does not return the exit code of the child
            * process under Windows98.
            * The documentation states:
            *   "If command is not NULL, system returns the value that is
            *    returned by the command interpreter.".
            * Thus it is probably a CR within 'command.com'.
            * Note that _spawnlp correctly returns the exit status of the
            * new process.
            */
           if (verbose) {
               err = _spawnlp(_P_WAIT, "jar", "jar", buffer+4, NULL);
               if (err !=0) {
                   fprintf(stderr, "%s\n", buffer);
                   perror("Error");
               }
           } else {
               int cstderr;
               int cstdout;
               FILE *logfile;

               /* Save stderr and stdout*/
               if ((cstderr = dup(_fileno(stderr))) == -1) {
                   fprintf(stderr, "Cannot copy dup stderr\n");
               }
               if ((cstdout = dup(_fileno(stdout))) == -1) {
                   fprintf(stderr, "Cannot copy dup stdout\n");
               }

               sprintf(tmpdir_buf, "%s\\%s", output_dir, "jarlog.txt");
               if ((logfile = fopen(tmpdir_buf, "w")) == NULL) {
                   fprintf(stderr, "Cannot create output file\n");
                   exit(1);
               }

               if (_dup2(_fileno(logfile), _fileno(stderr)) == -1) {
                   fprintf(stderr, "dup2 failed for stderr\n");
                   exit(1);
               }

               if (_dup2(_fileno(logfile), _fileno(stdout)) == -1) {
                   fprintf(stderr, "dup2 failed for stdout\n");
                   exit(1);
               }

               err = _spawnlp(_P_WAIT, "jar", "jar", buffer+4, NULL);
               if (err !=0) {
                   fprintf(stderr, "%s\n", buffer);
                   perror("Error");
               }

               fflush(stderr);
               fflush(stdout);
               fclose(logfile);

               /* Restore stderr and stdout */
               _dup2(cstderr, _fileno(stderr));
               _dup2(cstdout, _fileno(stdout));

               memset(tmpdir_buf, 0, sizeof(tmpdir_buf));
           }
#else
           err = system(buffer);
#endif

           if (err != 0) {
               /* jar file creation failed - return back the error */
               fprintf(stderr, "JAR file creation failed with error %d\n", err);
               if (!verbose)
                   fprintf(stderr,
"The preverified classes if any are in %s. See jar log of errors in %s%c%s \n",
                  tmp_dir, output_dir, (char)LOCAL_DIR_SEPARATOR, "jarlog.txt");
           } else {
               if (!verbose) {
                   /* remove the jar log file if no error occurred */
                   char *jarfn = NULL;
                   int error;

                   jarfn = (char *)malloc(strlen(output_dir)+10+1+1);

                   if (jarfn == NULL) {
                       fprintf(stderr, "ProcessJARfile: Out of memory");
                           exit(1);
                   }

                   sprintf(jarfn, "%s%c%s", output_dir,
                           (char)LOCAL_DIR_SEPARATOR, "jarlog.txt");
                   error = remove(jarfn);    /* remove the file */

                   if (jarfn != NULL)
                       sysFree(jarfn);
               }

               /* prepare to remove the tmp directory */
               /* copy the tmp directory name to a buffer */
               strcpy(tmpdir_buf, tmp_dir);
               tmpdir_len = strlen(tmp_dir);

               /* Append dir separator if it does not yet exist */
               if (tmpdir_buf[tmpdir_len - 1] != LOCAL_DIR_SEPARATOR &&
                   tmpdir_buf[tmpdir_len - 1] != DIR_SEPARATOR) {
                   tmpdir_buf[tmpdir_len] = LOCAL_DIR_SEPARATOR;
                   tmpdir_buf[tmpdir_len + 1] = 0;
               }

               /* remove the tmp_dir and all its contents recursively */
               remove_dir(tmpdir_buf, "");
            } /* jar creation returned no error */
        }
    } else {
        if (JAR_DEBUG && verbose)
            jio_fprintf(stderr, "JAR directories not found for JAR file [%s]\n", buf);
        if (fname != NULL)
            sysFree(fname);
        if (zipEntry != NULL)
            sysFree(zipEntry);
        return FALSE; /* could not locate JAR directories - invalid JAR file */
    }

    if (fname != NULL)
        sysFree(fname);

    if (zipEntry != NULL)
        sysFree(zipEntry);

    if (buffer != NULL)
        sysFree(buffer);

    if (jarName != NULL)
        sysFree(jarName);

    if ( err!=0 )
        exit(errorCode = 1);

    return JARfile;
}

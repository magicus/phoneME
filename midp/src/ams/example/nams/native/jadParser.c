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

#include <stdio.h>
#include <string.h>

#include <fileInstallerInternal.h>
#include <midp_logging.h>
#include <midpUtilKni.h>
#include <suitestore_common.h>

/**
 * @file
 *
 * Routines to parse a JAD file.
 */

/**
 * There are five properties that MUST be defined in the JAD file.
 * Those properties are:
 * MIDlet-Jar-Size
 * MIDlet-Jar-URL
 * MIDlet-Name
 * MIDlet-Vendor
 * MIDlet-Version
 * Also verification that MIDlet-Jar-Size has a correct value done.
 * Also verification that MIDlet-Jar-Size not to big done.
 *
 * @param jadsmp struct that contains parsed jad.
 * @return Same struct with struct.status value set accordingly.
 *         If some property is missing relevant status returned:
 *         NO_JAR_SIZE_PROP
 *         NO_SUITE_VENDOR_PROP
 *         NO_SUITE_NAME_PROP
 *         NO_SUITE_VERSION_PROP
 *         NO_JAR_URL_PROP
 *         If MIDlet-Jar-Size value has an incorrect value: NUMBER_ERROR
 *         If MIDlet-Jar-Size value is to big: OUT_OF_STORAGE
 */
static MidpProperties verifyJadMustProperties(MidpProperties jadsmp);

/**
 * Every property occupies one line of the input stream. Each line
 * is terminated by a line terminator which can be a LF, or (CR LF).
 * Lines from the input stream are processed until
 * end of file is reached on the input stream.
 * <p>
 * Every line describes one property to be added to the table.
 * The key consists of all the characters from beginning of the line
 * up to, but not including the first ASCII <code>:</code>.
 * All remaining characters on the line become part of the associated
 * element. The element is also trimmed of leading and trailing
 * whitespace.
 * <p>
 * This method will try to continue after a format error and load as
 * many properties it can, but return the last error encountered.
 *
 * @param jchar_buffer Buffer of jchars that contains jad file data
 *                     (null-terminated jchars string)
 * @return MidpProperties structure filled with keys and values from the jad
 */
static MidpProperties midpParseJad(jchar* jchar_buffer);

/**
 * Reads a line from a jad file.
 *
 * @param jadbuf pointer to the jad jchar_buffer; this pointer will move to
 *               the next line
 * @param result pointer to pcsl_string where the new line from jad file
 *               will be stored, the caller is responsible for freeing this
 *               string
 * @return error code
 */
static MIDPError readJadLine(jchar** jadbuf, pcsl_string * result);

/**
 * Check to see if all the chars in the key of a property are valid.
 *
 * @param jadkey key to check
 * @return BAD_JAD_KEY if a character is not valid for a key
 */
static MIDPError checkJadKeyChars(const pcsl_string * jadkey);

/**
 * Check to see if all the chars in the value of a property are valid.
 *
 * @param jadvalue value to check
 * @return BAD_JAD_VALUE if a character is not valid for a value
 */
static MIDPError checkJadValueChars(const pcsl_string * jadvalue);

/**
 * Counts a lines in jad file. Skips commented out and blank lines.
 *
 * @param buf buffer that contains the jad file
 * @return number of lines
 */
static int count_jad_lines(jchar* buf);

MidpProperties jad_main(char* jadbuf, int jadsize) {

    MidpProperties jadsmp = {0,ALL_OK,NULL};
    jchar* jchar_buffer = NULL;
    jchar* save_jchar_buffer = NULL;
    int jbufsize = -1;
#if REPORT_LEVEL <= LOG_INFORMATION
    int res = 0;
#endif
    jbufsize = jadsize * sizeof(jchar);

    jchar_buffer = (jchar*)midpMalloc(jbufsize+2);
    if (!jchar_buffer) {
        midpFree(jadbuf);
        jadsmp.status = OUT_OF_MEMORY;
        return jadsmp;
    }
    memset(jchar_buffer,0,(jbufsize + 2));

    convertChar2JChar(jadbuf,jchar_buffer,jadsize);
    midpFree(jadbuf);
    save_jchar_buffer = jchar_buffer;



    REPORT_INFO(LC_AMS, "####################### Start JAD parsing");

    /* during execution of this function jadbuf pointer will be changed
       status will be set during midpParseJad() execution */
    jadsmp = midpParseJad(jchar_buffer);
    midpFree(save_jchar_buffer);
    switch (jadsmp.status) {
    case OUT_OF_STORAGE:
        REPORT_WARN1(LC_AMS, "OUT_OF_STORAGE by JAD %d", jadsmp.status);
        midp_free_properties(&jadsmp);
        return jadsmp;

    case NO_JAR_URL_PROP:
        REPORT_WARN1(LC_AMS, "Jad property missing %d", jadsmp.status);
        midp_free_properties(&jadsmp);
        return jadsmp;

    case NO_SUITE_NAME_PROP:
        REPORT_WARN1(LC_AMS, "Jad property missing %d", jadsmp.status);
        midp_free_properties(&jadsmp);
        return jadsmp;

    case NO_SUITE_VENDOR_PROP:
        REPORT_WARN1(LC_AMS, "Jad property missing %d", jadsmp.status);
        midp_free_properties(&jadsmp);
        return jadsmp;

    case NO_SUITE_VERSION_PROP:
        REPORT_WARN1(LC_AMS, "Jad property missing %d", jadsmp.status);
        midp_free_properties(&jadsmp);
        return jadsmp;

    case NO_JAR_SIZE_PROP:
        REPORT_WARN1(LC_AMS, "Jad property missing %d", jadsmp.status);
        midp_free_properties(&jadsmp);
        return jadsmp;

    case NUMBER_ERROR:
        REPORT_INFO1(LC_AMS,
		     "Error during parsing JAR size written in JAD %d",
		     jadsmp.status);
        midp_free_properties(&jadsmp);
        return jadsmp;

    case BAD_PARAMS:
    case BAD_JAD_KEY:
    case BAD_JAD_VALUE:
        REPORT_INFO1(LC_AMS,
		     "Some NOT mandatory Jad properties is not valid %d",
		     jadsmp.status);
        break;

    case ALL_OK:
        REPORT_INFO1(LC_AMS, "Jad ALL_OK %d", jadsmp.status);
        break;

    default:
        /* for an unknown result assuming OUT_OF_MEMORY */
        REPORT_INFO1(LC_AMS, "JAD parse OUT_OF_MEMORY %d", jadsmp.status);
        return jadsmp;
    } /* end of switch */

#if REPORT_LEVEL <= LOG_INFORMATION
    reportToLog(LOG_INFORMATION, LC_AMS,
		"######################### End of JAD parsing\n"
		"jad_main() : number of JAD properties = %d",
		jadsmp.numberOfProperties);

    for (res = 0; res < jadsmp.numberOfProperties*2; res+=2 ) {
        printPcslStringWithMessage(" ", &jadsmp.pStringArr[res]);
        printPcslStringWithMessage(" ", &jadsmp.pStringArr[res+1]);
    }
#endif

    return jadsmp;
} /* end of jad_main */

/**
 * Every property occupies one line of the input stream. Each line
 * is terminated by a line terminator which can be a LF, or (CR LF).
 * Lines from the input stream are processed until
 * end of file is reached on the input stream.
 * <p>
 * Every line describes one property to be added to the table.
 * The key consists of all the characters from beginning of the line
 * up to, but not including the first ASCII <code>:</code>.
 * All remaining characters on the line become part of the associated
 * element. The element is also trimmed of leading and trailing
 * whitespace.
 * <p>
 * This method will try to continue after a format error and load as
 * many properties it can, but return the last error encountered.
 *
 * @param jchar_buffer Buffer of jchars that contains jad file data
 *                     (null-terminated jchars string)
 * @return MidpProperties structure filled with keys and values from the jad
 */

static MidpProperties midpParseJad(jchar* jchar_buffer) {
    MidpProperties jadsmp = {0, ALL_OK, NULL};
    pcsl_string jadkey;
    pcsl_string jadkey_trimmed;
    pcsl_string jadvalue;
    pcsl_string jadvalue_trimmed;
    pcsl_string jadline;
    MIDPError err;
    pcsl_string_status rc;

    int countLines = 0;
    jint index = 0;
    int count = 0;

    if (!jchar_buffer) {
        jadsmp.status = BAD_PARAMS;
        return jadsmp;
    }

    countLines = count_jad_lines(jchar_buffer);

    if (countLines <= 0) {
        REPORT_INFO(LC_AMS, "midpParseJad(): Empty jad file.");
        jadsmp.status = OUT_OF_MEMORY;
        return jadsmp;
    }

    jadsmp.pStringArr = alloc_pcsl_string_list(countLines * 2);
    if (jadsmp.pStringArr == NULL) {
        jadsmp.status = OUT_OF_MEMORY;
        return jadsmp;
    }

    jadsmp.numberOfProperties = countLines;

    for (count = 0; count < countLines * 2 ;
        /* count increased at the end of for */ ) {
        /* memory for the line is allocated here */
        err = readJadLine(&jchar_buffer, &jadline);
        if (OUT_OF_MEMORY == err) {
            midp_free_properties(&jadsmp);
            jadsmp.status = OUT_OF_MEMORY;
            return jadsmp;
        } else if (END_OF_JAD == err) {
            /* we are done */
            jadsmp.status = ALL_OK;
            break;
        }

        index = pcsl_string_index_of(&jadline, ':');
        if (index <= 0) {
            jadsmp.status = BAD_JAD_KEY;
            pcsl_string_free(&jadline);
            continue;
        }

        /* memory for key is allocated here */
        if (PCSL_STRING_OK !=
            pcsl_string_substring(&jadline, 0, index, &jadkey)) {
            midp_free_properties(&jadsmp);
            jadsmp.status = OUT_OF_MEMORY;
            pcsl_string_free(&jadline);
            return jadsmp;
        }

        rc = pcsl_string_trim(&jadkey, &jadkey_trimmed);
        pcsl_string_free(&jadkey);
        if (PCSL_STRING_OK != rc) {
            jadsmp.status = OUT_OF_MEMORY;
            midp_free_properties(&jadsmp);
            pcsl_string_free(&jadline);
            return jadsmp;
        }

        if (pcsl_string_length(&jadkey_trimmed) < 1) {
            jadsmp.status = BAD_PARAMS;
            pcsl_string_free(&jadline);
            pcsl_string_free(&jadkey_trimmed);
            continue;
        }

        err = checkJadKeyChars(&jadkey_trimmed);
        if (OUT_OF_MEMORY == err) {
            jadsmp.status = OUT_OF_MEMORY;
            midp_free_properties(&jadsmp);
            pcsl_string_free(&jadline);
            pcsl_string_free(&jadkey_trimmed);
            return jadsmp;
        } else if (BAD_JAD_KEY == err) {
            jadsmp.status = BAD_JAD_KEY;
            pcsl_string_free(&jadline);
            pcsl_string_free(&jadkey_trimmed);
            continue;
        }

        rc = pcsl_string_substring(&jadline, index + 1,
                                   pcsl_string_length(&jadline), &jadvalue);
        /* free the jadline once we have got the key and value */
        pcsl_string_free(&jadline);
        if (PCSL_STRING_OK != rc) {
            jadsmp.status = OUT_OF_MEMORY;
            midp_free_properties(&jadsmp);
            pcsl_string_free(&jadkey_trimmed);
            return jadsmp;
        }

        /* memory for value is allocated here */
        rc = pcsl_string_trim(&jadvalue, &jadvalue_trimmed);
        pcsl_string_free(&jadvalue);
        if (PCSL_STRING_OK != rc) {
            jadsmp.status = OUT_OF_MEMORY;
            midp_free_properties(&jadsmp);
            pcsl_string_free(&jadkey_trimmed);
            return jadsmp;
        }
        if (pcsl_string_is_null(&jadvalue_trimmed)) {
            jadsmp.status = NULL_LEN;
            pcsl_string_free(&jadkey_trimmed);
            continue;
        }

        err = checkJadValueChars(&jadvalue_trimmed);
        if (OUT_OF_MEMORY == err) {
            jadsmp.status = OUT_OF_MEMORY;
            midp_free_properties(&jadsmp);
            pcsl_string_free(&jadkey_trimmed);
            pcsl_string_free(&jadvalue_trimmed);
            return jadsmp;
        } else if (BAD_JAD_VALUE == err) {
            jadsmp.status = BAD_JAD_VALUE;
            pcsl_string_free(&jadkey_trimmed);
            pcsl_string_free(&jadvalue_trimmed);
            continue;
        }

        printPcslStringWithMessage("midpParseJad()", &jadkey_trimmed);
        printPcslStringWithMessage("midpParseJad()", &jadvalue_trimmed);

        /* Store key:value pair. */
        jadsmp.pStringArr[count] = jadkey_trimmed;
        jadsmp.pStringArr[count+1] = jadvalue_trimmed;

        count += 2;
    } /* end of for */

    jadsmp = verifyJadMustProperties(jadsmp);

    REPORT_INFO3(LC_AMS,
                 "End jad parsing. Status=%d, count=%d, countLines=%d.",
                 jadsmp.status, count, countLines);

    return jadsmp;
} /* end of midpParseJad */

static MidpProperties verifyJadMustProperties(MidpProperties jadsmp) {

    /* 5 MUST fields in JAD */
    pcsl_string * name = NULL;
    pcsl_string * version = NULL;
    pcsl_string * vendor = NULL;
    pcsl_string * jarUrl = NULL;
    pcsl_string * jarSizeString = NULL;

    int permittedJarSize = 0;
    jint jarSizeByJad = 0;

    jarUrl = midp_find_property(&jadsmp, &JAR_URL_PROP);
    if (pcsl_string_is_null(jarUrl)) {
        REPORT_INFO(LC_AMS,  "Missing Jar URL");
        jadsmp.status = NO_JAR_URL_PROP;
        return jadsmp;
    }

    jarSizeString = midp_find_property(&jadsmp, &JAR_SIZE_PROP);
    if (pcsl_string_is_null(jarSizeString)) {
        REPORT_INFO(LC_AMS,  "Missing Jar size");
        jadsmp.status = NO_JAR_SIZE_PROP;
        return jadsmp;
    }

    if (PCSL_STRING_OK !=
        pcsl_string_convert_to_jint(jarSizeString, &jarSizeByJad)) {
        /* NUMBER_ERROR */
        REPORT_INFO1(LC_AMS, "JAD size ERROR %d", jarSizeByJad);
        jadsmp.status = NUMBER_ERROR;
        return jadsmp;
    }

    /* verify that requested jar size is not to big  */
    permittedJarSize = midpGetMaxJarSizePermitted();
    if (jarSizeByJad > permittedJarSize) {
        REPORT_INFO2(LC_AMS, "Jar size requested by Jad is to big %d > %d",
		     jarSizeByJad, permittedJarSize);
        REPORT_INFO(LC_AMS,  "Out Of Storage");
        jadsmp.status = OUT_OF_STORAGE;
        return jadsmp;
    }

    name = midp_find_property(&jadsmp, &SUITE_NAME_PROP);
    if (pcsl_string_is_null(name)) {
        REPORT_INFO(LC_AMS, "Missing suite name");
        jadsmp.status = NO_SUITE_NAME_PROP;
        return jadsmp;
    }

    vendor = midp_find_property(&jadsmp, &SUITE_VENDOR_PROP);
    if (pcsl_string_is_null(vendor)) {
        REPORT_INFO(LC_AMS,  "Missing suite vendor");
        jadsmp.status =  NO_SUITE_VENDOR_PROP;
        return jadsmp;
    }

    version = midp_find_property(&jadsmp, &SUITE_VERSION_PROP);
    if (pcsl_string_is_null(version)) {
        REPORT_INFO(LC_AMS,  "Missing suite version");
        jadsmp.status = NO_SUITE_VERSION_PROP;
        return jadsmp;
    }
    if (!midpCheckVersion(version)) {
        REPORT_INFO(LC_AMS,  "Corrupted suite version");
        jadsmp.status = NO_SUITE_VERSION_PROP;
        return jadsmp;
    }
    return jadsmp;
} /* verifyJadMUSTProperties */

/**
 * Check to see if all the chars in the key of a property are valid.
 *
 * @param jadkey key to check
 * @return BAD_JAD_KEY if a character is not valid for a key
 */
static MIDPError checkJadKeyChars(const pcsl_string * jadkey) {
    jchar current;
    int i = 0;
    MIDPError err = ALL_OK;

    GET_PCSL_STRING_DATA_AND_LENGTH(jadkey)
    if (PCSL_STRING_PARAMETER_ERROR(jadkey)) {
        err = OUT_OF_MEMORY;
    } else {
        while (i < jadkey_len) {
            current = jadkey_data[i];
            if (current <= 0x1F
             || current == 0x7F
             || current == '('
             || current == ')'
             || current == '<'
             || current == '>'
             || current == '@'
             || current == ','
             || current == ';'
             || current == '\''
             || current == '"'
             || current == '/'
             || current == '['
             || current == ']'
             || current == '?'
             || current == '='
             || current == '{'
             || current == '}'
             || current == SP
             || current == HT) {
                printPcslStringWithMessage("checkJadKeyChars: BAD_JAD_KEY ",
                                           jadkey);
                err = BAD_JAD_KEY;
                break;
            }
            i++;
        } /* end of while */
    }
    RELEASE_PCSL_STRING_DATA_AND_LENGTH

    return err;
}

/**
 * Check to see if all the chars in the value of a property are valid.
 *
 * @param value value to check
 * @return BAD_JAD_VALUE if a character is not valid for a value
 */
static MIDPError checkJadValueChars(const pcsl_string * jadvalue) {
    jchar current;
    int i = 0;
    MIDPError err = ALL_OK;

    GET_PCSL_STRING_DATA_AND_LENGTH(jadvalue)
    if (PCSL_STRING_PARAMETER_ERROR(jadvalue)) {
        err = OUT_OF_MEMORY;
    } else {
        while (i < jadvalue_len) {
            current = jadvalue_data[i];
            /* if current is a CTL character, return an error */
            if ((current <= 0x1F || current == 0x7F) && (current != HT)) {
                printPcslStringWithMessage("checkJadValueChars: BAD_JAD_VALUE ",
                                           jadvalue);
                err = BAD_JAD_VALUE;
                break;
            }
            i++;
        } /* end of while */
    }
    RELEASE_PCSL_STRING_DATA_AND_LENGTH

    return err;
}


/**
 * Counts a lines in jad file. Skips commented out and blank lines.
 *
 * @param buf jad file data
 * @return number of lines
 */
/* IMPL NOTE: Looks like this function counts one line extra sometime. why ? */
static int count_jad_lines(jchar* buf) {
    int numberOfLines = 0;

    if ((!buf) || (!*buf)) {
        return -1;
    }

    while (*buf) {
        /* skip commented out lines and lines that start with space */
        if (COMMENTED_OUT(buf)) {
            /* run to the end of line */
            while (!NEW_LINE(buf)) {
                buf++;
            }
            /* skip all the new line characters an the end of line */
            while (NEW_LINE(buf)) {
                buf++;
            }
        } else {
            numberOfLines++;
            while (!NEW_LINE(buf)) {
                buf++;
            }
            while (NEW_LINE(buf)) {
                buf++;
            }
        }
    } /* end of while */

    return numberOfLines;
} /* end of count_jad_lines */

/**
 * Reads a line from a jad file.
 *
 * @param jadbuf pointer to the jad jchar_buffer; this pointer will move to
 *               the next line
 * @param result pointer to pcsl_string where the new line from jad file
 *               will be stored, the caller is responsible for freeing this
 *               string
 * @return error code
 */
static MIDPError readJadLine(jchar** jadbuf, pcsl_string * result) {

    jchar* lineStart = NULL;
    jchar* p = NULL;
    int count = 0;

    /* will use p to avoid all the *(*jadbuf) stuff and make it more readable */
    p = (*jadbuf);

    if (!(*p)) {
        /* end of jchar_buffer */
        *result = PCSL_STRING_NULL;
        return END_OF_JAD;
    }

    /* skip commented out and blank lines */
    while (COMMENTED_OUT(p) || NEW_LINE(p)) {
        while (!NEW_LINE(p)) {
            /* skip commented out line */
            p++;
        }
        while (NEW_LINE(p)) {
            /* skip new line */
            p++;
        }
    }

    lineStart = p;

    for (;*p ; ) {
        count++;
        p++;
        if (NEW_LINE(p)) {
            *p = 0x00; /* cut the line */
            if (*(p+1)) { /* if not end of jchar_buffer */
                p++; /* point to the next line beginning */
                break;
            } /* end of if */
        } /* end of if */
    } /* end of for */

    /* move jadbuf to point to the next line */
    (*jadbuf) = p;

    if (PCSL_STRING_OK !=
        pcsl_string_convert_from_utf16(lineStart, count, result)) {
        return OUT_OF_MEMORY;
    }

    return ALL_OK;
} /* end of readJadLine */

/**
 * Opens a file and fills the content of the file in the result_buf. <BR>
 * This function made memory allocation inside.
 *
 * @param filename   Path to the file.
 * @param result_buf Pointer to the buffer that will be filled by content of the file.
 * @return buffer file size in bytes
 */
long readJadFile(const pcsl_string * filename, char** result_buf) {
    int fd = 0;
    char* err = NULL;
    long bufsize = -1;
    int numread = 0;
    char* res = *result_buf;

    if (pcsl_string_length(filename) <= 0) {
        REPORT_INFO(LC_AMS, "readJadFile():No file name.");
        return BAD_PARAMS;
    }

    fd = storage_open(&err, filename, OPEN_READ);
    if(err != NULL) {
        REPORT_INFO1(LC_AMS, "readJadFile():Can't open jad file '%s'",err);
        storageFreeError(err);
        return NO_JAD_FILE;
    }

    bufsize = storageSizeOf(&err, fd);
    if((bufsize <= 0) || (err != NULL)) {
        REPORT_INFO1(LC_AMS,  "readJadFile():Problem getting file size: %s",
		     err );
        storageFreeError(err);
        return IO_ERROR;
    }

    res = (char*)midpMalloc(bufsize+1);
    if (res == NULL || (err != NULL)) {
        REPORT_INFO1(LC_AMS, "readJadFile():Can't allocate memory. %s", err);
        storageFreeError(err);
        return OUT_OF_MEMORY;
    }

    memset(res,0,(bufsize+1));

    REPORT_INFO2(LC_AMS, "fd = %d, bufsize = %ld\n", fd, bufsize);

    numread = storageRead(&err, fd, res, bufsize);
    if((numread <= 0) || (numread != bufsize) || (err)) {
        REPORT_INFO3(LC_AMS, "size = %ld, numread = %d, err = %s.",
               bufsize, numread, err);
        storageClose(&err, fd);
        return IO_ERROR;
    }

    REPORT_INFO2(LC_AMS, "size = %ld, numread = %d", bufsize, numread);

    storageClose(&err, fd);
    if(err != NULL) {
        REPORT_INFO1(LC_AMS, "Can't close jad file %s\n", err);
    }
    *result_buf = res;
    return bufsize;
} /* end of readJadFile */



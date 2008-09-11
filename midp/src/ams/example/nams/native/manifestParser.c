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

#include <midpMalloc.h>
#include <fileInstallerInternal.h>
#include <midp_logging.h>
#include <midpUtilKni.h>
#include <suitestore_common.h>

/* IMPL NOTE: why have separate static functions for manifest and jad parsing? */

/**
 * There are six properties that MUST be defined in the Manifest file.
 * Those properties are:
 * MIDlet-Name
 * MIDlet-Profile
 * MIDlet-Configuration
 * MIDlet-1
 * MIDlet-Vendor
 * MIDlet-Version
 * Check that Midlet-Version value is valid done also.
 *
 * @param mfsmp  struct that contains parsed Manifest.
 * @return Same struct with struct.status value set accordingly.
 *         If some property is missing relevant status returned:
 *         NO_SUITE_VENDOR_PROP
 *         NO_SUITE_NAME_PROP
 *         NO_SUITE_VERSION_PROP
 *         NO_MIDLET_ONE_PROP
 *         NO_MICROEDITION_PROFILE_PROP
 *         NO_MICROEDITION_CONFIGURATION_PROP
 *         If Midlet-Version property value is invalid:
 *         BAD_SUITE_VERSION_PROP
 */
static MidpProperties verifyMfMustProperties(MidpProperties mfsmp);

/**
 * Takes a jchar buffer with converted manifest there and parses it.<BR>
 * jchar_buffer should be allocated and freed outside of this function.
 * jchar_buffer pointer will be changed during parsing.
 * This pointer should be saved before execution to allow memory freeing after.
 *
 * @param jchar_buffer Buffer with manifest converted to jchars.
 * @return MidpProperties struct filled with parsed manifest key:value fields.
 */
static MidpProperties midpParseMf(jchar* jchar_buffer);

/**
 * Reads a line from a manifest file. Compacts continuation lines.<BR>
 * (Allocates memory for this line.)
 *
 * @param mfbuf  pointer to the manifest jchar_buffer; this pointer will move to
 *               the next line
 * @param result pointer to pcsl_string where the new line from manifest
 *               will be stored
 * @return error code
 */
static MIDPError readMfLine(jchar** mfbuf, pcsl_string * result);

/**
 * Check to see if all the chars in the value of a property are valid.
 *
 * @param mfvalue value to check
 * @return BAD_MF_VALUE if a character is not valid for a value
 */
static MIDPError checkMfValueChars(const pcsl_string * mfvalue);

/**
 * Check to see if all the chars in the key of a property are valid.
 *
 * @param mfkey key to check
 * @return BAD_MF_KEY if a character is not valid for a key
 */
static MIDPError checkMfKeyChars(const pcsl_string * mfkey);

/**
 * Counts a lines in manifest file. Skips commented out and blank lines.
 * If Manifest contains a line like:
 * #key : vaaaaaaaaaallll
 *  ue
 * I mean line with broken line with new line and space when only
 * the first line was commented out.
 * In this case second line will be skipped too.
 *
 * @param buf    Pointer to the buffer that contains a Manifest.
 *               Manifest should be already parsed to jchars.
 * @return Number of valid lines in Manifest.
 */
static int count_mf_lines(jchar* buf);

MidpProperties mf_main(char* mfbuf, int mflength) {

    MidpProperties mfsmp      = {0, ALL_OK, NULL};
    jchar* save_jchar_buffer  = NULL;
    jchar* jchar_buffer       = NULL;
    int jbufsize              = -1;
#if REPORT_LEVEL <= LOG_INFORMATION
    int res                   = 0;
#endif
    if ((mflength <= 0) || (!mfbuf)) {
        mfsmp.status = OUT_OF_MEMORY;
        return mfsmp;
    }

    jbufsize = mflength * sizeof(jchar);

    jchar_buffer = (jchar*)midpMalloc(jbufsize+2);
    if (!jchar_buffer) {
        midpFree(mfbuf);
        mfsmp.status = OUT_OF_MEMORY;
        return mfsmp;
    }
    memset(jchar_buffer,0,(jbufsize + 2));

    convertChar2JChar(mfbuf,jchar_buffer,mflength);

    midpFree(mfbuf);

    save_jchar_buffer = jchar_buffer;

    REPORT_INFO(LC_AMS,
		"#########################  Start of manifest parsing");

    /* during execution of this, jchar_buffer pointer will be changed */
    mfsmp = midpParseMf(jchar_buffer);
    midpFree(save_jchar_buffer);
    switch (mfsmp.status) {

    case NO_SUITE_NAME_PROP:
        midp_free_properties(&mfsmp);
        return mfsmp;

    case NO_SUITE_VENDOR_PROP:
        midp_free_properties(&mfsmp);
        return mfsmp;

    case NO_SUITE_VERSION_PROP:
        midp_free_properties(&mfsmp);
        return mfsmp;

    case NO_MIDLET_ONE_PROP:
        midp_free_properties(&mfsmp);
        return mfsmp;

    case NO_MICROEDITION_PROFILE_PROP:
        midp_free_properties(&mfsmp);
        return mfsmp;

    case NO_MICROEDITION_CONFIGURATION_PROP:
        REPORT_INFO1(LC_AMS,
		     "Manifest MUST property missing %d", mfsmp.status);
        return mfsmp;

    case BAD_PARAMS:
    case BAD_MF_KEY:
    case BAD_MF_VALUE:
        REPORT_INFO1(LC_AMS,
		     "Some NOT MUST Manifest property is not valid %d",
		     mfsmp.status);
        break;

    case ALL_OK:
        REPORT_INFO1(LC_AMS, "Manifest ALL_OK %d", mfsmp.status);
        break;

    default:
        REPORT_INFO1(LC_AMS, "MF parse OUT_OF_MEMORY %d", mfsmp.status);
        /* for unknown result assuming OUT_OF_MEMORY */
        return mfsmp;

    } /* end of switch */

#if REPORT_LEVEL <= LOG_INFORMATION
    reportToLog(LOG_INFORMATION, LC_AMS, "%s %s %d",
		"###########################  End   of manifest parsing:  ",
		"mf_main() : number of Manifest properties = ",
		mfsmp.numberOfProperties);
    for (res = 0; res < mfsmp.numberOfProperties * 2; res += 2) {
        printPcslStringWithMessage(" ", &mfsmp.pStringArr[res]);
        printPcslStringWithMessage(" ", &mfsmp.pStringArr[res+1]);
    }
#endif

    return mfsmp;
} /* end of mf_main */

static MidpProperties midpParseMf(jchar* jchar_buffer) {

    MidpProperties mfsmp = {0, ALL_OK, NULL};
    pcsl_string mfkey;
    pcsl_string mfkey_trimmed;
    pcsl_string mfvalue;
    pcsl_string mfvalue_trimmed;
    pcsl_string mfline;
    MIDPError err;
    pcsl_string_status rc;

    int countLines = 0;
    int index = 0;
    int count = 0;

    if (!jchar_buffer) {
        mfsmp.status = BAD_PARAMS;
        return mfsmp;
    }

    countLines = count_mf_lines(jchar_buffer);

    if (countLines <= 0) {
        REPORT_INFO(LC_AMS, "midpParseMf(): Empty manifest.");
        mfsmp.status = OUT_OF_MEMORY;
        return mfsmp;
    }

    mfsmp.pStringArr = alloc_pcsl_string_list(countLines * 2);
    if (mfsmp.pStringArr == NULL) {
        mfsmp.status = OUT_OF_MEMORY;
        return mfsmp;
    }

    mfsmp.numberOfProperties = countLines;

    for (count = 0; count < countLines * 2 ;
         /* count increased at the end of for */ ) {
        /* memory for the line is allocated here */
        /* line continuation striped out */
        err = readMfLine(&jchar_buffer, &mfline);
        if (OUT_OF_MEMORY == err) {
            midp_free_properties(&mfsmp);
            mfsmp.status = OUT_OF_MEMORY;
            return mfsmp;
        } else if (END_OF_MF == err) {
            /* we are done */
            mfsmp.status = ALL_OK;
            break;
        }

        index = pcsl_string_index_of(&mfline, ':');
        if (index <= 0) {
            mfsmp.status = BAD_MF_KEY;
            pcsl_string_free(&mfline);
            continue;
        }

        /* memory for mfkey is allocated here */
        if (PCSL_STRING_OK !=
            pcsl_string_substring(&mfline, 0, index, &mfkey)) {
            midp_free_properties(&mfsmp);
            mfsmp.status = OUT_OF_MEMORY;
            pcsl_string_free(&mfline);
            return mfsmp;
        }

        rc = pcsl_string_trim(&mfkey, &mfkey_trimmed);
        pcsl_string_free(&mfkey);
        if (PCSL_STRING_OK != rc) {
            mfsmp.status = OUT_OF_MEMORY;
            midp_free_properties(&mfsmp);
            pcsl_string_free(&mfline);
            return mfsmp;
        }

        if (pcsl_string_length(&mfkey_trimmed) < 1) {
            mfsmp.status = BAD_PARAMS;
            pcsl_string_free(&mfline);
            pcsl_string_free(&mfkey_trimmed);
            continue;
        }

        err = checkMfKeyChars(&mfkey_trimmed);
        if (OUT_OF_MEMORY == err) {
            mfsmp.status = OUT_OF_MEMORY;
            midp_free_properties(&mfsmp);
            pcsl_string_free(&mfline);
            pcsl_string_free(&mfkey_trimmed);
            return mfsmp;
        } else if (BAD_MF_KEY == err) {
            mfsmp.status = BAD_MF_KEY;
            pcsl_string_free(&mfline);
            pcsl_string_free(&mfkey_trimmed);
            continue;
        }

        rc = pcsl_string_substring(&mfline, index + 1,
                                   pcsl_string_length(&mfline), &mfvalue);
        /* free the mfline once we have got the key and value */
        pcsl_string_free(&mfline);
        if (PCSL_STRING_OK != rc) {
            mfsmp.status = OUT_OF_MEMORY;
            midp_free_properties(&mfsmp);
            pcsl_string_free(&mfkey_trimmed);
            return mfsmp;
        }

        /* memory for value is allocated here */
        rc = pcsl_string_trim(&mfvalue, &mfvalue_trimmed);
        pcsl_string_free(&mfvalue);
        if (PCSL_STRING_OK != rc) {
            mfsmp.status = OUT_OF_MEMORY;
            midp_free_properties(&mfsmp);
            pcsl_string_free(&mfkey_trimmed);
            return mfsmp;
        }
        if (pcsl_string_is_null(&mfvalue_trimmed)) {
            mfsmp.status = NULL_LEN;
            pcsl_string_free(&mfkey_trimmed);
            continue;
        }

        err = checkMfValueChars(&mfvalue_trimmed);
        if (OUT_OF_MEMORY == err) {
            mfsmp.status = OUT_OF_MEMORY;
            midp_free_properties(&mfsmp);
            pcsl_string_free(&mfkey_trimmed);
            pcsl_string_free(&mfvalue_trimmed);
            return mfsmp;
        } else if (BAD_MF_VALUE == err) {
            mfsmp.status = BAD_MF_VALUE;
            pcsl_string_free(&mfkey_trimmed);
            pcsl_string_free(&mfvalue_trimmed);
            continue;
        }

        printPcslStringWithMessage("midpParseMf()", &mfkey_trimmed);
        printPcslStringWithMessage(" = ", &mfvalue_trimmed);

        /* Store key:value pair. */
        mfsmp.pStringArr[count] = mfkey_trimmed;
        mfsmp.pStringArr[count+1] = mfvalue_trimmed;

        count += 2;
    } /* end of for */

    mfsmp = verifyMfMustProperties(mfsmp);
    REPORT_INFO3(LC_AMS,
		 "End of midpParseMf: Status=%d, count=%d, countLines=%d",
		 mfsmp.status, count, countLines);

    return mfsmp;
} /* end of midpParseMf */

static MidpProperties verifyMfMustProperties(MidpProperties mfsmp) {

    /* MUST fields in MANIFEST */
    /* pcsl_string MIDlet-<n> for each MIDlet */
    pcsl_string * midlet_1;
    pcsl_string * name;
    pcsl_string * version;
    pcsl_string * vendor;
    pcsl_string * profile;
    pcsl_string * configuration;

    name = midp_find_property(&mfsmp, &SUITE_NAME_PROP);
    if (pcsl_string_is_null(name)) {
        REPORT_WARN(LC_AMS, "Missing suite name");
        mfsmp.status = NO_SUITE_NAME_PROP;
        return mfsmp;
    }

    vendor = midp_find_property(&mfsmp, &SUITE_VENDOR_PROP);
    if (pcsl_string_is_null(vendor)) {
        REPORT_WARN(LC_AMS, "Missing suite vendor");
        mfsmp.status = NO_SUITE_VENDOR_PROP;
        return mfsmp;
    }

    version = midp_find_property(&mfsmp, &SUITE_VERSION_PROP);
    if (pcsl_string_is_null(version)) {
        REPORT_WARN(LC_AMS, "Missing suite version");
        mfsmp.status = NO_SUITE_VERSION_PROP;
        return mfsmp;
    }
    if (!midpCheckVersion(version)) {
        REPORT_WARN(LC_AMS, "Corrupted suite version");
        mfsmp.status = BAD_SUITE_VERSION_PROP;
        return mfsmp;
    }

    profile = midp_find_property(&mfsmp, &MICROEDITION_PROFILE_PROP);
    if (pcsl_string_is_null(profile)) {
        REPORT_WARN(LC_AMS, "Missing Midp-Profile");
        mfsmp.status = NO_MICROEDITION_PROFILE_PROP;
        return mfsmp;
    }

    configuration = midp_find_property(&mfsmp, &MICROEDITION_CONFIGURATION_PROP);
    if (pcsl_string_is_null(configuration)) {
        REPORT_WARN(LC_AMS, "Missing Midp-Configuration");
        mfsmp.status = NO_MICROEDITION_CONFIGURATION_PROP;
        return mfsmp;
    }

    midlet_1 = midp_find_property(&mfsmp, &MIDLET_ONE_PROP);
    if (pcsl_string_is_null(midlet_1)) {
        REPORT_WARN(LC_AMS, "Missing Midlet-1");
        mfsmp.status = NO_MIDLET_ONE_PROP;
        return mfsmp;
    }
    return mfsmp;
} /* verifyMfMustProperties */


/**
 * Reads a line from a manifest file. Compacts continuation lines.<BR>
 * (Allocates memory for this line.)
 *
 * @param mfbuf  pointer to the manifest jchar_buffer; this pointer will move to
 *               the next line
 * @param result pointer to pcsl_string where the new line from manifest
 *               will be stored
 * @return error code
 */
static MIDPError readMfLine(jchar** mfbuf, pcsl_string * result) {
    jchar* lineStart = NULL;
    int is_broken_line = 0;
    jchar* p = NULL;
    int count = 0;
    int i = 0;

    *result = PCSL_STRING_NULL;

    /* will use p to avoid all the *(*mfbuf) stuff and make it more readable */
    p = (*mfbuf);

    if (!(*p)) {
        /* end of jchar_buffer */
        return END_OF_MF;
    }

    /* skip commented out and blank lines */
    while (COMMENTED_OUT(p) || NEW_LINE(p)) {

        while (!NEW_LINE(p)) {
            p++; /* skip commented out line */
        }
        while (NEW_LINE(p)) {
            p++; /* skip new line */
        }

        /* now pointing to the next line */
        if (MF_SPACE(p)) { /* if starting with space */
            while (!NEW_LINE(p)) {
                p++; /* skip the line */
            }
        } /* end of if */

    } /* end of while */

    lineStart = p;

    for (;*p ;) {
        count++;
        p++;
        if (NEW_LINE(p) && !MF_BROKEN_LINE(p)) {
            *p = 0x00; /* cut the line */
            if (*(p+1)) { /* if not end of the buffer */
                p++; /* point to the next line beginning */
                break;
            }
        } else if (MF_BROKEN_LINE(p)) {
            while (!MF_SPACE(p)) { /* look for the space */
                count++;
                p++;
            }
            /* once space found, point to the next character and go ahead */
            count++;
            p++;
            is_broken_line = 1;
            continue;
        } /* end of else */

    } /* end of for */

    /* move mfbuf to point to the next line */
    (*mfbuf) = p;

    pcsl_string_predict_size(result, count);
    if (is_broken_line) {
        i = 0;
        while (*(lineStart+i)) {

            /* here once we have a new line it will be followed by space */
            if (NEW_LINE_1(lineStart+i)) {
                i+=3;
                count-=3;
            } else if (NEW_LINE(lineStart+i)) {
                i+=2;
                count-=2;
            }

            if (PCSL_STRING_OK !=
                pcsl_string_append_char(result, lineStart[i])) {
                pcsl_string_free(result);
                return OUT_OF_MEMORY;
            }
            i++;
        }
    } else {
        if (PCSL_STRING_OK !=
            pcsl_string_convert_from_utf16(lineStart, count, result)) {
            return OUT_OF_MEMORY;
        }
    }

    return ALL_OK;
} /* end of readMfLine */

/**
 * Counts a lines in manifest file. Skips commented out and blank lines.
 *
 * @param buf
 * @return
 */
static int count_mf_lines(jchar* buf) {
    int numberOfLines = 0;

    if ((!buf) || (!*buf)) {
        return -1;
    }

    while (*buf) {
        /* skip commented out lines and lines that start with space */
        if ((COMMENTED_OUT(buf)) || (MF_SPACE(buf))) {
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
} /* end of count_mf_lines */

/**
 * Check to see if all the chars in the key of a property are valid.
 *
 * @param key key to check
 *
 * @return an error if a character is not valid for a key
 */
static MIDPError checkMfKeyChars(const pcsl_string * mfkey) {
    /* IMPL NOTE: why chars in manifest key are different from jad key? */
    jchar current;
    int i = 0;
    MIDPError err = ALL_OK;

    GET_PCSL_STRING_DATA_AND_LENGTH(mfkey)
    if (PCSL_STRING_PARAMETER_ERROR(mfkey)) {
        err = OUT_OF_MEMORY;
    } else {
        while (i < mfkey_len) {
            current = mfkey_data[i];
            i++;

            if (current >= 'A' && current <= 'Z') {
                continue;
            }

            if (current >= 'a' && current <= 'z') {
                continue;
            }

            if (current >= '0' && current <= '9') {
                continue;
            }

            if ((i - 1) > 0 && (current == '-' || current == '_')) {
                continue;
            }

            printPcslStringWithMessage("checkMfKeyChars: BAD_MF_KEY ", mfkey);
            err = BAD_MF_KEY;
            break;
        } /* end of while */
    }
    RELEASE_PCSL_STRING_DATA_AND_LENGTH

    return err;
} /* end of checkMfKeyChars */

/**
 * Check to see if all the chars in the value of a property are valid.
 *
 * @param value value to check
 *
 * @return false if a character is not valid for a value
 */
static MIDPError checkMfValueChars(const pcsl_string * mfvalue) {
    jchar current;
    int i = 0;
    MIDPError err = ALL_OK;

    GET_PCSL_STRING_DATA_AND_LENGTH(mfvalue)
    if (PCSL_STRING_PARAMETER_ERROR(mfvalue)) {
        err = OUT_OF_MEMORY;
    } else {
        while (i < mfvalue_len) {
            current = mfvalue_data[i];
            /* if current is a CTL character, return an error */
            if ((current <= 0x1F || current == 0x7F) && (current != HT)) {
                printPcslStringWithMessage("checkMfValueChars: BAD_MF_VALUE ",
                                           mfvalue);
                err = BAD_MF_VALUE;
                break;
            }
            i++;
        } /* end of while */
    }
    RELEASE_PCSL_STRING_DATA_AND_LENGTH

    return err;
} /* end of checkMfValueChars */




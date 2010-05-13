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

#include <midpMalloc.h>
#include <midpStorage.h>
#include <midpDataHash.h>
#include <fileInstallerInternal.h>
#include <midp_logging.h>
#include <jvm.h>
#include <pcsl_string.h>
#include <midpUtilKni.h>
#include <push_server_export.h>
#include <suitestore_installer.h>
#include <suitestore_secure.h> /* for VERIFY_ONCE */

#ifdef ENABLE_JSR_211
#include <jsr211_nams_installer.h>
#endif  /* ENABLE_JSR_211 */

PCSL_DEFINE_ASCII_STRING_LITERAL_START(JAR_URL_PROP)
    {'M', 'I', 'D', 'l', 'e', 't', '-',
     'J', 'a', 'r','-', 'U', 'R', 'L', '\0'}
PCSL_DEFINE_ASCII_STRING_LITERAL_END(JAR_URL_PROP);

PCSL_DEFINE_ASCII_STRING_LITERAL_START(JAR_SIZE_PROP)
    {'M', 'I', 'D', 'l', 'e', 't', '-',
     'J', 'a', 'r','-', 'S', 'i', 'z', 'e', '\0'}
PCSL_DEFINE_ASCII_STRING_LITERAL_END(JAR_SIZE_PROP);

PCSL_DEFINE_ASCII_STRING_LITERAL_START(MIDLET_ONE_PROP)
    {'M', 'I', 'D', 'l', 'e', 't', '-', '1', '\0'}
PCSL_DEFINE_ASCII_STRING_LITERAL_END(MIDLET_ONE_PROP);

PCSL_DEFINE_ASCII_STRING_LITERAL_START(MICROEDITION_PROFILE_PROP)
    {'M', 'i', 'c', 'r', 'o',
     'E', 'd', 'i', 't', 'i','o', 'n',
     '-', 'P','r','o','f','i','l','e', '\0'}
PCSL_DEFINE_ASCII_STRING_LITERAL_END(MICROEDITION_PROFILE_PROP);

PCSL_DEFINE_ASCII_STRING_LITERAL_START(MICROEDITION_CONFIGURATION_PROP)
    {'M', 'i', 'c', 'r', 'o', 'E', 'd', 'i', 't', 'i', 'o', 'n',
     '-', 'C', 'o', 'n', 'f', 'i', 'g', 'u', 'r', 'a', 't', 'i', 'o', 'n', '\0'}
PCSL_DEFINE_ASCII_STRING_LITERAL_END(MICROEDITION_CONFIGURATION_PROP);

PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(class_suffix)
    {'.','c','l','a','s','s', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(class_suffix);

extern char* getApplicationDir(char *cmd);

PCSL_DEFINE_ASCII_STRING_LITERAL_START(ENTRY_NAME)
    {'M', 'E', 'T', 'A', '-', 'I', 'N', 'F', '/',
     'M','A', 'N', 'I', 'F', 'E', 'S', 'T',
     '.', 'M', 'F', '\0'}
PCSL_DEFINE_ASCII_STRING_LITERAL_END(ENTRY_NAME);

PCSL_DEFINE_ASCII_STRING_LITERAL_START(TEMP_JAR_NAME)
    {'t', 'e', 'm', 'p', '.', 'j', 'a', 'r', '\0'}
PCSL_DEFINE_ASCII_STRING_LITERAL_END(TEMP_JAR_NAME);

/* Dummy implementations of functions needed to run in NamsTestService mode. */
#if !ENABLE_I3_TEST
void nams_test_service_setup_listeners() {}
void nams_test_service_remove_listeners() {}
#endif

/**
 * Copies a file.
 *
 * @param srcName source file
 * @param destName destination file
 *
 * @return 0 for success
 */
static int copyFile(const pcsl_string * srcName, const pcsl_string * destName) {
    char* pszError = NULL;
    char* pszDummy = NULL;
    int src;
    int dest;
    char buffer[1024];
    long bytesRead;

    src = storage_open(&pszError, srcName, OPEN_READ);
    if (pszError == NULL) {
        dest = storage_open(&pszError, destName, OPEN_READ_WRITE_TRUNCATE);
        if (pszError == NULL) {
            bytesRead = storageRead(&pszError, src, buffer, sizeof (buffer));
            while (pszError == NULL && bytesRead > 0) {
                storageWrite(&pszError, dest, buffer, bytesRead);
                if (pszError == NULL) {
                    bytesRead = storageRead(&pszError, src, buffer,
                                            sizeof (buffer));
                }
            }

            storageClose(&pszDummy, dest);
            storageFreeError(pszDummy);
            pszDummy = NULL;
        }

        storageClose(&pszDummy, src);
        storageFreeError(pszDummy);
    }

    if (pszError != NULL) {
        REPORT_ERROR1(LC_AMS, "Error while copying file: %s", pszError);
        storageFreeError(pszError);
        return -1;
    }

    return 0;
}
/**
 * Converts char buffer to jchar buffer
 *
 * @param char_buf  pointer to already allocated char buffer
 * @param jchar_buf pointer to already allocated jchar buffer
 * @param char_buf_size
 *                  size of the char buf
 */
void convertChar2JChar(char* char_buf, jchar* jchar_buf, int char_buf_size) {
    while ((*char_buf) && (char_buf_size--)) {
        *jchar_buf = (jchar)*char_buf;
        char_buf++;
        jchar_buf++;
    }
} /* end of convertChar2JChar */

/**
 * Converts jchar buffer to char buffer
 *
 * @param char_buf  pointer to char buffer
 * @param jchar_buf pointer to jchar buffer
 * @param char_buf_size
 *                  size of the char buf
 */
void convertJChar2Char(jchar* jchar_buf, char* char_buf, int jchar_buf_size) {
    int i;
    for (i = 0; i < jchar_buf_size; i++) {
        *(char_buf+i) = (char)(*(jchar_buf+i));
    }
}/* end of convertJChar2Char */

/**
 * Version string may looks like Major:Minor:Micro
 *
 * @param ver    pcsl_string that contains a version
 * @param major  Major version
 * @param minor  Minor version
 * @param micro  Micro version
 * @return 1 == OK
 *         0 = BAD version
 */
int midpGetVersion(const pcsl_string * ver, int *major, int *minor, int *micro) {
    int ma = 0;
    int mi = 0;
    int mc = 0;
    int count = 0;
    int dot_count = 0;
    int segment_size = 0;
    const jchar* p = NULL;
    const jchar* ver_data;
    jsize ver_len = pcsl_string_utf16_length(ver);

    *major = -1;
    *minor = -1;
    *micro = -1;

    if ((ver_len <= 0) || (ver_len > 8)) {
        return 0;
    }
    printPcslStringWithMessage("ver", ver);

    ver_data = pcsl_string_get_utf16_data(ver);
    if (NULL == ver_data) {
        return 0;
    }

    /* most checking done here */
    for (count=0; count < ver_len; count++) {
        if ((ver_data[count] >= '0') && (ver_data[count] <= '9')) {
            segment_size++;
        } else if (ver_data[count] == '.') {
            if ((segment_size == 0) || (segment_size > 2)) {
                REPORT_ERROR1(LC_AMS, "segment size wrong %d",
                              segment_size);
                pcsl_string_release_utf16_data(ver_data, ver);
                return 0;
            }
            dot_count++;
            segment_size = 0;
        } else {
            pcsl_string_release_utf16_data(ver_data, ver);
            return 0;
        }
    } /* end of for */

    /* can't be more then 2 dots in version */
    if (dot_count > 2) {
        REPORT_ERROR1(LC_AMS, "too many dots (%d)", dot_count);
        pcsl_string_release_utf16_data(ver_data, ver);
        return 0;
    }

    /*
     * Get major version
     */
    for (p = ver_data, count = 0; (*p != '.') && (count < ver_len); ) {
        if (*p >= '0' && *p <= '9') {
            ma *= 10;
            ma += *p - '0';
        } else {
            pcsl_string_release_utf16_data(ver_data, ver);
            return 0;
        }
        count++;
        p++;
    } /* end of for */

    if(*p == '.') {
        p++;
        count++;
    }

    /*
     * Get minor version.
     */
    for ( ; (*p != '.') && (count < ver_len); ) {
        if (*p >= '0' && *p <= '9') {
            mi *= 10;
            mi += *p - '0';
        } else {
            pcsl_string_release_utf16_data(ver_data, ver);
            return 0;
        }
        count++;
        p++;
    }

    if(*p == '.') {
        p++;
        count++;
    }

    /*
     * Get micro version; if it exists..
     */
    for ( ; (*p != '.') && (count < ver_len); ) {
        if (*p >= '0' && *p <= '9') {
            mc *= 10;
            mc += *p - '0';
        } else {
            pcsl_string_release_utf16_data(ver_data, ver);
            return 0;
        }
        p++;
        count++;
    }
    *major = ma;
    *minor = mi;
    *micro = mc;
    pcsl_string_release_utf16_data(ver_data, ver);
    return 1;
}

/**
 * Compares two versions.
 * A version must be "<major>.<minor>".
 * Both <major> and <minor> must be a string of 1 to 3 decimal digits.
 * <BR>
 * <B>Note:</B>
 * (1) If both version strings are invalid, treat them as the same version.
 * (2) If only the second version is valid, treat the second version as newer.
 * (3) If only the first version is valid, treat the first version as newer.
 *
 * @param ver1   First version
 * @param ver2   Second version
 * @return < 0 if ver1 < ver2
 *         = 0 if ver1 = ver2
 *         > 0 if ver1 > ver2
 */
int midpCompareVersion(const pcsl_string * ver1, const pcsl_string * ver2) {
    int major1, minor1, micro1;
    int major2, minor2, micro2;
    int error1, error2;

    error1 = (midpGetVersion(ver1, &major1, &minor1, &micro1) != 1);
    error2 = (midpGetVersion(ver2, &major2, &minor2, &micro2) != 1);

    /*
     * (1) If both version strings are invalid, treat them as the same
     *     version.
     * (2) If only the second version is valid, treat the second version
     *     as newer.
     * (3) If only the first version is valid, treat the first version
     *     as newer.
     */
    if (error1 && error2) {
        return 0;
    }
    if (error1) {
        return -1;
    }
    if (error2) {
        return 1;
    }
    if (major1 < major2) {
        return -1;
    }
    if (major1 > major2) {
        return  1;
    }
    if (minor1 < minor2) {
        return -1;
    }
    if (minor1 > minor2) {
        return  1;
    }
    if (micro1 < micro2) {
        return -1;
    }
    if (micro1 > micro2) {
        return  1;
    }
    return 0;
}

/**
 *
 * @param ver
 * @return 1 == OK
 *         0 = BAD version
 */
int midpCheckVersion(const pcsl_string * ver) {
    int major = 0;
    int minor = 0;
    int micro = 0;
    return midpGetVersion(ver, &major, &minor, &micro);
}

/**
 * Return the max Jar size allow for this platform to install.
 *
 * @note you should change this according to your platform
 *       requirement
 * @return maximum JAR size permitted to download or install
 *         (in bytes).
 */
int midpGetMaxJarSizePermitted() {
    return 1024*512;
}

/**
 * Compares JAD and Manifest file properties that MUST be the same.
 * The properties are:
 * <B>MIDlet-Name</B>
 * <B>MIDlet-Vendor</B>
 * <B>MIDlet-Version</B>
 *
 * @param jadsmp MidpProperties struct that contains parsed JAD properties.
 * @param mfsmp  MidpProperties struct that contains parsed Manifest properties.
 * @return On success: ALL_OK
 *         On mismatch:
 *             SUITE_NAME_PROP_NOT_MATCH
 *             SUITE_VENDOR_PROP_NOT_MATCH
 *             SUITE_VERSION_PROP_NOT_MATCH
 *         On missing property:
 *             NO_SUITE_NAME_PROP
 *             NO_SUITE_VENDOR_PROP
 *             NO_SUITE_VERSION_PROP
 */
int compareJADandManifestProperties(MidpProperties* jadsmp, MidpProperties* mfsmp) {
    /* three properties MUST be similar in jad and manifest */
    const pcsl_string* jad_name = &PCSL_STRING_NULL;
    const pcsl_string* jad_version = &PCSL_STRING_NULL;
    const pcsl_string* jad_vendor = &PCSL_STRING_NULL;

    const pcsl_string* mf_name = &PCSL_STRING_NULL;
    const pcsl_string* mf_version = &PCSL_STRING_NULL;
    const pcsl_string* mf_vendor = &PCSL_STRING_NULL;
    /* compare jad and manifest properties */
    jad_name = midp_find_property(jadsmp, &SUITE_NAME_PROP);
    if (pcsl_string_is_null(jad_name)) {
        REPORT_ERROR(LC_AMS, "Can't get SUITE_NAME_PROP from JAD. This should not happen at this stage!");
        return NO_SUITE_NAME_PROP;
    }
    mf_name = midp_find_property(mfsmp, &SUITE_NAME_PROP);
    if (pcsl_string_is_null(mf_name)) {
        REPORT_ERROR(LC_AMS, "Can't get SUITE_NAME_PROP from Manifest.  This should not happen at this stage!");
        return NO_SUITE_NAME_PROP;
        /* handle it some how */
    }
    if ( ! pcsl_string_equals(jad_name, mf_name)) {
        /* versions are not equal */
        REPORT_ERROR(LC_AMS, "Manifest and Jad names are not equal.");
        return SUITE_NAME_PROP_NOT_MATCH;
    }

    jad_vendor = midp_find_property(jadsmp, &SUITE_VENDOR_PROP);
    if (pcsl_string_is_null(jad_vendor)) {
        REPORT_ERROR(LC_AMS, "Can't get SUITE_VENDOR_PROP from JAD.  This should not happen at this stage!\n");
        return NO_SUITE_VENDOR_PROP;
    }
    mf_vendor = midp_find_property(mfsmp, &SUITE_VENDOR_PROP);
    if (pcsl_string_is_null(mf_vendor)) {
        REPORT_ERROR(LC_AMS, "Can't get SUITE_VENDOR_PROP from Manifest. This should not happen at this stage!");
        return NO_SUITE_VENDOR_PROP;
    }

    if ( ! pcsl_string_equals(jad_vendor, mf_vendor)) {
        /* versions are not equal */
        REPORT_ERROR(LC_AMS, "Manifest and Jad vendors are not equal.");
        return SUITE_VENDOR_PROP_NOT_MATCH;
    }

    jad_version = midp_find_property(jadsmp, &SUITE_VERSION_PROP);
    if (pcsl_string_is_null(jad_version)) {
        REPORT_ERROR(LC_AMS, "Can't get SUITE_VERSION_PROP from JAD. This should not happen at this stage!");
        return NO_SUITE_VERSION_PROP;
    }
    mf_version = midp_find_property(mfsmp, &SUITE_VERSION_PROP);
    if (pcsl_string_is_null(mf_version)) {
        REPORT_ERROR(LC_AMS, "Can't get SUITE_VERSION_PROP from Manifest. This should not happen at this stage!");
        return NO_SUITE_VERSION_PROP;
    }
    {
        if (0 != midpCompareVersion(jad_version, mf_version)) {
            /* versions are not equal */
            REPORT_ERROR(LC_AMS, "Manifest and Jad versions are not equal.");
            return SUITE_VERSION_PROP_NOT_MATCH;
        }
    }

    return ALL_OK;
} /* end of compareJADandManifestProperties */

/**
 * Creates "MIDlet-n" pcsl_string instance.
 * n will be equal to midletNumber
 *
 * @param midletNumber MIDlet number
 * @param res receives the composed "MIDlet-n" string.
 *
 * @return PCSL_STRING_OK on success, error code otherwise
 */
pcsl_string_status prepareMidletTag(int midletNumber, pcsl_string* res) {
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(midletN)
        {'M', 'I', 'D', 'l', 'e', 't', '-', '\0'}
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(midletN);
    pcsl_string num;
    pcsl_string_status rc;

    *res = PCSL_STRING_NULL;

    rc = pcsl_string_convert_from_jint(midletNumber, &num);
    if (PCSL_STRING_OK == rc) {
        rc = pcsl_string_cat(&midletN, &num, res);
        pcsl_string_free(&num);
    }

    return rc;
} /* end of prepareMidletTag */

/**
 * For all midlet numbers n in the range from 1 to 999,
 * compares MIDlet-n name and class values in JAD and Manifest.
 * Name must not be NULL. Class must not be NULL. Name and class must be same in jad and jar.
 * Class must be exist in the jar.
 * The checking stops at n for which MIDlet-n does not exist.
 *
 * returns 0 if MIDlet-0 exists.
 *
 * @param jadsmp JAD properties
 * @param mfsmp Maniffest properties
 * @param jarHandle Handle to the JAR file.
 *
 * @return Number of MIDlets that are found to be ok.
 */
int
verifyMidletN(MidpProperties* jadsmp, MidpProperties* mfsmp, void* jarHandle) {
    const pcsl_string* pmidletNameJAD = &PCSL_STRING_NULL;
    const pcsl_string* pmidletNameMF  = &PCSL_STRING_NULL;
    pcsl_string midletN               = PCSL_STRING_NULL;
    pcsl_string fullClassName         = PCSL_STRING_NULL;
    pcsl_string nameMF                = PCSL_STRING_NULL;
    pcsl_string classMF               = PCSL_STRING_NULL;
    int        count         = 1;
    int        res           = 0;
    int        onlyMF        = 0;

    if ((jadsmp->pStringArr == NULL)&&(mfsmp->pStringArr == NULL)) {
        return RESOURCE_NOT_FOUND;
    } else if ((jadsmp->numberOfProperties == 0) &&
               (mfsmp->numberOfProperties == 0)) {
        return RESOURCE_NOT_FOUND;
    } else if (jadsmp->numberOfProperties == 0) {
        onlyMF = 1;
    }

    /* Create MIDlet-0 tag the should not exist */
    prepareMidletTag(0, &midletN);

    if (!onlyMF) {
        /* verify that it's no MIDlet-0 in the jad */
        pmidletNameJAD = midp_find_property(jadsmp, &midletN);
        if (pcsl_string_length(pmidletNameJAD) > 0) {
            REPORT_INFO(LC_AMS, "MIDlet-0 found in the JAD.");
            pcsl_string_free(&midletN);
            return 0;
        }
    }
    /* verify that it's no MIDlet-0 in the Manifest */
    pmidletNameJAD = midp_find_property(mfsmp, &midletN);
    if (pcsl_string_length(pmidletNameJAD) > 0) {
        REPORT_INFO(LC_AMS, "MIDlet-0 found in the Manifest.");
        pcsl_string_free(&midletN);
        return 0;
    }
    pcsl_string_free(&midletN);

    for (count=1; count < 999; count++) {

        prepareMidletTag(count, &midletN);

        if (!onlyMF) {
            pmidletNameJAD = midp_find_property(jadsmp, &midletN);
            if (pcsl_string_length(pmidletNameJAD) <= 0) {
                REPORT_WARN1(LC_AMS, "Can't find MIDlet-%d in the JAD.",
			     count);
                pcsl_string_free(&midletN);
                count-=1;
                break;
            }
        }

        pmidletNameMF = midp_find_property(mfsmp, &midletN);
        if (pcsl_string_length(pmidletNameMF) <= 0) {
            REPORT_WARN1(LC_AMS, "Can't find MIDlet-%d in the Manifest.",
			 count);
            pcsl_string_free(&midletN);
            count-=1;
            break;
        }

        pcsl_string_free(&midletN);

        if (!onlyMF) {
            if (!pcsl_string_equals(pmidletNameJAD, pmidletNameMF)) {
                /* MIDlet-n values are different in jad and manifest */
                REPORT_WARN1(LC_AMS,
			     "MIDlet-%d in JAD doesn't match the Manifest",
			     count);
                count-=1;
                break;
            }
        }

        /* Since we are here. jad midlet-n equals to manifest midlet-n
           We will continue working with manifest midlet-n only */

        /* check the name */
        midpParseMIDletN(pmidletNameMF, 0, &nameMF);
        if (pcsl_string_length(&nameMF) <= 0) {
            REPORT_WARN1(LC_AMS, "In MIDlet-%d name doesn't exist.",count);
            count-=1;
            pcsl_string_free(&nameMF);
            break;
        }
        /* if name part exists it's ok we don't need it anymore */
        printPcslStringWithMessage(" ", &nameMF);
        pcsl_string_free(&nameMF);

        /* check the class path */
        midpParseMIDletN(pmidletNameMF, 2, &classMF);
        if (pcsl_string_length(&classMF) <= 0) {
            REPORT_WARN1(LC_AMS, "In MIDlet-%d class doesn't exist.", count);
            count-=1;
            pcsl_string_free(&classMF);
            break;
        }
        printPcslStringWithMessage(" ", &classMF);
        /* create a class name */
        {
        /*here we just want to replace dost with slashes. But it's not easy */
            pcsl_string changedPath = PCSL_STRING_NULL;
            int was_error = 1;
            const pcsl_string* const classmf = &classMF;
            GET_PCSL_STRING_DATA_AND_LENGTH(classmf)
            jchar* data = (jchar*)midpMalloc(classmf_len * sizeof (jchar));
            int i;
            if(NULL != data) {
                was_error = 0;
                for (i = 0; i < classmf_len; i++) {
                    if (classmf_data[i] == '.') {
                        data[i] = '/';
                    } else {
                        data[i] = classmf_data[i];
                    }
                }
                if(PCSL_STRING_OK !=
                    pcsl_string_convert_from_utf16(data,
                                    classmf_len, &changedPath)) {
                    was_error = 1;
                }
                midpFree(data);
            }
            RELEASE_PCSL_STRING_DATA_AND_LENGTH
            pcsl_string_free(&classMF);
            classMF = changedPath;

            if (was_error) {
                REPORT_WARN(LC_AMS, "verifyMIDletN:  Out of memory");
                pcsl_string_free(&classMF);
                count = OUT_OF_MEMORY;
                break;
            }
        }

        if (PCSL_STRING_OK !=
            pcsl_string_cat(&classMF, &class_suffix, &fullClassName)) {
            REPORT_WARN(LC_AMS, "verifyMIDletN:  Out of memory");
            pcsl_string_free(&classMF);
            count = OUT_OF_MEMORY;
            break;
        }

#if REPORT_LEVEL <= LOG_INFORMATION
        reportToLog(LOG_INFORMATION, LC_AMS, "Looking for MIDlet-%d class",
                    count);
        printPcslStringWithMessage(" ", &fullClassName);
#endif

        /* check for class existance */
        res = midpJarEntryExists(jarHandle, &fullClassName);
        if (res == 0) {
            REPORT_ERROR1(LC_AMS, "Can't find JAR entry for MIDlet-%d", count);
        } else {
            REPORT_INFO1(LC_AMS, "Class for MIDlet-%d found.", count);
        }
        pcsl_string_free(&classMF);
        pcsl_string_free(&fullClassName);
    } /* end of for */
    return count;
} /* end of verifyMidletN */

/**
 * Parses a comma delimited string and returns the first, second or third
 * element based on the index
 * Sets p_out to PCSL_STRING_NULL in the case of an error.
 *
 * Good string: TilePuzzle, /icons/TilePuzzle.png, example.tilepuzzle.TilePuzzle
 * Good string: TilePuzzle,, example.tilepuzzle.TilePuzzle
 * Bad string: , /icons/TilePuzzle.png, example.tilepuzzle.TilePuzzle
 * Bad string: TilePuzzle, /icons/TilePuzzle.png,
 *
 * @param in pcsl_string with MIDlet-n value
 * @param index Index of MIDlet-n value required.
 * 0 = name
 * 1 = icon
 * 2 = class
 * @param p_out [out] pcsl_string that receives the required value,
 *                    or PCLS_STRING_NULL in the case of an error
 */
void midpParseMIDletN(const pcsl_string* in, int index, pcsl_string* p_out) {
    GET_PCSL_STRING_DATA_AND_LENGTH(in) {
        const jchar* p = in_data;
        const jchar* res = in_data;
        int counter = 0;
        int commaCounter = 0;
        int length = 0;
        int i = 0;

        *p_out = PCSL_STRING_NULL;

        do {

            if ((index > 2) || (index < 0)) {
                break;
            }

            if (in_len <= 0) {
                break;
            }

            for (i = 0, counter = 0; counter < in_len; counter++, p++) {
                if (*p == ',') {
                    i++;
                }
            }
            if (i != 2) {
                break;
            }


            for (counter = 0, p = in_data; counter < in_len; counter++,p++) {
                if (*p == ',') {
                    if (index == commaCounter)
                        break;
                    else {
                        commaCounter++;
                        res = p+1;
                    }
                }
            }

            if ((counter == in_len) && (commaCounter == 2)) {
                /* p is pointing out of the string, bring him back */
                p--;
            }

            /* remove leading spaces */
            while ((res < p) && IS_WHITE_SPACE(res)) {
                res++;
            }

            /* remove trailing spaces and comma */
            while ((p > res) && (IS_WHITE_SPACE(p) || (*p == ','))) {
                p--;
            }


            length = (p - res + 1);
            if (length <= 1) {
                break;
            }

            pcsl_string_convert_from_utf16(res,length,p_out);
        } while (0);
    } RELEASE_PCSL_STRING_DATA_AND_LENGTH
} /* end of midpParseMIDletN */

/*
 *
 *
 */
pcsl_string_status createRelativeURL(const pcsl_string * in,
                                     pcsl_string * result) {
    pcsl_string tmp     = PCSL_STRING_NULL;
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(pathSep)
        {'f', 'i', 'l', 'e', ':', '/', '/', '/', '\0'}
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(pathSep);
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(http_prefix)
        {'h', 't', 't', 'p', '\0'}
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(http_prefix);
    pcsl_string_status rc;

    /* if starting with http return same string copy */
    if (pcsl_string_starts_with(in, &http_prefix)) {
        rc = pcsl_string_dup(in, result);
    } else {
        jchar dot_temp[] = {'.', '\0'};
        pcsl_string dot_prefix = PCSL_STRING_NULL;
        dot_temp[1] = storageGetFileSeparator();
        rc = pcsl_string_convert_from_utf16(dot_temp, 2, &dot_prefix);
        if (PCSL_STRING_OK != rc) {
            return rc;
        }

        if (pcsl_string_starts_with(in, &dot_prefix)) {
            pcsl_string_free(&dot_prefix);
            rc = pcsl_string_substring(in, 2, pcsl_string_length(in), &tmp);
            if (PCSL_STRING_OK != rc) {
                return rc;
            }
            rc = pcsl_string_cat(&pathSep, &tmp, result);
            pcsl_string_free(&tmp);
        } else {
            rc = pcsl_string_cat(&pathSep, in, result);
        }
    }

    return rc;
} /* end of createRelativeURL */

/**************************************************************************/
/**
 * Installs a midlet suite from a file. This is an example of how to use
 * the public MIDP API.
 *
 * fileInstaller could be launched as executable or as a function call.
 *
 * @param argc The total number of arguments
 * @param argv An array of 'C' strings containing the arguments
 *
 * @return <tt>0</tt> for success, otherwise <tt>-1</tt>
 */
int fileInstaller(int argc, char* argv[]) {
    /* temporary variable to contain return results */
    int     res                    = 0;
    /* pointer to the manifest extracted from the jar */
    char*   mf_buf                 = NULL;
    /* pointer to the jad */
    char*   jad_buf                = NULL;
    int     jadsize                = 0;
    int     realJarSize            = 0;
    long    length                 = 0;
    void*   jarHandle              = NULL;
    int     pszError               = 0;
    jint    jarSizeByJad           = 0;
    int     permittedJarSize       = 0;
    int     usingWhat              = INIT_STAT;

    /* pointer to the structures that contains parsed jad */
    MidpProperties jadsmp          = {0, ALL_OK, NULL};
    /* pointer to the structures that contains parsed jad */
    MidpProperties mfsmp           = {0, ALL_OK, NULL};

    /* MIDlet-name from Manifest */
    pcsl_string * mf_name          = NULL;
    /* MIDlet-vendor from Manifest */
    pcsl_string * mf_vendor        = NULL;
    /* suiteId created from MIDlet-name and MIDlet-vendor */
    SuiteIdType suiteId            = UNUSED_SUITE_ID;
    /* The size of the jar as written in the jad */
    pcsl_string * jarSizeString    = NULL;

    /* path to the jad file as given by argv[1] */
    pcsl_string jadURL             = PCSL_STRING_NULL;
    /* path to the jar file as given by argv[1] or came from the jad */
    pcsl_string jarURL             = PCSL_STRING_NULL;
    pcsl_string * p_jarURL         = &jarURL;
    /* jad relative url file:///jadname.jad */
    pcsl_string jadRelativeURL     = PCSL_STRING_NULL;
    /* jar relative url file:///jadname.jar */
    pcsl_string jarRelativeURL     = PCSL_STRING_NULL;

    jboolean trusted = KNI_TRUE;

    /*
     * From Permissions.java, 50 permissions, level 1 is allowed. Don't
     * allow the first 2 permissions they are for internal system functions.
     * Use the "trusted" domain with no CA.
     */
    unsigned char permissions[] = {
        0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    int permLen = sizeof (permissions) / sizeof (char);
    MIDPError err;

#if VERIFY_ONCE
    unsigned char *verifyHash = NULL;
    int verifyHashLen = 0;
#endif

    char* appDir = NULL;
    PCSL_DEFINE_ASCII_STRING_LITERAL_START(domain)
        {'o', 'p', 'e', 'r', 'a', 't', 'o', 'r', '\0'}
    PCSL_DEFINE_ASCII_STRING_LITERAL_END(domain);

    if (argc != 2) {
        REPORT_ERROR1(LC_AMS,
                      "Too many or not enough arguments given. argc = %d",
                      argc);
        return BAD_PARAMS;
    }

    REPORT_INFO2(LC_AMS, "argv[0] = %s, argv[1] = %s", argv[0], argv[1]);

    /* get midp home directory, set it */
    appDir = getApplicationDir(argv[0]);
    if (appDir == NULL) {
        return -1;
    }
    /* set up appDir before calling initialize */
    midpSetAppDir(appDir);

    if (midpInitialize() != 0) {
        REPORT_ERROR(LC_AMS, "Not enough memory");
        return OUT_OF_MEMORY;
    }

    length = strlen(argv[1]);
    if (IS_JAR_FILE(argv[1], length)) {
        usingWhat = USING_JAR;
        pcsl_string_from_chars(argv[1], p_jarURL);
        createRelativeURL(p_jarURL, &jarRelativeURL);
        /* IMPL NOTE: can we do anything meaningful in case of out-of-memory? */
    } else if (IS_JAD_FILE(argv[1], length)) {
        usingWhat = USING_JAD;
        pcsl_string_from_chars(argv[1], &jadURL);
        createRelativeURL(&jadURL, &jadRelativeURL);
        /* IMPL NOTE: can we do anything meaningful in case of out-of-memory? */
    } else {
        REPORT_ERROR1(LC_AMS, "Bad input. %s\n",argv[1]);
        midpFinalize();
        return BAD_PARAMS;
    }

    if (usingWhat == USING_JAD) {

        /*
         * Get the jad file into the jad_buf.
         * Memory will be allocated in readJadFile().
         */
        jadsize = (int)readJadFile(&jadURL, &jad_buf);
        if ((jadsize <= 0) || (!jad_buf)) {
            REPORT_ERROR1(LC_AMS, "Can't open JAD file %s", argv[1]);
            pcsl_string_free(&jadURL);
            pcsl_string_free(&jadRelativeURL);
            midpFinalize();
            return NO_JAD_FILE;
        }

        /*
         * jad file will be parsed and verified for existance of mandatory
         * fields in jad_main(), jad_buf will be freed in jad_main() once it is
         * converted to jchar.
         */
        jadsmp = jad_main(jad_buf, jadsize);
        switch (jadsmp.status) {
        case BAD_PARAMS:
        case BAD_JAD_KEY:
        case BAD_JAD_VALUE:
            REPORT_INFO1(LC_AMS, "Some NOT mandatory Jad properties are not "
                         "valid. Continuing. %d", jadsmp.status);
            break;
        case ALL_OK:
            REPORT_INFO1(LC_AMS, "Jad ALL_OK %d", jadsmp.status);
            break;
        default:
            REPORT_INFO1(LC_AMS, "Can't continue status is %d.\n",
                         jadsmp.status);
            pcsl_string_free(&jadURL);
            pcsl_string_free(&jadRelativeURL);
            midpFinalize();
            return GENERAL_ERROR;
        } /* end of switch */

        /* We have started from JAD. Go get the JAR file URL */
        p_jarURL = midp_find_property(&jadsmp, &JAR_URL_PROP);
        if (pcsl_string_is_null(p_jarURL)) {
            REPORT_ERROR(LC_AMS, "Can't get JAR_URL_PROP from JAD. "
                         "This should not happen at this stage!");
            pcsl_string_free(&jadURL);
            pcsl_string_free(&jadRelativeURL);
            midp_free_properties(&jadsmp);
            midpFinalize();
            return GENERAL_ERROR;
        }

        createRelativeURL(p_jarURL, &jarRelativeURL);
        if (pcsl_string_is_null(&jarRelativeURL)) {
            REPORT_ERROR(LC_AMS,
                         "Can't create jarRelativeURL.Something wrong!");
        }

    }/* end of if using JAD */

    /* get the jar file */
    /* IMPL NOTE: jarURL is either absolute or relative to jad URL, but
       the open function needs a path relative to the current directory -- mg */
    jarHandle = midpOpenJar(&pszError, p_jarURL);
    if (pszError) {
        REPORT_ERROR(LC_AMS, "Can't open the jar file");
        if (usingWhat == USING_JAD) {
            pcsl_string_free(&jadURL);
            pcsl_string_free(&jadRelativeURL);
            midp_free_properties(&jadsmp);
        }
        if (usingWhat == USING_JAR) {
            pcsl_string_free(p_jarURL);
        }
        pcsl_string_free(&jarRelativeURL);
        midpFinalize();
        return NO_JAR_FILE;
    }
    /* get the jar file real size */
    realJarSize = midpGetJarSize(jarHandle);
    if (realJarSize <= 0) {
        REPORT_ERROR1(LC_AMS, "Jar size error %d", realJarSize);
        if (usingWhat == USING_JAD) {
            pcsl_string_free(&jadRelativeURL);
            pcsl_string_free(&jadURL);
            midp_free_properties(&jadsmp);
        }
        if (usingWhat == USING_JAR) {
            pcsl_string_free(p_jarURL);
        }
        pcsl_string_free(&jarRelativeURL);
        midpCloseJar(jarHandle);
        midpFinalize();
        return NUMBER_ERROR;
    }

    permittedJarSize = midpGetMaxJarSizePermitted();
    if (realJarSize > permittedJarSize) {
        REPORT_ERROR2(LC_AMS, "Jar size to big %d > %d",
                      realJarSize, permittedJarSize);
        if (usingWhat == USING_JAD) {
            pcsl_string_free(&jadRelativeURL);
            pcsl_string_free(&jadURL);
            midp_free_properties(&jadsmp);
        }
        if(usingWhat == USING_JAR) {
            pcsl_string_free(p_jarURL);
        }
        midpCloseJar(jarHandle);
        pcsl_string_free(&jarRelativeURL);
        midpFinalize();
        return OUT_OF_STORAGE;
    }

    if (usingWhat == USING_JAD) {
        jarSizeString = midp_find_property(&jadsmp, &JAR_SIZE_PROP);

        if (pcsl_string_is_null(jarSizeString)) {
            REPORT_ERROR(LC_AMS, "Can't get JAR_SIZE from JAD. "
                         "This should not happen at this stage!");
            midp_free_properties(&jadsmp);
            pcsl_string_free(&jadRelativeURL);
            pcsl_string_free(&jarRelativeURL);
            pcsl_string_free(&jadURL);
            midpCloseJar(jarHandle);
            midpFinalize();
            return -1;
        }

        if (PCSL_STRING_OK !=
            pcsl_string_convert_to_jint(jarSizeString, &jarSizeByJad)) {
            /* NUMBER_ERROR */
            REPORT_ERROR1(LC_AMS, "JAD size ERROR %d", jarSizeByJad);
            midp_free_properties(&jadsmp);
            pcsl_string_free(&jadRelativeURL);
            pcsl_string_free(&jarRelativeURL);
            pcsl_string_free(&jadURL);
            midpCloseJar(jarHandle);
            midpFinalize();
            return -1;
        }

        /* check that real jar size equal to the written in jad */
        if (realJarSize != jarSizeByJad) {
            REPORT_ERROR(LC_AMS,
                         "Real and defined in Jad Jar file sizes are mismatch");
            midp_free_properties(&jadsmp);
            pcsl_string_free(&jadRelativeURL);
            pcsl_string_free(&jarRelativeURL);
            pcsl_string_free(&jadURL);
            midpCloseJar(jarHandle);
            midpFinalize();
            return -1;
        }
    } /* end of if (usingWhat = JAD) */

    /* memory allocated in midpGetJarEntry and set to mf_buf
       mf_buf memory will be freed in manifestParser when it become unneeded */
    length = midpGetJarEntry(jarHandle, &ENTRY_NAME,
                             (unsigned char**)(void*)&mf_buf);
    if (length <= 0) {
        REPORT_ERROR1(LC_AMS,
                      "Jar entry error or entry does not exists. length = %ld",
                      length);
        if (usingWhat == USING_JAD) {
            pcsl_string_free(&jadRelativeURL);
            pcsl_string_free(&jadURL);
            midp_free_properties(&jadsmp);
        }
        if(usingWhat == USING_JAR) {
            pcsl_string_free(p_jarURL);
        }
        pcsl_string_free(&jarRelativeURL);
        midpCloseJar(jarHandle);
        midpFinalize();
        return NO_MF_FILE;
    }

    /*
     * Manifest file will be parsed and verified for existence
     * of mandatory fields in mf_main().
     */
    mfsmp = mf_main(mf_buf, length);
    switch (mfsmp.status) {
    case BAD_PARAMS:
    case BAD_MF_KEY:
    case BAD_MF_VALUE:
        REPORT_ERROR1(LC_AMS,
                      "Some NOT mandatory Manifest properties is not valid %d",
                      mfsmp.status);
        break;
    case ALL_OK:
        REPORT_INFO1(LC_AMS, "Manifest ALL_OK %d", mfsmp.status);
        break;
    default:
        REPORT_ERROR1(LC_AMS, "Can't continue status is %d.", mfsmp.status);
        if (usingWhat == USING_JAD) {
            midp_free_properties(&jadsmp);
            pcsl_string_free(&jadRelativeURL);
            pcsl_string_free(&jadURL);
        }
        if(usingWhat == USING_JAR) {
            pcsl_string_free(p_jarURL);
        }
        pcsl_string_free(&jarRelativeURL);
        midpCloseJar(jarHandle);
        midpFinalize();
        return GENERAL_ERROR;
    } /* end of switch */

    if (usingWhat == USING_JAD) {
        res = compareJADandManifestProperties(&jadsmp, &mfsmp);
        if (res != ALL_OK) {
            REPORT_ERROR(LC_AMS, "JAD and Manifest don't match.");
            midp_free_properties(&jadsmp);
            midp_free_properties(&mfsmp);
            pcsl_string_free(&jadURL);
            pcsl_string_free(&jadRelativeURL);
            pcsl_string_free(&jarRelativeURL);
            midpCloseJar(jarHandle);
            midpFinalize();
            return JAD_AND_MANIFEST_DOESNT_MATCH;
        }
    } /* end of using JAD */

    /*
     * verify that MIDlet-n values are equal in
     * JAD and in Manifest and exist in jar
     */
    /* if jadsmp is empty only manifest will be verified */
    res = verifyMidletN(&jadsmp, &mfsmp, jarHandle);

#ifdef ENABLE_JSR_211
    if (res >= 0) {
        if (jsr211_verify_handlers(jadsmp, mfsmp, jarHandle, trusted) < 0) {
            res = -1;
        }
    }
#endif  /* ENABLE_JSR_211 */

    midpCloseJar(jarHandle);
    if (res <= 0) {
        if(res == OUT_OF_MEMORY) {
            REPORT_ERROR(LC_AMS, "OUT OF MEMORY");
        } else {
            REPORT_ERROR(LC_AMS, "No MIDlets in the suite.");
        }
        if (usingWhat == USING_JAD) {
            midp_free_properties(&jadsmp);
            pcsl_string_free(&jadRelativeURL);
            pcsl_string_free(&jadURL);
        }
        if(usingWhat == USING_JAR) {
            pcsl_string_free(p_jarURL);
        }
        pcsl_string_free(&jarRelativeURL);
        midp_free_properties(&mfsmp);
        midpFinalize();
        return -1;
    }

    mf_name = midp_find_property(&mfsmp, &SUITE_NAME_PROP);
    if (pcsl_string_is_null(mf_name)) {
        REPORT_ERROR(LC_AMS,  "Can't get SUITE_NAME_PROP from Manifest. "
                     "This should not happen at this stage!");
        if (usingWhat == USING_JAD) {
            midp_free_properties(&jadsmp);
            pcsl_string_free(&jadRelativeURL);
            pcsl_string_free(&jadURL);
        }
        if(usingWhat == USING_JAR) {
            pcsl_string_free(p_jarURL);
        }
        pcsl_string_free(&jarRelativeURL);
        midp_free_properties(&mfsmp);
        midpFinalize();
        return -1;
    }

    mf_vendor = midp_find_property(&mfsmp, &SUITE_VENDOR_PROP);
    if (pcsl_string_is_null(mf_vendor)) {
        REPORT_ERROR(LC_AMS, "Can't get SUITE_VENDOR_PROP from Manifest. "
                     "This should not happen at this stage!");
        if (usingWhat == USING_JAD) {
            midp_free_properties(&jadsmp);
            pcsl_string_free(&jadRelativeURL);
            pcsl_string_free(&jadURL);
        }
        if(usingWhat == USING_JAR) {
            pcsl_string_free(p_jarURL);
        }
        pcsl_string_free(&jarRelativeURL);
        midp_free_properties(&mfsmp);
        midpFinalize();
        return -1;
    }

    /* verify that jar wasn't previously installed */
    err = midp_get_suite_id(mf_vendor, mf_name, &suiteId);
    /* err should be ALL_OK if the suite exists of NOT_FOUND if it doesn't */
    if (err == ALL_OK) {
        REPORT_ERROR(LC_AMS,
                     "Suite already exists. Remove the existing one.");
        if (usingWhat == USING_JAD) {
            pcsl_string_free(&jadURL);
            midp_free_properties(&jadsmp);
            pcsl_string_free(&jadRelativeURL);
        }

        if (usingWhat == USING_JAR) {
            pcsl_string_free(p_jarURL);
        }

        midp_free_properties(&mfsmp);
        pcsl_string_free(&jarRelativeURL);
        midpFinalize();
        return -1;
    }

    if (OUT_OF_MEM_LEN == midp_create_suite_id(&suiteId)) {
        REPORT_ERROR(LC_AMS, "Out Of Memory");
        if (usingWhat == USING_JAD) {
            midp_free_properties(&jadsmp);
            pcsl_string_free(&jadRelativeURL);
            pcsl_string_free(&jadURL);
        }
        if(usingWhat == USING_JAR) {
            pcsl_string_free(p_jarURL);
        }
        pcsl_string_free(&jarRelativeURL);
        midp_free_properties(&mfsmp);
        midpFinalize();
        return -1;
    }

    /* printPcslStringWithMessage("Ready to install: ", &suiteId); */

    /*
     * midp_store_suite assumes the JAR was copied over HTTP and so moves it
     * to avoid making another copy and deleting the downloaded one.
     * Since this JAR was not copied, copy it now.
     */
    if (copyFile(p_jarURL, &TEMP_JAR_NAME) != 0) {
        if (usingWhat == USING_JAD) {
            pcsl_string_free(&jadURL);
            midp_free_properties(&jadsmp);
            pcsl_string_free(&jadRelativeURL);
        }
        if(usingWhat == USING_JAR) {
            pcsl_string_free(p_jarURL);
        }
        midp_free_properties(&mfsmp);
        pcsl_string_free(&jarRelativeURL);
        midpFinalize();
        return -1;
    }

    /* preverify all classes within the JAR to do no more
     * classes verification duiring MIDlet launching and work */
#if VERIFY_ONCE
    do {
        int hashError;
        jsize tempJarNameLen = PCSL_STRING_LITERAL_LENGTH(TEMP_JAR_NAME) + 1;
        JvmPathChar *tempJarName = midpMalloc(tempJarNameLen
                                              * sizeof(JvmPathChar));
        res = ALL_OK;
        if (tempJarName == NULL) {
            res = OUT_OF_MEMORY;
            REPORT_ERROR(LC_AMS, "Out Of Memory");
            break;
        }

        ASSERT(sizeof(JvmPathChar) == sizeof(jchar));
        /*
         * Assuming that JvmPathChar is 16-bit (jchar): this is always true if
         * CLDC is based on PCSL.
         * In other case (which shouldn't happen) build error "Incompatible
         * pointer types" will occur. Then the following code should be used
         * instead of simple conversion to UTF-16:
         *
         *   if (sizeof(JvmPathChar) == 1) {
         *       rc = pcsl_string_convert_to_utf8(&TEMP_JAR_NAME, tempJarName,
         *                                        tempJarNameLen, NULL));
         *   } else {
         *       rc = pcsl_string_convert_to_utf16(&TEMP_JAR_NAME, tempJarName,
         *                                         tempJarNameLen, NULL));
         *   }
         */
        if (PCSL_STRING_OK !=
            pcsl_string_convert_to_utf16(&TEMP_JAR_NAME, tempJarName,
                                         tempJarNameLen, NULL)) {
            res = OUT_OF_MEMORY;
            REPORT_ERROR(LC_AMS, "Out Of Memory");
            break;
        }

        /* it's recommended for the MVM case to use Java API for classes
         * verification, but here we have native installer code and can't
         * guarantee there is working multitasking VM that can be asked for
         * verification of the suite being installed, so start new VM to
         * verify all suite classes */
        if (JVM_Verify(tempJarName) != KNI_TRUE) {
            res = GENERAL_ERROR;
        }
        midpFree(tempJarName);
        if (res != ALL_OK) {
            REPORT_ERROR(LC_AMS,
                "Not all classes within JAR passed verification");
            break;
        }

        /* all suite classes successfully passed verification,
         * evaluate hash value for JAR package and continue installation */
        hashError = midp_get_file_hash(&TEMP_JAR_NAME,
            &verifyHash, &verifyHashLen);
        if (hashError != MIDP_HASH_OK) {
            res = GENERAL_ERROR;
            REPORT_ERROR(LC_AMS,
                "Cannot evaluate hash value for JAR file");
            break;
        }
    } while(0);
    if (res != ALL_OK) {
        if (usingWhat == USING_JAD) {
            pcsl_string_free(&jadURL);
            midp_free_properties(&jadsmp);
            pcsl_string_free(&jadRelativeURL);
        }
        if(usingWhat == USING_JAR) {
            pcsl_string_free(p_jarURL);
        }
        midp_free_properties(&mfsmp);
        pcsl_string_free(&jarRelativeURL);
        midpFinalize();
        return -1;
    }
#endif /* VERIFY_ONCE */

    do {
        MidpInstallInfo installInfo;
        MidpSuiteSettings suiteSettings;
        MidletSuiteData suiteData;
        pcsl_string * AuthPath_tmp = NULL;

#ifdef ENABLE_JSR_211
        res = jsr211_store_handlers(suiteId);
        if (res != 0) {
            REPORT_ERROR(LC_AMS, "Content Handlers installation error\n");
            break;
        }
#endif  /* ENABLE_JSR_211 */

        /* save the midlet suite and properties in the storage */
        suiteSettings.suiteId = suiteData.suiteId = suiteId;
        suiteSettings.pPermissions = permissions;
        suiteSettings.permissionsLen = permLen;
        suiteSettings.pushInterruptSetting = 8; /* SESSION */
        suiteSettings.pushOptions = 0;
        suiteSettings.enabled = 1;

        installInfo.status   = ALL_OK;
        installInfo.jadUrl_s = jadURL;
        installInfo.jarUrl_s = jarRelativeURL;
        installInfo.domain_s = domain;
        installInfo.trusted  = trusted;

        /* IMPL_NOTE: currently are not supported by the File Installer */
        installInfo.authPath_as   = AuthPath_tmp;
        installInfo.authPathLen   = 0;
        /* --- */

        installInfo.pVerifyHash   = NULL;
        installInfo.verifyHashLen = 0;
        installInfo.jadProps      = jadsmp;
        installInfo.jarProps      = mfsmp;

        /* IMPL_NOTE: currently are not supported by the File Installer */
        suiteData.varSuiteData.pJarHash = NULL;
        suiteData.varSuiteData.midletClassName = PCSL_STRING_NULL;
        suiteData.varSuiteData.displayName = PCSL_STRING_NULL;
        suiteData.varSuiteData.suiteVersion = PCSL_STRING_NULL;
        suiteData.varSuiteData.iconName = PCSL_STRING_NULL;
        suiteData.varSuiteData.pathToJar = TEMP_JAR_NAME;
        suiteData.varSuiteData.pathToSettings = PCSL_STRING_NULL;
        suiteData.storageId = INTERNAL_STORAGE_ID;
        suiteData.numberOfMidlets = 0;
        suiteData.installTime = 0;
        suiteData.jadSize = 0;
        suiteData.jarSize = 0;
        /* --- */

        suiteData.varSuiteData.suiteVendor = *mf_vendor;
        suiteData.varSuiteData.suiteName = *mf_name;
        suiteData.isEnabled = 1;
        suiteData.isTrusted = trusted;
        suiteData.jarHashLen = 0;
        suiteData.type = COMPONENT_PREINSTALLED_SUITE;

        res = midp_store_suite(&installInfo, &suiteSettings, &suiteData);

        if (res != ALL_OK) {
            /* something wrong */
            REPORT_ERROR(LC_AMS, "Something wrong\n");
            break;
        }

#if VERIFY_ONCE

        if (verifyHash) {
            res = midp_suite_write_secure_resource(
                suiteId, &VERIFY_HASH_RESOURCENAME,
                verifyHash, verifyHashLen);
            midpFree(verifyHash);
            if (res != ALL_OK) {
                REPORT_ERROR(LC_AMS,
                  "Cannot store hash value for verified suite");
                break;

            }
        }

#endif

    } while (0);

    midp_free_properties(&mfsmp);
    printPcslStringWithMessage("jarRelativeURL", &jarRelativeURL);
    pcsl_string_free(&jarRelativeURL);

    if (usingWhat == USING_JAD) {
        printPcslStringWithMessage("jadRelativeURL", &jadRelativeURL);
        printPcslStringWithMessage("jadURL", &jadURL);
        pcsl_string_free(&jadRelativeURL);
        midp_free_properties(&jadsmp);
        pcsl_string_free(&jadURL);
    }
    if (usingWhat == USING_JAR) {
        printPcslStringWithMessage("jarURL",p_jarURL);
        pcsl_string_free(p_jarURL);
    }

    midpFinalize();
    return ALL_OK;

}/* end of fileInstaller main */




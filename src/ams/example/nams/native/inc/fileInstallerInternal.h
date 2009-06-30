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

#ifndef _FILE_INSTALLERINTERNAL_H_
#define _FILE_INSTALLERINTERNAL_H_

#include <suitestore_common.h>  /* To get definition of MidpProperties */
#include <midpAMS.h>

#if 1
/* IMPL NOTE; Remove these from here; they belong in the 'C' file(s) */
#include <midpJar.h>
#include <midpMalloc.h>
#include <midpStorage.h>
#include <midpDebug.h>
#endif /* 0 */

#ifdef __cplusplus
extern "C" {
#endif

enum UsingWhatToInstall {
    INIT_STAT = 0,
    USING_JAD = 1,
    USING_JAR = 2
};

/** End Of File - Unicode character 0x1A. */
#define  END_OF_FILE  0x1A

/** Carriage Return - Unicode character 0x0D. */
#define  CR   0x0D

/** Line Feed - Unicode character 0x0A. */
#define  LF   0x0A

/** the # sign */
#define RESHET 0x23

/**
 * All those macros should get a pointer as a parameter
 */

/**
 * Verify that pointer is not NULL
 */
#define CONTENT_NOT_NULL(c) (*(c))

/**
 * newline could be CR LF | LF | CR
 * Every NEW_LINE_n macro checks one of the options
 * NEW_LINE macro combines all three together
 */
#define NEW_LINE_1(c) (CONTENT_NOT_NULL((c)+1) && (*(c) == CR) && (*(c+1) == LF))
#define NEW_LINE_2(c) (CONTENT_NOT_NULL(c) && (*(c) == CR))
#define NEW_LINE_3(c) (CONTENT_NOT_NULL(c) && (*(c) == LF))

/**
 * NEW_LINE macro combines all three together
 */
#define NEW_LINE(c)   (NEW_LINE_1(c) || NEW_LINE_2(c) || NEW_LINE_3(c))

/**
 * Looking for space
 */
#define MF_SPACE(c)   (CONTENT_NOT_NULL(c) && (*(c) == SP))

/**
 * Manifest line could be broken by new line and space
 * We are looking here for different new line followed by space combinations
 */
#define MF_BR_LINE_1(c)   (NEW_LINE_1(c) && MF_SPACE(c+2))
#define MF_BR_LINE_2(c)   ((NEW_LINE_2(c) || NEW_LINE_3(c)) && MF_SPACE(c+1))

/**
 * MF_BROKEN_LINE macro combines all MF_BR_LINE_n together
 */
#define MF_BROKEN_LINE(c) (MF_BR_LINE_1(c) || MF_BR_LINE_2(c))

/**
 * Looks for # occurrence
 */
#define COMMENTED_OUT(c) (*(c) == RESHET)

#define IS_JAR_1(x,y) ((*((x)+((y)-4))=='.')&&(*((x)+((y)-3))=='j')&&(*((x)+((y)-2))=='a')&&(*((x)+((y)-1))=='r'))
#define IS_JAR_2(x,y) ((*((x)+((y)-4))=='.')&&(*((x)+((y)-3))=='J')&&(*((x)+((y)-2))=='A')&&(*((x)+((y)-1))=='R'))
#define IS_JAD_1(x,y) ((*((x)+((y)-4))=='.')&&(*((x)+((y)-3))=='j')&&(*((x)+((y)-2))=='a')&&(*((x)+((y)-1))=='d'))
#define IS_JAD_2(x,y) ((*((x)+((y)-4))=='.')&&(*((x)+((y)-3))=='J')&&(*((x)+((y)-2))=='A')&&(*((x)+((y)-1))=='D'))

/**
 * Checks that file extensione is jar or jad
 */
#define IS_JAR_FILE(x,y)  (IS_JAR_1((x),(y)) || IS_JAR_2((x),(y)))
#define IS_JAD_FILE(x,y)  (IS_JAD_1((x),(y)) || IS_JAD_2((x),(y)))

/**
 * definitions are in fileInstallerShare.c
 */
extern const pcsl_string JAR_URL_PROP;
extern const pcsl_string JAR_SIZE_PROP;
extern const pcsl_string MIDLET_ONE_PROP;
extern const pcsl_string MICROEDITION_PROFILE_PROP;
extern const pcsl_string MICROEDITION_CONFIGURATION_PROP;

/**
 * Concentrates all JAD parsing function calls framework.
 *
 * @param jadbuf Buffer with the jad file
 * @param jadsize Size of jad buffer in bytes
 * @return MidpProperties with parsed jad and status
 */
MidpProperties jad_main(char* jadbuf,  int jadsize);

/**
 * Concentrates all Manifest parsing function calls framework.
 *
 * @param mfbuf Buffer with the manifest file
 * @param mglength Size of manifest buffer in bytes
 * @return MidpProperties with parsed manifest and status
 */
MidpProperties mf_main(char* mfbuf, int mflength);

/**
 * Compares JAD and Manifest file properties that MUST be the same.
 * The properties are: MIDlet-Name, MIDlet-Vendor,  MIDlet-Version
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
int compareJADandManifestProperties(MidpProperties* jadsmp, MidpProperties* mfsmp);

/**
 * Converts char buffer to jchar buffer.
 * Buffer for jchar should be allocated by caller function.
 * This function doesn't set a 0x00 at the end of string.
 * Only char_buf_size characters will be converted
 *
 * @param char_buf Pointer to existing jchar buffer
 * @param jchar_buf  pointer to allocated char buffer
 * @param char_buf_size
 *                  size of the char buffer
 */
void convertChar2JChar(char* char_buf, jchar* jchar_buf, int char_buf_size);

/**
 * Converts jchar buffer to char buffer.
 * Buffer for char should be allocated by caller function.
 * This function doesn't set a 0x0 at the end of string.
 * Only jchar_buf_size characters will be converted
 *
 * @param jchar_buf Pointer to existing jchar buffer
 * @param char_buf  pointer to allocated char buffer
 * @param jchar_buf_size
 *                  size of the jchar buffer
 */
void convertJChar2Char(jchar* jchar_buf, char* char_buf, int jchar_buf_size);

/**
 * Opens a file and fills the content of the file in the result_buf. <BR>
 * This function made memory allocation inside.
 *
 * @param filename   Path to the file.
 * @param result_buf Pointer to the buffer that will be filled by content of the file.
 * @return buffer file size in bytes
 */
long readJadFile(const pcsl_string * filename, char** result_buf);


/**
 * Verifies that MIDlet version is valid
 * @param ver MIDlet version
 * @return 1 == OK
 *         0 = BAD version
 */
int midpCheckVersion(const pcsl_string * ver);

/**
 * Version string may look like Major:Minor:Micro
 *
 * @param ver    pcsl_string that contains a version
 * @param major  Major version
 * @param minor  Minor version
 * @param micro  Micro version
 * @return 1 == OK
 *         0 = BAD version
 */
int midpGetVersion(const pcsl_string * ver, int *major, int *minor, int *micro);

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
int midpCompareVersion(const pcsl_string * ver1, const pcsl_string * ver2);

/**
 * IMPL NOTE: It is a stub
 *
 * @return
 * maximum JAR size permitted to download or install
 * current number is 60KB.
 */
int midpGetMaxJarSizePermitted();

/**
 * Compares MIDlet-n name and class values in JAD and Manifest.
 * Name must not be NULL. Class must not be NULL. Name and class must be same in jad and jar.
 * Class must be exist in the jar.
 *
 * @param jadsmp JAD properties
 * @param mfsmp Maniffest properties
 * @param jarHandle Handle to the JAR file.
 *
 * @return Number of MIDlets written in JAD and Manifest and exist in the JAR file
 */
int verifyMidletN(MidpProperties* jadsmp, MidpProperties* mfsmp, void* jarHandle);

/**
 * Creates "MIDlet-n" pcsl_string instance.
 * n will be equal to midletNumber
 *
 * @param midletNumber MIDlet number
 * @param res receives the composed "MIDlet-n" string.
 *
 * @return PCSL_STRING_OK on success, error code otherwise
 */
pcsl_string_status prepareMidletTag(int midletNumber, pcsl_string* res);

/**
 * parses a comma delimited string and returns the first, second or third
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
 * @param p_out [out] pcsl_string that receives the required value
 *
 */
void midpParseMIDletN(const pcsl_string* in, int index, pcsl_string* p_out);

/**
 *
 * Creates a relative URL of the file.
 * In file installer will remove ./ from the beginning
 * and add file:/// prefix to the file path
 * @param in File path
 *
 * @return Relative URL of file
 */
pcsl_string_status createRelativeURL(const pcsl_string * in,
                                     pcsl_string * result);

#ifdef __cplusplus
}
#endif

#endif /* _FILE_INSTALLERINTERNAL_H_ */

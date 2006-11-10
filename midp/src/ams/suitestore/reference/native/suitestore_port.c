/*
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

#include <kni.h>
#include <midpMalloc.h>
#include <string.h>
#include <midpStorage.h>
#include <midpError.h>
#include <suitestore_intern.h>
#include <suitestore_util.h>
#include <suitestore_port.h>

PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(TMP_EXT)
    {'.', 't', 'm', 'p', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(TMP_EXT);

PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(DB_EXTENSION)
    {'.', 'd', 'b', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(DB_EXTENSION);

PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(IDX_EXTENSION)
    {'.', 'i', 'd', 'x', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(IDX_EXTENSION);

/**
 * Checks if the MIDlet suite is trusted or not.
 * 
 * @param suiteId The suite id used to identify the MIDlet suite
 * @param suiteIdLen  The length of the suiteId 
 * @return true if the application is trusted and false otherwise
 */
jboolean midpport_suite_is_trusted(jchar *suiteId, jint suiteIdLen) {
    (void)suiteId; (void)suiteIdLen;
    return KNI_TRUE;
}

/**
 * Gets the unique identifier of MIDlet suite.
 *
 * @param vendor name of the vendor that created the application, as
 *          given in a JAD file
 * @param vendorLen length of the vendor string
 * @param suiteName name of the suite, as given in a JAD file
 * @param suiteNameLen length of the suiteName string
 * @param suiteId in/out parameter to retrieve the suite id
 * @param suiteIdLen in/out parameter to retrieve the length of suiteId
 * @return one of the error codes:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_MEM,
 *       MIDP_ERROR_SUITE_NOT_FOUND, MIDP_ERROR_SUITE_CORRUPTED
 * </pre>
 */
MIDP_ERROR midpport_suite_get_id(jchar *vendor, jint vendorLen,
				 jchar *suiteName, jint suiteNameLen,
				 jchar **suiteId, jint *suiteIdLen) {
    int stat;
    pcsl_string param1;
    pcsl_string param2;
    pcsl_string result_str;
    pcsl_string_status pstat;

    if (PCSL_STRING_OK !=
        pcsl_string_convert_from_utf16(vendor, vendorLen, &param1)) {
        return MIDP_ERROR_OUT_MEM;
    }

    if (PCSL_STRING_OK !=
        pcsl_string_convert_from_utf16(suiteName, suiteNameLen, &param2)) {
        pcsl_string_free(&param1);
        return MIDP_ERROR_OUT_MEM;
    }

    stat = midp_get_suite_id(&param1, &param2, &result_str);
    pcsl_string_free(&param1);
    pcsl_string_free(&param2);
    switch(stat) {
    case OUT_OF_MEM_LEN:
      return MIDP_ERROR_OUT_MEM;
    case NULL_LEN:
      return MIDP_ERROR_AMS_SUITE_NOT_FOUND;
    case IO_ERROR:
      return MIDP_ERROR_AMS_SUITE_CORRUPTED;
    }
    pstat = text_buffer_from_pcsl_string(&result_str,suiteId, suiteIdLen);
    pcsl_string_free(&result_str);
    if (PCSL_STRING_OK != pstat) {
        return MIDP_ERROR_OUT_MEM;
    } else {
        return MIDP_ERROR_NONE;
    }
}

/**
 * Gets the handle for accessing MIDlet suite properties.
 * Also returns the number of name/value pairs read using that handle.
 *
 * @param suiteId   The suite id of the MIDlet suite 
 * @param suiteId   The length of the suiteId 
 * @param numProperties [out] The number of properties
 * @param propHadle [out] The property handle for accessing
 *                  MIDlet suite properties
  * @return one of the error codes:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_MEM,
 *       MIDP_ERROR_SUITE_NOT_FOUND, MIDP_ERROR_SUITE_CORRUPTED
 * </pre>
 */
MIDP_ERROR midpport_suite_open_properties(jchar *suiteIdAddr, jint suiteIdLen,
					  jint *numProperties,
					  jint *propHandle) {
    pcsl_string filename = PCSL_STRING_NULL;
    pcsl_string suiteID = PCSL_STRING_NULL;
    jint numStrings = 0;
    MIDP_ERROR errorCode = MIDP_ERROR_NONE;

    char* pszError = NULL;   
    *propHandle = -1;
    *numProperties = 0;
    do {
        if(PCSL_STRING_OK !=
            pcsl_string_convert_from_utf16(suiteIdAddr, suiteIdLen, &suiteID)) {
            errorCode = MIDP_ERROR_OUT_MEM;
            break;
        }

        errorCode = midp_example_get_property_file(&suiteID, KNI_TRUE, &filename);
        if (errorCode != MIDP_ERROR_NONE) {
            break;
        }

        *propHandle = storage_open(&pszError, &filename, OPEN_READ);
        if (pszError != NULL) {
          storageFreeError(pszError);
          errorCode = MIDP_ERROR_AMS_SUITE_CORRUPTED;
          break;
        }

        storageRead(&pszError, *propHandle, (char*)&numStrings, sizeof(jint));
        if (pszError != NULL) {
          storageFreeError(pszError);
          errorCode = MIDP_ERROR_AMS_SUITE_CORRUPTED;
          break;
        }

        *numProperties = numStrings/2;
    } while (0);
    pcsl_string_free(&filename);
    pcsl_string_free(&suiteID);
    return errorCode;
}

/**
 * Retrieves the next MIDlet suite property associated with the passed in
 * property handle. Note that the memory for in/out parameters 
 * key and property MUST be allocated  using midpMalloc(). 
 * The caller is responsible for freeing memory associated
 * with key and value. If NULL is returned for key and value then there are
 * no more properties to retrieve.
 *
 * @param propHandle    MIDlet suite property handle
 * @param key           An in/outparameter that will return 
 *                      the key part of the property 
 *                      (NULL is a valid return value)
 * @param keyLength     The length of the key string
 * @param value         An in/out parameter that will return
 *                      the value part of the property 
 *                      (NULL is a valid return value).
 * @param valueLength   The length of the value string
 * @return one of the error codes:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_MEM,
 *       MIDP_ERROR_SUITE_CORRUPTED, 
 * </pre>
 */
MIDP_ERROR midpport_suite_next_property(jint propHandle, 
					jchar **key, jint *keyLength,
					jchar **value, jint *valueLength) {

  char* pszError = NULL; 
  int bytesRead = 0;
  jchar *tempStr = NULL;
  jint tempLen = 0;

  *key = NULL;
  *value = NULL;
  *keyLength = 0;
  *valueLength = 0;

  /* load the key string */
  storageRead(&pszError, propHandle, (char*)&tempLen, sizeof (jint));
  if (pszError != NULL) {
    storageFreeError(pszError);
    return MIDP_ERROR_AMS_SUITE_CORRUPTED;
  }
  
  tempStr = (jchar*)midpMalloc(tempLen * sizeof (jchar));
  if (tempStr == NULL) {
      return MIDP_ERROR_OUT_MEM;
  }

  bytesRead = storageRead(&pszError, propHandle,
			  (char*)tempStr, tempLen * sizeof (jchar));
  if (pszError != NULL ||
      (bytesRead != (signed)(tempLen * sizeof (jchar)))) {
    midpFree(tempStr);
    storageFreeError(pszError);
    return MIDP_ERROR_AMS_SUITE_CORRUPTED;
  }
    
  *key = tempStr;
  *keyLength = tempLen;
  
  /* load the value string */
  storageRead(&pszError, propHandle, (char*)&tempLen, sizeof (jint));
  if (pszError != NULL) {
    storageFreeError(pszError);
    return MIDP_ERROR_AMS_SUITE_CORRUPTED;
  }
  
  tempStr = (jchar*)midpMalloc(tempLen * sizeof (jchar));
  if (tempStr == NULL) {
    return MIDP_ERROR_OUT_MEM;
  }

  bytesRead = storageRead(&pszError, propHandle,
			  (char*)tempStr, tempLen * sizeof (jchar));
  if (pszError != NULL ||
      (bytesRead != (signed)(tempLen * sizeof (jchar)))) {
    midpFree(tempStr);
    storageFreeError(pszError);
    return MIDP_ERROR_AMS_SUITE_CORRUPTED;
  }
    
  *value = tempStr;
  *valueLength = tempLen;

  return MIDP_ERROR_NONE;
}

/**
 * Closes the passed in MIDlet suite property handle.
 * It will be called with a valid propHandle returned
 * by midpport_suite_open_properties().
 * 
 * @param propHandle   The MIDlet suite property handle
 * @return one of the error codes:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_SUITE_CORRUPTED
 * </pre>
 */
MIDP_ERROR midpport_suite_close_properties(jint propHandle) {
    char* pszError = NULL; 
    storageClose(&pszError, propHandle);
    if (pszError != NULL) {
        storageFreeError(pszError);
	return MIDP_ERROR_AMS_SUITE_CORRUPTED;
    }
    return MIDP_ERROR_NONE;
}


/**
 * Determines if suite exists or not.
 * 
 * Note that memory for the in/out parameter path
 * MUST be allocated using midpMalloc.
 * The caller is responsible for freeing it.
 *
 * @param suiteId    The suite id used to identify the MIDlet suite
 * @param suiteIdLen    The length of the suiteId 
 * @return one of the error codes:
 * <pre>
 *       MIDP_ERROR_NONE, 
 *       MIDP_ERROR_SUITE_NOT_FOUND, MIDP_ERROR_SUITE_CORRUPTED, 
 * </pre>
 */
MIDP_ERROR midpport_suite_exists(jchar *suiteId, jint suiteIdLen) {
    pcsl_string suiteID_str;
    int rc;

    if(PCSL_STRING_OK
        != pcsl_string_convert_from_utf16(suiteId, suiteIdLen, &suiteID_str)) {
        return MIDP_ERROR_OUT_MEM;
    }
    rc = suite_exists(&suiteID_str);
    pcsl_string_free(&suiteID_str);

    switch(rc) {
    case 0:
        return MIDP_ERROR_AMS_SUITE_NOT_FOUND;
    case OUT_OF_MEM_LEN:
        return MIDP_ERROR_OUT_MEM;

        /* Ignore if suite is corrupted */
        /*  case SUITE_CORRUPTED_ERROR: */
 
    case IO_ERROR_LEN: 
        return MIDP_ERROR_AMS_SUITE_CORRUPTED;
    }
    
  return MIDP_ERROR_NONE;
}

/** Filename for the suites JAR. */
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(JAR_FILENAME)
    {'.', 'j', 'a', 'r', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(JAR_FILENAME);

MIDP_ERROR midpport_suite_general_class_path(jchar *suiteId, jint suiteIdLen,
                                             jchar **classPath, 
                                             jint *classPathLength,
                                             const pcsl_string * extension) {

    const pcsl_string * root = storage_get_root();
    pcsl_string path = PCSL_STRING_NULL;
 
    *classPath = NULL;
    *classPathLength = OUT_OF_MEM_LEN;

    /* performance hint: predict buffer capacity */
    pcsl_string_predict_size(&path, pcsl_string_length(root)
                                    + suiteIdLen
                                    + pcsl_string_length(extension));
    if (PCSL_STRING_OK != pcsl_string_append(&path, root)
     || PCSL_STRING_OK != pcsl_string_append_buf(&path, suiteId, suiteIdLen)
     || PCSL_STRING_OK != pcsl_string_append(&path, extension)) {
        pcsl_string_free(&path);
        return MIDP_ERROR_OUT_MEM;
    }

    if (PCSL_STRING_OK !=
        text_buffer_from_pcsl_string(&path, classPath, classPathLength)) {
        *classPath = NULL;
        *classPathLength = OUT_OF_MEM_LEN;
        pcsl_string_free(&path);
        return MIDP_ERROR_OUT_MEM;
    }
    pcsl_string_free(&path);

    return MIDP_ERROR_NONE;
}

/**
 * Gets the classpath for the specified MIDlet suite id.
 * 
 * Note that memory for the in/out parameter classPath
 * MUST be allocated using midpMalloc.
 * The caller is responsible for freeing it.
 *
 * @param suiteId        The suite id used to identify the MIDlet suite
 * @param suiteIdLen     The length of the suiteId 
 * @param classPath      The in/out parameter that contains returned class path
 * @param classPathLen   The in/out parameter that contains returned class path
 *                       length
 * @return one of the error codes:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_MEM
 * </pre>
 */
MIDP_ERROR midpport_suite_class_path(jchar *suiteId, jint suiteIdLen,
				     jchar **classPath, 
				     jint *classPathLength) {
    return midpport_suite_general_class_path(suiteId, suiteIdLen, 
      classPath, classPathLength, &JAR_FILENAME);
}

#if ENABLE_MONET
MIDP_ERROR midpport_suite_app_image_path(jchar *suiteId, jint suiteIdLen,
				     jchar **classPath, 
				     jint *classPathLength) {
    return midpport_suite_general_class_path(suiteId, suiteIdLen, 
      classPath, classPathLength, &APP_IMAGE_FILENAME);
}
#endif /* ENABLE_MONET */

/**
 * Gets the path for the specified rms for the MIDlet suite by ID.
 * 
 * Note that memory for the in/out parameter path
 * MUST be allocated using midpMalloc.
 * The caller is responsible for freeing it.
 *
 * @param suiteId     The suite id used to identify the MIDlet suite
 * @param suiteId     The length of the suiteId 
 * @param name        The name of the cached resource
 * @param nameLen     The length of the cached resource name
 * @param path        The in/out parameter for the path
 * @param pathLength  The in/out pathLength for the length of the path
 * @return one of the error codes:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_MEM
 * </pre>
 */
MIDP_ERROR midpport_suite_cached_resource_filename(jchar *suiteId,
                                                   jint suiteIdLen,
                                                   jchar *name, jint nameLen,
                                                   jchar **path, 
                                                   jint *pathLength) {

    const pcsl_string * root = storage_get_root();
    pcsl_string returnPath = PCSL_STRING_NULL;
    pcsl_string filename = PCSL_STRING_NULL;

    *path = NULL;
    *pathLength = OUT_OF_MEM_LEN;

    if (nameLen > 0) {
        pcsl_string temp = PCSL_STRING_NULL;
      
        if (PCSL_STRING_OK !=
            pcsl_string_convert_from_utf16(name, nameLen, &temp)) {
            return MIDP_ERROR_OUT_MEM;
        }

        {
            /* performance hint: predict buffer capacity */
            int filename_len = 
                ESCAPED_BUFFER_SIZE(pcsl_string_length(&temp)
                                    + PCSL_STRING_LITERAL_LENGTH(TMP_EXT));
            pcsl_string_predict_size(&filename, filename_len);
        }

        if ( /* Convert any slashes */
             PCSL_STRING_OK != pcsl_string_append_escaped_ascii(&filename, &temp)
             /* Add the extension */
          || PCSL_STRING_OK != pcsl_string_append(&filename, &TMP_EXT)) {
            pcsl_string_free(&temp);
            pcsl_string_free(&filename);
            return MIDP_ERROR_OUT_MEM;
        }
        pcsl_string_free(&temp);
    }

    /* performance hint: predict buffer capacity */
    pcsl_string_predict_size(&returnPath, pcsl_string_length(root)
                                             + suiteIdLen
                                             + pcsl_string_length(&filename));

    if (PCSL_STRING_OK != pcsl_string_append(&returnPath, root)
     || PCSL_STRING_OK != pcsl_string_append_buf(&returnPath,
                                                 suiteId, suiteIdLen)
     || PCSL_STRING_OK != pcsl_string_append(&returnPath, &filename)) {
        pcsl_string_free(&filename);
        pcsl_string_free(&returnPath);
        return MIDP_ERROR_OUT_MEM;
    }
    pcsl_string_free(&filename);

    if (PCSL_STRING_OK !=
        text_buffer_from_pcsl_string(&returnPath, path, pathLength)) {
        *path = NULL;
        *pathLength = OUT_OF_MEM_LEN;
        pcsl_string_free(&returnPath);
        return MIDP_ERROR_OUT_MEM;
    }
    pcsl_string_free(&returnPath);

    return MIDP_ERROR_NONE;
}

/**
 * Gets the cached resource path for the MIDlet suite by ID.
 * 
 * Note that memory for the in/out parameter path
 * MUST be allocated using midpMalloc.
 * The caller is responsible for freeing it.
 *
 * @param suiteId    The suite id used to identify the MIDlet suite
 * @param suiteId    The length of the suiteId 
 * @param extension  rms extension that can be MIDP_RMS_DG_EXT or
 *                            MIDP_RMS_IDX_EXT
 * @param name       The name of the cached resource
 * @param nameLen    The length of the cached resource name
 * @param path       The in/out parameter for the path
 * @param pathLength The in/out pathLength for the length of the path
 * @return one of the error codes:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_MEM
 * </pre>
 */
MIDP_ERROR midpport_suite_rms_filename(jchar *suiteId, jint suiteIdLen,
                                       jint extension,
                                       jchar *name, jint nameLen,
                                       jchar **path, jint *pathLength) {

    const pcsl_string * root = storage_get_root();
    pcsl_string returnPath = PCSL_STRING_NULL;
    pcsl_string filename = PCSL_STRING_NULL;

    *path = NULL;
    *pathLength = OUT_OF_MEM_LEN;

    if (nameLen > 0) {
        pcsl_string temp = PCSL_STRING_NULL;
        const pcsl_string * ext;
        jsize ext_len;

        if (MIDP_RMS_IDX_EXT == extension) {
            ext = &IDX_EXTENSION;
            ext_len = PCSL_STRING_LITERAL_LENGTH(IDX_EXTENSION);
        } else if (MIDP_RMS_DG_EXT == extension) {
            ext = &DB_EXTENSION;
            ext_len = PCSL_STRING_LITERAL_LENGTH(DB_EXTENSION);
        } else {
            return MIDP_ERROR_ILLEGAL_ARGUMENT;
        }

        if (PCSL_STRING_OK !=
            pcsl_string_convert_from_utf16(name, nameLen, &temp)) {
            return MIDP_ERROR_OUT_MEM;
        }

        {
            /* performance hint: predict buffer capacity */
            int filename_len =
                ESCAPED_BUFFER_SIZE(pcsl_string_length(&temp) + ext_len);
            pcsl_string_predict_size(&filename, filename_len);
        }

        if (PCSL_STRING_OK != pcsl_string_append_escaped_ascii(&filename, &temp)
         || PCSL_STRING_OK != pcsl_string_append(&filename, ext)) {
            pcsl_string_free(&temp);
            pcsl_string_free(&filename);
            return MIDP_ERROR_OUT_MEM;
        }
        pcsl_string_free(&temp);
    }

    /* performance hint: predict buffer capacity */
    pcsl_string_predict_size(&returnPath,
            pcsl_string_length(root) + suiteIdLen
            + pcsl_string_length(&filename));

    if (PCSL_STRING_OK != pcsl_string_append(&returnPath, root)
     || PCSL_STRING_OK != pcsl_string_append_buf(&returnPath,
                                                 suiteId, suiteIdLen)
     || PCSL_STRING_OK != pcsl_string_append(&returnPath, &filename)) {
        pcsl_string_free(&filename);
        pcsl_string_free(&returnPath);
        return MIDP_ERROR_OUT_MEM;
    }
    pcsl_string_free(&filename);

    if (PCSL_STRING_OK !=
        text_buffer_from_pcsl_string(&returnPath, path, pathLength)) {
        *path = NULL;
        *pathLength = OUT_OF_MEM_LEN;
        pcsl_string_free(&returnPath);
        return MIDP_ERROR_OUT_MEM;
    }
    pcsl_string_free(&returnPath);

    return MIDP_ERROR_NONE;
}

/**
 * Called to reset midlet suite storage state. 
 * This function will be called when the Java system is stopped. 
 * Implementation should clean up all its internal states and 
 * be ready to serve calls when Java system is restarted within 
 * the same OS process.
 */
void midpport_suite_storage_reset() {
    resetMidletSuiteStorage();
}

PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(SECURE_EXTENSION)
    {'.', 's', 's', 'r', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(SECURE_EXTENSION);

/**
 * Gets filename of the secure suite resource by suiteId and resource name
 *
 * Note that memory for the in/out parameter filename
 * MUST be allocated using midpMalloc.
 * The caller is responsible for freeing it.
 *
 * @param suiteId           The suite id used to identify the MIDlet suite
 * @param suiteIdLen        The length of the suiteId
 * @param resourceName      The name of secure resource to read from storage
 * @param resourceNameLen   The length of secure resource name
 * @param filename          The in/out parameter that will return filename
 *                          of the specified secure resource
 * @return one of the error codes:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_MEM,
 *       MIDP_ERROR_SUITE_NOT_FOUND, MIDP_ERROR_SUITE_CORRUPTED,
 *       MIDP_ERROR_ILLEGAL_ARG
 * </pre>
 */
static MIDP_ERROR getSecureResourceFile(jchar *suiteIdAddr, jint suiteIdLen,
    jchar *resourceName, jint resourceNameLen, pcsl_string *filename) {

    pcsl_string resourceID = PCSL_STRING_NULL;
    pcsl_string suiteID = PCSL_STRING_NULL;
    MIDP_ERROR errorCode;

    do {
        if (PCSL_STRING_OK !=
            pcsl_string_convert_from_utf16(suiteIdAddr, suiteIdLen, &suiteID)) {
            errorCode = MIDP_ERROR_OUT_MEM;
            break;
        }

        /* performance hint: predict buffer capacity */
        pcsl_string_predict_size(&resourceID,
                            resourceNameLen
                            + PCSL_STRING_LITERAL_LENGTH(SECURE_EXTENSION));
        if (PCSL_STRING_OK != pcsl_string_append_buf(&resourceID, resourceName,
                                                     resourceNameLen)
         || PCSL_STRING_OK != pcsl_string_append(&resourceID,
                                                 &SECURE_EXTENSION)) {
            errorCode = MIDP_ERROR_OUT_MEM;
            break;
        }

        errorCode = midp_example_get_suite_resource_file(&suiteID,
            &resourceID, KNI_FALSE, filename);

    } while (0);
    pcsl_string_free(&resourceID);
    pcsl_string_free(&suiteID);
    return errorCode;
}

/**
 * Reads named secure resource of the suite with specified suiteId
 * from secure persistent storage.
 *
 * Note that memory for the in/out parameter returnValue
 * MUST be allocated by callee using midpMalloc.
 * The caller is responsible for freeing it.
 *
 * @param suiteId           The suite id used to identify the MIDlet suite
 * @param suiteIdLen        The length of the suiteId
 * @param resourceName      The name of secure resource to read from storage
 * @param resourceNameLen   The length of secure resource name
 * @param returnValue       The in/out parameter that will return the
 *                          value part of the requested secure resource
 *                          (NULL is a valid return value)
 * @param valueSize         The length of the secure resource value
 *
 * @return one of the error codes:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_MEM,
 *       MIDP_ERROR_SUITE_NOT_FOUND, MIDP_ERROR_SUITE_CORRUPTED,
 *       MIDP_ERROR_ILLEGAL_ARG
 * </pre>
 */
MIDP_ERROR midpport_suite_read_secure_resource(jchar *suiteId, jint suiteIdLen,
                       jchar *resourceName, jint resourceNameLen,
                       jbyte **returnValue, jint *valueSize) {

    pcsl_string filename = PCSL_STRING_NULL;
    char *pszError = NULL;
    MIDP_ERROR errorCode;
    int bytesRead;
    int handle;

    *returnValue = NULL;
    *valueSize = 0;

    errorCode = getSecureResourceFile(suiteId, suiteIdLen,
        resourceName, resourceNameLen, &filename);
    if (errorCode != MIDP_ERROR_NONE) {
        pcsl_string_free(&filename);
        return errorCode;
    }

    handle = storage_open(&pszError, &filename, OPEN_READ);
    pcsl_string_free(&filename);
    if (pszError != NULL) {
        storageFreeError(pszError);
        return MIDP_ERROR_AMS_SUITE_CORRUPTED;
    }

    do {
        bytesRead = storageRead(&pszError, handle,
            (char*)valueSize, sizeof (int));
        if (bytesRead != sizeof (int) || *valueSize == 0)
            break;

        *returnValue = (jbyte*)midpMalloc(*valueSize * sizeof (jbyte));
        if (*returnValue == NULL) {
            errorCode = MIDP_ERROR_OUT_MEM;
            break;
        }

        bytesRead = storageRead(&pszError, handle, (char*)(*returnValue),
            *valueSize * sizeof (jbyte));

        if (pszError != NULL || bytesRead != *valueSize) {
            errorCode = MIDP_ERROR_AMS_SUITE_CORRUPTED;
            midpFree(*returnValue);
            *returnValue = NULL;
            break;
        }
    } while (0);

    storageClose(&pszError, handle);
    storageFreeError(pszError);
    return errorCode;
}

/**
 * Writes named secure resource of the suite with specified suiteId
 * to secure persistent storage.
 *
 * @param suiteId           The suite id used to identify the MIDlet suite
 * @param suiteIdLen        The length of the suiteId
 * @param resourceName      The name of secure resource to read from storage
 * @param resourceNameLen   The length of secure resource name
 * @param value             The value part of the secure resource to be stored
 * @param valueSize         The length of the secure resource value
 *
 * @return one of the error codes:
 * <pre>
 *       MIDP_ERROR_NONE, MIDP_ERROR_OUT_MEM,
 *       MIDP_ERROR_SUITE_NOT_FOUND, MIDP_ERROR_SUITE_CORRUPTED,
 *       MIDP_ERROR_ILLEGAL_ARG
 * </pre>
 */
MIDP_ERROR midpport_suite_write_secure_resource(jchar *suiteId, jint suiteIdLen,
                       jchar *resourceName, jint resourceNameLen,
                       jbyte *value, jint valueSize) {

    pcsl_string filename = PCSL_STRING_NULL;
    char *pszError = NULL;
    MIDP_ERROR errorCode;
    int handle;

    errorCode = getSecureResourceFile(suiteId, suiteIdLen,
        resourceName, resourceNameLen, &filename);
    if (errorCode != MIDP_ERROR_NONE) {
        pcsl_string_free(&filename);
        return errorCode;
    }
    
    handle = storage_open(&pszError, &filename, OPEN_READ_WRITE_TRUNCATE);
    pcsl_string_free(&filename);
    if (pszError != NULL) {
        storageFreeError(pszError);
        return MIDP_ERROR_AMS_SUITE_CORRUPTED;
    }

    do {
        storageWrite(&pszError, handle, (char*)&valueSize, sizeof (int));
        if (pszError != NULL) break;
        storageWrite(&pszError, handle, (char*)value,
            valueSize * sizeof (jbyte));
        if (pszError != NULL) break;
    } while (0);

    if (pszError != NULL) {
        errorCode = MIDP_ERROR_AMS_SUITE_CORRUPTED;
        storageFreeError(pszError);
    }

    storageClose(&pszError, handle);
    storageFreeError(pszError);
    return errorCode;
}

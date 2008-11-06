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

/**
 * @file
 *
 * This file is for utility function that depend on KNI VM functions
 * so that other files do not have to be dependent on the VM.
 */
#include <kni.h>
#include <midp_logging.h>
#include <midpMalloc.h>
#include <midpString.h>
#include <midpUtilKni.h>
#include <string.h>
#include <pcsl_memory.h>

#if ENABLE_DEBUG

/* some scratch areas for use during debugging */
int      scratchInt1;
int      scratchInt2;
jobject  scratchObject1;
jobject  scratchObject2;
jobject *scratchHandle1 = &scratchObject1;
jobject *scratchHandle2 = &scratchObject2;

#endif

/**
 * Gets the KNI field ID for an instance field of a class and checks it for
 * validity. See KNI_GetFieldID for further information.
 *
 * @param classHandle the handle to the containing object's class
 * @param name the field's name
 * @param signature the field's type
 */
jfieldID
midp_get_field_id(KNIDECLARGS jclass classHandle,
                  const char* name,
                  const char* signature)
{
    jfieldID id = KNI_GetFieldID(classHandle, name, signature);

    if (id == NULL) {
        REPORT_CRIT3(LC_CORE,
            "midp_get_field_id: can't find field, clazz=0x%x name=%s sig=%s",
            (unsigned int)classHandle, name, signature);
    }

    return id;
}

/**
 * Gets the KNI field ID for a static field of a class and checks it for
 * validity. See KNI_GetStaticFieldID for further information.
 *
 * @param classHandle the handle to the containing class
 * @param name the field's name
 * @param signature the field's type
 */
jfieldID
midp_get_static_field_id(KNIDECLARGS jclass classHandle,
                         const char* name,
                         const char* signature)
{
    jfieldID id = KNI_GetStaticFieldID(classHandle, name, signature);

    if (id == NULL) {
        REPORT_CRIT3(LC_CORE,
            "midp_get_static_field_id: "
            "can't find field, clazz=0x%x name=%s sig=%s",
            (unsigned int)classHandle, name, signature);
    }

    return id;
}

/**
 * Get a String from a field of an object and converts it to pcsl_string.
 *
 * @param obj a handle to Java object whose field will be set
 * @param classObj handle of the object's class
 * @param pszFieldName field name
 * @param fieldHandle handle where to put the resulting jstring
 * @param newValue a handle to the new Java value of the field
 * @param result pointer to the location where the result must be saved
 *
 * @return status of the operation
 */
pcsl_string_status midp_get_string_field(KNIDECLARGS jobject obj, jclass classObj,
                                  char* pszFieldName, jobject fieldHandle,
                                  pcsl_string* result) {
    KNI_GetObjectField(obj, midp_get_field_id(KNIPASSARGS classObj, pszFieldName,
        "Ljava/lang/String;"), fieldHandle);

    return midp_jstring_to_pcsl_string(fieldHandle, result);
}


/**
 * Set a jobject field from Java native functions.
 *
 * Always use KNI to set an object field instead of setting it directly
 * using SNI access, since there is more to setting a object in a field
 * than just moving a reference, there is a flag to tell the the garbage
 * collector that the field is set to an object and if this flag is not
 * set then the collector not count the field as a reference which can
 * lead to premature collection of the object the field is referencing
 * and then a crash since the field will reference will not be null, it
 * will be unchanged and invalid.
 *
 * @param obj a handle to Java object whose field will be set
 * @param fieldName field name
 * @param fieldSignature field signature string
 * @param newValue a handle to the new Java value of the field
 */
void midp_set_jobject_field(KNIDECLARGS jobject obj,
			    const char *fieldName, const char *fieldSignature,
			    jobject newValue) {

    if (KNI_IsNullHandle(obj)) {
        return;
    }

    KNI_StartHandles(1);
    KNI_DeclareHandle(clazz);

    KNI_GetObjectClass(obj, clazz);

    KNI_SetObjectField(obj,
		       midp_get_field_id(KNIPASSARGS 
					 clazz, fieldName, fieldSignature),
		       newValue);

    KNI_EndHandles();
}

/**
 * Create a new MIDP string from a KNI String object.
 * If KNI String is null, an NULL_LEN length MidpString will be returned.
 * If out of memory a OUT_OF_MEM_LEN length MidpString will be returned.
 * The caller is responsible for calling midpFreeString() after use.
 * This function should not be used directly,
 * use the midpNewString macro.
 *
 * @param jStringHandle KNI Java String object handle
 * @param filename provided by the midpNewString macro
 * @param line provided by the midpNewString macro
 *
 * @return a new unicode string
 */
MidpString midpNewStringImpl(jstring jStringHandle, char* filename,
       int line) {
    MidpString result;

    (void)filename;                               /* Avoid compiler warnings */
    (void)line;                                   /* Avoid compiler warnings */

    result.data = NULL;
    result.len  = KNI_GetStringLength(jStringHandle);

    if (result.len < 0) {
        result.len = NULL_LEN;
    } else if (result.len > 0) {
    	result.data = (jchar*)midpMallocImpl(result.len * sizeof (jchar),
					     filename, line);
	if (result.data == NULL) {
            result.len = OUT_OF_MEM_LEN;
	} else {
	    KNI_GetStringRegion(jStringHandle, 0, result.len, result.data);
	}
    }

    return result;
}

/**
 * Create a new MIDP string from a KNI CharArray object.
 * The caller is responsible for calling midpFreeString() after use.
 * This function should not be used directly,
 * use the midpNewStringFromArray macro.
 *
 * @param jCharArrayHandle handle to a jchar array
 * @param length desired length of the result
 * @param filename provided by the midpNewStringFromArray macro
 * @param line provided by the midpNewStringFromArray macro
 *
 * @return a new unicode string
 */
MidpString midpNewStringFromArrayImpl(jcharArray jCharArrayHandle, int length,
				      char* filename, int line) {
    MidpString result;

    (void)filename;                               /* Avoid compiler warnings */
    (void)line;                                   /* Avoid compiler warnings */

    result.data = NULL;
    result.len  = length;

    if (result.len < 0) {
        result.len = NULL_LEN;
    } else if (result.len > 0) {
    	result.data = (jchar*)midpMallocImpl(result.len * sizeof (jchar),
					     filename, line);
	if (result.data == NULL) {
            result.len = OUT_OF_MEM_LEN;
	} else {
	    KNI_GetRawArrayRegion(jCharArrayHandle, 0,
				  result.len * sizeof(jchar),
				  (jbyte *)result.data);
	}
    }

    return result;
}

/**
 * Create pcsl_string from the specified Java String object.
 * The caller is responsible for freeing the created pcsl_string when done.
 *
 * @param java_str pointer to the Java String instance
 * @param pcsl_str pointer to the pcsl_string instance
 * @return status of the operation
 */
pcsl_string_status midp_jstring_to_pcsl_string(jstring java_str,
					       pcsl_string * pcsl_str) {
  if (pcsl_str == NULL) {
    return PCSL_STRING_EINVAL;
  }

  if (KNI_IsNullHandle(java_str)) {
    * pcsl_str = PCSL_STRING_NULL;
    return PCSL_STRING_OK;
  } else {
    const jsize length  = KNI_GetStringLength(java_str);

    if (length < 0) {
      * pcsl_str = PCSL_STRING_NULL;
      return PCSL_STRING_ERR;
    } else if (length == 0) {
      * pcsl_str = PCSL_STRING_EMPTY;
      return PCSL_STRING_OK;
    } else {
      jchar * buffer = pcsl_mem_malloc(length * sizeof(jchar));

      if (buffer == NULL) {
	* pcsl_str = PCSL_STRING_NULL;
	return PCSL_STRING_ENOMEM;
      }

      KNI_GetStringRegion(java_str, 0, length, buffer);

      {
	pcsl_string_status status =
	  pcsl_string_convert_from_utf16(buffer, length, pcsl_str);

	pcsl_mem_free(buffer);

	return status;
      }
    }
  }
}

/**
 * Create pcsl_string from the specified KNI CharArray object.
 * The caller is responsible for freeing the created pcsl_string when done.
 *
 * @param java_arr pointer to the KNI CharArray instance
 * @param length length of the text in the CharArray
 * @param pcsl_str pointer to the pcsl_string instance
 * @return status of the operation
 */
pcsl_string_status
midp_jchar_array_to_pcsl_string(jcharArray java_arr, jint length,
                                pcsl_string * pcsl_str) {
    if (pcsl_str == NULL) {
        return PCSL_STRING_EINVAL;
    }

    if (KNI_IsNullHandle(java_arr)) {
        *pcsl_str = PCSL_STRING_NULL;
        return PCSL_STRING_OK;
    } else if (length < 0) {
        *pcsl_str = PCSL_STRING_NULL;
        return PCSL_STRING_ERR;
    } else if (length == 0) {
        *pcsl_str = PCSL_STRING_EMPTY;
        return PCSL_STRING_OK;
    } else {
        jchar * buffer = pcsl_mem_malloc(length * sizeof(jchar));

        if (buffer == NULL) {
              * pcsl_str = PCSL_STRING_NULL;
            return PCSL_STRING_ENOMEM;
        }

          KNI_GetRawArrayRegion(java_arr, 0,
                                          length * sizeof(jchar),
                                          (jbyte *) buffer);

        {
            pcsl_string_status status =
                  pcsl_string_convert_from_utf16(buffer, length, pcsl_str);

              pcsl_mem_free(buffer);

            return status;
        }
    }
}

/**
 * Create Java String object from the specified pcsl_string.
 *
 * @param pcsl_str pointer to the pcsl_string instance
 * @param java_str pointer to the Java String instance
 * @return status of the operation
 */
pcsl_string_status midp_jstring_from_pcsl_string(KNIDECLARGS
						 const pcsl_string * pcsl_str,
						 jstring java_str) {
  if (pcsl_str == NULL) {
    KNI_ReleaseHandle(java_str);
    return PCSL_STRING_EINVAL;
  } else {
    const jsize length = pcsl_string_utf16_length(pcsl_str);

    if (length < 0) {
      KNI_ReleaseHandle(java_str);
      return PCSL_STRING_EINVAL;
    } else {
      const jchar * buffer = pcsl_string_get_utf16_data(pcsl_str);

      if (buffer == NULL) {
	KNI_ReleaseHandle(java_str);
	return PCSL_STRING_ERR;
      } else {
	KNI_NewString(buffer, length, java_str);
	return PCSL_STRING_OK;
      }
    }
  }
}

/**
 * Convert a C string to a pcsl_string string.
 *
 * @param in C string specifying the text to be copied to the out parameter
 * @param out pcsl_string to receive a copy of the text specified by the in parameter
 *
 * @return jchar string
 */
pcsl_string_status pcsl_string_from_chars(const char* in, pcsl_string* out) {
    return pcsl_string_convert_from_utf8((jbyte*)in, strlen(in), out);
}

/**
 * Allocates a jchar array and copies all Unicode characters
 * from the given java.lang.String object (specified by jStringHandle)
 * to the allocated array.
 * A pointer to the array is stored in pAddr.
 * The caller MUST free the allocated array with midpFree() after use.
 *
 * If the given java.lang.String is null, NULL_LEN will be returned
 * and pAddr will be set to NULL.
 * If out of memory, OUT_OF_MEM_LEN will be returned
 * and pAddr will be set to NULL.
 *
 * @param jStringHandle KNI Java String object handle
 * @param pAddr points to a jchar* variable receiving the address of the buffer
 *              with a new unicode string. The caller MUST free this memory
 *              using midpFree when it's not needed anymore.
 *              Receives NULL if no memory has been allocated
 *              (error or null string).
 *
 * @return the new unicode string length, or one of the values:
 *          NULL_LEN if the string is null
 *          OUT_OF_MEM_LEN in the case of out-of-memory error
 */
jint midp_jstring_to_address_and_length(jstring jStringHandle, jchar* * pAddr) {
    jchar* result_data;
    jint result_len;

    result_data = NULL;
    result_len  = KNI_GetStringLength(jStringHandle);

    if (result_len < 0) {
        result_len = NULL_LEN;
    } else if (result_len > 0) {
    	result_data = (jchar*)midpMalloc(result_len * sizeof (jchar));
        if (result_data == NULL) {
            result_len = OUT_OF_MEM_LEN;
        } else {
            KNI_GetStringRegion(jStringHandle, 0, result_len, result_data);
        }
    }

    *pAddr = result_data;
    return result_len;
}


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

#ifndef _MIDP_UTIL_KNI_H_
#define _MIDP_UTIL_KNI_H_

/**
 * @file
 * @ingroup core_kni
 *
 * @brief Interface for KNI utility functions.
 *
 */


#include <kni.h>
#include <pcsl_string.h>
#include <midpString.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Gets the KNI field ID for an instance field of a class and checks it for
 * validity. See KNI_GetFieldID for further information.
 *
 * @param classHandle the handle to the containing object's class
 * @param name the field's name
 * @param signature the field's type
 */
jfieldID midp_get_field_id(KNIDECLARGS jclass classHandle,
                           const char* name,
                           const char* signature);


/**
 * Gets the KNI field ID for a static field of a class and checks it for
 * validity. See KNI_GetStaticFieldID for further information.
 *
 * @param classHandle the handle to the containing class
 * @param name the field's name
 * @param signature the field's type
 */
jfieldID midp_get_static_field_id(KNIDECLARGS jclass classHandle,
                                  const char* name,
                                  const char* signature);

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
                                  pcsl_string* result);

/**
 * Set a jobject field from Java platform native functions.
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
 * @param obj a handle to Java platform object whose field will be set
 * @param fieldName field name
 * @param fieldSignature field signature string
 * @param newValue a handle to the new Java platform value of the field
 */
void midp_set_jobject_field(KNIDECLARGS jobject obj,
			    const char *fieldName, const char *fieldSignature,
			    jobject newValue);

/**
 * Create a new MIDP string from a KNI String object.
 * If KNI String is null, an NULL_LEN length MidpString will be returned.
 * If out of memory a OUT_OF_MEM_LEN length MidpString will be returned.
 * The caller is responsible for calling midpFreeString() after use.
 * This function should not be used directly,
 * use the midpNewString macro.
 *
 * @param jStringHandle KNI Java platform String object handle
 * @param filename provided by the midpNewString macro
 * @param line provided by the midpNewString macro
 *
 * @return a new Unicode string
 */
MidpString midpNewStringImpl(jstring jStringHandle, char* filename, int line);

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
 * @return a new Unicode string
 */
MidpString midpNewStringFromArrayImpl(jcharArray jCharArrayHandle, int length,
				      char* filename, int line);

/**
 * @name Deprecated wrapper macros
 * Wrapper Macros so string leaks can be traced.
 * These names are deprecated since the MidpString data type is not used
 * anymore. Instead, pcsl_string is used.
 * @{
 */
#if REPORT_LEVEL <= LOG_WARNING

/** @see midpNewStringImpl */
#  define midpNewString(x)  midpNewStringImpl((x), __FILE__, __LINE__)
/** @see midpNewStringFromArrayImpl */
#  define midpNewStringFromArray(x, y) midpNewStringFromArrayImpl((x), (y), \
					__FILE__, __LINE__)

#else

/** @see midpNewStringImpl */
#  define midpNewString(x)  midpNewStringImpl((x), NULL, 0)
/** @see midpNewStringFromArrayImpl */
#  define midpNewStringFromArray(x, y) midpNewStringFromArrayImpl((x), (y), \
					NULL, 0)

#endif
/** @} */

/**
 * Create pcsl_string from the specified Java platform String object.
 * The caller is responsible for freeing the created pcsl_string when done.
 *
 * Use pcsl_string_free to free the created object.
 *
 * @param java_str pointer to the Java platform String instance
 * @param pcsl_str address of variable to receive the pcsl_string instance
 * @return status of the operation
 */
pcsl_string_status midp_jstring_to_pcsl_string(jstring java_str,
					       pcsl_string * pcsl_str);

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
                                pcsl_string * pcsl_str);

/**
 * Create Java platform String object from the specified pcsl_string.
 *
 * @param pcsl_str pointer to the pcsl_string instance
 * @param java_str pointer to the Java platform String instance
 * @return status of the operation
 */
pcsl_string_status midp_jstring_from_pcsl_string(KNIDECLARGS const pcsl_string * pcsl_str,
						 jstring java_str);

/**
 * Convert a C string to a pcsl_string string.
 *
 * @param in C string specifying the text to be copied to the out parameter
 * @param out pcsl_string to receive a copy of the text specified by the in parameter
 *
 * @return jchar string
 */
pcsl_string_status pcsl_string_from_chars(const char* in, pcsl_string* out);

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
 * @param jStringHandle KNI Java platform String object handle
 * @param pAddr points to a jchar* variable receiving the address of the buffer
 *              with a new Unicode string. The caller MUST free this memory
 *              using midpFree when it's not needed anymore.
 *              Receives NULL if no memory has been allocated
 *              (error or null string).
 *
 * @return the new Unicode string length, or one of the values:
 *          NULL_LEN if the string is null
 *          OUT_OF_MEM_LEN in the case of out-of-memory error
 */
jint midp_jstring_to_address_and_length(jstring jStringHandle, jchar* * pAddr);

/**
 * Starts the GET_PARAMETER_AS_PCSL_STRING...RELEASE_PCSL_STRING_PARAMETER
 * construct that provides read-only access to one string parameter.
 * The construct may be nested to provide access to multiple parameters.
 *
 * GET_PARAMETER_AS_PCSL_STRING(index,id) reads the value of the parameter
 * specified by index, assuming it is a string, and copies the text
 * to a new pcsl_string object.
 *
 * Usage 1:
 * <pre><code>
 *       KNI_StartHandles(..);
 *       GET_PARAMETER_AS_PCSL_STRING(number,parameter_name) {
 *          some code using parameter_name
 *       } RELEASE_PCSL_STRING_PARAMETER
 *       KNI_EndHandles();
 * </code></pre>
 *
 * The braces are not necessary since the construct generates a pair
 * of its own, so the below usage is equally valid:
 *
 * Usage 2:
 * <pre><code>
 *       KNI_StartHandles(..);
 *       GET_PARAMETER_AS_PCSL_STRING(number,parameter_name)
 *          some code using parameter_name
 *       RELEASE_PCSL_STRING_PARAMETER
 *       KNI_EndHandles();
 * </code></pre>
 *
 * In other words, GET_PARAMETER_AS_PCSL_STRING...RELEASE_PCSL_STRING_PARAMETER
 * defines a block. The variable whose name is passed
 * to GET_PARAMETER_AS_PCSL_STRING is declared within this block.
 * You do not have to make explicit declaration for this variable
 * (parameter_name in the above example).
 *
 * If the object specified by index cannot be converted to a pcsl_string,
 * the code between GET_PARAMETER_AS_PCSL_STRING and
 * RELEASE_PCSL_STRING_PARAMETER is not executed; instead, an out-of-memory
 * error is signaled (by executing
 * <code>KNI_ThrowNew(midpOutOfMemoryError, NULL);</code>).
 *
 * To access more than one argument string, nest the construct, as shown below:
 *
 * Usage 3:
 * <pre><code>
 *       KNI_StartHandles(2);
 *       GET_PARAMETER_AS_PCSL_STRING(1,someString)
 *       GET_PARAMETER_AS_PCSL_STRING(2,anotherString)
 *          some code using someString and anotherString
 *       RELEASE_PCSL_STRING_PARAMETER
 *       RELEASE_PCSL_STRING_PARAMETER
 *       KNI_EndHandles();
 * </code></pre>
 *
 * The macros GET_PARAMETER_AS_PCSL_STRING and RELEASE_PCSL_STRING_PARAMETER
 * must be balanced both at compile time and at runtime. In particular, you
 * MUST NOT use <code>return</code> from within this construct, because this
 * will lead to memory leaks.
 *
 * @param index the index of a string parameter in the native method
 *              invocation. Index value 1 refers to the leftmost
 *              parameter in the Java platform method.
 * @param id    the name of a variable that receives the string value.
 *              The variable is declared and visible within the scope
 *              starting from GET_PARAMETER_AS_PCSL_STRING and ending
 *              at the corresponding RELEASE_PCSL_STRING_PARAMETER.
 */
#define GET_PARAMETER_AS_PCSL_STRING(index,id) \
    { \
        pcsl_string id; \
        pcsl_string * const latest_pcsl_string_arg = &id; \
        KNI_DeclareHandle(id##_handle); \
        KNI_GetParameterAsObject(index, id##_handle); \
        if(PCSL_STRING_OK != midp_jstring_to_pcsl_string(id##_handle, &id)) { \
            KNI_ThrowNew(midpOutOfMemoryError, NULL); \
        } else { {

/**
 * Given a Java platform string handle (declared with KNI_DeclareHandle),
 * obtain the string text represented as a pcsl_string.
 * Must be balanced with RELEASE_PCSL_STRING_PARAMETER.
 * Declares a pcsl_string variable visible inside the
 * GET_JSTRING_AS_PCSL_STRING...RELEASE_PCSL_STRING_PARAMETER block.
 * This macro is particularly useful when we obtain a Java platform
 * string handle accessing a field of some Java object using
 * KNI_GetObjectField, and want it to ba converted to a pcsl_string.
 * See GET_PARAMETER_AS_PCSL_STRING.
 *
 * @param handle a Java platform string handle (declared with KNI_DeclareHandle)
 * @param id name for the declared and initialized pcsl_string.
 */
#define GET_JSTRING_AS_PCSL_STRING(handle,id) \
    { \
        pcsl_string id; \
        pcsl_string * const latest_pcsl_string_arg = &id; \
        if(PCSL_STRING_OK != midp_jstring_to_pcsl_string(handle, &id)) { \
            KNI_ThrowNew(midpOutOfMemoryError, NULL); \
        } else { {

/**
 * Closes the GET_PARAMETER_AS_PCSL_STRING...RELEASE_PCSL_STRING_PARAMETER
 * construct.
 *
 * Closes the block scope opened with the
 * matching GET_PARAMETER_AS_PCSL_STRING.
 * Frees the last pcsl_string argument.
 *
 */
#define RELEASE_PCSL_STRING_PARAMETER \
            } pcsl_string_free(latest_pcsl_string_arg); \
        } \
    }

/**
 * Given variable <code>id</code> of type pcsl_string*,
 * declare: <ul>
 * <li>id_data of type jchar const * const pointing to the string UTF-16 data, </li>
 * <li>id_len of type jint containing the string length.</li>  </ul>
 *
 * The scope of the above two names is the
 *  GET_PCSL_STRING_DATA_AND_LENGTH...RELEASE_PCSL_STRING_DATA_AND_LENGTH
 * construct.
 *
 * @param id the variable name, from which new names are derived.
 */
#define GET_PCSL_STRING_DATA_AND_LENGTH(id) \
    { \
        const jint id##_len = pcsl_string_utf16_length(id); \
        const jchar * const id##_data = pcsl_string_get_utf16_data(id); \
        const jchar * const * const last_pcsl_string_data = & id##_data; \
        const pcsl_string* const last_pcsl_string_itself = id; \
        {

/**
 * closes the
 *  GET_PCSL_STRING_DATA_AND_LENGTH...RELEASE_PCSL_STRING_DATA_AND_LENGTH
 * construct.
 */
#define RELEASE_PCSL_STRING_DATA_AND_LENGTH \
        } pcsl_string_release_utf16_data(*last_pcsl_string_data, last_pcsl_string_itself); \
    }

/**
 * Given name of pcsl_string specified as the GET_PCSL_STRING_DATA_AND_LENGTH
 * macro parameter, returns true if out-of-memory error has happened.
 *
 * @param id the variable name, from which new names are derived in
 *           GET_PCSL_STRING_DATA_AND_LENGTH macro
 */
#define PCSL_STRING_PARAMETER_ERROR(id) \
    ((id##_data) == NULL && (id##_len) > 0)


/**
 * Print a pcsl_string to the standard output. Useful for debugging.
 * Uses <code>printf</code>, requires <code>#include &lt;stdio.h&gt;</code>.
 *
 * @param fmt string literal, specifying the printf format,
 *            for example, "result=[%s]\n"
 * @param id  address of a string, of type pcsl_string*
 *
*/
#define PRINTF_PCSL_STRING(fmt,id) \
    { \
        const jbyte * const __data = pcsl_string_get_utf8_data(id); \
        printf(fmt,__data); \
        pcsl_string_release_utf8_data(__data, id); \
    }

/**
 * Print a MidpString to stdout.
 * Requires <code>#include &lt;stdio.h&gt;</code>
 */
#define PRINTF_MIDP_STRING(fmt,id) \
    GET_PCSL_STRING_FOR_MIDP_STRING(id) \
    PRINTF_PCSL_STRING(fmt,&id##_str); \
    RELEASE_PCSL_STRING_FOR_MIDP_STRING

/**
  * This macro is used for migration from MidpString to pcsl_string
  * Given a pointer to pcsl_string called someName,
  * defined a MidpString called midp_someName.
  * Opens a block, must be balanced with RELEASE_MIDP_STRING_FOR_PCSL_STRING.
  */
#define GET_MIDP_STRING_FOR_PCSL_STRING_PTR(id) \
    { \
        MidpString midp_##id; \
        MidpString * lastMidpString = &midp_##id; \
        midp_string_from_pcsl_string(id,&midp_##id); \
        {

/**
  * This macro is used for migration from MidpString to pcsl_string
  * Given a pcsl_string called someName,
  * defined a MidpString called midp_someName.
  * Opens a block, must be balanced with RELEASE_MIDP_STRING_FOR_PCSL_STRING.
  */
#define GET_MIDP_STRING_FOR_PCSL_STRING(id) \
    { \
        MidpString midp_##id; \
        MidpString * lastMidpString = &midp_##id; \
        midp_string_from_pcsl_string(&id,&midp_##id); \
        {

/**
 * This macro is used for migration from MidpString to pcsl_string
 * See GET_MIDP_STRING_FOR_PCSL_STRING, GET_MIDP_STRING_FOR_PCSL_STRING_PTR.
 */
#define RELEASE_MIDP_STRING_FOR_PCSL_STRING \
        } \
        MIDP_FREE_STRING(*lastMidpString); \
    }

/**
  * This macro is used for migration from MidpString to pcsl_string
  * Given a MidpString called someName,
  * defined a pcsl_string called someName_str.
  * Opens a block, must be balanced with RELEASE_PCSL_STRING_FOR_MIDP_STRING.
  */
#define GET_PCSL_STRING_FOR_MIDP_STRING(id) \
    { \
        pcsl_string id##_str; \
        pcsl_string * lastPcslString = &id##_str; \
        midp_string_to_pcsl_string(&id,&id##_str); \
        {

/**
 * This macro is used for migration from MidpString to pcsl_string.
 * See GET_PCSL_STRING_FOR_MIDP_STRING.
 */
#define RELEASE_PCSL_STRING_FOR_MIDP_STRING \
        } \
        pcsl_string_free(lastPcslString); \
    }

/**
 * The pair of macros AUX_MIDP_STRING...AUX_MIDP_STRING_TO_PCSL_STRING
 * is used for migration from MidpString to pcsl_string.
 * It enables us to get a MidpString output parameter as a pcsl_string.
 * See AUX_PCSL_STRING_TO_MIDP_STRING.
 */
#define AUX_MIDP_STRING(id) \
    { MidpString id = { NULL_LEN, NULL };

/**
 * The pair of macros AUX_MIDP_STRING...AUX_MIDP_STRING_TO_PCSL_STRING
 * is used for migration from MidpString to pcsl_string.
 * It enables us to get a MidpString output parameter as a pcsl_string.
 * See AUX_PCSL_STRING_TO_MIDP_STRING.
 */
#define AUX_MIDP_STRING_TO_PCSL_STRING(id,pid) \
        pcsl_string_convert_from_utf16(id.data,id.len,&pid); \
        if(id.data){ MIDP_FREE_STRING(id); } \
    }

/**
 * The pair of macros AUX_PCSL_STRING...AUX_PCSL_STRING_TO_MIDP_STRING
 * is used for migration from MidpString to pcsl_string.
 * It enables us to get a pcsl_string output parameter as a MidpString.
 * See AUX_PCSL_STRING_TO_MIDP_STRING.
 */
#define AUX_PCSL_STRING(pid) \
    { pcsl_string pid = PCSL_STRING_NULL;

/**
 * The pair of macros AUX_PCSL_STRING...AUX_PCSL_STRING_TO_MIDP_STRING
 * is used for migration from MidpString to pcsl_string.
 * It enables us to get a pcsl_string output parameter as a MidpString.
 * Usage example:
 * <pre><code>
 *  MIDP_ERROR midpExampleGetSuiteResourceFile(MidpString suiteId,
 *                        MidpString resourceName,
 *                        jboolean checkSuiteExists,
 *                        MidpString *filename) {
 *      MIDP_ERROR rc = MIDP_ERROR_OUT_MEM;
 *      GET_PCSL_STRING_FOR_MIDP_STRING(suiteId)
 *      GET_PCSL_STRING_FOR_MIDP_STRING(resourceName)
 *      AUX_PCSL_STRING(fn)
 *          rc = midp_example_get_suite_resource_file(&suiteId_str,
 *                              &resourceName_str, checkSuiteExists, &fn);
 *      AUX_PCSL_STRING_TO_MIDP_STRING(fn,*filename)
 *      RELEASE_PCSL_STRING_FOR_MIDP_STRING
 *      RELEASE_PCSL_STRING_FOR_MIDP_STRING
 *      return rc;
 *  }
 * </code></pre>
 *
 */
#define AUX_PCSL_STRING_TO_MIDP_STRING(pid,mid) \
        midp_string_from_pcsl_string(&pid,&mid); \
        pcsl_string_free(&pid); \
    }


#ifdef __cplusplus
}
#endif

#endif /* _MIDP_UTIL_KNI_H_ */

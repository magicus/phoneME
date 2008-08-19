/*
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
#ifndef __JSROP_KNI_H
#define __JSROP_KNI_H

#include <kni.h>
#include <jsrop_memory.h>
#include <jsrop_exceptions.h>
#include <javacall_defs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create javacall_utf16_string from the specified Java platform String object.
 * The caller is responsible for freeing the created javacall_utf16_string when done.
 *
 * @param java_str pointer to the Java platform String instance
 * @param utf16_str address of variable to receive the javacall_utf16_string instance
 * @return status of the operation
 */
javacall_result jsrop_jstring_to_utf16_string(jstring java_str,
					       javacall_utf16_string * utf16_str);

/**
 * Create fill buffer of javacall_utf16 from the specified Java platform String object.
 *
 * @param java_str pointer to the Java platform String instance
 * @param utf16_str address of buffer to fill
 * @param length length of buffer to fill
 * @return status of the operation
 */
javacall_result jsrop_jstring_to_utf16(jstring java_str,
					       javacall_utf16_string utf16_str, int length);

/**
 * Create Java platform String object from the specified javacall_utf16_string.
 *
 * @param utf16_str pointer to the javacall_utf16_string instance
 * @param java_str pointer to the Java platform String instance
 * @return status of the operation
 */
javacall_result
jsrop_jstring_from_utf16_string(KNIDECLARGS const javacall_utf16_string utf16_str,
						 jstring java_str);

/**
 * Create Java platform String object from the specified javacall_utf16_string.
 * exactly utf16_chars_n characters are copied
 * utf16_str can contain any number of zero characters in it
 *
 * @param utf16_str pointer to the javacall_utf16_string instance
 * @param utf16_chars_n number of utf16 characters to copy.. 
 * @param java_str pointer to the Java platform String instance
 * @return status of the operation
 */
javacall_result
jsrop_jstring_from_utf16_string_n(KNIDECLARGS const javacall_utf16_string utf16_str, int utf16_chars_n,
						 jstring java_str);


/**
 * Starts the GET_PARAMETER_AS_UTF16_STRING...RELEASE_UTF16_STRING_PARAMETER
 * construct that provides read-only access to one string parameter.
 * The construct may be nested to provide access to multiple parameters.
 *
 * GET_PARAMETER_AS_UTF16_STRING(index,id) reads the value of the parameter
 * specified by index, assuming it is a string, and copies the text
 * to a new javacall_utf16_string object.
 *
 * Usage 1:
 * <pre><code>
 *       KNI_StartHandles(..);
 *       GET_PARAMETER_AS_UTF16_STRING(number,parameter_name) {
 *          some code using parameter_name
 *       } RELEASE_UTF16_STRING_PARAMETER
 *       KNI_EndHandles();
 * </code></pre>
 *
 * The braces are not necessary since the construct generates a pair
 * of its own, so the below usage is equally valid:
 *
 * Usage 2:
 * <pre><code>
 *       KNI_StartHandles(..);
 *       GET_PARAMETER_AS_UTF16_STRING(number,parameter_name)
 *          some code using parameter_name
 *       RELEASE_UTF16_STRING_PARAMETER
 *       KNI_EndHandles();
 * </code></pre>
 *
 * In other words, GET_PARAMETER_AS_UTF16_STRING...RELEASE_UTF16_STRING_PARAMETER
 * defines a block. The variable whose name is passed
 * to GET_PARAMETER_AS_UTF16_STRING is declared within this block.
 * You do not have to make explicit declaration for this variable
 * (parameter_name in the above example).
 *
 * If the object specified by index cannot be converted to a javacall_utf16_string,
 * the code between GET_PARAMETER_AS_UTF16_STRING and
 * RELEASE_UTF16_STRING_PARAMETER is not executed; instead, an out-of-memory
 * error is signaled (by executing
 * <code>KNI_ThrowNew(jsropOutOfMemoryError, NULL);</code>).
 *
 * To access more than one argument string, nest the construct, as shown below:
 *
 * Usage 3:
 * <pre><code>
 *       KNI_StartHandles(2);
 *       GET_PARAMETER_AS_UTF16_STRING(1,someString)
 *       GET_PARAMETER_AS_UTF16_STRING(2,anotherString)
 *          some code using someString and anotherString
 *       RELEASE_UTF16_STRING_PARAMETER
 *       RELEASE_UTF16_STRING_PARAMETER
 *       KNI_EndHandles();
 * </code></pre>
 *
 * The macros GET_PARAMETER_AS_UTF16_STRING and RELEASE_UTF16_STRING_PARAMETER
 * must be balanced both at compile time and at runtime. In particular, you
 * MUST NOT use <code>return</code> from within this construct, because this
 * will lead to memory leaks.
 *
 * @param index the index of a string parameter in the native method
 *              invocation. Index value 1 refers to the leftmost
 *              parameter in the Java platform method.
 * @param id    the name of a variable that receives the string value.
 *              The variable is declared and visible within the scope
 *              starting from GET_PARAMETER_AS_UTF16_STRING and ending
 *              at the corresponding RELEASE_UTF16_STRING_PARAMETER.
 */
#define GET_PARAMETER_AS_UTF16_STRING(index,id) \
    { \
        javacall_utf16_string id; \
        javacall_utf16_string * const latest_utf16_string_arg = &id; \
        KNI_DeclareHandle(id##_handle); \
        KNI_GetParameterAsObject(index, id##_handle); \
        if(JAVACALL_OK != jsrop_jstring_to_utf16_string(id##_handle, &id)) { \
            KNI_ThrowNew(jsropOutOfMemoryError, NULL); \
        } else { {

/**
 * Given a Java platform string handle (declared with KNI_DeclareHandle),
 * obtain the string text represented as a javacall_utf16_string.
 * Must be balanced with RELEASE_UTF16_STRING_PARAMETER.
 * Declares a javacall_utf16_string variable visible inside the
 * GET_JSTRING_AS_UTF16_STRING...RELEASE_UTF16_STRING_PARAMETER block.
 * This macro is particularly useful when we obtain a Java platform
 * string handle accessing a field of some Java object using
 * KNI_GetObjectField, and want it to ba converted to a javacall_utf16_string.
 * See GET_PARAMETER_AS_UTF16_STRING.
 *
 * @param handle a Java platform string handle (declared with KNI_DeclareHandle)
 * @param id name for the declared and initialized javacall_utf16_string.
 */
#define GET_JSTRING_AS_UTF16_STRING(handle,id) \
    { \
        javacall_utf16_string id; \
        javacall_utf16_string * const latest_utf16_string_arg = &id; \
        if(JAVACALL_OK != jsrop_jstring_to_utf16_string(handle, &id)) { \
            KNI_ThrowNew(jsropOutOfMemoryError, NULL); \
        } else { {

/**
 * Closes the GET_PARAMETER_AS_UTF16_STRING...RELEASE_UTF16_STRING_PARAMETER
 * construct.
 *
 * Closes the block scope opened with the
 * matching GET_PARAMETER_AS_UTF16_STRING.
 * Frees the last javacall_utf16_string argument.
 *
 */
#define RELEASE_UTF16_STRING_PARAMETER \
            } if (*latest_utf16_string_arg) JAVAME_FREE(*latest_utf16_string_arg); \
        } \
    }

#ifdef __cplusplus
}
#endif

#endif /* __JSROP_KNI_H */


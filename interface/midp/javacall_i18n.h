/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */ 
 
#ifndef __JAVACALL_I18N_H_
#define __JAVACALL_I18N_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <javacall_defs.h>

/**
 * Check if native platform has resources for string data
 *
 * @return JAVACALL_TRUE if resource data is exist or JAVACALL_FALSE if not
 */
javacall_bool javacall_i18n_check_native_resouce();

/**
 * Get string data from native platform resource data
 *
 * @param index     Index of string
 * @param str       Return of string. It should be pointer to constant memory.
 * @param length    Length of str
 *
 * @return JAVACALL_OK or JAVACALL_FAIL
 */
javacall_result javacall_i18n_get_native_string(int index, 
        javacall_utf16** str, int* length);

#ifdef __cplusplus
}
#endif

#endif

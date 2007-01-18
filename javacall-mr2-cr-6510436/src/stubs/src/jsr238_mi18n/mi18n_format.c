/*
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

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_defs.h" 

/*
Implementation of low-level porting API for JSR 238 (MI18N).
*/


/**
 * Gets the number of supported locales for data formatting.
 *
 * @return the number of supported locales or 0 if something is wrong.
 */
int javacall_mi18n_get_format_locales_count() {
    return 0;
}

/**
 * Gets locale name for data formatting with the given index.
 *
 * @param loc    buffer for the locale.
 * @param len    buffer length
 * @param index  index of the locale.
 * @return JAVACALL_OK if all done successfuly, 
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_mi18n_get_format_locale(char* loc, int len, int index) {
    /* Suppress unused parameter warnings */
    (void)loc;
    (void)index;

    return JAVACALL_FAIL;
}

/**
 * Gets size of date and time formatting symbols.
 *
 * @param length  size of formatting symbols (in bytes).
 * @param locale_index  index of the locale.
 * @return JAVACALL_OK if all done successfuly, 
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_mi18n_get_date_format_symbols_length(
                                  int* length /* OUT */, int locale_index) {

    /* Suppress unused parameter warnings */
    (void)length;
    (void)locale_index;

    return JAVACALL_FAIL;
}

/**
 * Gets date and time formatting symbols as an array of bytes.
 *
 * @param symbols       buffer to store the formatting symbols.
 * @param sym_len       buffer size.
 * @param locale_index  index of the locale.
 * @return JAVACALL_OK if all done successfuly, 
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_mi18n_get_date_format_symbols(char* symbols, 
                                             int sym_len, int locale_index) {

    /* Suppress unused parameter warnings */
    (void)symbols;
    (void)sym_len;
    (void)locale_index;

    return -1;
}

/**
 * Gets size of number formatting symbols.
 *
 * @param length  size of formatting symbols (in bytes).
 * @param locale_index  index of the locale.
 * @return JAVACALL_OK if all done successfuly, 
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_mi18n_get_number_format_symbols_length(
                                  int* length /* OUT */, int locale_index) {
    /* Suppress unused parameter warnings */
    (void)length;
    (void)locale_index;

    return JAVACALL_FAIL;
}

/**
 * Gets number formatting symbols as an array of bytes.
 *
 * @param symbols       buffer to store the formatting symbols.
 * @param sym_len       buffer size.
 * @param locale_index  index of the locale.
 * @return JAVACALL_OK if all done successfuly, 
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_mi18n_get_number_format_symbols(char* symbols, 
                                            int sym_len, int locale_index) {

    /* Suppress unused parameter warnings */
    (void)symbols;
    (void)sym_len;
    (void)locale_index;

    return JAVACALL_FAIL;
}

#ifdef __cplusplus
}
#endif

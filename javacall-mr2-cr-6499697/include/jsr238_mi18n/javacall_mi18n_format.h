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

#ifndef __javacall_mi18n_format_h
#define __javacall_mi18n_format_h

/**
 * @defgroup JSR238 JSR238 Mobile Internationalization API (MI18N)
 * @ingroup stack
 * 
 * Porting interface for native implementation Mobile Internationalization API.
 * 
 * @{
 */
/** @} */

/**
 * @file javacall_mi18n_format.h
 * @ingroup JSR238
 * @brief JSR238 Mobile Internationalization API (MI18N)
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_defs.h" 

/**
 * @defgroup jsrMandatoryFormat Low-level formatting porting API
 * @ingroup JSR238
 * 
 * Porting interface for native implementation of data formatting functionality.
 * 
 * @{
 */

/**
 * Gets the number of supported locales for data formatting.
 *
 * @return the number of supported locales or 0 if something is wrong.
 */
int javacall_mi18n_get_format_locales_count();


/**
 * Gets locale name for data formatting with the given index.
 *
 * @param loc    buffer for the locale.
 * @param len    buffer length
 * @param index  index of the locale.
 * @return JAVACALL_OK if all done successfuly, 
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_mi18n_get_format_locale(char* loc, int len, int index);

typedef enum {
	SHORTDATE = 0,
	LONGDATE,
	SHORTTIME,
	LONGTIME,
	SHORTDATETIME,
	LONGDATETIME	
}JAVACALL_MI18N_DATETIME_FORMAT_STYLE;

/**
 * Formats a date as a date string for a specified locale.
 * 
 * @param locale index of locale to select.
 * @param year Year
 * @param month Month
 * @param dow Day of the week
 * @param dom Day of the month
 * @param hour Hours
 * @param min Minutes
 * @param sec Seconds
 * @param buffer the buffer to store formatted  utf16 encoded string
 * @param buffer_len the length of buffer, in javacall_utf16 chars, to store string
 * @return the length, in javacall_utf16 chars, of formatted string or zero in case of error
 * @note If <code>buffer_len</code> is zero, the function returns the number 
 *       of javacall_utf16 characters required to hold the formatted currency string,
 *       and the buffer is not used
 */
int javacall_mi18n_format_date_time(int locale, int year, int month, 
                                    int dow, int dom, 
                                    int hour, int min, int sec, 
                                    int style, char* buffer, int buffer_len);
/**
 * Formats a number string for a specified locale.
 * 
 * @param locale index of locale to select.
 * @param s1 pointer to a null-terminated  utf16 encoded string containing
 *        the number string to format. 
 *        This string can only contain the following characters: 
 * <ul>
 *     <li> Characters '0' through '9'. 
 *     <li> One decimal point (dot) if the number is a floating-point value. 
 *     <li> A minus sign in the first character position if the number 
 *          is a negative value. 
 * </ul>
 *        All other characters are invalid.
 * @param decimals the number of fractional digits
 * @param res the buffer to store formatted  utf16 encoded string
 * @param res_len the length of buffer, in javacall_utf16 chars, to store string
 * @return the length, in javacall_utf16 chars, of formatted string  or zero in case of error
 * @note If <code>res_len</code> is zero, the function returns the number 
 *       of javacall_utf16 characters required to hold the formatted currency string,
 *       and the buffer is not used
 */
int javacall_mi18n_format_number(int locale, char* s1, int decimals, char *res, int res_len);

/**
 * Formats a number string as a currency string for a specified locale.
 * 
 * @param locale index of locale to select.
 * @param s1 pointer to a null-terminated  utf16 encoded string containing
 *        the number string to format. 
 *        This string can only contain the following characters: 
 * <ul>
 *     <li> Characters '0' through '9'. 
 *     <li> One decimal point (dot) if the number is a floating-point value. 
 *     <li> A minus sign in the first character position if the number 
 *          is a negative value. 
 * </ul>
 *        All other characters are invalid.
 * @param s2  the ISO 4217 currency code to use (null-terminated utf16 encoded string)
 * @param res the buffer to store formatted  utf16 encoded string
 * @param res_len the length of buffer, in javacall_utf16 chars, to store string
 * @return the length, in javacall_utf16 chars, of formatted string or zero in case of error
 * @note If <code>res_len</code> is zero, the function returns the number 
 *       of javacall_utf16 characters required to hold the formatted currency string,
 *       and the buffer is not used
 */
int javacall_mi18n_format_currency(int locale, char* s1, char* s2, char *res, int res_len);

/**
 * Formats a number string as a percentage string customized for a specified locale.
 * 
 * @param locale_index index of locale to select.
 * @param s1 pointer to a null-terminated  utf16 encoded string containing
 *        the number string to format. 
 *        This string can only contain the following characters: 
 * <ul>
 *     <li> Characters '0' through '9'. 
 *     <li> One decimal point (dot) if the number is a floating-point value. 
 *     <li> A minus sign in the first character position if the number 
 *          is a negative value. 
 * </ul>
 *        All other characters are invalid.
 * @param decimals the number of fractional digits
 * @param res the buffer to store formatted utf16 encoded string 
 * @param res_len the length of buffer, in javacall_utf16 chars, to store string
 * @return the length, in javacall_utf16 chars, of formatted string or zero in case of error
 * @note If <code>res_len</code> is zero, the function returns the number 
 *       of javacall_utf16 characters required to hold the formatted currency string,
 *       and the buffer is not used
 */
int javacall_mi18n_format_percentage(int locale_index, char* s1, int decimals, char* res, int res_len);

/** @} */

#ifdef __cplusplus
}
#endif

#endif //__javacall_mi18n_format_h

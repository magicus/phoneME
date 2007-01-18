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
 * @ingroup MSA
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
 * @param count_out pointer to integer that recieves
 *					the number of supported locales
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 */
javacall_result javacall_mi18n_get_format_locales_count(/*OUT*/int* count_out);


/**
 * Gets a locale name for formatter for the index.
 *
 * @param locale_index  index of the locale.
 * @param locale_name_out  buffer for the locale.
 * @param plen	pointer to integer initially containing the buffer length
 *				and receiving length of result string in javacall_utf16 including terminating zero, 
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 * @note Zero index (neutral locale) must return name - empty string
 */
javacall_result javacall_mi18n_get_format_locale_name(int locale_index, javacall_utf16* locale_name_out, /*IN|OUT*/int* plen);


/**
 * Gets locale index used for date/number formatting by the given miroedition.locale name.
 *
 * @param loc    utf16 string containing requested locale name or null for neutral locale
 * @param index_out	pointer to integer receiving index of requested locale,
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 * @note Neutral (empty string) locale must be supported. It must have index 0
 */
javacall_result javacall_mi18n_get_format_locale_index(const javacall_utf16* locale, /*OUT*/int* index_out);

typedef enum {
	SHORTDATE = 0,
	LONGDATE,
	SHORTTIME,
	LONGTIME,
	SHORTDATETIME,
	LONGDATETIME	
} JAVACALL_MI18N_DATETIME_FORMAT_STYLE;

/**
 * Formats a date as a date string for a specified locale.
 * 
 * @param locale index of locale to select.
 * @param year Year
 * @param month Month
 * @param dow Day of the week - optional parameter can be ignored by implementation
 * @param dom Day of the month
 * @param hour Hours
 * @param min Minutes
 * @param sec Seconds
 * @param style formatting style
 * @param buffer the buffer to store formatted  utf16 encoded string
 * @param plen	pointer to integer initially containing the buffer length in javacall_utf16 characters
 *				and receiving length of result string in javacall_utf16 characters including terminating zero, 
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 * @note If <code>plen</code> points to zero, the function returns the number 
 *       of javacall_utf16 characters required to hold the formatted date/time string,
 *       and the value of buffer is ignored
 *       Not all date-time parameter shoud be filled in.
 *       Pass zero if some parameters are not used for given formatting style.
 */
javacall_result javacall_mi18n_format_date_time(int locale,
									int year, int month,int dow, int dom, 
                                    int hour, int min, int sec, 
                                    JAVACALL_MI18N_DATETIME_FORMAT_STYLE style,
									javacall_utf16* buffer, /*IN|OUT*/int* plen);



/**
 * Convert double value to a null terminating ascii string containing only decimal digits,
 * The position of the decimal point and the sign of value can be obtained from decimal and sign after the function call
 * @param d				double to convert
 * @param digits		number of digits to be stored
 * @param decimal		pointer to stored decimal-point position 
 * @param sign			pointer to stored sign indicator 
 * @param pbuffer_out	pointer to variable that receves pointer to static buffer
						used to keep each function call result
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 */
javacall_result javacall_mi18n_double_to_ascii(double d, int digits, int* decimal, int* sign, /*OUT*/char** pbuffer_out);


/**
 * Convert an unsigned long integer to an ascii string containing only decimal digits from '0' to '9'
 * @param l unsigned long integer to convert
 * @param pbuffer_out	pointer to variable that receves pointer to static buffer
						used to keep each function call result
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 */
javacall_result javacall_mi18n_long_to_ascii(unsigned long l, /*OUT*/char** pbuffer_out);


/**
 * Formats a number string for a specified locale.
 * 
 * @param locale index of locale to select.
 * @param s pointer to a null-terminated ASCII string containing meaning digits of the number string to format. 
 *        This string can only contain the characters '0' through '9' All other characters are invalid.
 * @param dotposition the virtual position of the decimal point in number string,
 *        a zero or negative value means that decimal point lies to the left of the first digit
 * @param isnegative non zero value if number is negtive or zero if positive
 * @param decimals the number of fractional digits in result, if decimals is set to -1 number of decimal is taken by default for given locale
 * @param res_buffer the buffer to store formatted  utf16 encoded string
 * @param plen	pointer to integer initially containing the buffer length in javacall_utf16 characters
 *				and receiving length of result string in javacall_utf16 characters including terminating zero, 
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 * @note If <code>plen</code> points to zero, the function returns the number 
 *       of javacall_utf16 characters required to hold the formatted currency string,
 *       and the buffer is not used
 */
javacall_result javacall_mi18n_format_number(int locale, const char* s, int dotposition, int isnegative, int decimals,
											 /*OUT*/javacall_utf16 *res_buffer, /*IN|OUT*/int* plen); 


/**
 * Formats a number string as a currency string for a specified locale.
 * 
 * @param locale index of locale to select.
 * @param s pointer to a null-terminated ASCII string containing meaning digits of the number string to format. 
 *        This string can only contain the digits.
 *        All other characters are invalid.
 * @param code  the ISO 4217 currency code to use (null-terminated utf16 encoded string)
 * @param dotposition the virtual position of the decimal point in number string,
 *        a zero or negative value means that decimal point lies to the left of the first digit
 * @param isnegative non zero value if number is negtive or zero if positive
 * @param res_buffer the buffer to store formatted  utf16 encoded string

 * @param plen	pointer to integer initially containing the buffer length in javacall_utf16 characters
 *				and receiving length of result string in javacall_utf16 characters including terminating zero, 
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 * @note If <code>plen</code> points to zero, the function returns the number 
 *       of javacall_utf16 characters required to hold the formatted currency string,
 *       and the buffer is not used
 **/
javacall_result javacall_mi18n_format_currency(int locale, const char* s, int dotposition, int isnegative,
											   const javacall_utf16 *code,
											   /*OUT*/javacall_utf16 *res_buffer, /*IN|OUT*/int* plen);


/**
 * Formats a number string as a percentage string customized for a specified locale.
 * 
 * @param locale index of locale to select.
 * @param s pointer to a null-terminated ASCII string containing meaning digits of the number string to format. 
 *        This string can only contain the digits.
 *        All other characters are invalid.
 * @param dotposition the virtual position of the decimal point in number string,
 *        a zero or negative value means that decimal point lies to the left of the first digit
 * @param isnegative non zero value if number is negtive or zero if positive
 * @param decimals the number of fractional digits in result, if decimals is set to -1 number of decimal is taken by default for given locale
 * @param res_buffer the buffer to store formatted  utf16 encoded string
 * @param buffer_len the length of buffer, in javacall_utf16 chars, to store string
 * @return the length, in javacall_utf16 chars including terminating zero of formatted string or -1 in case of error
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 * @note If <code>plen</code> points to zero, the function returns the number 
 *       of javacall_utf16 characters required to hold the formatted number string,
 *       and the buffer is not used
 *		 This function should not perform any rounding operations with number constaining in input string s,
 *		 all such operation must be copleted before function call when s is formed.
 *       All spare digits must be truncated if needed
 */
javacall_result javacall_mi18n_format_percentage(int locale, const char* s, int dotposition, int isnegative, int decimals,
												 /*OUT*/javacall_utf16 *res_buffer, /*IN|OUT*/int* plen);

/** @} */

#ifdef __cplusplus
}
#endif

#endif //__javacall_mi18n_format_h

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

#include "javacall_mi18n_format.h"
#include "windows.h"
#include "stdlib.h"

/*
Implementation of low-level porting API for JSR 238 (MI18N).
*/

// platform specific locale operation part (defined in mi18n_common.c)
int mi18n_enum_locales();
LCID mi18n_get_locale_id(int index);
javacall_result mi18n_get_locale_name(char* pBuff, int bufLen, int index);

/**
 * Gets the number of supported locales for data formatting.
 *
 * @return the number of supported locales or 0 if something is wrong.
 */
int javacall_mi18n_get_format_locales_count() {
    return mi18n_enum_locales();
}

/**
 * Gets locale name for data formatting with the given index.
 *
 * @param loc    buffer for the locale.
 * @param len    buffer len
 * @param index  index of the locale.
 * @return JAVACALL_OK if all done successfuly, 
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_mi18n_get_format_locale(char* loc, int len, int index) {
    return mi18n_get_locale_name(loc, len, index);
}


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
                                    int style, char* buffer, int buffer_len) {
    int len = 0;
    BOOL time2show = FALSE;
    BOOL date2show = FALSE;
    BOOL long_format = FALSE;
    SYSTEMTIME time = {year, month, dow, dom, hour, min, sec, 0 };
    LCID id = mi18n_get_locale_id(locale);
    switch (style) {
    case SHORTDATE:
        date2show = TRUE;
        break;
    case LONGDATE:
        date2show = TRUE;
        long_format = TRUE;
        break;
    case SHORTTIME:
        time2show = TRUE;
        break;
    case LONGTIME  :
        time2show = TRUE;
        long_format = TRUE;
        break;
    case SHORTDATETIME:
        date2show = TRUE;
        time2show = TRUE;
        break;
    default:
    case LONGDATETIME:
        date2show = TRUE;
        time2show = TRUE;
        long_format = TRUE;
        break;
    }

    if (date2show) {
        len = GetDateFormatW(id, long_format ? DATE_LONGDATE : DATE_SHORTDATE, 
                             &time, NULL, (LPWSTR)buffer, buffer_len);
    }
    if (time2show) {
        if (date2show) {
          if (buffer_len) { 
            // overwrite trailing zero with space
            memcpy(buffer + (len - 1) * sizeof(wchar_t), L" ", sizeof(wchar_t));
            buffer_len -= len;
          }
        }
        len += GetTimeFormatW(id, long_format ? 0 : TIME_NOSECONDS, 
                             &time, NULL, (LPWSTR)(buffer + len * sizeof(wchar_t)), buffer_len);
    }
    return len;
}

#define SEPARATOR_LEN 10
// Prepare formatting information for given locale with specified number of fractional digits
static void getNumberFormat(LCID id, LPNUMBERFMTW format, int decimals) {
    DWORD i;
    char str[10];

    // if decimals!= -1
    if (!(~decimals)) {
        GetLocaleInfoA(id,  LOCALE_IDIGITS|LOCALE_RETURN_NUMBER, (LPSTR)&i, sizeof(DWORD));
    } else {
        i = decimals;
    }
    format->NumDigits = i;
    GetLocaleInfoA(id,  LOCALE_ILZERO|LOCALE_RETURN_NUMBER, (LPSTR)&i, sizeof(DWORD));
    format->LeadingZero = (UINT)i;

    // force ASCII version of GetNumberFormat
    GetLocaleInfoA(id,  LOCALE_SGROUPING , (LPSTR)str, sizeof(str));
    // parsing
    format->Grouping = str[0] - '0';
    if (';' == str[1]) {
        if ('0' == str[2]) {
            // repeat last value
            format->Grouping = format->Grouping * 10 + format->Grouping;
        } else {
            format->Grouping = format->Grouping * 10 + str[2] - '0';
        }
    }

    GetLocaleInfoW(id,  LOCALE_SDECIMAL, format->lpDecimalSep, SEPARATOR_LEN);
    GetLocaleInfoW(id,  LOCALE_STHOUSAND, format->lpThousandSep, SEPARATOR_LEN);

    GetLocaleInfo(id,  LOCALE_INEGNUMBER|LOCALE_RETURN_NUMBER, (LPSTR)&i, sizeof(DWORD));
    format->NegativeOrder = (UINT)i;
}

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
int javacall_mi18n_format_number(int locale, char* s1, int decimals, char *res, int res_len) {
    LCID id = mi18n_get_locale_id(locale);
    char decimalSep[SEPARATOR_LEN], thousandSep[SEPARATOR_LEN];
    NUMBERFMTW format = {0, 0, 3, (LPWSTR)decimalSep, (LPWSTR)thousandSep, 0};
    int len, count;
    // win32 does'nt format string with the number of  fractional digits more than 9
    if (decimals > 9) {
        getNumberFormat(id, &format, 9);
    } else {
        getNumberFormat(id, &format, decimals);
    }
    len = GetNumberFormatW(id, 0, (LPWSTR)s1, &format, (LPWSTR)res, res_len);
    // the following code only for TCK
    // it does not take into account grouping rules
    if (decimals > 9) {
        if (res_len) {
            res_len = decimals - 9;
            // move pointer to the end of the formatted string
            res += (len - 1) * sizeof(wchar_t);

            // looking for unprocessed digits or end of string
            s1 = (char*)wcschr((wchar_t*)s1, (wchar_t)('.'));
            count  = 0;
            while (NULL != s1 && 0 != *(wchar_t*)s1 && count < 10/*+.*/) {
                s1 += sizeof(wchar_t);
                count++;
            }

            while (res_len--) {
                if (NULL != s1 && 0 != *(wchar_t*)s1) {
                    *(((wchar_t*)res)++) = *(wchar_t*)s1++;
                } else {
                    *(((wchar_t*)res)++) = (wchar_t)'0';
                }

            }
            // zero trailing
            *(wchar_t*)res = 0;
        }
        //
        len += decimals - 9;
    }
    return len;
}

static wchar_t monetary2search[6];
// this function is an application-defined function used with the EnumSystemLocales function
static BOOL CALLBACK currency_search_callback(LPTSTR lpLocaleString) {
    wchar_t monetary[4];
    char hexValue[11] = {'0','x'};
    LCID lcid;
    memcpy(hexValue+2, lpLocaleString, strlen(lpLocaleString)+1);
    lcid = strtoul(hexValue, NULL, 16);
    GetLocaleInfoW(lcid,  LOCALE_SINTLSYMBOL, monetary, sizeof(monetary));
    if (!wcsncmp(monetary, monetary2search, sizeof(monetary)/sizeof(wchar_t)-1)) {
        GetLocaleInfoW(lcid,  LOCALE_SCURRENCY, monetary2search, sizeof(monetary2search)/sizeof(wchar_t));
        return FALSE;
    }
    return TRUE;
}
// returns local monetary symbol by given international monetary symbol
static wchar_t *mi8n_get_currency_symbol(wchar_t *currency) {
    wcscpy(monetary2search, currency);
    EnumSystemLocales(currency_search_callback, LCID_INSTALLED);
    return monetary2search;
}

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
int javacall_mi18n_format_currency(int locale, char* s1, char* s2, char *res, int res_len) {
    LCID id = mi18n_get_locale_id(locale);
    CURRENCYFMTW format; 
    DWORD i;
    char str[10];
    char decimalSep[SEPARATOR_LEN], thousandSep[SEPARATOR_LEN];
    if (s2 > 0) {
        GetLocaleInfoA(id,  LOCALE_IDIGITS|LOCALE_RETURN_NUMBER, (LPSTR)&i, sizeof(DWORD));
        format.NumDigits = (UINT)i;
        GetLocaleInfoA(id,  LOCALE_ILZERO|LOCALE_RETURN_NUMBER, (LPSTR)&i, sizeof(DWORD));
        format.LeadingZero = (UINT)i;
        // force ASCII version of GetNumberFormat
        GetLocaleInfoA(id,  LOCALE_SMONGROUPING , (LPSTR)str, sizeof(str));
        // parsing
        format.Grouping = str[0] - '0';
        if (';' == str[1]) {
            // Values in the range 0 – 9 and 32 are valid
            if ('2' == str[2]) {
                // repeat last value
                format.Grouping = format.Grouping * 10 + 2;
            }
        }
        format.lpDecimalSep = (LPWSTR)decimalSep;
        format.lpThousandSep = (LPWSTR)thousandSep;
        GetLocaleInfoW(id,  LOCALE_SMONDECIMALSEP, format.lpDecimalSep, SEPARATOR_LEN);
        GetLocaleInfoW(id,  LOCALE_SMONTHOUSANDSEP, format.lpThousandSep, SEPARATOR_LEN);

        GetLocaleInfo(id,  LOCALE_INEGNUMBER|LOCALE_RETURN_NUMBER, (LPSTR)&i, sizeof(DWORD));
        format.NegativeOrder = (UINT)i;

        GetLocaleInfo(id,  LOCALE_ICURRENCY|LOCALE_RETURN_NUMBER, (LPSTR)&i, sizeof(DWORD));
        format.PositiveOrder = (UINT)i;

        format.lpCurrencySymbol = mi8n_get_currency_symbol((wchar_t*)s2);

        return GetCurrencyFormatW(id, 0, (LPWSTR)s1, &format, (LPWSTR)res, res_len);
    }
    return GetCurrencyFormatW(id, 0, (LPWSTR)s1, NULL, (LPWSTR)res, res_len);
}

// replace trailing zero with % sign
const char tail[4] = {'%', 0,  0, 0};

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
 * @note windows does not provide special functions for percentage formating. 
 *       Therefore we just add percent sign to the end of formatted number string.
 */
int javacall_mi18n_format_percentage(int locale_index, char* s1, int decimals, char* res, int res_len) {
    int size;
    size = javacall_mi18n_format_number(locale_index, s1, decimals, res, res_len);
    if (size > 0) {
        if (res_len > 0) {
	        if ((res_len - size + 1) * sizeof(wchar_t) - sizeof(tail) > 0) {
	            res_len = sizeof(tail);
	         } else {
              res_len = (res_len - size + 1) * sizeof(wchar_t);
           }
           // discard trailing zero
           memcpy(res + (size - 1) * sizeof(wchar_t), tail, res_len);
    	  }
        // just append percent sign
        size += 1;
        return size;
    } else {
        size = GetLastError();
        return 0;
    }
}
#ifdef __cplusplus
}
#endif

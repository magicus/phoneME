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


/*
Implementation of low-level porting API for JSR 238 (MI18N).
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif

#ifndef WINVER
#define WINVER 0x0500
#endif

#ifndef UNICODE
#define UNICODE
#endif

#include "windows.h"
#include "javacall_mi18n_format.h"
#include "mi18n_locales.h"
#include "stdlib.h"
#include "stdio.h"

//space char used to devide characters in one formatted entity
#define	NON_BREAKING_SPACE_CHAR 0x00A0

//space char used to devide date and time in datetime format
#define	DATE_TIME_SEPARATOR_CHAR NON_BREAKING_SPACE_CHAR

//maximum nuber of characters in currency name
#define MAX_CURRENCY_NAME 50

//maximum nuber of decimal digits in currency
#define CURRENCY_DECIMALS_MAX 5

//default percent sign
static const WCHAR* default_percent_sign=L"%";
//ar,fa,ur
static const WCHAR* arabic_percent_sign=L"\x066A";
//dz-BT
static const WCHAR* dzongkha_butan_percent_sign=L"\x0F56\x0F62\x0F92\x0F0B\x0F46\x0F71";

// locale set for formatter
#define FORMATTER_LOCALES_SET 0

// size of number format structures cache
#define NUMBER_FORMAT_CACHE_SIZE 6

// formatting modes
#define FORMAT_NUMBER_MODE 0
#define FORMAT_CURRENCY_MODE 1

#ifdef NOT_USE_NATIVE_DIGITS
static const WCHAR decimal_digits[10]={'0','1','2','3','4','5','6','7','8','9'};
#endif

typedef struct tagNumberFormatSymbols{
	LCID    lcid;
    UINT    fractionDigits;                 // number of decimal digits by default
    UINT    leadingZero;               // if leading zero in decimal fields
	UINT    negativeOrder;             // negative number ordering (LOCALE_INEGNUMBER)
#ifndef NOT_USE_NATIVE_DIGITS
	WCHAR   digits[10];				   // native digits
#endif
    unsigned char grouping[8];         // group size left of decimal
    WCHAR   decimalSep[5];             // decimal separator string
    WCHAR   thousandSep[5];            // thousand separator string
	WCHAR   negativeSign[6];		   // native minus sign

/** information needed for format_percentage functions **/
	const WCHAR*   percentSign;		   // native percent sign
	UINT    percentOrder;              // percent sign ordering
//										0 #%			  - default
//										1 # %             - used in Norwegian
//										2 %#			  - used in Farsi-Iran	


/** information needed for format_currency functions **/
	UINT    cur_positiveMode;		   // positive currency mode (LOCALE_ICURRENCY)
	UINT    cur_negativeMode;		   // modified negtive currency mode (LOCALE_INEGCURR) 
									   // 0 - 
									   // 1 - 
	WCHAR   cur_monetarySign[6];	   // default local monetary symbol (LOCALE_SCURRENCY)
	WCHAR   cur_monetaryIntlCode[4];   // default intl monetary symbol (LOCALE_SINTLSYMBOL)

	UINT    cur_fractionDigits;		   // Number of fractional digits for the local monetary format.(LOCALE_ICURRDIGITS)
    WCHAR   cur_decimalSep[5];         // decimal separator string used in currency format
    WCHAR   cur_thousandSep[5];        // thousand separator string used in currency format
    unsigned char cur_grouping[10];     // group size left of decimal used in currency format

} NumberFormatSymbols;


static NumberFormatSymbols number_format_cache[NUMBER_FORMAT_CACHE_SIZE]={{-1},{-1},{-1},{-1},{-1},{-1}};
int g_number_format_insert_index=-1;

// parse grouping string to byte array containing group numbers terminated by -1
static void parseGrouping(WCHAR* buf, unsigned char grouping[]){
	int len = 0;
	while (*buf && (len < sizeof(grouping)-1)) {	
		if (*buf>='0' && *buf<='9') {
			grouping[len++] = *buf - '0';	
		}
		++buf;
	}
	grouping[len] = -1;
}

static const NumberFormatSymbols* get_number_format_symbols(LCID lcid){
	int i;
	int res;
	NumberFormatSymbols* pfmt;
	WCHAR* buf;
	int lang, sublang;

	for (i= 0; i < NUMBER_FORMAT_CACHE_SIZE; ++i){
		if (number_format_cache[i].lcid == lcid) return &number_format_cache[i];
	}

	if (++g_number_format_insert_index == NUMBER_FORMAT_CACHE_SIZE) {
		g_number_format_insert_index = 0;
	}

	pfmt = &number_format_cache[g_number_format_insert_index];

	pfmt->lcid = -1; //fault guard

	// number of decimal digits
    res=GetLocaleInfoW(lcid,  LOCALE_IDIGITS|LOCALE_RETURN_NUMBER, (LPWSTR)&(pfmt->fractionDigits), sizeof(UINT)/sizeof(WCHAR));
	if (!res) return 0;

	// if leading zero in decimal fields
	res	=GetLocaleInfoW(lcid,  LOCALE_ILZERO|LOCALE_RETURN_NUMBER, (LPWSTR)&(pfmt->leadingZero), sizeof(UINT)/sizeof(WCHAR));
	if (!res) return 0;

	// negative number ordering
    res=GetLocaleInfoW(lcid,  LOCALE_INEGNUMBER|LOCALE_RETURN_NUMBER, (LPWSTR)&(pfmt->negativeOrder), sizeof(UINT)/sizeof(WCHAR));
	if (!res) return 0;
	
#ifndef NOT_USE_NATIVE_DIGITS
	// native digits
	res=GetLocaleInfoW(lcid,  LOCALE_SNATIVEDIGITS , pfmt->digits, 11);
	if (!res) return 0;
#endif

	// group size left of decimal
	buf = pfmt->decimalSep;//temp buf
	res=GetLocaleInfoW(lcid,  LOCALE_SGROUPING, buf, (WCHAR*)(pfmt + 1) - buf);
	if (!res) return 0;

	parseGrouping(buf,pfmt->grouping);

	// group size left of decimal used in currency format
	res=GetLocaleInfoW(lcid,  LOCALE_SMONGROUPING , buf, sizeof(pfmt->cur_grouping));
	if (!res) return 0;

	parseGrouping(buf,pfmt->cur_grouping);

	// decimal separator string
	res=GetLocaleInfoW(lcid,  LOCALE_SDECIMAL , pfmt->decimalSep, sizeof(pfmt->decimalSep)/sizeof(pfmt->decimalSep[0]));
	if (!res) return 0;

	// thousand separator string
	res=GetLocaleInfoW(lcid,  LOCALE_STHOUSAND , pfmt->thousandSep, sizeof(pfmt->thousandSep)/sizeof(pfmt->thousandSep[0]));
	if (!res) return 0;

	// string value for the negative sign
	res=GetLocaleInfoW(lcid,  LOCALE_SNEGATIVESIGN , pfmt->negativeSign, sizeof(pfmt->negativeSign)/sizeof(pfmt->negativeSign[0]));
	if (!res) return 0;

	lang = PRIMARYLANGID(LANGIDFROMLCID(lcid));
	sublang = SUBLANGID(LANGIDFROMLCID(lcid));

	// get percent sign
	if ((lang == LANG_ARABIC) || (lang == LANG_FARSI) ||  (lang == LANG_URDU)){
		pfmt->percentSign = arabic_percent_sign;
	} else {
		pfmt->percentSign = default_percent_sign;
	}

	if ((lang == LANG_FARSI)){
		pfmt->percentOrder = 2;
	} else if ((lang == LANG_NORWEGIAN)){
		pfmt->percentOrder = 1;
	} else {
		pfmt->percentOrder = 0;
	}

	// positive currency mode
	res=GetLocaleInfoW(lcid, LOCALE_ICURRENCY|LOCALE_RETURN_NUMBER,  (LPWSTR) &pfmt->cur_positiveMode, sizeof(UINT)/sizeof(WCHAR));
	if (!res) return 0;

	// negative currency mode
	res=GetLocaleInfoW(lcid, LOCALE_INEGCURR|LOCALE_RETURN_NUMBER, (LPWSTR) &pfmt->cur_negativeMode, sizeof(UINT)/sizeof(WCHAR));
	if (!res) return 0;

	if  (pfmt->cur_negativeMode == 2 || 
		 pfmt->cur_negativeMode == 3 || 
		 pfmt->cur_negativeMode == 5 ||
		 pfmt->cur_negativeMode == 6 ||
		 pfmt->cur_negativeMode == 11||
		 pfmt->cur_negativeMode == 12||
		 pfmt->cur_negativeMode == 13)
	{
		pfmt->cur_negativeMode = 0;
	} else {
		pfmt->cur_negativeMode =1;
	}


	// default local monetary symbol 
	res=GetLocaleInfoW(lcid,  LOCALE_SCURRENCY , pfmt->cur_monetarySign, sizeof(pfmt->cur_monetarySign)/sizeof(WCHAR));
	if (!res) return 0;

	// default intl monetary symbol 
	res=GetLocaleInfoW(lcid,  LOCALE_SINTLSYMBOL , pfmt->cur_monetaryIntlCode, sizeof(pfmt->cur_monetaryIntlCode)/sizeof(WCHAR));
	if (!res) return 0;

	// number of fractional digits for the local monetary format.
	res=GetLocaleInfoW(lcid,  LOCALE_ICURRDIGITS|LOCALE_RETURN_NUMBER , (WCHAR*)&pfmt->cur_fractionDigits, sizeof(UINT)/sizeof(WCHAR));
	if (!res) return 0;

	// decimal separator string used in currency format
	res=GetLocaleInfoW(lcid,  LOCALE_SMONDECIMALSEP , pfmt->cur_decimalSep, sizeof(pfmt->cur_decimalSep)/sizeof(WCHAR));
	if (!res) return 0;

	// thousand separator string used in currency format
	res=GetLocaleInfoW(lcid,  LOCALE_SMONTHOUSANDSEP , pfmt->cur_thousandSep, sizeof(pfmt->cur_thousandSep)/sizeof(WCHAR));
	if (!res) return 0;

	pfmt->lcid = lcid;

	return pfmt;
}

// add space to the end of string buffer, return new string length or -1 in case of error
static int add_space(javacall_utf16 *res_buffer, int len, int buffer_len){
	if (!buffer_len) return len+1;
	if (buffer_len<len+1) return -1;
	res_buffer[len++]=NON_BREAKING_SPACE_CHAR;
	return len;
}

// insert space to the insertpos of string buffer, return new string length or -1 in case of error
static int insert_space(javacall_utf16 *res_buffer, int len,int insertpos, int buffer_len){
		if (buffer_len<len+1 || buffer_len<=insertpos) return -1;
		if (insertpos<len){
			memcpy(res_buffer+insertpos+1,res_buffer+insertpos, (len-insertpos)*sizeof(WCHAR));
		}
		res_buffer[insertpos]=NON_BREAKING_SPACE_CHAR;
		return ++len;
}




/**
* Insert negative sign according to "negative order"
* returns new length of buffer or -1 in case of error
* modes:
* 0 (1.1)  
* 1 -1.1 
* 2 - 1.1 
* 3 1.1- 
* 4 1.1 - 
*/
static int negate_number(const NumberFormatSymbols* pfmt, javacall_utf16 *res_buffer, int len, int buffer_len, int negativeOrder){
		int minuslen = wcslen(pfmt->negativeSign);

#ifdef MINUS_ALWAYS_INFRONT
		if (negativeOrder==0) negativeOrder=1;else
		if (negativeOrder==3) negativeOrder=1;else
		if (negativeOrder==4) negativeOrder=2;
#endif
		if (!buffer_len){
			if (negativeOrder==0) return len+2;
			if (negativeOrder==2 || negativeOrder==4) return len+minuslen+1;
			return len+minuslen;
		}

		if (negativeOrder==0){
			if (buffer_len<len+1) return -1;
			memcpy(res_buffer+1,res_buffer,len*sizeof(WCHAR));
			res_buffer[0] = '(';
			++len;
			if (buffer_len<len+1) return -1;
			res_buffer[len] = ')';
			++len;
		}

		
		if (negativeOrder==1 || negativeOrder==2){
			if (buffer_len<len+minuslen) return -1;
			memcpy(res_buffer+minuslen,res_buffer,len * sizeof(WCHAR));
			memcpy(res_buffer,pfmt->negativeSign,minuslen * sizeof(WCHAR));
			len += minuslen;
		}
		if (negativeOrder==2){
			len=insert_space(res_buffer,len,minuslen,buffer_len);
			if (len<0) return len;
		}
		if (negativeOrder==4){
			len=add_space(res_buffer,len,buffer_len);
			if (len<0) return len;
		}
		if (negativeOrder==3 || negativeOrder==4){
			if (buffer_len<len+minuslen) return -1;
			memcpy(res_buffer+len,pfmt->negativeSign,minuslen * sizeof(WCHAR));
			len+= minuslen;
		}

		return len;
}

/**
 * Formats a number string for a specified locale.
 * @param pfmt formatting synbols information
 * @param s string containing formattin number digits
 * @param dotposition virtual position of decimal separator in string s, negative value means position left from first digit
 * @param decimals desired number of digits in fraction part
 * @param res_buffer
 * @param len current length of output string buffer
 * @param buffer_len maximum length of output string buffer
 * @param formatting_mode can be one of:
 *			FORMAT_NUMBER_MODE - use general number format symbols for locale
 *			FORMAT_CURRENCY_MODE - use currency specific format symbols for locale
 * @return new length of output string buffer or -1 in case of error
 */
static int format_number(const NumberFormatSymbols* pfmt, const char* s, int dotposition, int decimals, javacall_utf16 *res_buffer, int len, int buffer_len, int formatting_mode){ 
	const WCHAR* thousandSep; 
	const WCHAR* decimalSep; 
	const unsigned char* grouping;
	const WCHAR* digits;

#ifdef NOT_USE_NATIVE_DIGITS
	digits = decimal_digits; 
#else
	digits = pfmt->digits; 
#endif
	
	// defining formatting symbols
	if (formatting_mode == FORMAT_CURRENCY_MODE){
		thousandSep = pfmt -> cur_thousandSep;
		decimalSep = pfmt -> cur_decimalSep;
		grouping = pfmt -> cur_grouping;
		if (decimals == -1) decimals = pfmt->cur_fractionDigits;
	} else {
		thousandSep = pfmt -> thousandSep;
		decimalSep = pfmt -> decimalSep;
		grouping = pfmt -> grouping;
		if (decimals == -1) decimals = pfmt->fractionDigits;
	}
	
	// integer part
	if (dotposition>0){
		int thseplen;

		//skip trailing zeroes
		while (*s && *s=='0' && dotposition>1){
			++s;--dotposition;
		}

		// insert digits
		while (*s && dotposition){
			if (*s<'0' || *s>'9') return -1;
			if (buffer_len){
				if (buffer_len<len+1) return -1;
				res_buffer[len] = digits[*s-'0'];
			}
			++len;
			++s;--dotposition;
		}

		// thousands separators (grouping)
		thseplen = wcslen(thousandSep);
		if (thseplen){
			int group=-1, l=0;
			WCHAR* p = &res_buffer[len];
			while (p > res_buffer) {
				if (*grouping) group = *grouping++;
				if (group==-1) break;
				if (p - res_buffer > group){
					p -= group;
					l += group;
					if (buffer_len){
						if (buffer_len<len+thseplen) return 0;
						memcpy(p+thseplen, p, l * sizeof(WCHAR));
						memcpy(p,thousandSep,thseplen * sizeof(WCHAR));
					}
					l += thseplen;
					len += thseplen;
				} else 
					break;
			}

		}
	} else {
		if (buffer_len){
			if (buffer_len<len+1) return -1;
			res_buffer[len] = digits[0];
		}
		++len;
	}

	// fraction part
	if (decimals) {

		// decimal separator
		int dseplen = wcslen(decimalSep);
		if (buffer_len){
			if (buffer_len<len+dseplen) return -1;
			memcpy(&res_buffer[len],decimalSep,dseplen * sizeof(WCHAR));
		}
		len+=dseplen;
		 
		// trailing zeroes at the front
		while (dotposition && decimals){
			if (buffer_len){
				if (buffer_len < len +1) return -1;
				res_buffer[len] = digits[0];
			}
			++len;++dotposition;--decimals;
		}
		
		// meaning digits
		while (*s && decimals){
			
			if (*s<'0' || *s>'9') return -1;
			if (buffer_len){
				if (buffer_len < len +1) return 0;
				res_buffer[len] = digits[*s-'0'];
			}
			++len;--decimals;++s;
		}

		// trailing zeroes at the end
		while (decimals){
			if (buffer_len){
				if (buffer_len < len +1) return -1;
				res_buffer[len] = digits[0];
			}
			++len;--decimals;
		}
	}

    return len;
}


/**
* Insert percent sign according to "percent order"
* returns new length of buffer or -1 in case of error
* PercentOrder:
*	0: #%			  - default
*	1: # %            - used in Norwegian
*	2: %#			  - used in Farsi-Iran	
**/
 
static int insert_percent(const NumberFormatSymbols* pfmt, javacall_utf16 *res_buffer, int len, int buffer_len){
		int percentlen = wcslen(pfmt->percentSign);

		if (!buffer_len){
			if (pfmt->percentOrder==1) return len+percentlen+1;
			return len+percentlen;
		}

		if (pfmt->percentOrder==2){
			if (buffer_len<len+percentlen) return -1;
			memcpy(res_buffer+percentlen,res_buffer,len * sizeof(WCHAR));
			memcpy(res_buffer,pfmt->percentSign,percentlen * sizeof(WCHAR));
			len += percentlen;
		}
		if (pfmt->percentOrder==1){
			len = add_space(res_buffer,len,buffer_len);
			if (len<0) return len;
		}
		if (pfmt->percentOrder==0 || pfmt->percentOrder==1){
			if (buffer_len<len+percentlen) return -1;
			memcpy(res_buffer+len,pfmt->percentSign,percentlen * sizeof(WCHAR));
			len+= percentlen;
		}

		return len;
}


/** 
* Insert currency sign to buffer
* if code is -1 print default currency sign for locale else look for localized sign for code, if not found print code
* returns new length of buffer or -1 in case of error
* currencyOrder:
*	 0: Prefix, no separation, for example $1.1 
*	 1: Suffix, no separation, for example 1.1$ 
*	 2: Prefix, 1-character separation, for example $ 1.1 
*	 3: Suffix, 1-character separation, for example 1.1 $ 
*/
static int insert_currency_symbol(const NumberFormatSymbols* pfmt, const WCHAR* code, javacall_utf16 *res_buffer, int len, int buffer_len, int currencyOrder){
	const WCHAR* currencySign;
	int currencylen;

 	if (!code || !wcscmp(code,pfmt->cur_monetaryIntlCode)) {
		//default language code
		currencySign=pfmt->cur_monetarySign;
	} else {
		//LOCALE_SINTLSYMBOL
		currencySign=code;
	}

	currencylen=wcslen(currencySign);

	if (!buffer_len){
		if (currencyOrder==2 || currencyOrder==3) return len+currencylen+1;
		return len+currencylen;
	}

	if (currencyOrder==0 || currencyOrder==2){
		if (buffer_len<len+currencylen) return -1;
		memcpy(res_buffer+currencylen,res_buffer,len * sizeof(WCHAR));
		memcpy(res_buffer,currencySign,currencylen * sizeof(WCHAR));
		len += currencylen;
	}
	if (currencyOrder==2){
		len = insert_space(res_buffer,len,currencylen,buffer_len);
		if (len<0) return len;
	}
	if (currencyOrder==3){
		len = add_space(res_buffer,len,buffer_len);
		if (len<0) return len;
	}
	if (currencyOrder==1 || currencyOrder==3){
		if (buffer_len<len+currencylen) return -1;
		memcpy(res_buffer+len,currencySign,currencylen * sizeof(WCHAR));
		len+= currencylen;
	}

	return len;

 }

/*********************************************** API Implementation ****************************************************************/

/**
 * Gets the number of supported locales for data formatting.
 *
 * @param count_out pointer to integer that recieves
 *					the number of supported locales
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 */
javacall_result javacall_mi18n_get_format_locales_count(/*OUT*/int* count_out){
	if (!count_out) return JAVACALL_INVALID_ARGUMENT;
	*count_out = i18n_get_supported_locales_count(FORMATTER_LOCALES_SET);
	if (!*count_out) return JAVACALL_FAIL;
    return JAVACALL_OK;
}

/**
 * Gets a locale name for formatter for the index.
 *
 * @param locale_index  index of the locale.
 * @param locale_name_out  buffer for the locale.
 * @param plen	pointer to integer initially containing the buffer length
 *				and receiving length of result string in javacall_utf16 including terminating zero, 
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 */
javacall_result javacall_mi18n_get_format_locale_name(int locale_index, javacall_utf16* locale_name_out, /*IN|OUT*/int* plen){
    return mi18n_get_locale_name(locale_index, FORMATTER_LOCALES_SET, locale_name_out, plen);
}

/**
 * Gets locale index used for date/number formatting by the given miroedition.locale name.
 *
 * @param loc    utf16 string containing requested locale name or null for neutral locale
 * @param index_out	pointer to integer receiving index of requested locale,
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 * @note If neutral (empty string) locale is supported it must have index 0
 */
javacall_result javacall_mi18n_get_format_locale_index(const javacall_utf16* locale, /*OUT*/int* index_out){
	if (!index_out) return JAVACALL_INVALID_ARGUMENT;
	*index_out = mi18n_get_locale_index(locale,FORMATTER_LOCALES_SET);
	if (*index_out<0) return JAVACALL_FAIL;
	return JAVACALL_OK;
}

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
 *		 There is native limitation on year's value, it should be between 1601 and 30827
 */
javacall_result javacall_mi18n_format_date_time(int locale,
									int year, int month,int dow, int dom, 
                                    int hour, int min, int sec, 
                                    JAVACALL_MI18N_DATETIME_FORMAT_STYLE style,
									javacall_utf16* buffer, /*IN|OUT*/int* plen){

    int len = 0, buffer_len, res;
    BOOL time2show = FALSE;
    BOOL date2show = FALSE;
    BOOL long_format = FALSE;
	BOOL bres;
	DWORD calendar;
    SYSTEMTIME time = {year, month, dow, dom, hour, min, sec, 0 };
    LCID lcid,lcidused;

	if (!plen) return JAVACALL_INVALID_ARGUMENT;
	if (*plen<0) return JAVACALL_INVALID_ARGUMENT;
	if (*plen>0 && !buffer) return JAVACALL_INVALID_ARGUMENT;

	if (year < 1601 || year > 30827) return JAVACALL_INVALID_ARGUMENT;
	
	buffer_len = *plen;

	lcid = mi18n_get_locale_id(locale, FORMATTER_LOCALES_SET);
	if (lcid == ERROR_LOCALE) {
		return JAVACALL_FAIL;
	}
	lcidused = lcid;

	//set gregorian calendar
	bres = SetLocaleInfoW(lcid,LOCALE_ICALENDARTYPE,L"1"); 
	GetLocaleInfoW(lcid,LOCALE_ICALENDARTYPE,(WCHAR*)&calendar,sizeof(DWORD)/sizeof(WCHAR)); 
	if (!bres || calendar!=1){
		//Gregorian calendar is not supported for locale, use neutral locale formatting
		lcidused = LOCALE_NEUTRAL;
	}

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
        res = GetDateFormatW(lcidused, (long_format && (lcid==lcidused)) ? DATE_LONGDATE : DATE_SHORTDATE, 
                             &time, NULL, buffer, buffer_len);
		if (buffer_len) buffer_len-=res;
		len = res;
    }

    if (time2show) {
        res = GetTimeFormatW(lcidused, 0, &time, NULL, &buffer[len], buffer_len);
		if (!res) return JAVACALL_FAIL;

		if (date2show && *plen) {
			if (locale == 0){
				buffer[len-1] = 'T';
			} else {
				buffer[len-1] = DATE_TIME_SEPARATOR_CHAR;
			}
		}
		len+=res;
    }
#ifndef NOT_USE_NATIVE_DIGITS
	//in case of replacing lcid to neutral change digits in formatted data to local digits
	if (buffer_len && lcid!=lcidused){
		int i=len;
		const NumberFormatSymbols* pfmt;
		pfmt = get_number_format_symbols(lcid);
		if (!pfmt) return JAVACALL_FAIL;
		if (pfmt->digits[0]!='0'){
			while (i-- > 0){
				if (buffer[i]>='0' && buffer[i]<='9'){
					buffer[i] = pfmt->digits[buffer[i]-'0'];
				}
			}
		}
	}
#endif
	*plen = len;
    return JAVACALL_OK;
}

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
											 /*OUT*/javacall_utf16 *res_buffer, /*IN|OUT*/int* plen)
{ 
	const NumberFormatSymbols* pfmt;
    LCID lcid;
	int len=0;
	int buffer_len;

	if (!plen) return JAVACALL_INVALID_ARGUMENT;

	buffer_len = *plen;
	*plen = 0;

	if (buffer_len<0 || (buffer_len>0 && !res_buffer) || !s || locale<0) return JAVACALL_INVALID_ARGUMENT;

	lcid = mi18n_get_locale_id(locale, FORMATTER_LOCALES_SET);
	if (lcid == ERROR_LOCALE) return JAVACALL_FAIL;
	
	pfmt = get_number_format_symbols(lcid);
	if (!pfmt) return JAVACALL_FAIL;

	len = format_number(pfmt,s,dotposition, decimals,res_buffer,len, buffer_len,FORMAT_NUMBER_MODE);
	if (len<0) return JAVACALL_FAIL;

	// add minus sign if needed
	if (isnegative){
		len = negate_number(pfmt, res_buffer, len, buffer_len, pfmt->negativeOrder);
		if (len<0) return JAVACALL_FAIL;
	}

	if (buffer_len){
		if (buffer_len<len+1) return JAVACALL_FAIL;
		res_buffer[len] = '\0';
	}
	
	*plen = ++len;
    return JAVACALL_OK;

}

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
											   /*OUT*/javacall_utf16 *res_buffer, /*IN|OUT*/int* plen)
{

	const NumberFormatSymbols* pfmt;
    LCID lcid;
    int len=0;
	int buffer_len;

	if (!plen) return JAVACALL_INVALID_ARGUMENT;

	buffer_len = *plen;

	if (buffer_len<0 || (buffer_len>0 && !res_buffer) || !s || locale<0) return JAVACALL_INVALID_ARGUMENT;

	lcid = mi18n_get_locale_id(locale, FORMATTER_LOCALES_SET);
	if (lcid == ERROR_LOCALE) return JAVACALL_FAIL;
	
	pfmt = get_number_format_symbols(lcid);
	if (!pfmt) return JAVACALL_FAIL;

	// insert number part
    len = format_number(pfmt, s, dotposition, -1, res_buffer, len, buffer_len, FORMAT_CURRENCY_MODE);
	if (len<0) return JAVACALL_FAIL;

	// negative sign is set prior currency sigh)
	if (isnegative	&&  pfmt->cur_negativeMode == 0)
	{
		len = negate_number(pfmt, res_buffer, len, buffer_len, pfmt ->negativeOrder);
		if (len<0) return JAVACALL_FAIL;
	}

	// insert currency symbol
	len = insert_currency_symbol(pfmt, code, res_buffer, len, buffer_len, pfmt->cur_positiveMode);
	if (len<0) return JAVACALL_FAIL;

	// negative sign is set after currency sigh)
	if (isnegative	&&  pfmt->cur_negativeMode == 1)
	{
		len = negate_number(pfmt, res_buffer, len, buffer_len, pfmt ->negativeOrder);
		if (len<0) return JAVACALL_FAIL;
	}

	// insert terminate zero
	if (buffer_len){
		if (buffer_len<len+1) return -1;
		res_buffer[len] = '\0';
	}
	
	*plen = ++len;
    return JAVACALL_OK;
}



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
												 /*OUT*/javacall_utf16 *res_buffer, /*IN|OUT*/int* plen){ 
	const NumberFormatSymbols* pfmt;
    LCID lcid;
    int len=0,lang,pslen;
	int buffer_len;

	if (!plen) return JAVACALL_INVALID_ARGUMENT;

	buffer_len = *plen;
	if (buffer_len<0 || (buffer_len>0 && !res_buffer) || !s || locale<0) return JAVACALL_INVALID_ARGUMENT;

	lcid = mi18n_get_locale_id(locale, FORMATTER_LOCALES_SET);
	if (lcid == ERROR_LOCALE) return JAVACALL_FAIL;
	
	pfmt = get_number_format_symbols(lcid);
	if (!pfmt) return JAVACALL_FAIL;


	lang = PRIMARYLANGID(LANGIDFROMLCID(lcid));
	pslen = wcslen(pfmt->percentSign);


    len = format_number(pfmt, s, dotposition, decimals, res_buffer, len, buffer_len, FORMAT_NUMBER_MODE);
	if (len<0) return JAVACALL_FAIL;
	

	if (lang == LANG_FARSI){
		len = insert_percent(pfmt, res_buffer, len, buffer_len);
		if (len<0) return JAVACALL_FAIL;
	}

	// add minus sign if needed
	if (isnegative){
		len = negate_number(pfmt, res_buffer, len, buffer_len, pfmt->negativeOrder);
		if (len<0) return JAVACALL_FAIL;
	}

	if (lang != LANG_FARSI){
		len = insert_percent(pfmt, res_buffer, len, buffer_len);
		if (len<0) return JAVACALL_FAIL;
	}


	if (buffer_len){
		if (buffer_len<len+1) return JAVACALL_FAIL;
		res_buffer[len] = '\0';
	}
	
	*plen = ++len;
    return JAVACALL_OK;
}


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
javacall_result javacall_mi18n_double_to_ascii(double d, int digits, int* decimal, int* sign, /*OUT*/char** pbuffer_out){
	if (!pbuffer_out) return JAVACALL_INVALID_ARGUMENT;
	*pbuffer_out =  _ecvt(d, digits, decimal, sign);
	if (!*pbuffer_out) return JAVACALL_FAIL;
	return JAVACALL_OK;
}


/**
 * Convert an unsigned long integer to an ascii string containing only decimal digits from '0' to '9'
 * @param l unsigned long integer to convert
 * @param pbuffer_out	pointer to variable that receves pointer to static buffer
						used to keep each function call result
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 */
javacall_result javacall_mi18n_long_to_ascii(unsigned long l, /*OUT*/char** pbuffer_out){
	static char buffer[22];
	if (!pbuffer_out) return JAVACALL_INVALID_ARGUMENT;
	*pbuffer_out = ultoa(l,buffer,10);
	if (!*pbuffer_out) return JAVACALL_FAIL;
	return JAVACALL_OK;
}

#ifdef __cplusplus
}
#endif

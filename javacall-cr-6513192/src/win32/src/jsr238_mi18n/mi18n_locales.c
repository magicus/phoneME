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


#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif

#ifndef WINVER
#define WINVER 0x0500
#endif

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif

#include <tchar.h>
#include "windows.h"
#include "mi18n_locales.h" 

#define LOCALES_CACHE_SIZE 4
#define LOCALES_SET_SIZE 2
#define MAX_LOCALE_NAME 50
#define DEFAULT_LOCALES_SET LCID_SUPPORTED

#define RESOURCE_STRING_TABLE_ID 51

#if(WINVER >= 0x0500)
static const WCHAR* csz_default_sort_order = L"Default";
#endif

typedef struct tagLocale{
	int index;
	LCID lcid;
} Locale;

struct tagLocalesCache;

typedef struct tagLocalesCache{
	DWORD locales_set;
	int locales_count;
	int locales_insert_index;
	Locale cache[LOCALES_CACHE_SIZE]; 
} LocalesCache;

int g_locales_set_size=0;
int g_locales_set_insert_index=0;
LocalesCache g_pLocalesCaches[LOCALES_SET_SIZE]={-1L};
LocalesCache* g_pLocalesCache;

int g_search_counter;
int g_index_to_search;
int g_index_found;
const wchar_t* g_name_to_search;

extern HMODULE hResModule;

wchar_t* get_locale_name(LCID lcid);

static BOOL checkLocaleIsComplient(LCID lcid) {
	int res;
	if (!(g_pLocalesCache->locales_set & INCLUDE_SORTING_VARIANTS) && (lcid & 0xFFFF0000)){
		return FALSE;
	}
	res = GetLocaleInfo(lcid,  LOCALE_SISO639LANGNAME, 0, 0);
	if (res!=3) return FALSE;
	res = GetLocaleInfo(lcid,  LOCALE_SISO3166CTRYNAME, 0, 0);
	if (res!=3) return FALSE;
	return TRUE;
}

static BOOL CALLBACK enumCountLocalesProc(LPTSTR lpLocaleString) {
	LCID lcid = _tcstoul(lpLocaleString, NULL, 16);
	if (checkLocaleIsComplient(lcid)) {
		++(g_pLocalesCache->locales_count);
	}
	return TRUE;
}


static BOOL CALLBACK enumSearchLocalesByIndexProc(LPTSTR lpLocaleString) {
	LCID lcid = _tcstoul(lpLocaleString, NULL, 16);
	if (checkLocaleIsComplient(lcid)) {
		if (++g_search_counter == g_index_to_search){
			if (++g_pLocalesCache->locales_insert_index == LOCALES_CACHE_SIZE) {
				g_pLocalesCache->locales_insert_index = 0;
			}
			g_index_found = g_pLocalesCache->locales_insert_index;
			g_pLocalesCache->cache[g_index_found].index = g_search_counter; 
			g_pLocalesCache->cache[g_index_found].lcid = lcid;
			return FALSE;
		}
	}
	return TRUE;
}

static BOOL CALLBACK enumSearchLocalesByNameProc(LPTSTR lpLocaleString) {
	LCID lcid = _tcstoul(lpLocaleString, NULL, 16);
	if (checkLocaleIsComplient(lcid)) {
		const wchar_t* name = get_locale_name(lcid);
		++g_search_counter;
		if (name && !wcscmp(name,g_name_to_search)){
				if (++g_pLocalesCache->locales_insert_index == LOCALES_CACHE_SIZE) {
					g_pLocalesCache->locales_insert_index = 0;
				}
				g_index_found = g_pLocalesCache->locales_insert_index;
				g_pLocalesCache->cache[g_index_found].index = g_search_counter; 
				g_pLocalesCache->cache[g_index_found].lcid = lcid;
				return FALSE;
		}
	}
	return TRUE;
}


static BOOL CALLBACK enumResLocProc(HMODULE hModule,LPCTSTR lpType,LPCTSTR lpName, WORD  wLanguage, LONG lParam)
{
	LCID lcid = MAKELCID(wLanguage,0);
	if (checkLocaleIsComplient(lcid)) {
		++(g_pLocalesCache->locales_count);
	}
	return TRUE;
}

static BOOL CALLBACK enumResLocByIndexProc(HMODULE hModule,LPCTSTR lpType,LPCTSTR lpName, WORD  wLanguage, LONG lParam) {
	LCID lcid = MAKELCID(wLanguage,0);
	if (checkLocaleIsComplient(lcid)) {
		if (++g_search_counter == g_index_to_search){
			if (++g_pLocalesCache->locales_insert_index == LOCALES_CACHE_SIZE) {
				g_pLocalesCache->locales_insert_index = 0;
			}
			g_index_found = g_pLocalesCache->locales_insert_index;
			g_pLocalesCache->cache[g_index_found].index = g_search_counter; 
			g_pLocalesCache->cache[g_index_found].lcid = lcid;
			//return FALSE;
		}
	}
	return TRUE;
}

static BOOL CALLBACK enumResLocByNameProc(HMODULE hModule,LPCTSTR lpType,LPCTSTR lpName, WORD  wLanguage, LONG lParam) {
	LCID lcid = MAKELCID(wLanguage,0);
	const wchar_t* name = get_locale_name(lcid);
	++g_search_counter;
	if (name && !wcscmp(name,g_name_to_search)){
			if (++g_pLocalesCache->locales_insert_index == LOCALES_CACHE_SIZE) {
				g_pLocalesCache->locales_insert_index = 0;
			}
			g_index_found = g_pLocalesCache->locales_insert_index;
			g_pLocalesCache->cache[g_index_found].index = g_search_counter; 
			g_pLocalesCache->cache[g_index_found].lcid = lcid;
			//return FALSE;
	}
	
	return TRUE;
}


static void select_locales_cache(DWORD dwLocalesSet){
	int i;
	for (i=0;i<g_locales_set_size;++i){
		if (g_pLocalesCaches[i].locales_set == dwLocalesSet){
			g_pLocalesCache = &g_pLocalesCaches[i];
			return;
		}
	}
	g_pLocalesCache = &g_pLocalesCaches[g_locales_set_insert_index++];
	if (g_locales_set_size<g_locales_set_insert_index) g_locales_set_size=g_locales_set_insert_index;
	if (g_locales_set_insert_index==LOCALES_SET_SIZE) g_locales_set_insert_index=0;

	g_pLocalesCache->locales_set = dwLocalesSet;
	g_pLocalesCache->locales_count = -1L;
	g_pLocalesCache->locales_insert_index = 0;
	for (i=0; i < LOCALES_CACHE_SIZE; ++i){
		g_pLocalesCache->cache[i].index = -1L;
	}

}

static DWORD locales_set_to_enum_flag(DWORD dwLocalesSet){
	DWORD res = DEFAULT_LOCALES_SET;
	if (dwLocalesSet & INCLUDE_SORTING_VARIANTS) res|=LCID_ALTERNATE_SORTS;
	return res;
}

// returns count of all possible locales supported by platform
int i18n_get_supported_locales_count(DWORD dwLocalesSet){
	select_locales_cache(dwLocalesSet);
	if (g_pLocalesCache->locales_count==-1){
		g_pLocalesCache->locales_count=0;
		if (dwLocalesSet==RESOURCE_LOCALES){
			if (!hResModule) return -1;
			if (!EnumResourceLanguages(hResModule,RT_STRING,MAKEINTRESOURCE(RESOURCE_STRING_TABLE_ID),enumResLocProc,0)){ 
				return -1;
			}
		} else {
			if (!EnumSystemLocales(enumCountLocalesProc, locales_set_to_enum_flag(g_pLocalesCache->locales_set))) {
				return -1;
			}
		}
	}
	return g_pLocalesCache->locales_count+1; // add neutral locale
}

// search for locale by index
static int update_cache(index){
	g_index_to_search = index;
	g_search_counter = -1;
	g_index_found = -1;

	if (g_pLocalesCache->locales_set==RESOURCE_LOCALES){
		if (!hResModule) return -1;
		enumResLocByIndexProc(0,0,0,0,0); //add neutral locale
		if (!EnumResourceLanguages(hResModule,RT_STRING,MAKEINTRESOURCE(RESOURCE_STRING_TABLE_ID),enumResLocByIndexProc,0)){
			return -1;
		}
	} else {
		enumSearchLocalesByIndexProc(_T("0")); //add neutral locale
		if (!EnumSystemLocales(enumSearchLocalesByIndexProc, locales_set_to_enum_flag(g_pLocalesCache->locales_set))){ 
				return -1;
		}
	}
	return g_index_found;
}

// search for locale by name
static int update_cache_by_name(const wchar_t* pcsz_locale_name){
	g_name_to_search = pcsz_locale_name;
	g_search_counter = -1;
	g_index_found = -1;
	if (g_pLocalesCache->locales_set==RESOURCE_LOCALES){
		if (!hResModule) return -1;
		enumResLocByIndexProc(0,0,0,0,0); //add neutral locale
		if(!EnumResourceLanguages(hResModule,RT_STRING,MAKEINTRESOURCE(RESOURCE_STRING_TABLE_ID),enumResLocByNameProc,0)){ 
			return -1;
		}
	} else {
		enumSearchLocalesByNameProc(_T("0"));//add neutral locale
		if (!EnumSystemLocales(enumSearchLocalesByNameProc, locales_set_to_enum_flag(g_pLocalesCache->locales_set))){
			return -1;
		}
	}
	return g_index_found;
}

static Locale* getLocale(int index){
	int i,cache_index;
	//search index in cache
	for (i=0; i<LOCALES_CACHE_SIZE; ++i){
		if (g_pLocalesCache->cache[i].index == index){
			return &g_pLocalesCache->cache[i]; 
		}
	}

	// not in cache yet
	cache_index = update_cache(index);
		
	// no such index
	if (cache_index==-1) {
		return NULL; 
	}

	return &g_pLocalesCache->cache[cache_index]; 
}

static Locale* getLocaleByName(const wchar_t* name){
	int i,cache_index;
	//search index in cache
	for (i=0; i<LOCALES_CACHE_SIZE; ++i){
		if (g_pLocalesCache->cache[i].index>=0){
			const wchar_t* localename = get_locale_name(g_pLocalesCache->cache[i].lcid);
			if (localename && !wcscmp(name,localename)){
				return &g_pLocalesCache->cache[i]; 
			}
		}
	}

	// not in cache yet
	cache_index = update_cache_by_name(name);
		
	// no such index
	if (cache_index==-1) {
		return NULL; 
	}

	return &g_pLocalesCache->cache[cache_index]; 
}

// extract script name from sublanguage name
// script name included in parentheses like: Uzbek (Latin) - > Latin
static int extractscript(wchar_t* from, wchar_t* to){
	int res = 1;
	while (*from){
		if (*from++=='('){
			while (*from && *from!=')'){
				*to++ = *from++;
				++res;
			}
			break;
		}
	}
	*to=0;
	return res;
}

wchar_t* get_locale_name(LCID lcid) {
	static wchar_t buf[MAX_LOCALE_NAME];
	int len;
	int res;
	int needabbrev = 0;

	if (lcid == LOCALE_NEUTRAL){
		*buf=0;
		return buf;
	}

	//get iso 639 language name
	res = GetLocaleInfoW(lcid,  LOCALE_SISO639LANGNAME, buf, MAX_LOCALE_NAME);
	if (res<3) {
		return 0;//error
	}
	len = res;

	//get iso 3166 country name
	res = GetLocaleInfoW(lcid,  LOCALE_SISO3166CTRYNAME, &buf[len], MAX_LOCALE_NAME-len);
	if (!res) return 0;//error
	if (res==1) return buf;//no country
	buf[len-1]='-';
	len += res;

	//check on uniqueness of language_name-COUNTRY_NAME (actual for Spanish (Castilian) and Spanish (Spain))
	{
		WORD lang = PRIMARYLANGID(LANGIDFROMLCID(lcid));
		WORD sublang = SUBLANGID(LANGIDFROMLCID(lcid));
		wchar_t CNTRY[3];
		int i;
		for (i=1;i<sublang;++i){
			res = GetLocaleInfoW(MAKELCID(MAKELANGID(lang,i),0),  LOCALE_SISO3166CTRYNAME, CNTRY, 3);
			if (!res) return 0;//error
			if (CNTRY[0]==buf[3] && CNTRY[1]==buf[4]){
				needabbrev = 1;
				break;
			}
		}
	}

		//get full sublanguage name and extract script name from it included in parentheses
		res = GetLocaleInfoW(lcid,  LOCALE_SENGLANGUAGE  , &buf[len], MAX_LOCALE_NAME-len);
		if (!res) return 0;
		if (res>1){
			res = extractscript(&buf[len],&buf[len]);
			if (res>1){
				buf[len-1]='-';
				len +=res;
				needabbrev = 0;
			} 
		}
		if (needabbrev){
			//get sublanguage abbreviation if locale is not still unique
			res = GetLocaleInfoW(lcid,  LOCALE_SABBREVLANGNAME , &buf[len], MAX_LOCALE_NAME-len);
			if (!res) return 0;
			if (res>1){
				buf[len-1]='-';
				len +=res;
			}
		}


#if(WINVER >= 0x0500)
	//sort order name (preceeded by @ sign)
	if (SORTIDFROMLCID(lcid)) {
		res = GetLocaleInfoW(lcid,  LOCALE_SSORTNAME, &buf[len+1], MAX_LOCALE_NAME-(len+1));
		if (!res) return 0;
		if (res>1){
			buf[len-1]='-';
			buf[len]='@';
			len +=res;
		}
	}
#endif
	
	return buf;
}


javacall_result mi18n_get_locale_name(int index, DWORD dwLocalesSet, wchar_t* pBuff, int* bufLen) {
	Locale* pLoc;
	wchar_t* localename;
	int len;

	if (index<0 || !bufLen || *bufLen<0 || (*bufLen && !pBuff)) {
		return JAVACALL_INVALID_ARGUMENT;
	}
	if (index==0){
		if (*bufLen) *pBuff=0;
		*bufLen = 1;
		return JAVACALL_OK;
	}
	
	select_locales_cache(dwLocalesSet);
	pLoc = getLocale(index);
	if (!pLoc) {
		if (*bufLen) *pBuff=0;
		*bufLen=0;
		return JAVACALL_FAIL;
	}

	localename = get_locale_name(pLoc->lcid);
	if (!localename) {
		if (*bufLen) *pBuff=0;
		*bufLen=0;
		return JAVACALL_FAIL;
	}

	len = wcslen(localename) + 1;

	if (*bufLen){
		if (*bufLen<len) {
			*pBuff=0;
			*bufLen=0;
			return JAVACALL_FAIL;
		}
		wcscpy(pBuff,localename);
	}

	*bufLen = len;
	return JAVACALL_OK;
}

LCID mi18n_get_locale_id(int index, DWORD dwLocalesSet){
	Locale* pLoc;
	if (index<0) return ERROR_LOCALE;
	if (index==0) return LOCALE_NEUTRAL;
	select_locales_cache(dwLocalesSet);
	pLoc = getLocale(index);
	if (!pLoc) {
		return ERROR_LOCALE;
	}
	return pLoc->lcid;
}

// returns index of locale or -1 if not found
// for neutral locale returns 0
int mi18n_get_locale_index(const wchar_t* pcsz_locale_name, DWORD dwLocalesSet){
	Locale* pLoc;
	if (!pcsz_locale_name || !*pcsz_locale_name) return 0;
	select_locales_cache(dwLocalesSet);
	pLoc = getLocaleByName(pcsz_locale_name);
	if (!pLoc) return -1;
	return pLoc->index;
}

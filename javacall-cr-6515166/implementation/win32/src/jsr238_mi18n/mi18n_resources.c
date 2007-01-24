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

#include "javacall_defs.h" 
#include "javacall_mi18n_resources.h" 
#include "mi18n_locales.h" 

static const wchar_t* resource_module_name=L"user32.dll";
#define RESOURCE_STRING_TABLE_ID 51
#define RESOURCE_STRING_ID_MIN 800
#define RESOURCE_STRING_ID_MAX 810
#define RESOURCE_CACHE_MAX 6

HMODULE hResModule = NULL;

typedef struct tagSystemResources{
	LCID    lcid;
	WCHAR* stringtable;
}ResourceStringTable;

int g_resource_cache_insert_index=-1;
int g_resource_cache_size=0;
ResourceStringTable resource_cache[RESOURCE_CACHE_MAX];


static BOOL load_module(){
	if (!hResModule) hResModule = LoadLibraryEx(TEXT("user32.dll"),NULL,LOAD_LIBRARY_AS_DATAFILE);
	return (hResModule!=NULL);
}


static int get_utf8_len_from_utf16(WCHAR* src, int src_len){
	int i;int len=0;
	for (i=0; i<src_len; ++i){
			WCHAR ch=src[i];
			// surrogates				
			if (ch>=0xD800 && ch<=0xDFFF) {
				if (ch>=0xDC00) return FALSE;
				if (i>src_len-2 || src[i+1]<0xDC00 || src[i+1]>0xDFFF) return -1;
				ch=((ch-0xD800)<<10)+0x10000 + (src[i+1] & 0x3FF);
				{
					len+=4;
				}
				i++;
				continue;
			}
			if (ch <= 0x7F) { 
				len++;
			} 
			else
				if (ch <= 0x7FF) { 
						len+=2;
					}
					else
					{
						len+=3;
					}
	}
	return len;
}
// fill buffer from utf16 string by utf8 string starting from offset utf8 byte on dst_len bytes
// return number of bytes copied
static int utf16_to_utf8(WCHAR* src, int src_len, char* dst, int offset, int dst_len){
	int i;
	int len=-offset-1;
	for (i=0; i<src_len; ++i){
			WCHAR ch=src[i];
			// surrogates				
			if (ch>=0xD800 && ch<=0xDFFF) {
				if (ch>=0xDC00) return FALSE;
				if (i>src_len-2 || src[i+1]<0xDC00 || src[i+1]>0xDFFF) return -1;
				ch=((ch-0xD800)<<10)+0x10000 + (src[i+1] & 0x3FF);
				{
					if (++len>=dst_len) return len; if (len>=0) dst[len]=(0xF0 + (ch>>18)); 
					if (++len>=dst_len) return len-1; if (len>=0) dst[len]=(0x80 + ((ch>>12)&0x37));
					if (++len>=dst_len) return len-2; if (len>=0) dst[len]=(0x80 + ((ch>>6)&0x37)); 
					if (++len>=dst_len) return len-3; if (len>=0) dst[len]=(0x80 + (ch&0x37));
				}
				i++;
				continue;
			}
			if (ch <= 0x7F) { 
				if (++len>=dst_len) return len; if (len>=0) dst[len]=(char)(ch);
			} 
			else
				if (ch <= 0x7FF) { 
						if (++len>=dst_len) return len; if (len>=0) dst[len]=(0xC0 + (ch>>6)); 
						if (++len>=dst_len) return len-1; if (len>=0) dst[len]=(0x80 + (ch&37)); 
					}
					else
					{
						if (++len>=dst_len) return len; if (len>=0) dst[len]=(0xE0 + (ch>>12));
						if (++len>=dst_len) return len-1; if (len>=0) dst[len]=(0x80 + ((ch>>6)&0x37));
						if (++len>=dst_len) return len-2; if (len>=0) dst[len]=(0x80 + (ch&0x37));
					}
	}
	return ++len;
}

static int get_resource_length(ResourceStringTable* rst, int id){
	int i;
	WCHAR* stringtable = rst->stringtable;
	unsigned short res_length;
	if (!stringtable) return -1;
	for (i=RESOURCE_STRING_ID_MIN; i<=id; ++i){
		res_length = *((unsigned short*)stringtable++);
		if (i==id) return get_utf8_len_from_utf16(stringtable,res_length);
		stringtable+=res_length;
	}
	return -1;
}

// copy resource data from offset on dst_len bytes
// return number of bytes copied
static int copy_resource(ResourceStringTable* rst,int id, char* buf, int offset, int length){
	int i;
	WCHAR* stringtable = rst->stringtable;
	unsigned short res_length;
	if (!stringtable) return FALSE;
	for (i=RESOURCE_STRING_ID_MIN; i<=id; ++i){
		res_length = *((unsigned short*)stringtable++);
		if (i==id) {
			return utf16_to_utf8(stringtable, res_length, buf, offset, length );
		}
		stringtable+=res_length;
	}
	return FALSE;
}

static ResourceStringTable* get_ResourceStringTable(LCID lcid){
	int i;
	HGLOBAL hg;
	HRSRC hrsrc;
	ResourceStringTable* rst;

	for (i= 0; i < g_resource_cache_size; ++i){
		if (resource_cache[i].lcid == lcid) 
			return &resource_cache[i];
	}
	hrsrc=FindResourceEx(hResModule,RT_STRING,MAKEINTRESOURCE(RESOURCE_STRING_TABLE_ID),LANGIDFROMLCID(lcid));
	if (!hrsrc) return NULL;
	hg = LoadResource(hResModule,hrsrc);
	if (!hg) return NULL;

	if (++g_resource_cache_insert_index == RESOURCE_CACHE_MAX) {
		g_resource_cache_insert_index = 0;
	}

	if (g_resource_cache_insert_index >= g_resource_cache_size) {
		g_resource_cache_size = g_resource_cache_insert_index + 1;
	}

	rst = &resource_cache[g_resource_cache_insert_index];
	rst->lcid = lcid;	
	rst->stringtable = LockResource(hg);

	return rst;
}

/*********************************************** API Implementation ****************************************************************/


/**
 * Gets the number of supported locales for string collation.
 * @param count_out pointer to integer that recieves
 *					the number of supported locales
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 */
javacall_result javacall_mi18n_get_resource_locales_count(/*OUT*/int* count_out){
	if (!count_out) return JAVACALL_INVALID_ARGUMENT;
	if (!load_module()) return JAVACALL_FAIL;
	*count_out = i18n_get_supported_locales_count(RESOURCE_LOCALES);
	if (!*count_out) return JAVACALL_FAIL;
    return JAVACALL_OK;
}

/**
 * Gets a locale name for device resources for the index.
 *
 * @param locale_index  index of the locale.
 * @param locale_name_out  buffer for the locale.
 * @param plen	pointer to integer initially containing the buffer length
 *				and receiving length of result string in wchars including terminating zero, 
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
*/
javacall_result javacall_mi18n_get_resource_locale_name(int locale_index, javacall_utf16* locale_name_out,
							/*IN|OUT*/int* plen)
{
    return mi18n_get_locale_name(locale_index, RESOURCE_LOCALES, locale_name_out, plen);
}

/**
 * Gets locale index used for accessing device resources by the given miroedition.locale name.
 *
 * @param locale    utf16 string containing requested locale name or null for neutral locale
 * @param index_out	pointer to integer receiving index of requested locale,
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 * @note If neutral (empty string) locale is supported it must have index 0
 */
javacall_result javacall_mi18n_get_resource_locale_index(const javacall_utf16* locale, /*OUT*/int* index_out)
{
	if (!index_out) return JAVACALL_INVALID_ARGUMENT;
	if (!load_module()) return JAVACALL_FAIL;
	*index_out = mi18n_get_locale_index(locale,RESOURCE_LOCALES);
	if (*index_out<0) return JAVACALL_FAIL;
	return JAVACALL_OK;
}

/**
 * Gets a resource data for pointed reource identifier and locale.
 * String resources must be UTF8 encoded
 *
 * @param locale_index  index of the locale.
 * @param resource_id   resource identifier.
 * @param resource      buffer for the resource.
 * @param offset	offset of first byte of resource data 
 * @param length	pointer to integer that set to desired number of bytes to copy from resource data
 *					receiving length of copied resource data in bytes, 
 *					if length is less than remaining resource size only length bytes are copied		
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 */
javacall_result javacall_mi18n_get_resource(int locale_index, int resource_id,
											/* OUT */char* resource, int offset, /*IN|OUT*/ int* length){
	LCID lcid;
	ResourceStringTable* rst;
	int res_len;

    if (!length  || *length<0 || offset<0) return JAVACALL_FAIL;

	if (resource_id<RESOURCE_STRING_ID_MIN || resource_id>RESOURCE_STRING_ID_MAX) return JAVACALL_INVALID_ARGUMENT;
	
	lcid = mi18n_get_locale_id(locale_index, RESOURCE_LOCALES);
	if (lcid < 0) lcid = LOCALE_NEUTRAL;

	rst = get_ResourceStringTable(lcid);
	if (!rst) return JAVACALL_FAIL;

	res_len = get_resource_length(rst, resource_id);
	if (res_len<0) return JAVACALL_FAIL;
	if ( offset >= res_len){
		*length = 0;
		return JAVACALL_OK;
	}

	res_len -= offset;

	if (*length < res_len) res_len = *length;

	res_len = copy_resource(rst,resource_id, resource, offset, res_len);
	if (res_len > 0){
		*length = res_len;
		return JAVACALL_OK;
	}

    return JAVACALL_FAIL;
}

/**
 * Gets a resource type for pointed resource identifier and locale.
 *
 * @param locale_index  index of the locale.
 * @param resource_id   resource identifier.
 * @param resType  (out) resource type.
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 */
javacall_result javacall_mi18n_get_resource_type(int locale_index,int resource_id,
						 /* OUT */int* resType ) {
	if (!resType || resource_id<RESOURCE_STRING_ID_MIN || resource_id>RESOURCE_STRING_ID_MAX) return JAVACALL_INVALID_ARGUMENT;
	*resType=STRING_RESOURCE_TYPE;
    return JAVACALL_OK;
}

/**
 * Checks if resource with given identifier exists.
 *
 * @param locale_index  index of the locale.
 * @param resource_id   resource identifier.
 * @param result	poiner to boolean that is set to
					JAVACALL_TRUE if resource ID is valid, 
					JAVACALL_FALSE if resource ID is not valid
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 */
javacall_result javacall_mi18n_is_valid_resource_id(int locale_index,int resource_id,javacall_bool* result){
	if (!result) return JAVACALL_INVALID_ARGUMENT;

	if (resource_id<RESOURCE_STRING_ID_MIN || resource_id>RESOURCE_STRING_ID_MAX)
		*result = JAVACALL_FALSE;
	else
		*result = JAVACALL_TRUE;

	return JAVACALL_OK;
}

/**
 * Gets a resource length for pointed reource identifier and locale.
 *
 * @param locale_index  index of the locale.
 * @param resource_id   resource identifier.
 * @param length  (out) size of the resource (in bytes).
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 */
javacall_result javacall_mi18n_get_resource_length(int locale_index, int resource_id,
												   /* OUT */int* length) {
	int res;
	LCID lcid;
	ResourceStringTable* rst;

    if (!length || resource_id<RESOURCE_STRING_ID_MIN || resource_id>RESOURCE_STRING_ID_MAX) return JAVACALL_INVALID_ARGUMENT;
	
	lcid = mi18n_get_locale_id(locale_index, RESOURCE_LOCALES);
	if (lcid == -1L) lcid = LOCALE_NEUTRAL;

	rst = get_ResourceStringTable(lcid);
	if (!rst) return JAVACALL_FAIL;
	res = get_resource_length(rst,resource_id);
	if (res>=0){
		*length = res;
		return JAVACALL_OK;
	} else {
		*length = 0;
		return JAVACALL_FAIL;
	}
}



#ifdef __cplusplus
}
#endif

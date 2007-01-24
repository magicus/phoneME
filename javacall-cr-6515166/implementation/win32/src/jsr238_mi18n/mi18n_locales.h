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


#ifndef __MI18NLOCALESH_H_
#define __MI18NLOCALESH_H_

#ifdef __cplusplus
extern "C" {
#endif

#define INCLUDE_SORTING_VARIANTS 1
#define INCLUDE_CURENCY_VARIANTS 2
#define RESOURCE_LOCALES 3

#define ERROR_LOCALE ((LCID)-1L)

#include "javacall_defs.h" 

// retruns count of all possible locales supported by platform
// dwLocalesSet bit mask
int i18n_get_supported_locales_count(DWORD dwLocalesSet);

// platform specific locale operation part
LCID mi18n_get_locale_id(int index,DWORD dwLocalesSet);

// get locale name by stored locale index
// returns size of copied string or -1 if error (buff too small)
// if pBuff is null returns needed size
javacall_result mi18n_get_locale_name(int index, DWORD dwLocalesSet, wchar_t* pBuff, int* bufLen);

// get locale name by lcid
// uses temporary buffer that altered each time function called
wchar_t* get_locale_name(LCID lcid);

// returns index of locale or -1 if not found
int mi18n_get_locale_index(const wchar_t* pcsz_locale_name,DWORD dwLocalesSet);


#ifdef __cplusplus
}
#endif

#endif
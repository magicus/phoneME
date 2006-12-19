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
#include "javacall_mi18n_collation.h"
#include "windows.h"
#include "stdlib.h"

// en-US locale for TCK
static const char *enUS = "en-US";
// fi-FI locale for TCK
static const char *fiFI = "fi-FI";

// prefix for hex value
static char hexValue[11] = {'0','x'};
// locale name
static char localeName[6] = {0,0,'-',0,0,0};
// the number of locales supported by platform
static int locCount;
// locale to be searched 
static int localeToSearch;
// locale ID
static LCID id;

static void get_locale_name(LCID lcid) {
    int res = GetLocaleInfo(lcid,  LOCALE_SISO639LANGNAME, localeName, 2);
    res = GetLocaleInfo(lcid,  LOCALE_SISO3166CTRYNAME, localeName+3, 2);
}

/*
 * The enumLocalesProc function is an application-defined function
 * used with the <code>EnumSystemLocales</code> function. 
 * It receives a pointer to a buffer containing a locale identifier.
 */
static BOOL CALLBACK enumLocalesProc(LPTSTR lpLocaleString) {
    memcpy(hexValue+2, lpLocaleString, strlen(lpLocaleString)+1);
    id = strtoul(hexValue, NULL, 16);
    get_locale_name(id);
#if 1
    // only for TCK, we are going to  limit a number of supported locales
    // in order to reduce the number of interactive tests
    if (0 == memcmp(localeName, enUS, sizeof(enUS) - 1) || 
        0 == memcmp(localeName, fiFI, sizeof(fiFI) - 1)) {
        if (localeToSearch != 0)  locCount++;
        return FALSE;
    }
    return TRUE;
#else
    if (locCount == localeToSearch) {
        return FALSE;
    }
    locCount++;
    return TRUE;
#endif
}

int mi18n_enum_locales() {
    locCount = 0;
    localeToSearch = -1;
    // NOTE: will block until end of enumeration
    EnumSystemLocales(enumLocalesProc, LCID_INSTALLED);
    return locCount;
}

LCID mi18n_get_locale_id(int index) {
    if (index != locCount) {
        localeToSearch = index;
        locCount = 0;
        EnumSystemLocales(enumLocalesProc, LCID_INSTALLED);
        return id;
    } else {
        return id;
    }
}

javacall_result mi18n_get_locale_name(char* pBuff, int bufLen, int index) {
    if (index != locCount) {
        localeToSearch = index;
        locCount = 0;
        EnumSystemLocales(enumLocalesProc, LCID_INSTALLED);
    }
    if (index == locCount && bufLen >= 6) {
        memcpy(pBuff, localeName, 6);
        return JAVACALL_OK;
    }
    return JAVACALL_FAIL;
}

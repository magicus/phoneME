/*
 * @(#)locale_str.h	1.5 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.  
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
 *
 */

/*
 * Windows Langage ID to ISO639 and ISO3166 string mapping table
 */
typedef struct {
    WORD langID;
    char* ISO639LangName;
    char* ISO3166RegionName;
    char* tzName;
} LangIDToLocaleStr;

static LangIDToLocaleStr langIDToLocaleStrTable [] = {
    { MAKELANGID(LANG_BULGARIAN, SUBLANG_DEFAULT),             "bg", "BG", "EET" },
    { MAKELANGID(LANG_CHINESE,   SUBLANG_CHINESE_SIMPLIFIED),  "zh", "CN", "CTT" },
    { MAKELANGID(LANG_CHINESE,   SUBLANG_CHINESE_TRADITIONAL), "zh", "TW", "CTT" },
    { MAKELANGID(LANG_CHINESE,   SUBLANG_CHINESE_HONGKONG),    "zh", "HK", "CTT" },
    { MAKELANGID(LANG_CHINESE,   SUBLANG_CHINESE_SINGAPORE),   "zh", "SG", "CTT" },
    { MAKELANGID(LANG_CROATIAN,  SUBLANG_DEFAULT),             "hr", "HR", "ECT" },
    { MAKELANGID(LANG_CZECH,     SUBLANG_DEFAULT),             "cs", "CZ", "ECT" },
    { MAKELANGID(LANG_DANISH,    SUBLANG_DEFAULT),             "da", "DK", "ECT" },
    { MAKELANGID(LANG_DUTCH,     SUBLANG_DUTCH),               "nl", "NL", "ECT" },
    { MAKELANGID(LANG_DUTCH,     SUBLANG_DUTCH_BELGIAN),       "nl", "BE", "ECT" },
    { MAKELANGID(LANG_ENGLISH,   SUBLANG_DEFAULT),             "en", "US", "PST" },
    { MAKELANGID(LANG_ENGLISH,   SUBLANG_ENGLISH_UK),          "en", "GB", "GMT" },
    { MAKELANGID(LANG_ENGLISH,   SUBLANG_ENGLISH_AUS),         "en", "AU", "AET" },
    { MAKELANGID(LANG_ENGLISH,   SUBLANG_ENGLISH_CAN),         "en", "CA", "PST" },
    { MAKELANGID(LANG_ENGLISH,   SUBLANG_ENGLISH_NZ),          "en", "NZ", "NST" },
    { MAKELANGID(LANG_ENGLISH,   SUBLANG_ENGLISH_EIRE),        "en", "IE", "GMT" },
    { MAKELANGID(LANG_FINNISH,   SUBLANG_DEFAULT),             "fi", "FI", "EET" },
    { MAKELANGID(LANG_FRENCH,    SUBLANG_FRENCH),              "fr", "FR", "ECT" },
    { MAKELANGID(LANG_FRENCH,    SUBLANG_FRENCH_BELGIAN),      "fr", "BE", "ECT" },
    { MAKELANGID(LANG_FRENCH,    SUBLANG_FRENCH_CANADIAN),     "fr", "CA", "EST" },
    { MAKELANGID(LANG_FRENCH,    SUBLANG_FRENCH_SWISS),        "fr", "CH", "ECT" },
    { MAKELANGID(LANG_GERMAN,    SUBLANG_GERMAN),              "de", "DE", "ECT" },
    { MAKELANGID(LANG_GERMAN,    SUBLANG_GERMAN_SWISS),        "de", "CH", "ECT" },
    { MAKELANGID(LANG_GERMAN,    SUBLANG_GERMAN_AUSTRIAN),     "de", "AT", "ECT" },
    { MAKELANGID(LANG_GREEK,     SUBLANG_DEFAULT),             "el", "GR", "EET" },
    { MAKELANGID(LANG_HUNGARIAN, SUBLANG_DEFAULT),             "hu", "HU", "ECT" },
    { MAKELANGID(LANG_ICELANDIC, SUBLANG_DEFAULT),             "is", "IS", "GMT" },
    { MAKELANGID(LANG_ITALIAN,   SUBLANG_ITALIAN),             "it", "IT", "ECT" },
    { MAKELANGID(LANG_ITALIAN,   SUBLANG_ITALIAN_SWISS),       "it", "CH", "ECT" },
    { MAKELANGID(LANG_JAPANESE,  SUBLANG_DEFAULT),             "ja", "JP", "JST" },
    { MAKELANGID(LANG_KOREAN,    SUBLANG_DEFAULT),             "ko", "KO", "JST" },
    { MAKELANGID(LANG_NORWEGIAN, SUBLANG_NORWEGIAN_BOKMAL),    "no", "NO", "ECT" },
    { MAKELANGID(LANG_NORWEGIAN, SUBLANG_NORWEGIAN_NYNORSK),   "no", "NO", "ECT" },
    { MAKELANGID(LANG_POLISH,    SUBLANG_DEFAULT),             "pl", "PL", "ECT" },
    { MAKELANGID(LANG_PORTUGUESE,SUBLANG_PORTUGUESE),          "pt", "PT", "ECT" },
    { MAKELANGID(LANG_PORTUGUESE,SUBLANG_PORTUGUESE_BRAZILIAN),"pt", "BR", "IET" },
    { MAKELANGID(LANG_ROMANIAN,  SUBLANG_DEFAULT),             "ro", "RO", "ECT" },
    { MAKELANGID(LANG_RUSSIAN,   SUBLANG_DEFAULT),             "ru", "RU", "EET" },
    { MAKELANGID(LANG_SLOVAK,    SUBLANG_DEFAULT),             "sk", "SK", "ECT" },
    { MAKELANGID(LANG_SLOVENIAN, SUBLANG_DEFAULT),             "sl", "SI", "ECT" },
    { MAKELANGID(LANG_SPANISH,   SUBLANG_SPANISH),             "es", "ES", "ECT" },
    { MAKELANGID(LANG_SPANISH,   SUBLANG_SPANISH_MEXICAN),     "es", "MX", "PST" },
    { MAKELANGID(LANG_SPANISH,   SUBLANG_SPANISH_MODERN),      "es", "ES", "ECT" },
    { MAKELANGID(LANG_SWEDISH,   SUBLANG_DEFAULT),             "sv", "SE", "ECT" },
    { MAKELANGID(LANG_TURKISH,   SUBLANG_DEFAULT),             "tr", "TR", "EET" },
    { 0 , NULL, NULL, NULL }
};

/*
 * ISO639 locale string to Encoding converter 
 * default file.encoding is "8859_1".
 */
typedef struct {
    char* ISO639LangName;
    char* ISO3166RegionName;
    char* encodingName;
} LocaleStrToEncodingName;

static LocaleStrToEncodingName localeStrToEncodingNameTable[] = {
    { "ar", NULL, "Cp1256" },
    { "be", NULL, "Cp1251" },
    { "bg", NULL, "Cp1251" },
    { "cs", NULL, "Cp1250" },
    { "el", NULL, "Cp1253" },
    { "et", NULL, "Cp1257" },
    { "iw", NULL, "Cp1255" },
    { "hu", NULL, "Cp1250" },
    { "ja", NULL, "SJIS"   },
    { "ko", NULL, "EUC_KR" },
    { "lt", NULL, "Cp1257" },
    { "lv", NULL, "Cp1257" },
    { "mk", NULL, "Cp1251" },
    { "pl", NULL, "Cp1250" },
    { "ro", NULL, "Cp1250" },
    { "ru", NULL, "Cp1251" },
    { "sh", NULL, "Cp1250" },
    { "sk", NULL, "Cp1250" },
    { "sl", NULL, "Cp1250" },
    { "sq", NULL, "Cp1250" },
    { "sr", NULL, "Cp1251" },
    { "th", NULL, "MS874"  },
    { "tr", NULL, "Cp1254" },
    { "uk", NULL, "Cp1251" },
    { "zh", NULL, "GBK"    },
    { "zh", "TW", "MS950"  },
    { NULL, NULL, NULL }
};

/*
 * Windows long TimeZone string to
 * user.timezone representation table.
 * This is a temporary fix for JDK 1.1.6; for 1.2 we will go to the full names, shown
 * below in comments. [LIU]
 */
static char *timezone_names[] = {
    /* Win32 zone name                 Java zone name                     Win32 offset */
    /* ---------------                 --------------                     ------------ */
    "Dateline Standard Time",          "Pacific/Auckland",                /* GMT-12:00 */
    "Samoa Standard Time",             "MIT" /*"Pacific/Apia"*/,          /* GMT-11:00 */
    "Hawaiian Standard Time",          "HST" /*"Pacific/Honolulu"*/,      /* GMT-10:00 */
    "Alaskan Standard Time",           "AST" /*"America/Anchorage"*/,     /* GMT-09:00 */
    "Pacific Standard Time",           "PST" /*"America/Los_Angeles"*/,   /* GMT-08:00 */
    "Mountain Standard Time",          "America/Denver",                  /* GMT-07:00 */
    "US Mountain Standard Time",       "MST" /*"America/Phoenix"*/,       /* GMT-07:00 */
    "Canada Central Standard Time",    "America/Costa_Rica",              /* GMT-06:00 */
    "Mexico Standard Time",            "America/Costa_Rica",              /* GMT-06:00 */
    "Central Standard Time",           "CST" /*"America/Chicago"*/,       /* GMT-06:00 */
    "Eastern Standard Time",           "EST" /*"America/New_York"*/,      /* GMT-05:00 */
    "SA Pacific Standard Time",        "America/Indianapolis",            /* GMT-05:00 */
    "US Eastern Standard Time",        "America/Indianapolis",            /* GMT-05:00 */
    "SA Western Standard Time",        "America/Caracas",                 /* GMT-04:00 */
    "Atlantic Standard Time",          "PRT" /*"America/Halifax"*/,       /* GMT-04:00 */
    "Newfoundland Standard Time",      "CNT" /*"America/St_Johns"*/,      /* GMT-03:30 */
    "E. South America Standard Time",  "BET" /*"America/Sao_Paulo"*/,     /* GMT-03:00 */
    "SA Eastern Standard Time",        "AGT" /*"America/Buenos_Aires"*/,  /* GMT-03:00 */
    "Mid-Atlantic Standard Time",      "Atlantic/South_Georgia",          /* GMT-02:00 */
    "Azores Standard Time",            "CAT" /*"Atlantic/Azores"*/,       /* GMT-01:00 */
    "GMT",                             "Europe/London",                   /* GMT       */
    "GMT Standard Time",               "GMT" /*"Africa/Casablanca"*/,     /* GMT       */
    "Central Europe Standard Time",    "Europe/Paris",                    /* GMT+01:00 */
    "Romance Standard Time",           "ECT" /*"Europe/Paris"*/,          /* GMT+01:00 */
    "W. Europe Standard Time",         "Europe/Paris",                    /* GMT+01:00 */
    "Israel Standard Time",            "Asia/Jerusalem",                  /* GMT+02:00 */
    "South Africa Standard Time",      "Africa/Johannesburg",             /* GMT+02:00 */
    "GFT Standard Time",               "Europe/Istanbul",                 /* GMT+02:00 */
    "Egypt Standard Time",             "EET" /*"Africa/Cairo"*/,          /* GMT+02:00 */
    "E. Europe Standard Time",         "Asia/Beirut",                     /* GMT+02:00 */
    "Saudi Arabia Standard Time",      "EAT" /*"Asia/Riyadh"*/,           /* GMT+03:00 */
    "Russian Standard Time",           "Europe/Moscow",                   /* GMT+03:00 */
    "Iran Standard Time",              "MET" /*"Asia/Tehran"*/,           /* GMT+03:30 */
    "Arabian Standard Time",           "NET" /*"Asia/Yerevan"*/,          /* GMT+04:00 */
    "Afghanistan Standard Time",       "Asia/Kabul",                      /* GMT+04:30 */
    "West Asia Standard Time",         "PLT" /*"Asia/Karachi"*/,          /* GMT+05:00 */
    "India Standard Time",             "IST" /*"Asia/Calcutta"*/,         /* GMT+05:30 */
    "Central Asia Standard Time",      "BST" /*"Asia/Dacca"*/,            /* GMT+06:00 */
    "Bangkok Standard Time",           "VST" /*"Asia/Bangkok"*/,          /* GMT+07:00 */
    "Taipei Standard Time",            "Asia/Shanghai",                   /* GMT+08:00 */
    "China Standard Time",             "CTT" /*"Asia/Shanghai"*/,         /* GMT+08:00 */
    "Tokyo Standard Time",             "JST" /*"Asia/Tokyo"*/,            /* GMT+09:00 */
    "AUS Central Standard Time",       "Australia/Darwin",                /* GMT+09:30 */
    "Cen. Australia Standard Time",    "ACT" /*"Australia/Adelaide"*/,    /* GMT+09:30 */
    "Tasmania Standard Time",          "Australia/Hobart",                /* GMT+10:00 */
    "Sydney Standard Time",            "AET" /*"Australia/Brisbane"*/,    /* GMT+10:00 */
    "West Pacific Standard Time",      "Asia/Vladivostok",                /* GMT+10:00 */
    "Central Pacific Standard Time",   "SST" /*"Pacific/Guadalcanal"*/,   /* GMT+11:00 */
    "Fiji Standard Time",              "Pacific/Fiji",                    /* GMT+12:00 */
    "New Zealand Standard Time",       "NST" /*"Pacific/Auckland"*/,      /* GMT+12:00 */
    "",
};


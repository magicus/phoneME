/*
 * @(#)locale_str.h	1.33 06/10/10
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
 * Mappings from partial locale names to full locale names
 */
static const char * const locale_aliases[] = {
    "ar", "ar_EG",
    "be", "be_BY",
    "bg", "bg_BG",
    "ca", "ca_ES",
    "cs", "cs_CZ",
    "cz", "cs_CZ",
    "da", "da_DK",
    "de", "de_DE",
    "el", "el_GR",
    "en", "en_US",
    "es", "es_ES",
    "et", "et_EE",
    "fi", "fi_FI",
    "fr", "fr_FR",
    "iw", "iw_IL",
    "hr", "hr_HR",
    "hu", "hu_HU",
    "is", "is_IS",
    "it", "it_IT",
    "ja", "ja_JP",
    "ko", "ko_KR",
    "lt", "lt_LT",
    "lv", "lv_LV",
    "mk", "mk_MK",
    "nl", "nl_NL",
    "no", "no_NO",
    "pl", "pl_PL",
    "pt", "pt_PT",
    "ro", "ro_RO",
    "ru", "ru_RU",
    "sh", "sh_YU",
    "sk", "sk_SK",
    "sl", "sl_SI",
    "sq", "sq_AL",
    "sr", "sr_YU",
    "su", "fi_FI",
    "sv", "sv_SE",
    "th", "th_TH",
    "tr", "tr_TR",
    "uk", "uk_UA",
    "zh", "zh_CN",
    "tchinese", "zh_TW",
    "big5", "zh_TW.Big5",
    "japanese", "ja_JP",
    ""
 };

/*
 * Solaris language string to ISO639 string mapping table.
 */
static const char * const language_names[] = {
    "C", "en",
    "POSIX", "en",
    "ar", "ar",
    "be", "be",
    "bg", "bg",
    "ca", "ca",
    "chinese", "zh",
    "cs", "cs",
    "cz", "cs",
    "da", "da",
    "de", "de",
    "el", "el",
    "en", "en",
    "es", "es",
    "et", "et",
    "fi", "fi",
    "su", "fi",
    "fr", "fr",
    "he", "iw",
    "hr", "hr",
    "hu", "hu",
    "is", "is",
    "it", "it",
    "iw", "iw",
    "ja", "ja",
    "japanese", "ja",
    "ko", "ko",
    "korean", "ko",
    "lt", "lt",
    "lv", "lv",
    "mk", "mk",
    "nl", "nl",
    "no", "no",
    "nr", "nr",
    "pl", "pl",
    "pt", "pt",
    "ro", "ro",
    "ru", "ru",
    "sh", "sh",
    "sk", "sk",
    "sl", "sl",
    "sq", "sq",
    "sr", "sr",
    "sv", "sv",
    "th", "th",
    "tr", "tr",
    "uk", "uk",
    "zh", "zh",
    "",
};

/*
 * Solaris country string to ISO3166 string mapping table.
 * Currently only different string is UK/GB.
 */
static const char * const region_names[] = {
    "AT", "AT",
    "AU", "AU",
    "AR", "AR",
    "BE", "BE",
    "BR", "BR",
    "BO", "BO",
    "CA", "CA",
    "CH", "CH",
    "CL", "CL",
    "CN", "CN",
    "CO", "CO",
    "CR", "CR",
    "EC", "EC",
    "GT", "GT",
    "IE", "IE",
    "IL", "IL",
    "JP", "JP",
    "KR", "KR",
    "MX", "MX",
    "NI", "NI",
    "NZ", "NZ",
    "PA", "PA",
    "PE", "PE",
    "PY", "PY",
    "SV", "SV",
    "TH", "TH",
    "UK", "GB",
    "US", "US",
    "UY", "UY",
    "VE", "VE",
    "TW", "TW",
    "",
};

/*
 * Solaris variant string to Java variant name mapping table.
 */
static const char * const variant_names[] = {
    "euro", "EURO",
    "",
};

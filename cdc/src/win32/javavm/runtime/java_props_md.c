/*
 * @(#)java_props_md.c	1.20 06/10/10
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
#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#ifndef WINCE
#include <tchar.h>
#include <locale.h>
#endif
/*
 DMRLNX: force ARCH definition to i386 string.
 Force ARCH to be set to i386 for now
 sys/systeminfo.h does not appear to exist...
*/
#include <stdlib.h>
#include <string.h>
#ifdef WINCE
#include "javavm/include/porting/ansi/stddef.h"
#endif
#include "jni_util.h"
#include "locale_str.h"
#include "javavm/include/porting/java_props.h"
#include "generated/javavm/include/build_defs.h"
#include "javavm/include/wceUtil.h"
#include "javavm/include/winntUtil.h"

/*
#ifdef ALT_CODESET_KEY
#define CODESET ALT_CODESET_KEY
#endif


#ifndef __USE_XOPEN
#define CODESET _NL_CTYPE_CODESET_NAME
#else
#if __GLIBC__ == 2 && __GLIBC_MINOR__ == 0
#warning [jk] glibc 2.0 defines __USE_XOPEN but doesn"'"t have CODESET
#define CODESET _NL_CTYPE_CODESET_NAME
#endif
#endif
*/

/* Take an array of string pairs (map of key->value) and a string (key).
 * Examine each pair in the map to see if the first string (key) matches the
 * string.  If so, store the second string of the pair (value) in the value and
 * return 1.  Otherwise do nothing and return 0.  The end of the map is
 * indicated by an empty string at the start of a pair (key of "").
 */
static int
mapLookup(char* map[], const char* key, char** value) {
    int i;
    for (i = 0; strcmp(map[i], ""); i += 2){
        if (!strcmp(key, map[i])){
            *value = map[i + 1];
            return 1;
        }
    }
    return 0;
}


CVMBool
CVMgetJavaProperties(java_props_t *sprops)
{
    const char *v; /* tmp var */

    if (sprops->user_dir) {
        return CVM_TRUE;
    }

    /* tmp dir */
    {
      DWORD err;
      char consdir[MAX_PATH];        
        TCHAR tmpdir[MAX_PATH];
        if (GetTempPath(MAX_PATH, tmpdir) <= 0) {         
          char msg[1024];
          jio_snprintf(msg, 1024,
                       "GetTempPath: error=%d", GetLastError());
          return CVM_FALSE;
        }
        // convert wide character to sequence of multibyte characters. 
        copyToMCHAR(consdir, tmpdir, MAX_PATH);
        sprops->tmp_dir = strdup(consdir);
    }

    /* Printing properties */
    sprops->printerJob = "sun.awt.motif.PSPrinterJob";

    /* Java 2D properties */
    sprops->graphics_env = NULL;
    sprops->awt_toolkit = NULL;

    v = getenv("JAVA_FONTS");

    /* If JAVA_FONTS is not set, the system-dependent fontpath will
     * be evaluated later
     */
    sprops->font_dir = v ? v : "";


    sprops->cpu_isalist = NULL;

    /* endianness of platform */
    {
        unsigned int endianTest = 0xff000000;
        if (((char*)(&endianTest))[0] != 0)
            sprops->cpu_endian = "big";
        else
            sprops->cpu_endian = "little";
    }

    /* os properties */
    {
        OSVERSIONINFO si;

	si.dwOSVersionInfoSize = sizeof(OSVERSIONINFO); 
	if (GetVersionEx(&si) == 0) {
		printf("GetVersion Error \n");
	}

	switch (si.dwPlatformId) {
	case VER_PLATFORM_WIN32s:
		sprops->os_name = strdup("Windows 3.1");
		sprops->os_version = strdup("Unknown");
		break;
	case VER_PLATFORM_WIN32_WINDOWS:
		if (si.dwMinorVersion == 0)
		    sprops->os_name = strdup("Windows 95");
		else
		    sprops->os_name = strdup("Windows 98");
		sprops->os_version = strdup((char*)si.szCSDVersion);
		break;
	case VER_PLATFORM_WIN32_NT:
          if (si.dwMajorVersion <= 4) {
	        sprops->os_name = strdup("Windows NT");
          } else if (si.dwMajorVersion == 5) {
	        switch (si.dwMinorVersion) {
		case 0:
		    sprops->os_name = strdup("Windows 2000");
		    break;
	        case 1:
		    sprops->os_name = strdup("Windows XP");
		    break;
	        case 2:
		    sprops->os_name = strdup("Windows 2003");
		    break;
	        default:
		    sprops->os_name = strdup("Windows NT (unknown)");
		    break;
                }
          } else {
            sprops->os_name = strdup("Windows NT (unknown)");
          }

          sprops->os_version = malloc(30);
          if (sprops->os_version == NULL) break;
          sprintf((char*)sprops->os_version, "%d.%d", si.dwMajorVersion, si.dwMinorVersion);
          break;

#ifdef WINCE
	case VER_PLATFORM_WIN32_CE: 
		sprops->os_name = strdup("Windows CE");
		sprops->os_version = malloc(32);
		if (sprops->os_name == NULL || sprops->os_version == NULL)
		    break;
		sprintf((char*)sprops->os_version, "%d.%d build %d",
			si.dwMajorVersion, si.dwMinorVersion,
			si.dwBuildNumber);
		break;
#endif
	default:
	        sprops->os_name = strdup("Unknown");
		sprops->os_version = strdup("Unknown");
		break;
	}
	if (sprops->os_name == NULL || sprops->os_version == NULL) {
	    goto out_of_memory;
	}


#ifdef WINCE

	/* Set WINCE processor properties using SYSTEM_INFO */
	{
	  SYSTEM_INFO si;
	  GetSystemInfo (&si);
	  switch (si.wProcessorArchitecture) {
	  case PROCESSOR_ARCHITECTURE_INTEL:
	    sprops->os_arch = strdup("x86");
	    break;
	  case PROCESSOR_ARCHITECTURE_MIPS:
	    sprops->os_arch = strdup("mips");
	    break;
	  case PROCESSOR_ARCHITECTURE_SHX:
	    sprops->os_arch = strdup("shx");
	    break;
	  case PROCESSOR_ARCHITECTURE_ARM:
	    sprops->os_arch = strdup("arm");
	    break;
	  case PROCESSOR_ARCHITECTURE_UNKNOWN:
	  default:
	    sprops->os_arch = strdup("unknown");   
	  }
	}

#else  /* not WINCE */

	/* Set Windows XP/2000 processor properties using SYSTEM_INFO */
	{
	  SYSTEM_INFO si;
	  GetSystemInfo (&si);
	  switch (si.wProcessorArchitecture) {
	  case PROCESSOR_ARCHITECTURE_AMD64:
	    sprops->os_arch = strdup("amd64");
	    break;
	  case PROCESSOR_ARCHITECTURE_IA64:
	    sprops->os_arch = strdup("ia64");
	    break;
	  case PROCESSOR_ARCHITECTURE_INTEL:
	    sprops->os_arch = strdup("x86");
	    break;
	  case PROCESSOR_ARCHITECTURE_UNKNOWN:
	  default:
	    sprops->os_arch = strdup("unknown");   
	  }
	}

#endif /* WINCE */
    }

    /* Determing the language, country, and encoding from the host,
     * and store these in the user.language, user.region, and
     * file.encoding system properties. */
    {
        char *lc = "UTF-8";
#ifndef WINCE
        lc = setlocale(LC_CTYPE, "");
        if (lc == NULL) {
            /*
             * 'lc == null' means system doesn't support user's environment
             * variable's locale.
             */
          setlocale(LC_ALL, "C");
          sprops->language = "en";
          sprops->encoding = "UTF-8";
#else
        if (lc == NULL || !strcmp(lc, "C") || !strcmp(lc, "POSIX")) {
            lc = "en_US";
#endif
        } else {
            /*
             * locale string format in Solaris is
             * <language name>_<region name>.<encoding name>
             * <region name> and <encoding name> are optional.
             */
            char temp[64], *language = NULL, *region = NULL, *encoding = NULL;
            char *std_language = NULL, *std_region = NULL,
                 *std_encoding = NULL;
            char region_variant[64], *variant = NULL, *std_variant = NULL;
            char *p, encoding_variant[64];

            strcpy(temp, lc);
#ifndef WINCE
            setlocale(LC_ALL, lc);
#endif
            /* Parse the language, region, encoding, and variant from the
             * locale.  Any of the elements may be missing, but they must occur
             * in the order language_region.encoding@variant, and must be
             * preceded by their delimiter (except for language).
             *
             * If the locale name (without .encoding@variant, if any) matches
             * any of the names in the locale_aliases list, map it to the
             * corresponding full locale name.  Most of the entries in the
             * locale_aliases list are locales that include a language name but
             * no country name, and this facility is used to map each language
             * to a default country if that's possible.  It's also used to map
             * the Solaris locale aliases to their proper Java locale IDs.
             */
            if ((p = strchr(temp, '.')) != NULL) {
                strcpy(encoding_variant, p); /* Copy the leading '.' */
                *p = '\0';
            } else if ((p = strchr(temp, '@')) != NULL) {
                 strcpy(encoding_variant, p); /* Copy the leading '@' */
                 *p = '\0';
            } else {
                *encoding_variant = '\0';
            }
            
            if (mapLookup(locale_aliases, temp, &p)) {
                strcpy(temp, p);
            }
            
            language = temp;
            if ((region = strchr(temp, '_')) != NULL) {
                *region++ = '\0';
            }
            
            p = encoding_variant;
            if ((encoding = strchr(p, '.')) != NULL) {
                p[encoding++ - p] = '\0';
                p = encoding;
            }
            if ((variant = strchr(p, '@')) != NULL) {
                p[variant++ - p] = '\0';
            }

            /* Normalize the language name */
            std_language = "en";
            if (language != NULL) {
                mapLookup(language_names, language, &std_language);
            }
            sprops->language = std_language;

            /* Normalize the variant name.  Do this BEFORE handling the region
             * name, since the variant name will be incorporated into the
             * user.region property.
             */
            if (variant != NULL) {
                mapLookup(variant_names, variant, &std_variant);
            }

            /* Normalize the region name.  If there is a std_variant, then
             * append it to the region.  This applies even if there is no
             * region, in which case the empty string is used for the region.
             * Note that we only use variants listed in the mapping array;
	     * others are ignored.
             */
            *region_variant = '\0';
            if (region != NULL) {
                std_region = region;
                mapLookup(region_names, region, &std_region);
                strcpy(region_variant, std_region);
            }
            if (std_variant != NULL) {
               strcat(region_variant, "_");
               strcat(region_variant, std_variant);
            }
            if ((*region_variant) != '\0') {
               sprops->region = strdup(region_variant);
	       if (sprops->region == NULL) {
		   goto out_of_memory;
	       }
            }

            /* Normalize the encoding name.  Note that we IGNORE the string
             * 'encoding' extracted from the locale name above.  Instead, we 
             * use the more reliable method of calling nl_langinfo(CODESET).  
             * This function returns an empty string if no encoding is set for
             * the given locale (e.g., the C or POSIX locales); we use the
             * default ISO 8859-1 converter for such locales.  We don't need 
             * to map from the Solaris string to the Java identifier at this 
             * point because that mapping is handled by the character
             * converter alias table in CharacterEncoding.java.
             */
/*
            p = nl_langinfo(CODESET);
	    if (*p == '\0' || strcmp(p, "ANSI_X3.4-1968") == 0) {
		std_encoding = "8859_1";
	    } else {
		std_encoding = p;
	    }
*/
	    std_encoding = "UTF-8";

            sprops->encoding = std_encoding;
        }
    }
    
    sprops->unicode_encoding = "UnicodeLittle";

    /* 
     *  User Properties 
     */
    {  
      DWORD len = 256;
      char *uname, *userprofile;
      int length;   

      /* User Name */
      {
        sprops->user_name = (char *)malloc(len);
        if (sprops->user_name == NULL) {
          goto out_of_memory;
        }
        uname = getenv("USERNAME");
        if (uname != NULL && strlen(uname) > 0) {
          strcpy((char *)sprops->user_name, uname);
        } else {
          GetUserNameA((char*)sprops->user_name, &len);
        }
      }

      /* User Home Directory */
      {
        userprofile = getenv("USERPROFILE");
        if (userprofile != NULL) {
	  length = strlen(userprofile);
          sprops->user_home = (char *)malloc(length + 1);  
          if (sprops->user_home == NULL) {
            goto out_of_memory;
          }
          strcpy((char*)sprops->user_home, userprofile); 
        }          
        else {
          sprops->user_home = strdup("");
        }        
      }
    }
    
    /* User TIMEZONE */
    {
	/*
	 * We defer setting up timezone until it's actually necessary.
	 * Refer to TimeZone.getDefault(). However, the system
	 * property is necessary to be able to be set by the command
	 * line interface -D. Here temporarily set a null string to
	 * timezone.
	 */
#ifndef WINCE
	_tzset(); /* for compatibility */
#endif
	sprops->timezone = "";

    }

    /* Current directory */
#ifndef WINCE
    sprops->user_dir = (char *)malloc(256);
    if (sprops->user_dir == NULL) {
	goto out_of_memory;
    }
    GetCurrentDirectoryA(256, (char*)sprops->user_dir);
#else
    sprops->user_dir ="\\";
#endif

    sprops->file_separator = "\\";
    sprops->path_separator = ";";
    sprops->line_separator = "\n";

    /* Generic Connection Framework (GCF): CommConnection Property */
    /* NOTE: comma-delimited (no spaces) list of available comm (serial)
       ports */
    sprops->commports = "com0";

    return CVM_TRUE;

 out_of_memory:
    CVMreleaseJavaProperties(sprops);
    return CVM_FALSE;
}

/*
 * Free up memory allocated by CVMgetJavaProperties().
 */
void CVMreleaseJavaProperties(java_props_t *sprops)
{
    if (sprops->os_name != NULL) {
	free((char*)sprops->os_name);
    }
    if (sprops->os_version != NULL) {
	free((char*)sprops->os_version);
    }
    if (sprops->region != NULL) {
	free((char*)sprops->region);
    }
    if (sprops->user_name != NULL) {
	free((char*)sprops->user_name);
    }
    if (sprops->user_home != NULL) {
	free((char*)sprops->user_home);
    }
#ifndef WINCE
    if (sprops->user_dir != NULL) {
	free((char*)sprops->user_dir);
    }
#endif
}

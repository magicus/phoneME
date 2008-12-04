/*
 * @(#)java_props_md.c	1.42 06/10/10
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

#include <pwd.h>
#include <locale.h>
#ifndef ARCH
#include <sys/systeminfo.h>	/* For os_arch */
#endif
#include <sys/utsname.h>	/* For os_name and os_version */
#include <langinfo.h>           /* For nl_langinfo */
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/param.h>
#include <time.h>

#include "jni_util.h"

#include "locale_str.h"
#include "javavm/include/porting/java_props.h"
#include "generated/javavm/include/build_defs.h"
#include "javavm/include/porting/endianness.h"

/*
 * altzone is not used
 *
#ifdef HAVE_ALTZONE
#else
static long altzone = 0;
#endif
*/

#ifdef ALT_CODESET_KEY
#define CODESET ALT_CODESET_KEY
#endif

/* Take an array of string pairs (map of key->value) and a string (key).
 * Examine each pair in the map to see if the first string (key) matches the
 * string.  If so, store the second string of the pair (value) in the value and
 * return 1.  Otherwise do nothing and return 0.  The end of the map is
 * indicated by an empty string at the start of a pair (key of "").
 */
static int
mapLookup(const char * const map[], const char* key, char** value) {
    int i;
    for (i = 0; strcmp(map[i], ""); i += 2){
        if (!strcmp(key, map[i])){
            *value = (char*)map[i + 1];
            return 1;
        }
    }
    return 0;
}

#ifndef P_tmpdir
#define P_tmpdir "/var/tmp"
#endif

CVMBool
CVMgetJavaProperties(java_props_t *sprops)
{
    const char *v; /* tmp var */

    if (sprops->user_dir) {
        return CVM_TRUE;
    }

    /* tmp dir */
    sprops->tmp_dir = P_tmpdir;

#ifndef JAVASE
    /* Printing properties */
    sprops->printerJob = NULL;

    /* Java 2D properties */
    sprops->graphics_env = NULL;
#else
    /* Printing properties */
    sprops->printerJob = "sun.print.PSPrinterJob";

    /* Preferences properties */
    sprops->util_prefs_PreferencesFactory =
                                "java.util.prefs.FileSystemPreferencesFactory";

    /* patches/service packs installed */
    sprops->patch_level = "unknown";

    sprops->graphics_env = "sun.awt.X11GraphicsEnvironment";
#endif

    sprops->awt_toolkit = NULL;

    v = getenv("JAVA_FONTS");
    /*
    sprops->font_dir = v ? v :
    "/usr/openwin/lib/X11/fonts/Type1:/usr/openwin/lib/X11/fonts/TrueType";
    */

    /* If JAVA_FONTS is not set, the system-dependent fontpath will
     * be evaluated later
     */
    sprops->font_dir = v ? v : "";

#ifdef SI_ISALIST
    /* supported instruction sets */
    {
        char list[258];
        sysinfo(SI_ISALIST, list, sizeof(list));
        sprops->cpu_isalist = strdup(list);
	if (sprops->cpu_isalist == NULL) {
	    goto out_of_memory;
	}
    }
#else
    sprops->cpu_isalist = NULL;
#endif

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
        struct utsname name;
	uname(&name);
	sprops->os_name = strdup(name.sysname);
	sprops->os_version = strdup(name.release);
	if (sprops->os_name == NULL || sprops->os_version == NULL) {
	    goto out_of_memory;
	}

#ifdef ARCH
        sprops->os_arch = ARCH;
#else
	/* on solaris, uname does not return what we want for the os_arch */
	sysinfo(SI_ARCHITECTURE, name.machine, sizeof(name.machine));
	{
	    char* arch = name.machine;
	    if (arch[0] == 'i' && arch[2] == '8' && arch[3] == '6' &&
		(arch[1] >= '3' && arch[1] <= '6')) {
		/* use x86 to match the value received on win32 */
		sprops->os_arch = strdup("x86");
	    } else if (strncmp(arch, "sparc", 5) == 0) {
		sprops->os_arch = strdup("sparc");
	    } else {
		sprops->os_arch = strdup(arch);
	    }
	    if (sprops->os_arch == NULL) {
		goto out_of_memory;
	    }
	}
#endif
    }

    /* Determing the language, country, and encoding from the host,
     * and store these in the user.language, user.region, and
     * file.encoding system properties. */
    {
        char *lc;
        lc = setlocale(LC_CTYPE, "");
        if (lc == NULL) {
            /*
             * 'lc == null' means system doesn't support user's environment
             * variable's locale.
             */
          setlocale(LC_ALL, "C");
          sprops->language = "en";
          sprops->encoding = "ISO8859_1";
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

	    /*
	     * Bug 4201684: Xlib doesn't like @euro locales.  Since we don't
	     * depend on the libc @euro behavior, drop it.
	     */
	    lc = strdup(lc);	/* keep a copy, setlocale trashes original. */
	    if (lc == NULL) {
		goto out_of_memory;
	    }
            strcpy(temp, lc);
	    p = strstr(temp, "@euro");
	    if (p != NULL) 
		*p = '\0';
            setlocale(LC_ALL, temp);

            strcpy(temp, lc);

            /* Release lc now that we don't need it anymore: */
            free(lc);
            lc = NULL;

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
            p = nl_langinfo(CODESET);
	    if (strcmp(p, "ANSI_X3.4-1968") == 0 || *p == '\0') {
		std_encoding = "ISO8859_1";
	    } else {
		std_encoding = p;
	    }
	    p = nl_langinfo(CODESET);
	    /* return same result nl_langinfo would return for en_UK,
	     * in order to use optimizations. */
            std_encoding = (*p != '\0') ? p : "ISO8859_1";
            sprops->encoding = std_encoding;
        }
    }
    
#if (CVM_ENDIANNESS == CVM_LITTLE_ENDIAN)
   sprops->unicode_encoding = "UnicodeLittle";
#else
   sprops->unicode_encoding = "UnicodeBig";
#endif

    /* user properties */
    {
        struct passwd *pwent = getpwuid(getuid());
	sprops->user_name = strdup(pwent ? pwent->pw_name : "?");
	sprops->user_home = strdup(pwent ? pwent->pw_dir : "?");
	if (sprops->user_name == NULL || sprops->user_home == NULL) {
	    goto out_of_memory;
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
	tzset();	/* for compatibility */
	sprops->timezone = "";
    }

    /* Current directory */
    {
        char buf[MAXPATHLEN];
        getcwd(buf, sizeof(buf));
	sprops->user_dir = strdup(buf);
	if (sprops->user_dir == NULL ) {
	    goto out_of_memory;
	}
    }

    sprops->file_separator = "/";
    sprops->path_separator = ":";
    sprops->line_separator = "\n";

    /* Generic Connection Framework (GCF): CommConnection Property */
    /* NOTE: comma-delimited (no spaces) list of available comm (serial)
       ports */
    sprops->commports = "/dev/term/a";

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
#ifdef SI_ISALIST
    if (sprops->cpu_isalist != NULL) {
	free((char*)sprops->cpu_isalist);
    }
#endif
    if (sprops->os_name != NULL) {
	free((char*)sprops->os_name);
    }
#ifndef ARCH
    if (sprops->os_arch != NULL) {
	free((char*)sprops->os_arch);
    }
#endif
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
    if (sprops->user_dir != NULL) {
	free((char*)sprops->user_dir);
    }
}

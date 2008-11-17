/*
 * @(#)timezone_md.c	1.12 06/10/10
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
#include "javavm/include/defs.h"
#include <limits.h>
#include <string.h>
#include <windows.h>

#include "jvm.h"
#include "javavm/include/wceUtil.h"

/*
 * CVMtimezoneFindJavaTZ() maps Solaris timezone ID to Java timezone ID using
 * <java_home>/lib/tzmappings. If the TZ value is not found, it falls
 * back to the GMT+/-hh:mm form. `region', which can be null, is not
 * used for Solaris.
 */

#ifndef TIME_ZONE_ID_INVALID
#define TIME_ZONE_ID_INVALID ((DWORD)0xFFFFFFFF)
#endif

static int
WIN32getGMTOffset()
{
    time_t offset;
    char sign, buf[16];
    TIME_ZONE_INFORMATION tzi;
    DWORD id = GetTimeZoneInformation(&tzi);
    LONG bias;

    if (id == TIME_ZONE_ID_INVALID) {
	return 0;
    }
    if (id == TIME_ZONE_ID_DAYLIGHT) {
	bias = tzi.Bias + tzi.DaylightBias;
    } else if (id == TIME_ZONE_ID_STANDARD) {
	bias = tzi.Bias + tzi.StandardBias;
    } else {
	bias = tzi.Bias;
    }

    return bias * 60;
}

/**
 * Returns a GMT-offset-based time zone ID. (e.g., "GMT-08:00")
 */
char *
CVMgetGMTOffsetID()
{
    time_t offset;
    char sign, buf[16];
#ifdef WINCE
    int timezone = WIN32getGMTOffset();
#endif
    if (timezone == 0) {
	return strdup("GMT");
    }

    /* Note that the time offset direction is opposite. */
    if (timezone > 0) {
	offset = timezone;
	sign = '-';
    } else {
	offset = -timezone;
	sign = '+';
    }
    sprintf(buf, (const char *)"GMT%c%02d:%02d",
	    sign, (int)(offset/3600), (int)((offset%3600)/60));
#if 0
    assert(timezone == WIN32getGMTOffset());
#endif
    return strdup(buf);
}


/*ARGSUSED1*/
char *
CVMtimezoneFindJavaTZ(const char *java_home_dir, const char *region)
{
    char *tz;
    char *javatz = NULL;

    if (java_home_dir == NULL) {
	goto tzerr;
    }
#ifdef WINCE
    tz = NULL;
    {
	TIME_ZONE_INFORMATION tzi;
	DWORD id = GetTimeZoneInformation(&tzi);
	if (id != TIME_ZONE_ID_INVALID) {
	    tz = createMCHAR(tzi.StandardName);
	}
    }
#else
    tz = getenv("TZ");	/* TZ default to US PACIFIC at user's logon time */
#endif
    if (tz != NULL) {
	FILE *tzmapf;
	char mapfilename[MAX_PATH+1];
	char line[256];
	int linecount = 0;

	strcpy(mapfilename, java_home_dir);
	strcat(mapfilename, "/lib/tzmappings");

	if ((tzmapf = fopen(mapfilename, "r")) == NULL) {
	    jio_fprintf(stderr, "can't open %s\n", mapfilename);
	    goto tzerr;
	}

	/* If java.home is set to CVM home and tzmappings file exists in lib
	 * directory, tzmappings file is used to map US PACIFIC to 
	 * America Los_Angel.
	 */
	while (fgets(line, sizeof(line), tzmapf) != NULL) {
	    char *p = line;
	    char *sol = line;
	    char *java;
	    int result;

	    linecount++;
	    /*
	     * Skip comments and blank lines
	     */
	    if (*p == '#' || *p == '\n') {
		continue;
	    }

	    /*
	     * Get the first field, Solaris zone ID
	     */
	    while (*p != '\0' && *p != '\t') {
		p++;
	    }
	    if (*p == '\0') {
		/* mapping table is broken! */
		jio_fprintf(stderr, "tzmappings: Illegal format at near line %d.\n", linecount);
		break;
	    }
	    *p++ = '\0';

	    if ((result = strcmp(tz, sol)) == 0) {
		/*
		 * If this is the current Solaris zone ID,
		 * take the Java time zone ID (2nd field).
		 */
		java = p;
		while (*p != '\0' && *p != '\n') {
		    p++;
		}
		if (*p == '\0') {
		    /* mapping table is broken! */
		    jio_fprintf(stderr, "tzmappings: Illegal format at line %d.\n", linecount);
		    break;
		}
		*p = '\0';
		javatz = strdup(java);
	    } else if (result < 0) {
		break;
	    }
	}
	(void) fclose(tzmapf);
    }

 tzerr:
    /* If TZ environment variable is not set or tzmappings file is not found,
     * GMT-08:00 is returned when timezone is defined by the system.
     * Otherwise, GMT is returned.
     */ 
    if (javatz == NULL) {
	time_t offset;
	char sign, buf[16];
#ifdef WINCE
	int timezone = WIN32getGMTOffset();
#endif

	/* Note that the time offset direction is opposite. */
	if (timezone > 0) {
	    offset = timezone;
	    sign = '-';
	} else {
	    offset = -timezone;
	    sign = '+';
	}

	sprintf(buf, "GMT%c%02d:%02d",
		sign, (int)(offset/3600), (int)((offset%3600)/60));
	javatz = strdup(buf);
	if (javatz == NULL) {
	    javatz = strdup("GMT");
	}
    }

    return javatz;
}

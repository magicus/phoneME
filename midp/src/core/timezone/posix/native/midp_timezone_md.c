/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

/**
 * @file
 *
 * Functions that enable MIDP to retrieve platform timezone setting.
 */

#include <time.h>

#include <stdio.h>

#include <midp_logging.h>

/**
 * Return local timezone.
 *
 * @return Local timezone ID string pointer. The ID string should be in the
 *	   format of GMT+/-??:??. For example, GMT-08:00 for PST.
 */

#include <midp_properties_port.h>
#include <midpTimeZone.h>

/**
 * Return local timezone ID string. This string is maintained by this 
 * function internally. Caller must NOT try to free it.
 *
 * This function should handle daylight saving time properly. For example,
 * for time zone America/Los_Angeles, during summer time, this function
 * should return GMT-07:00 and GMT-08:00 during winter time.
 *
 * @return Local timezone ID string pointer. The ID string should be in the
 *         format of GMT+/-??:??. For example, GMT-08:00 for PST.
 */
char* getLocalTimeZone() {
    static char tz[12]; /* No longer than "GMT-10:00" */
    time_t ct;
    struct tm ltm, gtm;
    int mindiff;

    time(&ct); /* current time in seconds since EPOC */
    ltm = *localtime(&ct); /* Translate to local time */
    gtm = *gmtime(&ct);    /* Translate to GMT time */

    mindiff = ((ltm.tm_yday - gtm.tm_yday)*24 + ltm.tm_hour - gtm.tm_hour)*60
            + (ltm.tm_min - gtm.tm_min);

    sprintf(tz, "GMT%+03d:%02d",
            mindiff/60, (mindiff >= 0 ? mindiff%60 : -mindiff%60));

    return tz;
}

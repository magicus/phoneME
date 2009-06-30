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
#ifndef _MIDPTIMEZONE_H_
#define _MIDPTIMEZONE_H_


/**
 * @defgroup core_timezone Timezone Porting Interface
 * @ingroup core
 */

/**
 * @file
 * @ingroup core_timezone
 *
 * @brief Porting interface for time zone library
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Default time zone property name.
 * Should be kept in sync with CLDC class com.sun.cldc.util.j2me.TimeZoneImpl.
 */
#define TIMEZONE_PROP_NAME "com.sun.cldc.util.j2me.TimeZoneImpl.timezone"

/**
 * Return local time zone ID string. This string is maintained by this 
 * function internally. Caller must NOT try to free it.
 * 
 * This function should handle daylight saving time properly. For example,
 * for time zone America/Los_Angeles, during summer this function
 * should return GMT-07:00 and GMT-08:00 during winter.
 *
 * @return Local time zone ID string pointer. The ID string should be in the
 *         format of GMT+/-??:??. For example, GMT-08:00 for PST.
 */
char* getLocalTimeZone();

#ifdef __cplusplus
}
#endif

#endif /* _MIDPTIMEZONE_H_ */

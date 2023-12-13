/*
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
 */
#include <stddef.h>
#include <java_types.h>
#include <secure_random_port.h>
#include <stdio.h>
#include <kni.h>

#include <Windows.h>
#include <stdint.h> // portable: uint64_t   MSVC: __int64 

int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
    // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
    // This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
    // until 00:00:00 January 1, 1970 
    static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;

    GetSystemTime( &system_time );
    SystemTimeToFileTime( &system_time, &file_time );
    time =  ((uint64_t)file_time.dwLowDateTime )      ;
    time += ((uint64_t)file_time.dwHighDateTime) << 32;

    tp->tv_sec  = (long) ((time - EPOCH) / 10000000L);
    tp->tv_usec = (long) (system_time.wMilliseconds * 1000);
    return 0;
}

jboolean get_random_bytes_port(unsigned char*buffer, jint bufsize) {

  /* DO NOT USE THIS TIME-BASED CODE FOR END USER PRODUCTS !!! */

  /* IMPL_NOTE:
   * The problem this function must solve is to obtain a set of really
   * unpredictable bits for use in cryptography.
   * There is no portable solution to this problem.
   *
   * The current time-based implementation generates a
   * cryptographically weak seed and MUST NOT be used in end-user's devices.
   * For real-world use, it MUST be replaced by something else 
   * by the porting engineers, and the replacement code will be 
   * platform-specific.
   *
   * Current time MUST NOT be used for seed generation because time is
   * predictable with a good precision, the accuracy of measurement
   * is limited (for example, a function that returns time in microseconds
   * may be just multiplying tenths of second by 100000), which makes
   * the number of really unpredictable bits small.
   *
   * System configuration parameters also are not a suitable source
   * of randomness because they may be learned from real-world sources
   * or obtained by an installed MIDlet.
   * External events, such as network packet arrival times, can
   * also be manipulated by adversary.
   *
   * (see IETF RFC 1750, Randomness Recommendations for Security,
   *  http://www.ietf.org/rfc/rfc1750.txt)
   */

    struct timeval tv;
    jlong res;
    int i;
    gettimeofday(&tv,NULL);
    res = tv.tv_usec + (1000000 * tv.tv_sec);
    for(i=0; i<bufsize; i++) {
        buffer[i] = (unsigned char)res;
        res >>= 8;
    }
    return KNI_TRUE;
}



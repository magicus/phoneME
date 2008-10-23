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

#include <suspend_resume_port.h>
#include <midp_logging.h>
#include <midpServices.h>
#include <midpNativeThread.h>

/* Only required for default (testing) port. See midp_checkResumeRequest(). */
#include <suspend_resume_test.h>

/**
 * Stack resume timeout. Test purposes only.
 */
long sr_resume_timeout = DEFAULT_TIMEOUT;

/**
 * Testing purposes only. Used for fake implementation of
 * midp_checkResumeRequest().
 */
#define SUSPEND_TIMEOUT 10000

/**
 * This implementation causes java stack to resume after
 * standard timeout after having been suspended.
 */
jboolean midp_checkResumeRequest() {
    static long lastSuspendStart = -1;
    long time_passed;
    jboolean result = KNI_FALSE;

    if (lastSuspendStart == -1) {
        REPORT_INFO(LC_LIFECYCLE, "midp_checkResumeRequest(): init timeout");
        lastSuspendStart = midp_getCurrentTime();
    }

    time_passed = midp_getCurrentTime() - lastSuspendStart;
    if (time_passed >= sr_resume_timeout) {
        lastSuspendStart = -1;
        sr_resume_timeout = DEFAULT_TIMEOUT;
        result = KNI_TRUE;
    } else {
        /*
         * IMPL_NOTE: Sleep delay 1 here means 1 second since
         * midp_sleepNativeThread() takes seconds. Beter solution
         * is rewriting midp_sleepNativeThread() for it to take
         * milliseconds and use SR_RESUME_CHECK_TIMEOUT here.
         */
        midp_sleepNativeThread(1);
    }

    return result;
}

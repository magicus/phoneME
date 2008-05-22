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

static jboolean g_needToResume = KNI_FALSE;

/**
 * Checks if there is a request for java stack to resume normal operation.
 *
 * This function requires porting only if midp_checkAndResume() is used for
 * stack resuming. In case midp_resume() is called directly, this function
 * can be removed from the implementation as well as midp_checkAndResume().
 *
 * @return KNI_TRUE if java stack is requested to resume, KNI_FALSE
 *         if it is not.
 */
jboolean midp_checkResumeRequest() {
    jboolean result = KNI_FALSE;

    if (g_needToResume) {
        g_needToResume = KNI_FALSE;
        result = KNI_TRUE;
    }

    return result;
}

/**
 * Forces midp_checkResumeRequest() to return KNI_TRUE.
 */
void midp_request_resume() {
    /*
     * IMPL_NOTE: if called from a native thread different from the
     * one where MIDP is running, some synchronization may be needed.
     */
    g_needToResume = KNI_TRUE;
}

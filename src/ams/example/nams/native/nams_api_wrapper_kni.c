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

#include <kni.h>
#include <midpError.h>
#include <midpUtilKni.h>
#include <midpNativeAppManager.h>
#include <pcsl_memory.h> /* to have definition of NULL */

/**
 * Invokes NAMS API method that starts midlet.
 *
 * @param suiteId suite id
 * @param className name of the main midlet class (including class path,
                    as gets stored in jad)
 * @param appId ID assigned by the external application manager
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NamsAPIWrapper_midletCreateStart(void) {
    SuiteIdType suiteId = KNI_GetParameterAsInt(1);

    KNI_StartHandles(1);

    GET_PARAMETER_AS_PCSL_STRING(2, v_className)
    pcsl_string* const className = &v_className;
    jint appId = KNI_GetParameterAsInt(3);

    GET_PCSL_STRING_DATA_AND_LENGTH(className)
    if (PCSL_STRING_PARAMETER_ERROR(className)) {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    } else {
        midp_midlet_create_start(suiteId,
            (jchar*)className_data, className_len,
            appId, NULL); /* IMPL_NOTE: add runtime info */
    }
    RELEASE_PCSL_STRING_DATA_AND_LENGTH

    RELEASE_PCSL_STRING_PARAMETER

    KNI_EndHandles();
}

/**
 * Invokes NAMS API method that resumes midlet.
 *
 * @param appId ID assigned by the external application manager
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NamsAPIWrapper_midletResume(void) {
    jint appId = KNI_GetParameterAsInt(1);
    midp_midlet_resume(appId);
}

/**
 * Invokes NAMS API method that pauses midlet.
 *
 * @param appId ID assigned by the external application manager
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NamsAPIWrapper_midletPause(void) {
    jint appId = KNI_GetParameterAsInt(1);
    midp_midlet_pause(appId);
}

/**
 * Invokes NAMS API method that destroys midlet.
 *
 * @param appId ID assigned by the external application manager
 * @param timeout timeout in milliseconds
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NamsAPIWrapper_midletDestroy(void) {
    jint appId   = KNI_GetParameterAsInt(1);
    jint timeout = KNI_GetParameterAsInt(2);
    midp_midlet_destroy(appId, timeout);
}

/**
 * Invokes NAMS API method that sets foreground midlet.
 *
 * @param appId ID assigned by the external application manager
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NamsAPIWrapper_midletSetForeground(void) {
    jint appId = KNI_GetParameterAsInt(1);
    midp_midlet_set_foreground(appId);
}

/**
 * Invokes NAMS API method that sets foreground midlet.
 *
 * @param appId ID assigned by the external application manager
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NamsAPIWrapper_midpSystemStop(void) {
    midp_system_stop();
}

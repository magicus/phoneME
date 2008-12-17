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

#include <jvmconfig.h>
#include <kni.h>
#include <jvm.h>
#include <midpServices.h>
#include <midp_logging.h>

#include <jvmspi.h>
#include <sni.h>

#include <midpMIDletProxyList.h>

/**
 * The ID of the isolate in which the AMS is running. In SVM mode, this is 0
 * and always remains 0.  In MVM mode, it is 0 when the VM is not running, and
 * it contains the actual isolate ID (always nonzero) when the VM is running.
 */
static int amsIsolateId = 0;

/**
 * @file
 * Native methods of MIDletSuiteLoader.
 */

/**
 * Get the current Isolate ID.
 *
 * @return ID of the current Isolate
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_main_MIDletSuiteUtils_getIsolateId) {
    KNI_ReturnInt(getCurrentIsolateId());
}

/**
 * Get the Isolate ID of the AMS Isolate.
 *
 * @return Isolate ID of AMS Isolate
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_main_MIDletSuiteUtils_getAmsIsolateId) {
    KNI_ReturnInt(amsIsolateId);
}

/**
 * Check whether current Isolate is an AMS Isolate
 *
 * @return true if the current Isolate is an AMS Isolate,
 *   false otherwise.
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_main_MIDletSuiteUtils_isAmsIsolate) {
#if ENABLE_MULTIPLE_ISOLATES
    KNI_ReturnBoolean(getCurrentIsolateId() == amsIsolateId);
#else
    KNI_ReturnBoolean(KNI_TRUE);
#endif
}

/**
 * Get maximal number of Isolates allowed by AMS
 * @return maximal Isolates number
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_main_MIDletSuiteUtils_getMaxIsolates) {
    KNI_ReturnInt(getMaxIsolates());
}

/**
 * Registers the currently running isolate as the AMS isolate. This function 
 * must be called from within the context of a native method.
 */
void midpRegisterAmsIsolateId(void) {
#if ENABLE_MULTIPLE_ISOLATES
    amsIsolateId = JVM_CurrentIsolateId();
#else
    amsIsolateId = 0;
#endif
}

/**
 * Gets the isolate ID of the AMS isolate.
 *
 * @return isolate ID of AMS isolate
 */
int midpGetAmsIsolateId(void) {
    return amsIsolateId;
}

/**
 * Unregisters the AMS isolate ID. 
 */
void midpUnregisterAmsIsolateId(void) {
    amsIsolateId = 0;
}

/**
 * Register the Isolate ID of the AMS Isolate by making a native
 * method call that will call JVM_CurrentIsolateId and set
 * it in the proper native variable.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_main_MIDletSuiteUtils_registerAmsIsolateId) {
    midpRegisterAmsIsolateId();
    KNI_ReturnVoid();
}

/**
 * Send hint to VM about the begin of MIDlet startup phase
 * to allow the VM to fine tune its internal parameters to
 * achieve optimal peformance
 *
 * @param midletIsolateId ID of the started MIDlet isolate
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_main_MIDletSuiteUtils_vmBeginStartUp) {
    int midletIsolateId = KNI_GetParameterAsInt(1);
#if !ENABLE_CDC
    JVM_SetHint(midletIsolateId, JVM_HINT_BEGIN_STARTUP_PHASE, 0);
#endif
#if REPORT_LEVEL <= LOG_INFORMATION
    reportToLog(LOG_INFORMATION, LC_AMS,
        "Hint VM about MIDlet startup begin within isolate %d\n",
        midletIsolateId);
#endif
    KNI_ReturnVoid();
}

/**
 * Send hint to VM about the end of MIDlet startup phase
 * to allow the VM to restore its internal parameters
 * changed on startup time for better performance
 *
 * @param midletIsolateId ID of the started MIDlet isolate
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_main_MIDletSuiteUtils_vmEndStartUp) {
    int midletIsolateId = KNI_GetParameterAsInt(1);
#if !ENABLE_CDC
    JVM_SetHint(midletIsolateId, JVM_HINT_END_STARTUP_PHASE, 0);
#endif

#if REPORT_LEVEL <= LOG_INFORMATION
    reportToLog(LOG_INFORMATION, LC_AMS,
        "Hint VM about MIDlet startup end within isolate %d\n",
        midletIsolateId);
#endif
    KNI_ReturnVoid();
}

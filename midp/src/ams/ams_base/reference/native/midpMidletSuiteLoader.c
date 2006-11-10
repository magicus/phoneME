/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
Java_com_sun_midp_main_MIDletSuiteLoader_getIsolateId(void) {
    KNI_ReturnInt(getCurrentIsolateId());
}

/**
 * Get the Isolate ID of the AMS Isolate.
 *
 * @return Isolate ID of AMS Isolate
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_main_MIDletSuiteLoader_getAmsIsolateId(void) {
    KNI_ReturnInt(amsIsolateId);
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
Java_com_sun_midp_main_MIDletSuiteLoader_registerAmsIsolateId(void) {
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
Java_com_sun_midp_main_MIDletSuiteLoader_vmBeginStartUp(void) {
    int midletIsolateId = KNI_GetParameterAsInt(1);
    JVM_SetHint(midletIsolateId, JVM_HINT_BEGIN_STARTUP_PHASE, 0);
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
Java_com_sun_midp_main_MIDletSuiteLoader_vmEndStartUp(void) {
    int midletIsolateId = KNI_GetParameterAsInt(1);
    JVM_SetHint(midletIsolateId, JVM_HINT_END_STARTUP_PHASE, 0);
#if REPORT_LEVEL <= LOG_INFORMATION
    reportToLog(LOG_INFORMATION, LC_AMS,
        "Hint VM about MIDlet startup end within isolate %d\n",
        midletIsolateId);
#endif
    KNI_ReturnVoid();
}

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

#include <suspend_resume.h>
#include <suspend_resume_vm.h>
#include <suspend_resume_test.h>
#include <kni.h>
#include <midp_logging.h>

/**
 * Flag identifying whether VM should stop processing byte code after
 * stack suspending. Testing purposes only.
 */
static jboolean vmSuspendMode = KNI_TRUE;

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_suspend_test_TestUtil_setNoVMSuspendMode() {
    REPORT_INFO(LC_LIFECYCLE, "TestUtil_setNoVMSuspendMode()");
    if (vmSuspendMode) {
        vmSuspendMode = KNI_FALSE;
        sr_unregisterResource((void*)&vm);
    }
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_suspend_test_TestUtil_setVMSuspendMode() {
    REPORT_INFO(LC_LIFECYCLE, "TestUtil_setVMSuspendMode()");
    if (!vmSuspendMode) {
        vmSuspendMode = KNI_TRUE;
        sr_registerResource((void*)&vm, &suspend_vm, &resume_vm);
    }
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_suspend_test_TestUtil_resumeMidp() {
    REPORT_INFO(LC_LIFECYCLE, "TestUtil_resumeMidp()");
    sr_resume_timeout = RESUME_NOW_TIMEOUT;
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_suspend_test_TestUtil_suspendMidp() {
    REPORT_INFO(LC_LIFECYCLE, "TestUtil_suspendMidp()");
    midp_suspend();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_suspend_test_TestUtil_suspendAndResumeMidp() {
    REPORT_INFO(LC_LIFECYCLE, "TestUtil_suspendMidp()");
    midp_suspend();
    sr_resume_timeout = KNI_GetParameterAsInt(1);
    KNI_ReturnVoid();
}

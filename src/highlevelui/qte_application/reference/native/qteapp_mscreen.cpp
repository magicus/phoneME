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
 * 
 * This source file is specific for Qt-based configurations.
 */

#include <qteapp_mscreen.h>
#include <qteapp_export.h>
#include <suspend_resume.h>
#include <jvm.h>

MScreen::MScreen() {
    vm_suspended = false;
}

/**
 * Suspend VM. VM will not receive time slices until resumed.
 */
void MScreen::suspendVM() {
    if (!vm_suspended) {
        vm_suspended = true;
        setNextVMTimeSlice(-1);
    }
}

/**
 * Resume VM to normal operation.
 */
void MScreen::resumeVM() {
    if (vm_suspended) {
        vm_suspended = false;
        setNextVMTimeSlice(0);
    }
}

/**
 * Requests MIDP system (including java applications, VM and resources)
 * to suspend.
 */
void MScreen::pauseAll() {
    midp_suspend();
}

/**
 * Requests MIDP system to resume.
 */
void MScreen::activateAll() {
    midp_resume();
}

/**
 * Implementation of slotTimeout() shared between distinct ports.
 * IMPL_NOTE: due to MOC restrictions slotTimeout() is defined in
 *            subclasses that inherit from QWidget directly.
 */
void MScreen::slotTimeoutImpl() {
    jlong ms;

    if (vm_stopped) {
        return;
    }

    /* check and align stack suspend/resume state */
    midp_checkAndResume();

    ms = vm_suspended ? SR_RESUME_CHECK_TIMEOUT : JVM_TimeSlice();

    /* There is a testing mode with VM running while entire stack is
     * considered to be suspended. Next invocation shold be scheduled
     * to SR_RESUME_CHECK_TIMEOUT in this case.
     */
    if (midp_getSRState() == SR_SUSPENDED) {
        ms = SR_RESUME_CHECK_TIMEOUT;
    }

    /* Let the VM run for some time */
    if (ms <= -2) {
        /*
         * JVM_Stop was called. Avoid call JVM_TimeSlice again until
         * startVM is called.
         */
        vm_stopped = true;
        qteapp_get_application()->exit_loop();
    } else if (ms == -1) {
        /*
         * Wait forever -- we probably have a thread blocked on IO or GUI.
         * No need to set up timer from here
         */
    } else {
        if (ms > 0x7fffffff) {
            vm_slicer.start(0x7fffffff, TRUE);
        } else {
            vm_slicer.start((int)(ms & 0x7fffffff), TRUE);
        }
    }
}

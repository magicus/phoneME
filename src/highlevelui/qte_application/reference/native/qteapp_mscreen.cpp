/*
 *  
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
 * 
 * This source file is specific for Qt-based configurations.
 */

#include <qteapp_mscreen.h>
#include <suspend_resume.h>

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

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

#include <java_types.h>

#include <windows.h>
#include <stdio.h>
#include <win32app_export.h>
#include <limits.h>

/**
 * @file
 *
 * Implementation of a alarm timer.
 */

int
createTimerHandle(int alarmHandle, jlong time) {
    /*
     * The alarm handle must be greater than zero. But timer handles need be
     * greater than the event loop timer handle so not to be mistaken for
     * a wake up event.
     */
    alarmHandle += EVENT_TIMER_ID;

    if (time > INT_MAX) {
	time = INT_MAX;
    }

    SetTimer(win32app_get_window_handle(), (UINT)alarmHandle, (UINT)time, NULL);
    return alarmHandle;
}

int
destroyTimerHandle(int timerHandle) {
    KillTimer(win32app_get_window_handle(), timerHandle);
    return 0;
}

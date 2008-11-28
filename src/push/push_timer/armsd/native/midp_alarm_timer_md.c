/*
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

#include <stdio.h>

#include <kni.h>
#include <jvm.h>
#include <jvmspi.h>
#include <sni.h>
#include <java_types.h>

#include <midpServices.h>
#include <midp_logging.h>
#include <timer_export.h>

/**
 * @file
 *
 * Implementation of a alarm timer.
 */

int
createTimerHandle(int alarmHandle, jlong time) {
    /* alarmHandle is really an address to push entry */
    REPORT_WARN(LC_PUSH, "createTimerHandle: Stubbed out.");
    (void)alarmHandle;
    (void)time;
    return -1;
}

int
destroyTimerHandle(int timerHandle) {
    (void)timerHandle;
    REPORT_WARN(LC_PUSH, "createTimerHandle: Stubbed out.");
    return 0;
}


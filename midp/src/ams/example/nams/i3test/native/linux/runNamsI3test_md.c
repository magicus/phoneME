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

#include <string.h>
#include <stdio.h>

#include <kni.h>

#include <midpInit.h>
#include <midpMalloc.h>

#include <midpStorage.h>
#include <midpString.h>
#include <midpServices.h>
#include <midpNativeThread.h>
#include <midpNativeAppManager.h>

/**
 * @file
 *
 * This file contains platform specific thread routine.
 */

extern void nams_process_command(int command, int param);

void* midlet_starter_routine(midp_ThreadRoutineParameter param) {
    /* 
     * this routine's signature is platform specific - 
     * see midp_ThreadRoutine declaration 
     */

    int* cmd = (int*)param; 
    /*
    printf("DEBUG: thread routine: cmd = %i, param = %i\n", cmd[0], cmd[1]);
    */

    /* sleep for a few seconds to let java subsystem to be initialized */
    midp_sleepNativeThread(5);
    
    nams_process_command(cmd[0], cmd[1]);

    return NULL;
}


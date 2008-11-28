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

#ifndef _SUSPEND_RESUME_LCDUI_H_
#define _SUSPEND_RESUME_LCDUI_H_

#include <kni.h>
#include <midpError.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Structure type defined to represent system state that affects lcdui.
 */
typedef struct _lcduiState {
    /**
     * Identifies whether display was rotated or not.
     */
    jboolean isDisplayRotated;
    
    /**
     * Cached value of locale microedition.locale system variable
     */
     char* locale;
       

} LCDUIState;

/**
 * Suspends lcdui, saves current sytem state to analize changes after resume.
 * This function is registered in suspend/resume system as suspending
 * routine for the LCDUIState.
 */
extern MIDPError suspend_lcdui(void *resource);

/**
 * Resumes lcdui state.
 * This function is registered in suspend/resume system as resuming
 * routine for the LCDUIState.
 */
extern MIDPError resume_lcdui(void *resource);

#ifdef __cplusplus
}
#endif

#endif // _SUSPEND_RESUME_LCDUI_H_

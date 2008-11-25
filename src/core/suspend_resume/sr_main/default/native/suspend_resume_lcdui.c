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

#include <suspend_resume_lcdui.h>
#include <lcdlf_export.h>
#include <midpEventUtil.h>



/**
 * Default implementation of suspending routine for the LCDUI.
 */
MIDPError suspend_lcdui(void *resource) {
    LCDUIState *st = (LCDUIState*) resource;
    /* Saving lcdui state */
    st->isDisplayRotated =
        lcdlf_get_reverse_orientation(lcdlf_get_current_hardwareId());

    return ALL_OK;
}

/**
 * Default implementation of resuming routine for the LCDUI.
 */
MIDPError resume_lcdui(void *resource) {
    LCDUIState *st = (LCDUIState*) resource;
    /* Restoring lcdui state */
    jboolean orient = lcdlf_get_reverse_orientation(lcdlf_get_current_hardwareId());
    
    if (orient != st->isDisplayRotated) {
        MidpEvent midpEvent;
        MIDP_EVENT_INITIALIZE(midpEvent);
        midpEvent.type    = ROTATION_EVENT;
        midpStoreEventAndSignalForeground(midpEvent);
    }
    
    return ALL_OK;
}

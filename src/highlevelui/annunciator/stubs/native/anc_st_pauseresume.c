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

#include <midpPauseResume.h>
#include <midp_logging.h>

/**
 * @file
 *
 * Platform dependent native code to handle VM pause and resume.
 */

/**
 * Platform handling code for VM pause notification call.
 */
void pdMidpNotifyPausedAll() {
    
    REPORT_CALL_TRACE(LC_CORE, "LF:STUB:pdMidpNotifyPausedAll()\n");
}

/**
 * Platform handling code for VM resume notification call.
 */
void pdMidpNotifyResumedAll() {

    REPORT_CALL_TRACE(LC_CORE, "LF:STUB:pdMidpNotifyResumedAll()\n");
}

/**
 * Platform handling code for VM suspend notification call.
 */
void pdMidpNotifyInternalPausedAll() {
    REPORT_CALL_TRACE(LC_CORE, "LF:STUB:pdMidpNotifyInternalPausedAll()\n");
}

/**
 * Platform handling code for VM continue notification call.
 */
void pdMidpNotifyInternalResumedAll() {
    REPORT_CALL_TRACE(LC_CORE, "LF:STUB:pdMidpNotifyInternalResumedAll()\n");
}


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

/**
 * @file
 *
 * Utility Functions shared between the platform widgets and java widgets
 * build.
 */
#include <midp_foreground_id.h>
#include <midpMidletSuiteUtils.h>
#include <midpEventUtil.h>
#include <midp_logging.h>

#include <jvmconfig.h>
#include <jvm.h>

#if ENABLE_MULTIPLE_DISPLAYS
#include <lcdlf_export.h>
#endif /* ENABLE_MULTIPLE_DISPLAYS */



#if ENABLE_ISOLATES
/* this is a work around for jvm.h that has wrong case for this prototype */
extern int JVM_CurrentIsolateId();

#endif  /* ENABLE_ISOLATES */

#include <jvmspi.h>
/**
 * Add a native MIDP event to Java event queue of the AMS Isolate.
 * This is done asynchronously.
 *
 * @param evt The event to store
 */
void midpStoreEventAndSignalAms(MidpEvent evt) {
    StoreMIDPEventInVmThread(evt, midpGetAmsIsolateId());
}

/**
 * Add a native MIDP event to Java event queue.
 * This is done asynchronously.
 *
 * @param evt The event to store
 */
void midpStoreEventAndSignalForeground(MidpEvent evt) {
#if ENABLE_MULTIPLE_ISOLATES
	/*
	 * In NAMS when no midlets are running ID of the foreground
	 * isolate can be 0. In this case the event is dropped.
	 */
	if (gForegroundIsolateId <= 0) {
        REPORT_WARN2(LC_CORE, "Ignoring the event %d sent to wrong isolate (%d)",
                     evt.type, gForegroundIsolateId); 
		return;
	}
#endif

#if ENABLE_MULTIPLE_DISPLAYS
  evt.DISPLAY = gForegroundDisplayIds[lcdlf_get_current_hardwareId()];
#else
  evt.DISPLAY = gForegroundDisplayId;
#endif /* ENABLE_MULTIPLE_DISPLAYS */
  StoreMIDPEventInVmThread(evt, gForegroundIsolateId);
}

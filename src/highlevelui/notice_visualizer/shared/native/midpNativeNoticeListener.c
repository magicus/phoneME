/*
 *
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

#include "midp_jc_event_defs.h"
#include "javacall_lcdui_notice.h"

/* Need to be synchronized with Notice.REMOVED */
#define NOTICE_REMOVED 	1

/**
 * Notify the native application manager of the MIDlet foreground change.
 *
 * @param externalAppId ID assigned by the external application manager
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NativeNoticeListener_noticeUpdate0(void) {

}

/**
 * Notifies all listeners(isolates) about Notice action. The 
 * receivers are every task NoticeManager instances. 
 * 
 * @param uid 		ID of the notice
 * @param status 	its status (selected, dismissed)
 */
void javanotify_lcdui_notice_status(javacall_int uid, javacall_notice_status status) {
	midp_jc_event_union event;
	event.eventType = MIDP_JC_NOTICE_ANNOUNCEMENT_EVENT;
	event.data.noticeEvent.uid = uid;
	event.data.noticeEvent.operation = NOTICE_REMOVED;
	event.data.noticeEvent.status = status;
	midp_jc_event_send(&event);
}

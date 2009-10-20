/*
 *  
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

/*
 * @file
 *
 * @brief Implementation of javacall security notification functions..
 */

#include <kni.h>
#include <midp_security.h>
#include <javacall_security.h>

/** permission listener function pointer */
static MIDP_SECURITY_PERMISSION_LISTENER pListener;

/**
 * Sets the permission result listener.
 *
 * @param listener            The permission checking result listener
 */
void security_set_permission_listener(
	MIDP_SECURITY_PERMISSION_LISTENER listener) {
    pListener = listener;
}


void javanotify_security_permission_check_result(const javacall_suite_id suite_id, 
                                                 const unsigned int session,
                                                 const unsigned int result) {
    (void)suite_id;
    if (NULL != pListener) {
        pListener(session, (jboolean)(CONVERT_PERMISSION_STATUS(result)>0));
    }
}

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

#ifndef _MIDP_SECURITY_H_
#define _MIDP_SECURITY_H_

/**
 * @file
 * @ingroup nams_port
 *
 * @brief Native security manager porting interface
 *
 * This interface defines porting interface for Java platform security
 * subsystem based on external native Security Manager that interprets
 * security settings and current MIDlet suite's security domain.
 */

/**
 * The typedef of the security permission listener that is notified 
 * when the native security manager has permission checking result.
 *
 * @param handle  - The handle for the permission check session
 * @param granted - true if the permission is granted, false if denied.
 */
typedef void (*MIDP_SECURITY_PERMISSION_LISTENER)(jint handle, jboolean granted);

/**
 * Permission check status from
 * javacall_security_permission_result to MIDP defined values.
 * <p>
 * They are the same now.
 */
#define CONVERT_PERMISSION_STATUS(x) x

/**
 * Sets the permission result listener.
 *
 * @param listener            The permission checking result listener
 */
void midpport_security_set_permission_listener(MIDP_SECURITY_PERMISSION_LISTENER listener);
                                        	   
#endif /* _MIDP_SECURITY_H_ */

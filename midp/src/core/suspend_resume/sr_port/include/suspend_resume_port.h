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

#ifndef _SUSPEND_RESUME_PORT_H_
#define _SUSPEND_RESUME_PORT_H_

#include <kni.h>

/**
 * @file SUSPEND_RESUME_PORT_port.h
 *
 * Suspend/Resume porting interface
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Checks if there is a request for java stack to resume normal operation.
 * This function is called from midp_checkAndResume() and requires porting.
 *
 * This function requires porting only if midp_checkAndResume() is used for
 * stack resuming. In case midp_resume() is called directly, this function
 * can be removed from the implementation as well as midp_checkAndResume().
 *
 * @return KNI_TRUE if java stack is requested to resume, KNI_FALSE
 *         if it is not.
 */
extern jboolean midp_checkResumeRequest();

#ifdef __cplusplus
}
#endif

#endif /* _SUSPEND_RESUME_PORT_H_ */

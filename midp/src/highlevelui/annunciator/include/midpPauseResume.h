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

#ifndef _MIDP_PAUSERESUME_H_
#define _MIDP_PAUSERESUME_H_

/**
 * @file
 * @ingroup highui_anc
 *
 * @brief Porting interface to handle suspend/resume display.
 */


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Porting layer for MIDP VM pause function.
 *
 * The system will call this routine to notify
 * platform that all apps have been suspended.
 */
void pdMidpNotifySuspendAll(void);

/**
 * Porting layer for MIDP VM resume function.
 *
 * The system will call this routine to notify
 * platform that all apps have been resumed.
 */
void pdMidpNotifyResumeAll(void);

#ifdef __cplusplus
}
#endif

#endif /* _MIDP_PAUSERESUME_H_ */

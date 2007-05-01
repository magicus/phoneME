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

#ifndef _LFP_REGISTRY_H_
#define _LFP_REGISTRY_H_

/**
 * @file
 * @ingroup highui_lfp
 *
 * @brief  Native resource registry for platform widgets.
 */

#include <lfpport_displayable.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Delete all MIDP components when VM is exiting.
 */
void MidpDeleteAllComponents();

/**
 * Pointer to the currently visible screen. The screen receives all of
 * the user events. The screen could be either a displayable, or a
 * system dialog (such as a menu). This global variable is defined in
 * midp_lcdui.c.
 */
extern MidpFrame* MidpCurrentScreen;

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* _LFP_REGISTRY_H_ */

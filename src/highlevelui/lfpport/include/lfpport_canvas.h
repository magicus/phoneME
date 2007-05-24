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

#ifndef _LFPPORT_CANVAS_H_
#define _LFPPORT_CANVAS_H_


/**
 * @file
 * @ingroup highui_lfpport
 *
 * @brief Canvas-specific porting functions and data structures.
 */
#include <lfpport_displayable.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a canvas's native peer but does not display it.
 * The MIDP implementation should call this function in the background and
 * display the canvas afterward it is given all of its content.
 * When this function returns successfully, *canvasPtr will be filled.
 *
 * @param canvasPtr pointer to the canvas's MidpDisplayable structure.
 * @param title title string.
 * @param tickerText ticker text.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_canvas_create(MidpDisplayable* canvasPtr,
                const pcsl_string* title, const pcsl_string* tickerText);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LFPPORT_CANVAS_H_ */

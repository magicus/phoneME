/*
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

#ifndef _LFPPORT_DISPLAYABLE_H_
#define _LFPPORT_DISPLAYABLE_H_


/**
 * @file
 * @ingroup highui_lfpport
 *
 * @brief Displayable-specific porting functions and data structures.
 */
#include <lfpport_component.h>
#include <lfpport_displayable.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _MidpDisplayable MidpDisplayable; /**< Type alias */

/**
 * Common interface of the native peers of the canvas, alert, and form
 * displayables. A pointer of this type can be used as a pointer to a
 * MidpFrame.
 */
struct _MidpDisplayable {
    /**
     * Common structure for both Displayable and system dialog.
     */
    MidpFrame frame;

    /**
     * Sets the title of the displayable.
     *
     * @param screenPtr pointer to the screen's MidpFrame structure.
     * @param label the title to be set for this displayable.
     * 
     * @return an indication of success or the reason for failure
     */
    MidpError (*setTitle)(MidpDisplayable* screenPtr, const pcsl_string* label);

    /**
     * Sets the ticker for the displayable.
     * 
     * @param screenPtr pointer to the screen's MidpFrame structure.
     * @param text the text to be used to display on the ticker.
     *
     * @return an indication of success or the reason for failure
     */
    MidpError (*setTicker)(MidpDisplayable* screenPtr, const pcsl_string* text);

};


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LFPPORT_DISPLAYABLE_H_ */

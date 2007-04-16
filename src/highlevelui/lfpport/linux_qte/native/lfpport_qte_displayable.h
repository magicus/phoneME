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
 * 
 * This source file is specific for Qt-based configurations.
 */

/**
 * @file
 * Defines platform-specific functions for handling the two components common
 * to all displayables: the title and the ticker.
 */

#ifndef _LFPPORT_QTE_DISPLAYABLE_H_
#define _LFPPORT_QTE_DISPLAYABLE_H_

#include <lfpport_displayable.h>

/**
 * Makes the title of the given displayable the given string.
 *
 * @param screenPtr pointer to the displayable that will receive the title
 * @param label the title for the displayable.
 *
 * @return an indication of success or the reason for failure
 */
extern "C" MidpError
displayable_set_title(MidpDisplayable* screenPtr, const pcsl_string* label);

/**
 * Gives the ticker of the given displayable the given string
 * contents.
 *
 * @param screenPtr pointer to the displayable that will receive the ticker
 * @param label the content that the ticker should display.
 *
 * @return an indication of success or the reason for failure

*/
extern "C" MidpError
displayable_set_ticker(MidpDisplayable* screenPtr, const pcsl_string* text);

#endif /* _LFPPORT_QTE_DISPLAYABLE_H_ */

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

#ifndef _LFPPORT_STRINGITEM_H_
#define _LFPPORT_STRINGITEM_H_

/**
 * @file
 * @ingroup highui_lfpport
 *
 * @brief StringItem-specific porting functions and data structures.
 */

#include <lfpport_displayable.h>
#include <lfpport_item.h>
#include <lfpport_font.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a string item's native peer, but does not display it.  When
 * this function returns successfully, the fields in *itemPtr should be
 * set.
 *
 * @param itemPtr pointer to the string item's MidpItem structure. 
 * @param ownerPtr pointer to the item's owner(form)'s MidpDisplayable 
 *                 structure.
 * @param label the item label.
 * @param layout the item layout directive.
 * @param text the text to be used to create this string item.
 * @param fontPtr pointer to the font to be used.
 * @param appearanceMode appearance mode for the string 
 *                       item (eg. PLAIN, HYPERLINK and so on).
 * 
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_stringitem_create(MidpItem* itemPtr, 
				    MidpDisplayable* ownerPtr,
				    const pcsl_string* label, int layout,
				    const pcsl_string* text,
				    PlatformFontPtr fontPtr, 
				    int appearanceMode);


/**
 * Notifies the native peer of a change in the item's content.
 *
 * @param itemPtr pointer to the string item's MidpItem structure. 
 * @param text the new string.
 * @param appearanceMode the appearance mode of the passed in text.
 * 
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_stringitem_set_content(MidpItem* itemPtr, 
					 const pcsl_string* text,
					 int appearanceMode);

/**
 * Notifies the native peer of a change in the item's font.
 *
 * @param itemPtr pointer to the string item's MidpItem structure.
 * @param fontPtr the new font.
 * 
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_stringitem_set_font(MidpItem* itemPtr, 
				      PlatformFontPtr fontPtr);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LFPPORT_STRINGITEM_H_ */

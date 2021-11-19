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


#ifndef _LFPPORT_CUSTOMITEM_H_
#define _LFPPORT_CUSTOMITEM_H_

/**
 * @file
 * @ingroup highui_lfpport
 *
 * @brief CustomItem-specific porting functions and data structures.
 */

/**
 * Porting interface of CustomItem.
 */
#include <lfpport_displayable.h>
#include <lfpport_item.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a custom item's native peer, but does not display it.
 * When this function returns successfully, *itemPtr will be filled.
 *
 * @param itemPtr pointer to the custom item's MidpItem structure.
 * @param ownerPtr pointer to the item's owner(form)'s MidpDisplayable 
 *                 structure.
 * @param label the item label.
 * @param layout the item layout directive.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_customitem_create(MidpItem* itemPtr, 
				    MidpDisplayable* ownerPtr,
				    const pcsl_string* label, int layout);

/**
 * Causes an immediate bitblt on the specified rectangle.
 * The given x and y  values are relative to the custom item's 
 * content's co-ordinate system.
 *
 * @param itemPtr pointer to the custom item's MidpItem structure.
 * @param x x-coordinate of the refresh area.
 * @param y y-coordinate of the refresh area.
 * @param width width of area to be refreshed.
 * @param height height of area to be refreshed.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_customitem_refresh(MidpItem* itemPtr,
				     int x, int y,
				     int width, int height);

/**
 * Gets the width of the custom item's native label.
 *
 * @param widthRet the calculated label width, based on tentative 
 *                 width passed in.
 * @param width the tentative width for the label.
 * @param ciPtr pointer to the custom item's MidpItem structure.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_customitem_get_label_width(int *widthRet,
					     int width,
					     MidpItem* ciPtr);

/**
 * Gets the height of the custom item's native label. 
 *
 * @param width the tentative width for the label.
 * @param heightRet the calculated label height, based on tentative 
 *                  width passed in.
 * @param ciPtr pointer to the custom item's MidpItem structure.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_customitem_get_label_height(int width,
					      int *heightRet,
					      MidpItem* ciPtr);

/**
 * Gets the padding for the custom item.
 *
 * @param pad the padding for the custom item.
 * @param ciPtr pointer to the custom item's MidpItem structure.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_customitem_get_item_pad(int *pad, MidpItem* ciPtr);

/**
 * Sets the content buffer. All paints are done to that buffer.
 * When paint is processed snapshot of the buffer is flushed to
 * the native resource content area.
 * 
 * @param ciPtr pointer to the custom item's MidpItem structure.
 * @param imgPtr pointer to the native resource corresponding
 *        to the Java platform offscreen buffer for CustomItem content
 */
MidpError lfpport_customitem_set_content_buffer(MidpItem* ciPtr, 
						unsigned char* imgPtr);
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

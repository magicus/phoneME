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

#ifndef _LFPPORT_IMAGEITEM_H_
#define _LFPPORT_IMAGEITEM_H_

/**
 * @file
 * @ingroup highui_lfpport
 *
 * @brief ImageItem-specific porting functions and data structures.
 */

#include <lfpport_displayable.h>
#include <lfpport_item.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates an image item's native peer, but does not display it.
 * When this function returns successfully, the fields in *itemPtr should
 * be set.
 *
 * @param itemPtr pointer to the image item's MidpItem structure.
 * @param ownerPtr pointer to the item's owner(form)'s MidpDisplayable 
 *                 structure.
 * @param label the item label.
 * @param layout the item layout directive.
 * @param imgPtr pointer to the image used to create this item.
 * @param altText alternative text to be displayed to the user in case of 
 *                an image not being displayed.
 * @param appearanceMode appearance mode for the image 
 *                       item (eg. PLAIN/HYPERLINK).
 * 
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_imageitem_create(MidpItem* itemPtr, 
				   MidpDisplayable* ownerPtr,
				   const pcsl_string* label, int layout,
				   unsigned char* imgPtr,
				   const pcsl_string* altText, int appearanceMode);

/**
 * Notifies the native peer of a content change in the corresponding
 * image item.
 *
 * @param itemPtr pointer to the image item's MidpItem structure.
 * @param imgPtr the new image
 * @param altText the new alternative text
 * @param appearanceMode the new appearance mode
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_imageitem_set_content(MidpItem* itemPtr,
					unsigned char* imgPtr,
					const pcsl_string* altText, 
					int appearanceMode);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LFPPORT_IMAGEITEM_H_ */

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


#ifndef _LFPPORT_ITEM_H_
#define _LFPPORT_ITEM_H_

/**
 * @file
 * @ingroup highui_lfpport
 *
 * @brief Item-specific porting functions and data structures.
 */

#include <lfpport_component.h>
#include <lfpport_displayable.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _MidpItem MidpItem; /**< Type alias */

/**
 * Common interface of the native peer of a MIDP item.
 */
struct _MidpItem {
    /**
     * Common data structure for a MIDP item used by the platform 
     * independent layer.
     * Platform specific porting functions should only read its values
     * without any changes.
     */
    MidpComponent component;

    /**
     * Native widget ID/pointer to a MIDP item.
     */
    PlatformItemWidgetPtr widgetPtr;

    /**
     * Owner screen pointer.
     */
    MidpDisplayable* ownerPtr;

    /**
     * Item layout constraints as defined in MIDP.
     */
    int layout;

    /**
     * Gets the minimum width of the item, in pixels. The minimum width
     * is the smallest at which the item can function and display its
     * contents, though perhaps not optimally. For example, for text
     * field, the minimum with could be the size required to show one word
     * and a horizontal scrollbar.
     * 
     * @param width pointer that will be to the fewest pixels in width.
     *        required to display the item. This function sets width's value.
     * @param itemPtr pointer to the item's MidpItem structure.
     * 
     * @return an indication of success or the reason for failure
     */
    MidpError (*getMinimumWidth)(int* width, MidpItem* itemPtr);
    
    /**
     * Get the minimum height of the given item, in pixels.  The minimum
     * height is smallest at which the item can function and display its
     * contents, though perhaps not optimally. For example, for a text field,
     * this could be the size to show one word and a vertical scrollbar.
     *
     * @param height pointer that will hold the fewest pixels in eight
     *        required to display the item. This function sets height's value.
     * @param itemPtr pointer to the item's MidpItem structure.
     * 
     * @return an indication of success or the reason for failure
     */
    MidpError (*getMinimumHeight)(int* height, MidpItem* itemPtr);

    /**
     * Gets the preferred width of the item, in pixels.  The preferred
     * width is the one at which the item can function and display its
     * contents optimally.
     * 
     * @param width pointer that will be to the optimum number of pixels in
     *        width. This function sets width's value.
     * @param itemPtr pointer to the item's MidpItem structure.
     * @param lockedHeight required height of the item in pixels, or -1 if
     *        the item's height is not locked.
     * 
     * @return an indication of success or the reason for failure
     */
    MidpError (*getPreferredWidth)(int* width, MidpItem* itemPtr,
				    int lockedHeight);
    
    /**
     * Gets the preferred height of the item, in pixels.  The preferred
     * height is one at which the item can function and display its
     * contents optimally.
     *
     * @param height pointer that will be to the optimum number of pixels in
     *        height. This function sets height's value.
     * @param itemPtr pointer to the item's MidpItem structure.
     * @param lockedWidth required width of the item in pixels, or -1 if
     *        the item's width is not locked.
     * 
     * @return an indication of success or the reason for failure
     */
    MidpError (*getPreferredHeight)(int* height, MidpItem* itemPtr,
				    int lockedWidth);

    /**
     * Sets the item's label.
     *
     * @param itemPtr pointer to the item's MidpItem structure.
     * @param label the label to be set for this item.
     * 
     * @return an indication of success or the reason for failure
     */
    MidpError (*setLabel)(MidpItem* itemPtr, const pcsl_string* label);

    /**
     * Shows the given item.  This function will only be called on an
     * item that is not already shown.
     *
     * @param itemPtr pointer to the item's MidpItem structure.
     * 
     * @return an indication of success or the reason for failure
     */
    MidpError (*show)(MidpItem* itemPtr);

    /**
     * Resets the item's location to the given coordinates, which
     * identify the item's upper-left corner in its owner's coordinate
     * system. This function will only be called on an item that is
     * already shown. It moves the item to the new location; it does not
     * change the item's size.
     * 
     * @param itemPtr pointer to the item's MidpItem structure.
     * @param x new x coordinate for the item's upper-left corner. The
     *        value is in the coordinate system of the item's owner.
     * @param y new y coordinate for the item's upper-left corner. The
     *        value is in the coordinate system of the item's owner.
     * 
     * @return an indication of success or the reason for failure
     */
    MidpError (*relocate)(MidpItem* itemPtr, int x, int y);

    /**
     * Resets the item's size to the given width and height in pixels.
     * This function will only be called on an item that is already shown.
     * It resizes the item; it does not change the location of the item's
     * upper-left corner.
     *
     * @param itemPtr pointer to the item's MidpItem structure.
     * @param width new width in pixels.
     * @param height new height in pixels.
     * 
     * @return an indication of success or the reason for failure
     */
    MidpError (*resize)(MidpItem* itemPtr, int width, int height);

    /**
     * Hides the item.
     * 
     * @param itemPtr pointer to the item's MidpItem structure.
     *
     * @return an indication of success or the reason for failure
     */
    MidpError (*hide)(MidpItem* itemPtr);

    /**
     * Deletes any resources associated with the item.
     * 
     * @param itemPtr pointer to the item's MidpItem structure.
     *
     * @return an indication of success or the reason for failure
     */
    MidpError (*destroy)(MidpItem* itemPtr);

    /**
     * Handles the item's events. This function handles changes to the
     * item's data model by calling MidpItemPeerNotify. It handles
     * traversal out of an item by calling pdMidpFormTraverseOutItem to
     * notify the form.
     *
     * @param itemPtr pointer to the item's MidpItem structure.
     * @param eventPtr pointer to the item's event to be handled.
     *
     * @return true if the event has been handled and should not be further
     *         dispatched, false otherwise.
     */
    jboolean (*handleEvent)(MidpItem* itemPtr, PlatformEventPtr eventPtr);

};


#ifdef __cplusplus
} /* extern C */
#endif

#endif /* _LFPPORT_ITEM_H_ */

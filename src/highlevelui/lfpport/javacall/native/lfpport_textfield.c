/*
 *
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

/**
 * @file
 * @ingroup highui_lfpport
 *
 * @brief TextField-specific porting functions and data structures.
 */

#include <lfpport_displayable.h>
#include <lfpport_item.h>
#include <lfpport_textfield.h>
#include "lfpport_gtk.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Creates a text field's native peer, but does not display it.
 * When this function returns successfully, the fields in *itemPtr should
 * be set.
 *
 * @param itemPtr pointer to the text field's MidpItem structure.
 * @param ownerPtr pointer to the item's owner(form)'s MidpDisplayable
 *                 structure.
 * @param label the item label.
 * @param layout the item layout directive.
 * @param text the initial text for the text field.
 * @param maxSize maximum size of the text field.
 * @param constraints constraints to be validated against, during
 *                    text input.
 * @param initialInputMode suggested input mode on creation.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_textfield_create(MidpItem* itemPtr,
				   MidpDisplayable* ownerPtr,
				   const pcsl_string* label, int layout,
				   const pcsl_string* text, int maxSize,
				   int constraints,
				   const pcsl_string* initialInputMode){
    printf(">>>%s\n", __FUNCTION__);
    printf("<<<%s\n", __FUNCTION__);
    return -1;
}

/**
 * Notifies the native peer of a change in the text field's content.
 *
 * @param itemPtr pointer to the text field's MidpItem structure.
 * @param text the new string.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_textfield_set_string(MidpItem* itemPtr, const pcsl_string* text){
    printf(">>>%s\n", __FUNCTION__);
    printf("<<<%s\n", __FUNCTION__);
    return -1;
}

/**
 * Gets the current contents of the text field.
 *
 * @param text pointer to the text field's current content. This
 *             function sets text's value.
 * @param newChange pointer to a flag that is true when the text field's
 *        content has changed since the last call to this function, and is
 *        false otherwise. This function sets newChange's value.
 * @param itemPtr pointer to the text field's MidpItem structure.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_textfield_get_string(pcsl_string* text, jboolean* newChange,
				       MidpItem* itemPtr){
    printf(">>>%s\n", __FUNCTION__);
    printf("<<<%s\n", __FUNCTION__);
    return -1;
}

/**
 * Notifies the native peer of a change in the maximum size of its text field.
 *
 * @param itemPtr pointer to the text field's MidpItem structure.
 * @param maxSize the new maximum size.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_textfield_set_max_size(MidpItem* itemPtr, int maxSize){
    printf(">>>%s\n", __FUNCTION__);
    printf("<<<%s\n", __FUNCTION__);
    return -1;
}

/**
 * Gets the current input position.
 *
 * @param position pointer to current position of the text field's
 *        caret. This function set's position's value.
 * @param itemPtr pointer to the text field's MidpItem structure.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_textfield_get_caret_position(int *position,
					       MidpItem* itemPtr){
    printf(">>>%s\n", __FUNCTION__);
    printf("<<<%s\n", __FUNCTION__);
    return -1;
}

/**
 * Notifies the native peer of a change in the text field's constraints.
 *
 * @param constraints the new input constraints.
 * @param itemPtr pointer to the text field's MidpItem structure.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_textfield_set_constraints(MidpItem* itemPtr,
					    int constraints){
    printf(">>>%s\n", __FUNCTION__);
    printf("<<<%s\n", __FUNCTION__);
    return -1;
}

#ifdef __cplusplus
} /* extern "C" */
#endif


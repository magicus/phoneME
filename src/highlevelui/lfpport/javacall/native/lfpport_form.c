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
 * @brief Form-specific porting functions and data structures.
 */

#include <lfpport_displayable.h>
#include <lfpport_item.h>
#include <lfpport_form.h>
#include "lfpport_gtk.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates the form's native peer (the container window for a form), but
 * does not display it.
 *
 * The MIDP implementation should call this function in the background and
 * display the form after it is given all of its content. When this
 * function returns successfully, the fields in *formPtr will be set.
 *
 * @param formPtr pointer to the form's MidpDisplayable structure.
 * @param title title of the form.
 * @param tickerText text to be displayed in the ticker.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_form_create(MidpDisplayable* formPtr,
			      const pcsl_string* title, const pcsl_string* tickerText){
    printf(">>>%s\n", __FUNCTION__);
    printf("<<<%s\n", __FUNCTION__);
    return -1;
}

/**
 * Sets the virtual size of the form. The size assumes an infinitely
 * large screen, without scrolling.
 *
 * @param formPtr pointer to the form's MidpDisplayable structure.
 * @param w width of the form.
 * @param h height of the form.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_form_set_content_size(MidpDisplayable* formPtr,
					int w, int h){
    printf(">>>%s\n", __FUNCTION__);
    printf("<<<%s\n", __FUNCTION__);
    return -1;
}

/**
 * Sets the form's current item (the item that has focus) to the
 * given item and makes the form visible.
 *
 * @param itemPtr pointer to the item to be made current.
 * @param yOffset offset relative to the item's y co-ordinate.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_form_set_current_item(MidpItem* itemPtr, int yOffset){
    printf(">>>%s\n", __FUNCTION__);
    printf("<<<%s\n", __FUNCTION__);
    return -1;
}

/**
 * Gets the native peer's current "y" scroll position.
 *
 * @param pos pointer that will be to the user's current "y" position on
 *        the form. This function sets pos's value.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_form_get_scroll_position(int *pos){
    printf(">>>%s\n", __FUNCTION__);
    printf("<<<%s\n", __FUNCTION__);
    return -1;
}

/**
 * Sets the native peer's current "y" scroll position.
 *
 * @param pos new current "y" position on the form.
 *
 */
MidpError lfpport_form_set_scroll_position(int pos) {
    printf(">>>%s\n", __FUNCTION__);
    printf("<<<%s\n", __FUNCTION__);
    return -1;
}

/**
 * Gets the native peer's current viewport height.
 *
 * @param height pointer that will be pointing to the current viewport height
 *        in the form. This function sets height's value.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_form_get_viewport_height(int *height){
    printf(">>>%s\n", __FUNCTION__);
    printf("<<<%s\n", __FUNCTION__);
    return -1;
}


#ifdef __cplusplus
} /* extern "C" */
#endif


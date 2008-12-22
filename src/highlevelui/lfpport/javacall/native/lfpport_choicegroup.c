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
 * @brief ChoiceGroup-specific porting functions and data structures.
 */

/*
 * Note that elements in a choice group are indexed beginning
 * with zero.
 */

#include <lfpport_displayable.h>
#include <lfpport_item.h>
#include <lfpport_font.h>
#include <lfpport_choicegroup.h>
#include "lfpport_gtk.h"

#ifdef __cplusplus
extern "C" {
#endif


MidpError lfpport_choicegroup_item_show_cb(MidpItem* itemPtr){
    GtkWidget *widget = (GtkWidget*)itemPtr->widgetPtr;
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    gtk_widget_show_all(widget);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}
MidpError lfpport_choicegroup_item_hide_cb(MidpItem* itemPtr){
    GtkWidget *widget = (GtkWidget*)itemPtr->widgetPtr;
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    gtk_widget_hide_all(widget);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_choicegroup_item_set_label_cb(MidpItem* itemPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}
MidpError lfpport_choicegroup_item_destroy_cb(MidpItem* itemPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}

MidpError lfpport_choicegroup_item_get_min_height_cb(int *height, MidpItem* itemPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    *height = STUB_MIN_HEIGHT;
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_choicegroup_item_get_min_width_cb(int *width, MidpItem* itemPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    *width = STUB_MIN_WIDTH;
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_choicegroup_item_get_pref_height_cb(int* height,
                                                 MidpItem* itemPtr,
                                                 int lockedWidth){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    *height = STUB_PREF_HEIGHT;
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_choicegroup_item_get_pref_width_cb(int* width,
                                                MidpItem* itemPtr,
                                                int lockedHeight){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    *width = STUB_PREF_WIDTH;
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_choicegroup_item_handle_event_cb(MidpItem* itemPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}

MidpError lfpport_choicegroup_item_relocate_cb(MidpItem* itemPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}

MidpError lfpport_choicegroup_item_resize_cb(MidpItem* itemPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}


/**
 * Creates a choice group's native peer, but does not display it.
 * When this function returns successfully, the fields in *cgPtr will be
 * set.
 *
 * @param cgPtr pointer to the choice group's <tt>MidpItem</tt> structure.
 * @param ownerPtr pointer to the item's owner(form)'s MidpDisplayable
 *                 structure.
 * @param label the item label.
 * @param layout the layout directive of choicegroup.
 * @param choiceType the type of choicegroup.
 * @param choices an array of choice elements.
 * @param numOfChoices size of the array.
 * @param selectedIndex index of the element that is selected.
 * @param fitPolicy the text wrapping policy, for example, TEXT_WRAP_ON if
 *                  wrapping is enabled.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_choicegroup_create(MidpItem* cgPtr,
				     MidpDisplayable* ownerPtr,
				     const pcsl_string* label, int layout,
				     MidpComponentType choiceType,
				     MidpChoiceGroupElement* choices,
				     int numOfChoices,
				     int selectedIndex,
				     int fitPolicy){
    int i;
    GtkFrame *frame;
    GSList *group;
    gchar label_buf[MAX_TEXT_LENGTH];
    PangoFontDescription* font_descr;
    GtkWidget *button;
    GtkWidget *box;
    GtkWidget *vbox;
    GtkWidget *form;
    int label_len;


    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("%s type=%d\n", __FUNCTION__, choiceType);

    // do arguments check here
    pcsl_string_convert_to_utf8(label, label_buf, MAX_TEXT_LENGTH, &label_len);
    frame = gtk_frame_new(label_buf);
    box = gtk_vbox_new(FALSE, 0);

    if (choiceType == MIDP_MULTIPLE_CHOICE_GROUP_TYPE) {
        for (i = 0; i < numOfChoices; i++) {
            pcsl_string_convert_to_utf8(&choices[i].string, label_buf, MAX_TEXT_LENGTH, &label_len);
            button = gtk_check_button_new_with_label(label_buf);
            gtk_widget_modify_font(button, choices[i].font);
            gtk_toggle_button_set_active(button, choices[i].selected);
            gtk_box_pack_start(GTK_BOX (box), button, FALSE, FALSE, 0);
        }
        gtk_container_add(GTK_CONTAINER (frame), box);
    }
    else if (choiceType == MIDP_EXCLUSIVE_CHOICE_GROUP_TYPE) {
        if (numOfChoices > 0) {
            pcsl_string_convert_to_utf8(&choices[0].string, label_buf, MAX_TEXT_LENGTH, &label_len);
            button = gtk_radio_button_new_with_label(NULL, label_buf);
            gtk_widget_modify_font(button, choices[0].font);
            gtk_box_pack_start(GTK_BOX (box), button, FALSE, FALSE, 0);
            group = gtk_radio_button_get_group(button);
            for (i = 1; i < numOfChoices; i++) {
                //add group
                pcsl_string_convert_to_utf8(&choices[i].string, label_buf, MAX_TEXT_LENGTH, &label_len);
                button = gtk_radio_button_new_with_label(group, label_buf);
                group = gtk_radio_button_get_group(button);
                gtk_widget_modify_font(button, choices[i].font);
                gtk_box_pack_start(GTK_BOX (box), button, FALSE, FALSE, 0);
            }
            gtk_container_add(GTK_CONTAINER (frame), box);
        }
    }
    else if (choiceType == MIDP_POPUP_CHOICE_GROUP_TYPE) {
        LIMO_TRACE("%s choiceType MIDP_POPUP_CHOICE_GROUP_TYPE not implemented yet \n", __FUNCTION__);
        return -1;
    }
    else {
        LIMO_TRACE("%s unexpected choiceType %d \n", __FUNCTION__, choiceType);
        return -1;
    }

    form = ownerPtr->frame.widgetPtr;
    vbox = gtk_object_get_user_data(form);
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

    /* set font */
    cgPtr->widgetPtr = frame;
    cgPtr->ownerPtr = ownerPtr;
    cgPtr->layout = layout;

    cgPtr->show = lfpport_choicegroup_item_show_cb;
    cgPtr->hide = lfpport_choicegroup_item_hide_cb;
    cgPtr->setLabel = lfpport_choicegroup_item_set_label_cb;
    cgPtr->destroy = lfpport_choicegroup_item_destroy_cb;

    //cgPtr->component
    cgPtr->getMinimumHeight = lfpport_choicegroup_item_get_min_height_cb;
    cgPtr->getMinimumWidth = lfpport_choicegroup_item_get_min_width_cb;
    cgPtr->getPreferredHeight = lfpport_choicegroup_item_get_pref_height_cb;
    cgPtr->getPreferredWidth = lfpport_choicegroup_item_get_pref_width_cb;
    cgPtr->handleEvent = lfpport_choicegroup_item_handle_event_cb;
    cgPtr->relocate = lfpport_choicegroup_item_relocate_cb;
    cgPtr->resize = lfpport_choicegroup_item_resize_cb;

    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

/**
 * Notifies the native peer that the given element was inserted into the
 * choice group at the specified position.
 *
 * @param cgPtr pointer to the choice group's <tt>MidpItem</tt> structure.
 * @param elementNum index of the inserted element.
 * @param element the element to be inserted.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_choicegroup_insert(MidpItem* cgPtr,
				     int elementNum,
				     MidpChoiceGroupElement element){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}

/**
 * Notifies the native peer that the element at the given position
 * was deleted from the choice group.
 *
 * @param cgPtr pointer to the choice group's <tt>MidpItem</tt> structure.
 * @param elementNum index of the deleted element.
 * @param selectedIndex index of the newly selected element in choice group.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_choicegroup_delete(MidpItem* cgPtr, int elementNum,
				     int selectedIndex){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}

/**
 * Notifies the native peer that all of the elements were deleted from the
 * choice group.
 *
 * @param cgPtr pointer to the choice group's <tt>MidpItem</tt> structure.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_choicegroup_delete_all(MidpItem* cgPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}

/**
 * Notifies the native peer that the element at the given position in the
 * choice group has been replaced by the given element.
 *
 * @param cgPtr pointer to the choice group's <tt>MidpItem</tt> structure.
 * @param elementNum the index of the reset element.
 * @param element the new element that replaces the element at elementNum.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_choicegroup_set(MidpItem* cgPtr,
				  int elementNum,
				  MidpChoiceGroupElement element){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}

/**
 * Notifies the native peer that the element at the given position
 * was selected or deselected in the choice group.
 *
 * @param cgPtr pointer to the choice group's <tt>MidpItem</tt> structure.
 * @param elementNum index of the selected or deselected element.
 * @param selected the new state of the element: true means that
 *        the element is now selected, and false means it is deselected.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_choicegroup_set_selected_index(MidpItem* cgPtr,
						 int elementNum,
						 jboolean selected){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}

/**
 * Gets the index of the native peer's currently selected element.
 *
 * @param elementNum pointer to the index of the selected
 *        element. This function sets elementNum's value.
 * @param cgPtr pointer to the choice group's <tt>MidpItem</tt> structure.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_choicegroup_get_selected_index(int* elementNum,
						 MidpItem* cgPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}

/**
 * Notifies the native peer that the selected state of several
 * elements was changed. (The choice group must be <code>MULTIPLE</code>.)
 *
 * @param cgPtr pointer to the choice group's <tt>MidpItem</tt> structure.
 * @param selectedArray array of indexes of the elements that have
 *        a changed selection status.
 * @param selectedArrayNum the number of elements in selectedArray.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_choicegroup_set_selected_flags(MidpItem* cgPtr,
						 jboolean* selectedArray,
						 int selectedArrayNum){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}

/**
 * Gets the indexes of the native peer's selected elements.
 *
 * @param numSelected pointer to the number of elements
 *        in selectedArray_return. This function sets numSelected's value.
 * @param cgPtr pointer to the choice group's <tt>MidpItem</tt> structure.
 * @param selectedArray_return array in which this function places
 *        the indexes of the selected elements.
 * @param selectedArrayLength the size of selectedArray_return.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_choicegroup_get_selected_flags(int *numSelected,
						 MidpItem* cgPtr,
					     jboolean* selectedArray_return,
						 int selectedArrayLength){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}

/**
 * Tests whether the element at the given position in the choice group
 * is selected.
 *
 * @param selected pointer to true if the element is selected,
 *        false otherwise. This function sets selected's value.
 * @param cgPtr pointer to the choice group's <tt>MidpItem</tt> structure.
 * @param elementNum the index of the element to be tested.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_choicegroup_is_selected(jboolean *selected, MidpItem* cgPtr,
					  int elementNum){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}

/**
 * Notifies the native peer that the choice group has a new text-fit policy
 * (The definitions of the fit policies are in the <i>MIDP
 * Specification</i>.)
 *
 * @param cgPtr pointer to the choice group's <tt>MidpItem</tt> structure.
 * @param fitPolicy preferred fit-policy for the choice group's elements.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_choicegroup_set_fit_policy(MidpItem* cgPtr, int fitPolicy){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}

/**
 * Notifies the native peer that the element at the given index has been
 * updated to have the given font.
 *
 * @param cgPtr pointer to the choice group's <tt>MidpItem</tt> structure.
 * @param elementNum the index of the element for which the font is being set.
 * @param fontPtr the preferred font to be used to render the element.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_choicegroup_set_font(MidpItem* cgPtr,
				       int elementNum,
				       PlatformFontPtr fontPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}

/**
 * Dismisses a choice group popup.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_choicegroup_dismiss_popup(){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

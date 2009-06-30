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

#ifndef _LFPPORT_CHOICEGROUP_H_
#define _LFPPORT_CHOICEGROUP_H_

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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Information associated with the element of a choice group.
 */
typedef struct {
  /**
   * Text to be displayed in the choicegroup element.
   */
  pcsl_string string;

  /**
   * Image to be displayed in the choicegroup element.
   */
  unsigned char* image;
  /**
   * Indicates whether the choicegroup element is selected or not.
   */
  jboolean selected;

  /**
   * Font to show the text of the choicegroup element.
   */
  PlatformFontPtr font;
} MidpChoiceGroupElement;

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
				     int fitPolicy);

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
				     MidpChoiceGroupElement element);

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
				     int selectedIndex);

/**
 * Notifies the native peer that all of the elements were deleted from the
 * choice group.
 *
 * @param cgPtr pointer to the choice group's <tt>MidpItem</tt> structure.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_choicegroup_delete_all(MidpItem* cgPtr);

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
				  MidpChoiceGroupElement element);

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
						 jboolean selected);

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
						 MidpItem* cgPtr);

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
						 int selectedArrayNum);

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
						 int selectedArrayLength);

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
					  int elementNum);

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
MidpError lfpport_choicegroup_set_fit_policy(MidpItem* cgPtr, int fitPolicy);

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
				       PlatformFontPtr fontPtr);

/**
 * Dismisses a choice group popup.
 * 
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_choicegroup_dismiss_popup();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LFPPORT_CHOICEGROUP_H_ */

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

#ifndef _JSR120_LIST_ELEMENT_H
#define _JSR120_LIST_ELEMENT_H

/**
 * @file
 * @defgroup listelement List Interface
 * @ingroup wma
 * @brief General purpose list porting interface. \n
 * ##include <kni.h>
 * @{
 *
 * This file defines the data structure associated with a list, in addition to
 * various porting interfaces for creating, destroying and manipulating that
 * list.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <kni.h>
#include <app_package.h>

/**
 * ListElement data structure.
 */
typedef struct _list_element {

    /* Two different forms of identifiers associated with a list element. */

    /* Numeric identifier (Port number for SMS; Message identifier for CBS.). */
    jint id;

    /* String identifier (Application ID for MMS.). */
    unsigned char* strid;

    /* Midlet suite ID */
    AppIdType msid;

    /* Pointer to any user data */
    void* userData;

    /* Pointer to user callback function. */
    void* userDataCallback;

    /*
     * Registration flag. <code>true</code> when the element is registered;
     * <code>false</code>, otherwise. This flag is a special addition to what
     * would otherwise be a normal list element. Registration has little to do
     * with messages, but more to do with registered IDs for listeners.
     */
    jint isRegistered;

    /* The new message flag. */
    jint isNewMessage;

    /* Pointer to next item in linked list */
    struct _list_element* next;

} ListElement;

/*
 * Helper methods to create new list elements.
 *
 * jsr120_list_new_by_number  Creates a new list element with numeric identifier.
 * jsr120_list_new_by_name    Creates a new list element with string identifier.
 *
 */

/**
 * Create a new list element with a numeric identifier.
 *
 * @param head A pointer to the first element in the list.
 * @param num The number that identifies this element.
 * @param msid The MIDlet suite identifier.
 * @param userData  Anything the user needs to attach to this element.
 * @param userDataCallBack  A user-defined callback.
 *
 * @return A pointer to the element that was created.
 */
ListElement* jsr120_list_new_by_number(ListElement** head, jint num,
    AppIdType msid, void* userData, void* userDataCallback);

/**
 * Create a new list element with a string identifier.
 *
 * @param head A pointer to the first element in the list.
 * @param name The name which identifies this element. The name is duplicated.
 * @param msid The MIDlet suite identifier.
 * @param userData  Anything the user needs to attach to this element.
 * @param userDataCallBack  A user-defined callback.
 *
 * @return A pointer to the element that was created.
 */
ListElement* jsr120_list_new_by_name(ListElement** head, unsigned char* name,
    AppIdType msid, void* userData, void* userDataCallback);

/*
 * ListElement methods
 *
 * jsr120_list_add_first
 *     Add an element to the front of the list.
 *
 * jsr120_list_add_last
 *     Add an element to the end of the list.
 *
 * jsr120_list_remove_first
 *     Remove the first element in the list.
 *
 * jsr120_list_remove_first_by_number
 *     Remove the first element matching the numeric identifier.
 *
 * jsr120_list_remove_first_by_name
 *     Remove the first element matching the string identifier.
 *
 * jsr120_list_remove_first_by_msID
 *     Remove the first element matching the MIDlet suite identifier.
 *
 * jsr120_list_get_by_number
 *     Get the first element matching the numeric identifier.
 *
 * jsr120_list_get_by_name
 *     Get the first element matcdhing the string identifier.
 *
 * jsr120_list_destroy
 *     Destroy the list and free all memory.
 */

/**
 * Add a list element to the top.
 *
 * @param head A pointer to the first element in the list.
 * @param item The item to be inserted as the first element in the list.
 */
void jsr120_list_add_first(ListElement** head, ListElement* item);

/**
 * Add a list element to the bottom.
 *
 * @param head A pointer to the first element in the list.
 * @param item The item to be appended as the last element in the list.
 */
void jsr120_list_add_last(ListElement** head, ListElement* item);

/**
 * Remove the first element in the list.
 *
 * @param head A pointer to the first element in the list.
 *
 * @return A pointer to the element that was removed.
 */
ListElement* jsr120_list_remove_first(ListElement** head);

/**
 * Remove the first item in the list that matches a given numeric identifier.
 *
 * @param head A pointer to the first element in the list.
 * @param num The number identifier to be matched.
 *
 * @return A pointer to the element that was removed.
 */
ListElement* jsr120_list_remove_first_by_number(ListElement** head, jint num);

/**
 * Remove the first item in the list that matches a given name identifier.
 *
 * @param head A pointer to the first element in the list.
 * @param name The name identifier to be matched.
 *
 * @return A pointer to the element that was removed.
 */
ListElement* jsr120_list_remove_first_by_name(ListElement** head, unsigned char* name);

/**
 * Remove the first element of the list that matches the given MIDlet suite
 * identifier.
 *
 * @param head A pointer to the first element in the list.
 * @param msid The MIDlet suite identifier to be matched.
 *
 * @return A pointer to the element that was removed.
 */
ListElement* jsr120_list_remove_first_by_msID(ListElement** head,
    AppIdType msid);

/**
 * Retrieve the element matching the given MIDlet suite identifier
 *
 * @param head A pointer to the first element in the list.
 * @param msid The MIDlet suite identifier to be matched.
 *
 * @return  A pointer to the element that matched the MIDlet suite 
 *  identifier, or <code>NULL</code> if no element could be found.
 */
ListElement* jsr120_list_get_first_by_msID(ListElement* head, AppIdType msid);

/**
 * Retrieve the element matching the numeric identifier.
 *
 * @param head A pointer to the first element in the list.
 * @param id The numeric identifier to be matched.
 *
 * @return  A pointer to the element that matched the number, or
 *     <code>NULL</code> if no element could be found.
 */
ListElement* jsr120_list_get_by_number(ListElement* head, jint id);

/**
 * Retrieve the element matching the string identifier.
 *
 * @param head A pointer to the first element in the list.
 * @param name The name identifier to be matched.
 *
 * @return  A pointer to the element that matched the name, or
 *     <code>NULL</code> if no element could be found.
 */
ListElement* jsr120_list_get_by_name(ListElement* head, unsigned char* id);

/**
 * Retrieve the element matching the numeric identifier.
 *
 * @param head A pointer to the first element in the list.
 * @param id The numeric identifier to be matched.
 * @param isNew Get the new message only when not 0.
 *
 * @return  A pointer to the element that matched the number, or
 *	<code>NULL</code> if no element could be found.
 */
ListElement* jsr120_list_get_by_number1(ListElement* head, jint num, jint isNew);

/**
 * Retrieve the element matching the string identifier.
 *
 * @param head A pointer to the first element in the list.
 * @param name The name identifier to be matched.
 * @param isNew Get the new message only when not 0.
 *
 * @return  A pointer to the element that matched the name, or
 *	<code>NULL</code> if no element could be found.
 */
ListElement* jsr120_list_get_by_name1(ListElement* head, unsigned char* name,
    jint isNew);

/**
 * Deconstruct the entire list and free all memory.
 *
 * @param head The first element of the list to be deconstructed.
 */
void jsr120_list_destroy(ListElement* head);

/*
 * Special list methods
 *
 * The following methods deal with list elements related to listeners. When an
 * element is added to the list, it defaults to being registered. That is, the
 * numeric or string identifier and callback that must match incoming messages
 * become a registered pair in the list. These methods are used to seek out
 * specific numeric or string identifiers and callbacks so that those elements
 * can be removed from the list.
 *
 * jsr120_list_unregister_by_number
 *     Unregister all elements matching both the numeric identifier and callback.
 *
 * jsr120_list_unregister_by_name
 *     Unregister all elements matching both the string identifier and callback.
 */

/**
 * Unregister all elements in the list that match the numeric identifier as well
 * as the callback. If there is no match, no action is taken.
 *
 * @param head A pointer to the first element in the list.
 * @param num The numeric identifier to be matched.
 * @param userDataCallback The callback function to be matched.
 */
void jsr120_list_unregister_by_number(ListElement** head, jint num,
    void* userDataCallback);

/**
 * Unregister all elements in the list that match the string identifier as well
 * as the callback. If there is no match, no action is taken.
 *
 * @param head A pointer to the first element in the list.
 * @param name The string identifier to be matched.
 * @param userDataCallback The callback function to be matched.
 */
void jsr120_list_unregister_by_name(ListElement** head, unsigned char* name,
    void* userDataCallback);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ifndef _JSR120_LIST_ELEMENT_H */


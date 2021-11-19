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

#include <string.h>
#include <jsr120_list_element.h>
#ifndef ENABLE_PCSL
  #define pcsl_mem_malloc malloc
  #define pcsl_mem_free free
  #define pcsl_mem_strdup(x) strcpy(malloc(strlen(x)+1), x)
#else
  #include <pcsl_memory.h>
#endif
#include <app_package.h>

#ifndef NULL
#define NULL 0
#endif


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
 * @param head	A pointer to the first element in the list.
 * @param num The number that identifies this element.
 * @param msid The MIDlet suite identifier.
 * @param userData  Anything the user needs to attach to this element.
 * @param userDataCallBack  A user-defined callback.
 *
 * @return A pointer to the element that was created.
 */
ListElement* jsr120_list_new_by_number(ListElement** head, jint num,
    AppIdType msid, void* userData, void* userDataCallback) {

    ListElement* q      = (ListElement*) pcsl_mem_malloc(sizeof(ListElement));
    q->next             = NULL;
    q->id               = num;
    q->msid         = msid;
    q->strid = NULL;
    q->userData = userData;
    q->userDataCallback = userDataCallback;
    q->isRegistered = 1;
    q->isNewMessage = 1;
    jsr120_list_add_first(head, q);

    return q;
}

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
    AppIdType msid, void* userData, void* userDataCallback) {

    ListElement* q = (ListElement*) pcsl_mem_malloc(sizeof(ListElement));
    q->next = NULL;
    q->id = -1;
    /* Duplicate the name. */
    if (name != NULL) {
        q->strid = (unsigned char*)pcsl_mem_strdup((char*)name);
    } else {
        q->strid = NULL;
    }
    q->msid = msid;
    q->userData = userData;
    q->userDataCallback = userDataCallback;
    q->isRegistered = 1;
    q->isNewMessage = 1;
    jsr120_list_add_first(head, q);
    return q;
}

/*
 * ListElement methods
 *
 * jsr120_list_add_first
 *	Add an element to the front of the list.
 *
 * jsr120_list_add_last
 *	Add an element to the end of the list.
 *
 * jsr120_list_remove_first
 *	Remove the first element in the list.
 *
 * jsr120_list_remove_first_by_number
 *	Remove the first element matching the numeric identifier.
 *
 * jsr120_list_remove_first_by_name
 *	Remove the first element matching the string identifier.
 *
 * jsr120_list_remove_first_by_msID
 *	Remove the first element matching the MIDlet suite identifier.
 *
 * jsr120_list_get_by_number
 *	Get the first element matching the numeric identifier.
 *
 * jsr120_list_get_by_name
 *	Get the first element matcdhing the string identifier.
 *
 * jsr120_list_destroy
 *	Destroy the list and free all memory.
 */

/**
 * Add a list element to the top.
 *
 * @param head A pointer to the first element in the list.
 * @param item The item to be inserted as the first element in the list.
 */
void jsr120_list_add_first(ListElement** head, ListElement* item) {
    if (head == NULL) {
        return;
    }
    item->next = *head;
    *head = item;
}

/**
 * Add a list element to the bottom.
 *
 * @param head	The list.
 * @param item The list item to be appended as the last item in the list.
 */
void jsr120_list_add_last(ListElement** head, ListElement* newItem) {
    if (head == NULL) {
        return;
    }
    for ( ; (*head) != NULL; head = &((*head)->next)) {
    }
    (*head) = newItem;
}

/**
 * Remove the first element in the list.
 *
 * @param head A pointer to the first element in the list.
 *
 * @return A pointer to the element that was removed.
 */
ListElement* jsr120_list_remove_first(ListElement** head) {
    ListElement* result;
    if (head == NULL) {
        return NULL;
     }
    if (*head == NULL) {
        return NULL;
    }
    result = *head;
    *head = result->next;
    result->next = NULL;
    return result;
}

/**
 * Remove the first item in the list that matches a given numeric identifier.
 *
 * @param head A pointer to the first element in the list.
 * @param num The number identifier to be matched.
 *
 * @return A pointer to the element that was removed.
 */
ListElement* jsr120_list_remove_first_by_number(ListElement** head, jint num) {

    ListElement* next;

    if (head == NULL) {
        return NULL;
    }

    for (next = *head; next != NULL; head = &(next->next),
        next = next->next) {

        if ((next) && (next->id == num)) {
            *head = next->next;
            next->next = NULL;
            return next;
        }
    }

    return NULL;
}

/**
 * Remove the first item in the list that matches a given name identifier.
 *
 * @param head A pointer to the first element in the list.
 * @param name The name identifier to be matched.
 *
 * @return A pointer to the element that was removed.
 */
ListElement* jsr120_list_remove_first_by_name(ListElement** head, unsigned char* name) {
    ListElement* next;
    if (head == NULL) {
        return NULL;
    }

    for (next = *head; next != NULL; head = &(next->next), next=next->next) {
        if (name && next && next->strid && (strcmp((char*)next->strid, (char*)name) == 0)) {
            *head = next->next;
            next->next = NULL;
            return next;
        }
    }

    /* No match. */
    return NULL;
}

/**
 * Remove the first element of the list that matches the given MIDlet suite
 * identifier.
 *
 * @param head A pointer to the first element in the list.
 * @param msid The MIDlet suite identifier to be matched.
 *
 * @return A pointer to the element that was removed.
 */
ListElement* jsr120_list_remove_first_by_msID(ListElement** head, AppIdType msid) {
    ListElement* next;

    if (head == NULL) {
        return NULL;
    }

    for (next = *head; next != NULL; head = &(next->next), next=next->next) {
	if ((msid != UNUSED_APP_ID) && next && next->msid &&
                (next->msid == msid)) {
            *head = next->next;
            next->next = NULL;
            return next;
	}
    }
    return NULL;
}

/**
 * Retrieve the element matching the given MIDlet suite identifier
 *
 * @param head A pointer to the first element in the list.
 * @param msid The MIDlet suite identifier to be matched.
 *
 * @return  A pointer to the element that matched the MIDlet suite 
 *  identifier, or <code>NULL</code> if no element could be found.
 */
ListElement* jsr120_list_get_first_by_msID(ListElement* head, AppIdType msid) {

    for ( ; head != NULL ; head = head->next) {
        if (head->msid == msid && head->isRegistered) {
            return head;
        }
    }

    /* No match. */
    return NULL;
}

/**
 * Retrieve the element matching the numeric identifier.
 *
 * @param head A pointer to the first element in the list.
 * @param id The numeric identifier to be matched.
 *
 * @return  A pointer to the element that matched the number, or
 *	<code>NULL</code> if no element could be found.
 */
ListElement* jsr120_list_get_by_number(ListElement* head, jint num) {
    return jsr120_list_get_by_number1(head, num, 0);
}

/**
 * Retrieves the element matching the numeric identifier.
 *
 * @param head A pointer to the first element in the list.
 * @param id The numeric identifier to be matched.
 * @param isNew Get the new message only when not 0.
 *
 * @return  A pointer to the element that matched the number, or
 *	<code>NULL</code> if no element could be found.
 * The message is flagged as no longer new after it is
 * reported the first time.
 */
ListElement* jsr120_list_get_by_number1(ListElement* head, jint num, jint isNew) {

    for ( ; head != NULL ; head = head->next) {
        if (head->id == num && head->isRegistered && (!isNew || head->isNewMessage)) {
            head->isNewMessage = 0; /* Message is not new */
            return head;
        }
    }

    /* No match. */
    return NULL;
}

/**
 * Retrieve the element matching the string identifier.
 *
 * @param head A pointer to the first element in the list.
 * @param name The name identifier to be matched.
 *
 * @return  A pointer to the element that matched the name, or
 *	<code>NULL</code> if no element could be found.
 */
ListElement* jsr120_list_get_by_name(ListElement* head, unsigned char* name) {
    return jsr120_list_get_by_name1(head, name, 0);
}

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
    jint isNew) {

    if (name == NULL) {
        return NULL;
    }

    for ( ; head != NULL; head = head->next) {
        if ((strcmp((char*)head->strid, (char*)name) == 0) && head->isRegistered &&
            (!isNew || head->isNewMessage)) {
            head->isNewMessage = 0; /* Message is not new */
            return head;
        }
    }

    /* No match. */
    return NULL;
}

/**
 * Deconstruct the entire list and free all memory.
 *
 * @param head The first element of the list to be deconstructed.
 */
void jsr120_list_destroy(ListElement* head) {

    /* If the list doesn't exist, exit now. */
    if (head == NULL) {
        return;
    }

    /* If the next element exists, recursively destroy. */
    if (head->next) {
        jsr120_list_destroy(head->next);
    }

    /* Release any string ID memory. */
    pcsl_mem_free(head->strid);

    /* Release the memory required by the list element. */
    pcsl_mem_free(head);
}

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
 *	Unregister all elements matching both the numeric identifier and callback.
 *
 * jsr120_list_unregister_by_name
 *	Unregister all elements matching both the string identifier and callback.
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
    void* userDataCallback) {

    ListElement* next;

    if (head == NULL) {
        return;
    }

    for (next = *head; next != NULL; head = &(next->next), next=next->next) {

        if (next && (next->id == num) &&
            (next->userDataCallback == userDataCallback)) {

            /* There is a match, so clear the flag. */
            next->isRegistered = 0;
            return;
        }
    }
}

/**
 * Unregister all elements in the list that match the string identifier as well
 * as the callback. If there is no match, no action is taken.
 *
 * @param head A pointer to the first element in the list.
 * @param nameThe string identifier to be matched.
 * @param userDataCallback The callback function to be matched.
 */
void jsr120_list_unregister_by_name(ListElement** head, unsigned char* name,
    void* userDataCallback) {

    ListElement* next;

    if ((head == NULL) || (name == NULL)) {
        return;
    }

    for (next = *head; next != NULL; head = &(next->next), next=next->next) {

        if (next && next->strid && (strcmp((char*)next->strid, (char*)name) == 0) &&
            (next->userDataCallback == userDataCallback)) {

            /* There is a match, so clear the flag. */
            next->isRegistered = 0;
            return;
        }
    }
}


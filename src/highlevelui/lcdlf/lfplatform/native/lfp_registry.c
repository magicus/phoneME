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

/**
 * @file
 *
 * Implementation of UI Component Registry.
 */

#include <kni.h>

#include "lfp_intern_registry.h"

#include <lfpport_displayable.h>
#include <lfpport_item.h>
#include <midpError.h>
#include <midpMalloc.h>
#include <midpString.h>

/**
 * Global variable pointing to current visible screen. This screen receives
 * all the user events. It could be either a displayable, or a system dialog
 * like menu.
 * Declared in midp_lcdui.h.
 */
MidpFrame* MidpCurrentScreen;

/**
 * Beginning of a linked list of all MidpFrame structures.
 */
static MidpFrame* MidpFirstScreen;

/**
 * Beginning of a linked list of all MidpItem structures without owner screen.
 */
static MidpItem* MidpFirstOrphanItem;

/**
 * Create a component resource structure for a Displayable.
 * The MidpComponent structure portion will be initialized as:
 * <ul>
 *	<li>type = type argument</li>
 *	<li>modelVersion = 0</li>
 *	<li>next = NULL</li>
 *	<li>child = NULL</li>
 * </ul>
 *
 * The rest of MidpDisplayable structure remains un-initialized. Platform
 * specific layer should populate these remaining data fields before use.
 * 
 *
 * @param type component type of new resource
 * @return pointer to the newly created MidpDisplayable structure,
 * 	   null if failed.
 */
MidpDisplayable* MidpNewDisplayable(MidpComponentType type) {
    MidpDisplayable* p = (MidpDisplayable *)midpMalloc(sizeof(MidpDisplayable));
    
    if (p) {
        p->frame.component.type = type;
        p->frame.component.modelVersion = 0;
        p->frame.component.next = (MidpComponent *)MidpFirstScreen;
        p->frame.component.child = NULL;
        MidpFirstScreen = (MidpFrame *)p;

        /*
        * The rest of the structure is not yet initialized.
        * If MidpDeleteDisplayable() is called on this pointer before
        * they are initialized, this flag will help preventing calling
        * hideAndDelete() unnecessarily
        */
        p->frame.widgetPtr = NULL;
    }

    return p;
}

/**
 * Delete all native resource of a Displayable.
 * Resources that will be freed are:
 * <ul>
 * 	<li>For each child Item: its platform dependent resource
 * 	<li>For each child Item: MidpItem structure
 * 	<li>Platform dependent resource
 * 	<li>MidpDisplayable structure
 * </ul>
 *
 * @param displayablePtr pointer to the MidpDisplayable structure
 */
void MidpDeleteDisplayable(MidpDisplayable *displayablePtr) {
    MidpComponent *p, *c;

    if (displayablePtr == NULL) {
        return;
    }

    /* If this displayable is current screen, clear current screen pointer */
    if (MidpCurrentScreen == &displayablePtr->frame) {
        MidpCurrentScreen = NULL;
    }

    /* First Delete all children */
    while (displayablePtr->frame.component.child != NULL) {
        MidpDeleteItem((MidpItem *)displayablePtr->frame.component.child);
    }

    /* Then detach this displayable from displayable linked list */
    if (MidpFirstScreen == (MidpFrame *)displayablePtr ||
        MidpFirstScreen == NULL) {
        MidpFirstScreen = (MidpFrame *)displayablePtr->frame.component.next;
    } else {
        p = (MidpComponent *)MidpFirstScreen;
        c = p->next;
        while (c != NULL) {
            if (c == (MidpComponent *)displayablePtr) {
                p->next = c->next;
                break;
            } else {
                p = c;
                c = c->next;
            }
        }
    }

    /* Next destroy platform dependent resource */
    if (displayablePtr->frame.widgetPtr) {
        displayablePtr->frame.hideAndDelete(&displayablePtr->frame, KNI_FALSE);
    }
    
    /* Last free the structure */
    midpFree(displayablePtr);
}

/**
 * Create a component resource structure for an Item.
 * The MidpComponent structure portion will be initialized as:
 * <ul>
 *	<li>type = type argument</li>
 *	<li>modelVersion = 0</li>
 *	<li>next = NULL</li>
 *	<li>child = NULL</li>
 * </ul>
 *
 * The rest of MidpItem structure remains un-initialized. Platform
 * specific layer should populate these remaining data fields before use.
 *
 * @param ownerPtr owner screen pointer, null if no owner
 * @param type component type of new resource
 * @return pointer to the newly created MidpItem structure,
 * 	   null if failed.
 */
MidpItem* MidpNewItem(MidpDisplayable *ownerPtr, MidpComponentType type) {
    MidpItem *p = (MidpItem *)midpMalloc(sizeof(MidpItem));

    if (p) {
	p->component.type = type;
	p->component.modelVersion = 0;
	p->component.child = NULL;

	if (ownerPtr == NULL) {
	    p->component.next = (MidpComponent *)MidpFirstOrphanItem;
	    MidpFirstOrphanItem = p;
	} else {
	    p->component.next = ownerPtr->frame.component.child;
	    ownerPtr->frame.component.child = (MidpComponent *)p;
	}

	p->ownerPtr = ownerPtr;

	/*
	 * The rest of the structure is not yet initialized.
	 * If MidpDeleteItem() is called on this pointer before
	 * they are initialized, this flag will help preventing calling
	 * destroy() unnecessarily
	 */
	p->widgetPtr = NULL;
    }

    return p;
}

/**
 * Delete all native resource of an Item.
 * Resources that will be freed are:
 * <ul>
 * 	<li>Platform dependent resource
 * 	<li>MidpItem structure
 * </ul>
 *
 * @param itemPtr pointer to the MidpItem structure
 */
void MidpDeleteItem(MidpItem *itemPtr) {
    MidpComponent *p, *c;
    
    if (itemPtr == NULL) {
        return;
    }

    /* First detach this item from its owner's children list */
    if (itemPtr->ownerPtr == NULL) {
        p = (MidpComponent *)MidpFirstOrphanItem;
        if (p == (MidpComponent *)itemPtr || p == NULL) {
            MidpFirstOrphanItem = (MidpItem *)itemPtr->component.next;
            c = NULL;
        } else {
            c = p->next;
        }
    } else {
        p = itemPtr->ownerPtr->frame.component.child;
        if (p == (MidpComponent *)itemPtr || p == NULL) {
            itemPtr->ownerPtr->frame.component.child = itemPtr->component.next;
            c = NULL;
        } else {
            c = p->next;
        }
    }

    while (c != NULL) {
        if (c == (MidpComponent *)itemPtr) {
            p->next = c->next;
            break;
        } else {
            p = c;
            c = c->next;
        }
    }

    /* Then free all platform dependent resource */
    if (itemPtr->widgetPtr) {
        itemPtr->destroy(itemPtr);
    }

    /* Last free the MidpItem structure */
    midpFree(itemPtr);
}

/**
 * Map a platform widget pointer to its Item's structure pointer.
 * @param owner owner screen's structure pointer, null to search for orphan item
 * @param itemWidgetPtr platform widget pointer to be used as key
 * @return MidpItem* of its Item, null if not found.
 */
MidpItem* MidpFindItem(MidpDisplayable *ownerPtr,
		       PlatformItemWidgetPtr itemWidgetPtr) {
    MidpComponent *c;

    if (itemWidgetPtr == NULL) {
	return NULL;
    }

    c = (ownerPtr == NULL) ? (MidpComponent *)MidpFirstOrphanItem
			   : ownerPtr->frame.component.child;

    while (c != NULL) {
	if (((MidpItem *)c)->widgetPtr == itemWidgetPtr) {
	    return (MidpItem *)c;
	} else {
	    c = c->next;
	}
    }

    return NULL; /* no match */
}

/**
 * Delete all MIDP components when VM is exiting.
 */
void MidpDeleteAllComponents() {
    while (MidpFirstOrphanItem != NULL) {
        MidpDeleteItem(MidpFirstOrphanItem);
    }
}

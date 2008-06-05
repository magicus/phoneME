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
 * Cross platform functions for Form.
 */

#include <kni.h>

#include <midpError.h>
#include <midpMalloc.h>
#include <midpEvents.h>
#include <midpEventUtil.h>
#include <midpUtilKni.h>

#include <lfpport_form.h>
#include <lfp_registry.h>
#include "lfp_intern_registry.h"

/**
 * Sub types of peer notification events
 *
 */
enum {
    PEER_FOCUS_CHANGED  = 0,
    PEER_VIEWPORT_CHANGED  = 1,
    PEER_ITEM_CHANGED  = 2,
    PEER_TRAVERSE_REQUEST  = 3
};

/**
 * KNI function that makes native form visible.
 * CLASS:         javax.microedition.lcdui.FormLFImpl
 * Java Prototype: int createNativeResource0(String title,
 * 	                                    String tickerText)
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_FormLFImpl_createNativeResource0() {

    MidpError err = KNI_OK;
    MidpDisplayable *formPtr = NULL;

    KNI_StartHandles(2);

    GET_PARAMETER_AS_PCSL_STRING(1, title)
    GET_PARAMETER_AS_PCSL_STRING(2, tickerText)
    /* NULL and empty strings are acceptable. */

	formPtr = MidpNewDisplayable(MIDP_FORM_TYPE);
	if (formPtr == NULL) {
	    err = KNI_ENOMEM;
	} else {
	    /* Fill in platform dependent portion of form structure */
	    err = lfpport_form_create((MidpDisplayable *)formPtr,
				      &title, &tickerText);
	    /*
	      IMPL_NOTE: Move this to midp_displayable.c
	      extern C in mscreen.cpp:
	    */
	   /* pdSetTicker(&tickerText);*/
	    formPtr->setTicker(formPtr, &tickerText);
	}

    RELEASE_PCSL_STRING_PARAMETER
    RELEASE_PCSL_STRING_PARAMETER
    KNI_EndHandles();

    if (err != KNI_OK) {
        if (formPtr != NULL) {
            MidpDeleteDisplayable(formPtr);
        }
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_ReturnInt(formPtr);
}

/**
 * KNI function that makes native form visible.
 * Java Prototype: void showNativeResource0(int nativeId,
 * 					   int modelVersion,
 *                                         int w, int h) 
 * CLASS:         javax.microedition.lcdui.FormLFImpl
 * Param: nativeId Id of previously created Form native resource
 * Param: modelVersion version id of the data passed in from Java
 * Param: w width of all form content
 * Param: h height of all form content
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_FormLFImpl_showNativeResource0() {
    int w, h;
    MidpError err;
    MidpDisplayable *formPtr = (MidpDisplayable *)KNI_GetParameterAsInt(1);
    
    /* Initialize data model version for this new visible period */
    formPtr->frame.component.modelVersion = KNI_GetParameterAsInt(2);
    w = KNI_GetParameterAsInt(3);
    h = KNI_GetParameterAsInt(4);

    /* Set form window virtual size */
    err = lfpport_form_set_content_size(formPtr, w, h);
    if (err != KNI_OK) goto cleanup;

    if (MidpCurrentScreen != &formPtr->frame) {
	/* Becomes the new current screen that receives events */
	MidpCurrentScreen = &formPtr->frame;
    
	/*
	 * NOTE: MidpCurrentScreen is needed before it is shown because
	 * on show its children may want to notify their appearance.
	 */

	/* Show Form window */
	err = formPtr->frame.show(&formPtr->frame);
    }

cleanup:
    if (err != KNI_OK) {
	KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_ReturnVoid();
}

/**
 * KNI function that scroll Form to show and focus on an Item.
 * Java Prototype: void setCurrentItem0(int nativeId, int itemId, int yOffset)
 * CLASS:         javax.microedition.lcdui.FormLFImpl
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_FormLFImpl_setCurrentItem0() {

    int yOffset;
    MidpError err;

    MidpItem* itemPtr = (MidpItem *)KNI_GetParameterAsInt(2);
    yOffset = KNI_GetParameterAsInt(3);

    err = lfpport_form_set_current_item(itemPtr, yOffset);

    if (err != KNI_OK) {
	KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_ReturnVoid();
}

/*  private static native int getScrollPosition0 () ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_FormLFImpl_getScrollPosition0() {

  int pos = 0;
  
  MidpError err = lfpport_form_get_scroll_position(&pos);
  
  if (err != KNI_OK) {
    KNI_ThrowNew(midpOutOfMemoryError, NULL);
  }
  
  KNI_ReturnInt(pos);
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_FormLFImpl_setScrollPosition0() {

  int pos;
  MidpError err;

  pos = KNI_GetParameterAsInt(1);
  
  err = lfpport_form_set_scroll_position(pos);
  
  if (err != KNI_OK) {
    KNI_ThrowNew(midpOutOfMemoryError, NULL);
  }
  
  KNI_ReturnVoid();
}

/*  private static native int getViewportHeight0 ( ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_FormLFImpl_getViewportHeight0() {

  int height = 0;
  
  MidpError err = lfpport_form_get_viewport_height(&height);
  
  if (err != KNI_OK) {
    KNI_ThrowNew(midpOutOfMemoryError, NULL);
  }
  
  KNI_ReturnInt(height);
}

/**
 * Query whether current Form is running with only a single Item in it.
 * The Item's label handling could be different depending on this return value.
 *
 * This is NOT a platform dependent function and do NOT need to be ported.
 *
 * IMPL_NOTE:scan all MidpItem structures to find how many items this form has
 */
KNIEXPORT jboolean
MidpFormInSingleItemMode(MidpDisplayable* formPtr) {
    /* Not yet implemented */

    /* Suppress unused-parameter warning */
    (void)formPtr;

    return KNI_FALSE;
}

/**
 * Store a peer changed event to Java event queue.
 *
 * @param subType sub type of the event
 * @param compPtr pointer to the MidpComponent structure
 * @param hint some value up to Java peer to interpret
 */
static void
storePeerChangedEvent(int subType, MidpComponent *compPtr, int hint) {
    MidpEvent event;

    MIDP_EVENT_INITIALIZE(event);

    event.type = MIDP_PEER_CHANGED_EVENT;

    /* Will be compared to FormLFImpl.modelVersion */
    event.intParam1 = MidpCurrentScreen->component.modelVersion;
    /* Sub type of event */
    event.intParam2 = subType;
    /* Will be compared to FormLFImpl.nativeId or
     * to FormLFImpl.itemLFs[0..numOfLFs].nativeId */
    event.intParam3 = (int)compPtr;
    /* Some integer that is up to Java peer to interpret */
    event.intParam5 = hint;

    midpStoreEventAndSignalForeground(event);
}

/**
 * Notify Form Java peer that focus has changed to new Item.
 *
 * This is NOT a platform dependent function and do NOT need to be ported.
 *
 * @param itemWidgetPtr native widget of the item where focus is changed to.
 *	  null if no focus.
 */
void MidpFormFocusChanged(PlatformItemWidgetPtr itemWidgetPtr) {

    if (MidpCurrentScreen != NULL &&
        MidpCurrentScreen->component.type == MIDP_FORM_TYPE) {
    	/*
        compPtr is the pointer of the new focused item, which 
        can be NULL, which means no focused item.
        Hint is unused.
        */
        MidpItem *itemPtr = MidpFindItem((MidpDisplayable *)MidpCurrentScreen,
                                            itemWidgetPtr);
        storePeerChangedEvent(PEER_FOCUS_CHANGED, (MidpComponent *)itemPtr, 0);
    }
}

/**
 * Notify Java peer that one of its native items' state has changed.
 *
 * This is NOT a platform dependent function and do NOT need to be ported.
 *
 * @param itemWidgetPtr native widget of the item where state change occurred.
 * @param hint some value up to the item's Java peer to interpret
 */
void MidpFormItemPeerStateChanged(PlatformItemWidgetPtr itemWidgetPtr, int hint)
{

    if (MidpCurrentScreen != NULL &&
        MidpCurrentScreen->component.type == MIDP_FORM_TYPE) {

          /* Translate item's widgetPtr to its (MidpItem *) */
        MidpItem *itemPtr = MidpFindItem((MidpDisplayable *)MidpCurrentScreen,
            itemWidgetPtr);

        if (itemPtr != NULL) {
            storePeerChangedEvent(PEER_ITEM_CHANGED, (MidpComponent *)itemPtr, hint);
        }
    }
}

/**
 * Notify Java peer that viewport location or size has changed
 *
 * This is NOT a platform dependent function and do NOT need to be ported.
 *
 * @param formPtr native widget of the form where the change occurred
 * @param hint some value up to the form's Java peer to interpret
 */
void MidpFormViewportChanged(PlatformScreenWidgetPtr formPtr, int hint){

    if (MidpCurrentScreen != NULL &&
        MidpCurrentScreen->component.type == MIDP_FORM_TYPE &&
        formPtr == ((MidpFrame *)MidpCurrentScreen)->widgetPtr) 
    {
        storePeerChangedEvent(PEER_VIEWPORT_CHANGED, (MidpComponent *)MidpCurrentScreen, hint);
    }
}

/**
 * Notify Java peer that traverse is requested
 *
 * This is NOT a platform dependent function and do NOT need to be ported.
 *
 * @param formPtr native widget of the form where the request occurred
 * @param hint 1 if next item, 0 if previous
 */
void MidpFormTraverseRequest(PlatformScreenWidgetPtr formPtr, int hint){

    if (MidpCurrentScreen != NULL &&
        MidpCurrentScreen->component.type == MIDP_FORM_TYPE &&
        formPtr == ((MidpFrame *)MidpCurrentScreen)->widgetPtr) 
    {
        storePeerChangedEvent(PEER_TRAVERSE_REQUEST, (MidpComponent *)MidpCurrentScreen, hint);
    }
}


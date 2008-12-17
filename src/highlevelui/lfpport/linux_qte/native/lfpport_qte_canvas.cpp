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
 * 
 * This source file is specific for Qt-based configurations.
 */

/**
 * @file lfpport_qte_canvas.cpp
 *
 * Qt port of Canvas. Draw to MScreen's viewport widget directly.
 */
#include "lfpport_qte_util.h"
#include "lfpport_qte_mscreen.h"

#include <lfp_registry.h>
#include <lfpport_canvas.h>
#include "lfpport_qte_displayable.h"
#include <midpUtilKni.h>

/**
 * Show Canvas screen.
 *
 * @param screenPtr pointer to the Screen
 * @return status of this call
 */
extern "C" MidpError
canvas_show(MidpFrame* screenPtr) {

    ((QWidget *)screenPtr->widgetPtr)->show();
    return KNI_OK;
}

/**
 * Hide and delete resource function pointer.
 * This function should notify its Items to hide as well.
 *
 * @param screenPtr pointer to the Screen
 * @param onExit - true if this is called during VM exit.
 * 		   All native resource must be deleted in this case.
 * @return status of this call
 */
extern "C" MidpError
canvas_hide_and_delete(MidpFrame* screenPtr, jboolean onExit) {
    // Suppress unused parameter compilation warning
    (void)onExit;

    // Since MScreen is shared with other Displayables, 
    // do not delete it here. Simply reset it focus.
    ((QWidget *)screenPtr->widgetPtr)->clearFocus();
    return KNI_OK;
}

/**
 * Screen event handling function pointer.
 * Return true if the event has been handled and should not be further
 * dispatched.
 * If the event is:
 * - For a particular Item (identified by its pointer/id)
 *   Forward it to that Item's handleEvent() function.
 * - Ticker or scroll bar events
 *   Handle it locally.
 *
 * Qt Note: Qt uses SIGNAL/SLOT to deliver events. This function
 * is not used. Do nothing here.
 *
 * @param screenPtr pointer to the Canvas
 * @param eventPtr event to be processed
 * @return true if handled
 */
extern "C" jboolean
canvas_handle_event(MidpFrame* screenPtr, PlatformEventPtr eventPtr) {

    // Work around compiler warning
    (void)screenPtr;
    (void)eventPtr;

    return KNI_FALSE; // Do nothing
}


/**
 * Create a Canvas's native peer without showing yet.
 * Upon successful return, *canvasPtr will be filled properly.
 * Sound and abstract command buttons should not be handled in this
 * function. Separated function calls are used.
 *
 * @param canvasPtr pointer to MidpDisplayable structure to be filled in
 * @param title title string
 * @param tickerText ticker text
 * @param canvasType canvas type as defined in MidpComponentType
 * @return error code
 */
extern "C" MidpError
lfpport_canvas_create(MidpDisplayable* canvasPtr, const pcsl_string* title,
		      const pcsl_string* tickerText) {
  PlatformMScreen * mscreen = PlatformMScreen::getMScreen();

  /* Suppress unused-parameter warning */
  (void)tickerText;

    // Fill in MidpDisplayable structure
    canvasPtr->frame.widgetPtr	 = mscreen;
    canvasPtr->frame.show	 = canvas_show;
    canvasPtr->frame.hideAndDelete = canvas_hide_and_delete;
    canvasPtr->frame.handleEvent = canvas_handle_event;
    canvasPtr->setTitle		 = displayable_set_title;
    canvasPtr->setTicker	 = displayable_set_ticker;

    canvasPtr->setTitle(canvasPtr, title);
    /*canvasPtr->setTicker(canvasPtr, tickerText);*/

    // Give focus to mscreen so it can pass key/mouse events to Java
    mscreen->setFocus();

    // NOTE: if screen is in full mode different height will be set
    // later by MScreen::setFullScreenMode()
    mscreen->resizeContents(mscreen->getDisplayWidth(), 
                            mscreen->getScreenHeight());

    return KNI_OK;
}

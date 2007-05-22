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
 * @file lfpport_qte_form.cpp
 * Qt port of Form porting API.
 */

#include "lfpport_qte_mscreen.h"
#include "lfpport_qte_util.h"

#include <lfpport_form.h>
#include "lfpport_qte_displayable.h"
#include "lfpport_qte_item.h"
#include "lfpport_qte_form.h"
#include <moc_lfpport_qte_form.cpp>

/**
 * Form container constructor. This container is later
 * added as a child of the mscreen that is why it is
 * constructed with no parent. All items are later
 * added to this container. 
 */
Form::Form() : QWidget(0) {
  connect(PlatformMScreen::getMScreen(), SIGNAL(contentsMoving(int, int)),
	  this, SLOT(viewportChanged(int, int)));
}

/**
 * Finds a new widget to give the keyboard focus to, 
 * returns TRUE if is can find a new widget and FALSE if it can't
 *
 * Override to notify Java.
 */
bool Form::focusNextPrevChild ( bool next ) {

    MidpFormTraverseRequest(this, next ? 1 : 0);
    return FALSE;
}

/**
 * Form's slot that is called when viewport scroll
 * location changes.
 *
 * @param vpX the x coordinate of the new viewport scroll
 *            location
 * @param vpY the y coordinate of the new viewport scroll
 *            location
 */
void Form::viewportChanged(int vpX, int vpY) {
  /* suppress compiler warnings */
  (void)vpX;

  MidpFormViewportChanged(this, vpY);
}

/** Show function pointer */
extern "C" MidpError
form_show(MidpFrame* framePtr) {

  Form *f = (Form *)framePtr->widgetPtr;
  PlatformMScreen::getMScreen()->addChild(f);

  f->show();

 return KNI_OK;
}
/**
 * Hide and delete resource function pointer.
 * This function should notify its Items to hide as well.
 * @param onExit - true if this is called during VM exit.
 * 		   All native resource must be deleted in this case.
 */
extern "C" MidpError
form_hide_and_delete(MidpFrame* framePtr, jboolean onExit) {

  /* Suppress unused-parameter warning */
  (void)onExit;

    PlatformMScreen::getMScreen()->removeChild((QWidget *)framePtr->widgetPtr);

    delete ((QWidget *)framePtr->widgetPtr);

    return KNI_OK;
}

/**
 * Create the container window for a Form. 
 * The container window should be created in background and displayed
 * after its content is populated.
 * Return the error condition.
 * On successful return, fields in *formPtr should be set properly.
 */
extern "C" MidpError
lfpport_form_create(MidpDisplayable* dispPtr,
		    const pcsl_string* title, const pcsl_string* tickerText) {

  /* Suppress unused-parameter warning */
  (void)tickerText;

  // create container widget for items
  // and set its size to be at least of the size of the viewport
  QWidget *container = new Form();
  MScreen * mscreen = PlatformMScreen::getMScreen();

  // we need to resize container to the default viewport width 
  // and height because items with use the default size as the 
  // base for there own
  container->resize(mscreen->getScreenWidth(), mscreen->getScreenHeight());
    
  // Fill in MidpDisplayable structure
  dispPtr->frame.widgetPtr	 = container;
  dispPtr->frame.show		 = form_show;
  dispPtr->frame.hideAndDelete   = form_hide_and_delete;
  dispPtr->frame.handleEvent	 = NULL; // Not used in Qt
  dispPtr->setTitle		 = displayable_set_title;
  dispPtr->setTicker		 = displayable_set_ticker;
  
  dispPtr->setTitle(dispPtr, title);
/*  dispPtr->setTicker(dispPtr, tickerText);*/

  return KNI_OK;
}

/**
 * Set content of the container window.
 */
extern "C" MidpError
lfpport_form_set_content_size(MidpDisplayable* dispPtr, int w, int h) {

  QWidget *container = (QWidget *)((MidpFrame *)dispPtr)->widgetPtr;
  PlatformMScreen * mscreen = PlatformMScreen::getMScreen();

  if (h <= mscreen->getDisplayHeight()) {
    w = mscreen->getDisplayWidth();
    h = mscreen->getDisplayHeight(); 
  }

  container->resize(w, h);
  mscreen->resizeContents(w, h);

  return KNI_OK;
}

/**
 * Set current Item.
 */
extern "C" MidpError
lfpport_form_set_current_item(MidpItem* itemPtr, int yOffset) {
    PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
    QWidget* itemWidgetPtr = (QWidget *)itemPtr->widgetPtr;
    // Make it visible
    mscreen->ensureVisible(itemWidgetPtr->x(), itemWidgetPtr->y() + yOffset, 0, 0);
    // Give focus to it
    itemWidgetPtr->setFocus();

    return KNI_OK;
}

/**
 * returns current scroll Y position
 * \param pos return value
 */
extern "C" MidpError
lfpport_form_get_scroll_position(int *pos)
{
  PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
  TRACE_CI(pdMidpFormGetScrollPosition);
  *pos = mscreen->getScrollPosition();
 
  return KNI_OK;
}

/**
 * sets current scroll Y position
 */
extern "C" MidpError
lfpport_form_set_scroll_position(int pos)
{
  PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
  TRACE_CI(pdMidpFormSetScrollPosition);
  mscreen->setScrollPosition(pos);
 
  return KNI_OK;
}

/**
 * returns current viewport height
 * \param viewport height return value
 */
extern "C" MidpError
lfpport_form_get_viewport_height(int *height)
{
  PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
  TRACE_CI(lfpport_form_get_viewport_height);
  *height = mscreen->visibleHeight();
 
  return KNI_OK;
}

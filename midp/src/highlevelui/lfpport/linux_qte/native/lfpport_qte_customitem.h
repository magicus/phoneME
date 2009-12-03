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
 * @file
 * The custom item widget and its helper classes.
 */

#ifndef _LFPPORT_QTE_CUSTOM_ITEM_H_
#define _LFPPORT_QTE_CUSTOM_ITEM_H_

#include <midpEvents.h>

#include <lfpport_customitem.h>
#include <lfpport_error.h>
#include "lfpport_qte_item.h"

#include <qwidget.h>
#include <qevent.h>
#include <qpixmap.h>


/**
 * CustomItem body widget.
 * The image that CustomItem subclass paints is shown in this widget.
 */
class DrawableArea : public QWidget
{
public :

  /**
   * Construct a drawable widget as CustomItem body.
   * 


   * @param parent CustomItem widget
   */
  DrawableArea(QWidget *parent);

  /**
   * Sets offscreen buffer for this CustomItem body.
   * Java paints to the this pixmap.
   * @param p offscreen buffer for this CustomItem body
   */
  void setContentBuffer(QPixmap *p);

  /**
   * Repaints specified area of CustomItem body.
   * @param x the x coordinate of the repaint area top left corner
   * @param y the y coordinate of the repaint area top left corner
   * @param width the width of the repaint area 
   * @param height the height of the repaint area 
   */
  void refresh(int x, int y, int width, int height);

protected :

 /**
   * Repaints the rectangle within the current screen requested by the given
   * system event.
   *
   * @param e device event corresponding to the repaint request.
   */
  void paintEvent(QPaintEvent *e);

  // mouse and key handling:
  /**
   * Notify Java peer of mouse pressed event.
   * @param mouse mouse event
   */
  void mousePressEvent( QMouseEvent *mouse);

  /**
   * Notify Java peer of mouse move event.
   * @param mouse mouse event
   */
  void mouseMoveEvent( QMouseEvent *mouse);

  /**
   * Notify Java peer of mouse release event.
   * @param mouse mouse event
   */
  void mouseReleaseEvent(QMouseEvent *mouse);

 private:

  /**
   * Whether the mouse is currently pressed: true if it is pressed,
   * and false otherwise. <b>Note:</b> This event is needed only for
   * the emulator, which might send a mouse-move event when it is not
   * pressed. A touch-enabled device won't send a mouse-move event
   * when the pen is not pressed.
   */
  bool mouseIsPressed;

  /**
   * Pixmap that contains what CustomItem subclass paints.
   */
  QPixmap *bodyPix;
};


// **********************************************************************
// **********************************************************************
// ** class CustomItem definition
// **********************************************************************
// **********************************************************************

/**
 * CustomItem widget.
 */
class CustomItem : public Item
{

  /**
   * The CustomItem's body widget, used as a drawable area
   */
  DrawableArea *drawable;

public:
  
  /**
   * Construct a CustomItem widget.
   * 
   * @param parent owner screen's widget
   * @param label label text
   * @param layout layout directive associated with this customitem
   */
  CustomItem(QWidget *parent, const QString &label, int layout);
  
  /**
   * CustomItem widget's destructor.
   */
  ~CustomItem(); 

  /**
   * Redraws the part of the custom item specified by the given coordinates
   * and dimensions. The coordinates are the x and y values of the top-left
   * corner of the area to be redrawn, relative to the custom item's drawable
   * area.
   *
   * <p><b>Implementation Note:</b> This function routes from
   * <tt>midp_customitem.c</tt> to <tt>lfpport_qte_customitem.cpp</tt>.
   *
   * @param x horizontal location, relative to drawable, for the upper-left
   *        corner of the area to redraw.
   * @param y vertical location, relative to drawable, for the upper-left
   *        corner of the area to redraw.
   * @param width width of the area to redraw
   * @param height height of the area to redraw
   *
   * @return an indication of success or the reason for failure
   */
  MidpError refresh( int x,
		     int y,
		     int width,
		     int height);

  /**
   * Sets the offscreen buffer for the content area of this custom item
   * @param buffer new offscreen pixmap buffer for the content
   */
  void setContentBuffer(QPixmap *buffer);

protected :
 
  /**
   * Move body widget to new location.
   * The coordinates are within this CustomItem widget's coordinate
   * system.
   *
   * @param x X coordinate of the body widget's top left corner
   * @param y Y coordinate of the body widget's top left corner
   */
  void bodyRelocate(int x, int y);

  /**
   * Resize body widget.
   *
   * @param w W width of the body widget
   * @param h H height of the body widget
   */
  void bodyResize(int w, int h);

  /**
   * Calculate body widget height when its width is limited to a given value.
   *
   * @param takenWidth return value for used body widget width
   * @param w maximum width
   * @return body widget height
   */
  int  bodyHeightForWidth(int *takenWidth, int w);

  /**
   * Calculate body widget width when its height is limited to a given value.
   *
   * @param takenHeight return value for used body widget height
   * @param h maximum height
   * @return body widget width
   */
  int  bodyWidthForHeight(int *takenHeight, int h);

  /**
   * Makes this custom item have focus, enabling any item-specific commands; in
   * addition, if the given event was caused by a user action, notifies the
   * Java platform of the change in focus.
   *
   * @param event device event corresponding to the change in focus.
   */
  void focusInEvent(QFocusEvent *event);

 /**
   * Repaints the rectangle within the custom item requested by the given
   * system event. Focus rectangle is painted here as well.
   *
   * @param e device event corresponding to the repaint request.
   */
  void paintEvent(QPaintEvent *e);

  /**
   * Notify Java peer of key press event.
   * @param key key event
   */
  void keyPressEvent(QKeyEvent *key);

  /**
   * Notify Java peer of key release event.
   * @param key key event
   */
  void keyReleaseEvent(QKeyEvent *key);
};

#endif /* _LFPPORT_QTE_CUSTOM_ITEM_H_ */

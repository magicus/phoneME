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
 * Base Qt widget of an item.
 */

#ifndef _LFPPORT_QTE_ITEM_H_
#define _LFPPORT_QTE_ITEM_H_

#include <lfpport_error.h>
#include <lfpport_displayable.h>
#include <lfpport_item.h>

#include <qwidget.h>
#include <qsimplerichtext.h>
#include <qstring.h>

/**
 * Default id for un-initialized resources.
 */
#define INVALID_NATIVE_ID 0

/**
 * Upper limit of any Item's minimum width.
 * Should be in range (0, SCREEN_WIDTH].
 */
#define MIN_WIDTH_LIMIT	(qteapp_get_mscreen()->getScreenWidth()>>1)

/**
 * Upper limit of any Item's preferred height.
 * Should be in range (0, +infinite).
 */
#define PREF_HEIGHT_LIMIT (qteapp_get_mscreen()->getScreenHeight()<<2)

/**
 * Number of pixels around an item's boundary.
 */
#define ITEM_BOUND_PAD 4

/**
 * Number of pixels between label and body when both fit on a single line.
 */
#define LABEL_BODY_HRZN_PAD ITEM_BOUND_PAD

/**
 * Number of pixels between label and body when they are on separated lines.
 * This value must be kept in sync with CustomItemLFImpl.java.
 */
#define LABEL_BODY_VERT_PAD 0

/**
 * Initialize the common pointers of an MidpItem struct.
 *
 * @param itemPtr pointer the MidpItem struct that is to be filled in
 * @param ownerPtr pointer to owner screen
 */
extern MidpError initItemPtr(MidpItem *itemPtr, MidpDisplayable *ownerPtr);

/**
 * Main widget for an Item.
 */
class Item : public QWidget {

  Q_OBJECT

 protected:

  /**
   * Override QWidget::paintEvent in order to paint the label.
   */
  void paintEvent(QPaintEvent *evt);

  /**
   * Initialize label related resource.
   */
  /*static */void init();

  /**
   * Returns whether this item's label can share a line with its content.
   *
   * @return true if the label and content of the item can be on the same
   * line; false otherwise
   */
  bool labelCanBeOnSameLine(int labelHeight);


  /**
   * Draw the label text.
   *
   * @param painter painter to use
   * @param x X coordinate of the drawing region's top left corner
   * @param y Y coordinate of the drawing region's top left corner
   * @param w width of the drawing region
   * @param h height of the drawing region
   */
  void drawLabel(QPainter *painter, int x, int y, int w, int h);

  /**
   * Move body widget.
   * Coordinates are relative to top left corner of this main widget.
   *
   * @param x X coordinate
   * @param y Y coordinate
   */
  virtual void bodyRelocate(int x, int y) = 0;

  /**
   * Resize body widget.
   *
   * @param w new width
   * @param h new height
   */
  virtual void bodyResize(int w, int h) = 0;

  /**
   * Calculate body widget's height when its width is limited to a given value.
   *
   * @param takenWidth return value of real width used
   * @param w maximum width
   * @return body widget's height
   */
  virtual int  bodyHeightForWidth(int *takeWidth, int width) = 0;

  /**
   * Calculate body widget's width when its height is limited to a given value.
   *
   * @param takenHeight return value of real height used
   * @param h maximum height
   * @return body widget's width
   */
  virtual int  bodyWidthForHeight(int *takenHeight, int height) = 0;

  /**
   * Test whether body is considered on a single line if its body height
   * is the given value.
   *
   * @param bodyHeight body height
   * @return true if its body widget is considered fitting one single line
   */
  virtual bool bodyCanBeOnSameLine(int bodyHeight);

  /**
   * This item's label.
   */
  QString label;

  /**
   * Font used to display this item's label
   */
  QFont labelFont;

  /**
   * Height in pixels required for line that displays this item's
   * label
   */
  int labelLineHeight;

  /**
   * True if label and body should always on separated lines.
   */
  bool alwaysVertLayout;

  /**
   * The layout associated with this item
   */
  int layout;

public :
  /**
   * Constructor.
   *
   * @param parent owner screen widget
   * @param label label text
   * @param layout layout directive associated with this item
   * @param alwaysVertLayout true if label/body should always on separated lines
   */
  Item (QWidget *parent, const QString &label, int layout, 
	bool alwaysVertLayout=false);

  /**
   * Item's destructor.
   * Implementation should delete everything that is not deleted 
   * automatically by the native environment.
   */
  virtual ~Item();

  /**
   * set or replace current label with this label
   * <i>called from common code</i>
   * @param &label new wlabel
   * @return MIDP error code (KNI_OK, KNI_ERR)
   */
  MidpError setLabel(const QString &label);

  /**
   * the implementation should place this item in (x,y) coordinate relative
   * to the main container (for example, the scrollable area used for the
   * form).
   * <i>called from common code</i>
   * @param x coordinate relative to the left edge of the parent container
   * @param y coordinate relative to the top edge of the parent container
   * @return MIDP error code (KNI_OK, KNI_ERR)
   */
  MidpError relocate(int x, int y);

  /**
   * Set the size of the widget, taking care of label and item pad within
   * this area. This size is granted by the form layout.
   * The body would take the remaining space.
   * (i.e. if the label is on the same row with the body, than the body height
   * would be (h-2*ITEM_PAD) and the width would be (w-label width-3*ITEM_PAD))
   * <i>overriden by CustomItem</i>
   * <i>called from common code</i>
   * @param w allocated width for the item
   * @param h allocated height for the item
   * @return MIDP error code (KNI_OK, KNI_ERR)
   */
  virtual MidpError setSize(int w, int h);

  /**
   * Height preferred by this item, including item pad and label.
   * No need to take care of locked sizes, as this is done in the java level.
   * <i>called from common code</i>
   * @param *h pointer to set the preferred height
   * @param w tentative width in pixels, or -1 if not yet computed
   * @return MIDP error code (KNI_OK, KNI_ERR)
   */
  MidpError preferredHeight(int *h, int w);

  /**
   * Width preferred by this item, including item pad and label.
   * No need to take care of locked sizes, as this is done in the java level.
   * <i>called from common code</i>
   * @param *w pointer to set the preferred width
   * @param h tentative height in pixels, or -1 if not yet computed
   * @return MIDP error code (KNI_OK, KNI_ERR)
   */
  MidpError preferredWidth(int *w, int h);

  /**
   * Minimum height required by this item, including item pad and label.
   * <i>called from common code</i>
   * @param *h pointer to set the preferred height
   * @return MIDP error code (KNI_OK, KNI_ERR)
   */
  MidpError minimumHeight(int *h);

  /**
   * Minimum width required by this item, including item pad and label.
   * <i>called from common code</i>
   * @param *w pointer to set the preferred width
   * @return MIDP error code (KNI_OK, KNI_ERR)
   */
  MidpError minimumWidth(int *w);

  /**
   * Gets actual width used by the label based on a passed content width.
   * @param contentWidht  the width available for the label layout
   * @return width that label needs to occupy to display itself
   */
  int getLabelWidth(int contentWidth);

  /**
   * Gets label height used based on the passed in width
   * @param w the width available for the label layout
   * @return height that label needs to occupy to display the label in the
   *         given width
   */
  int getLabelHeight(int w);

  /**
   * Gets layout associated with this item.
   * @return layout associated with this itm
   */
  int getLayout();

};

#endif /* _LFPPORT_QTE_ITEM_H_ */

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
 * Definitions of Qt port of string items and image items.
 */

#ifndef _LFPPORT_QTE_STRINQ_IMAGE_ITEM_H_
#define _LFPPORT_QTE_STRINQ_IMAGE_ITEM_H_

#include <lfpport_error.h>
#include <lfpport_form.h>
#include "lfpport_qte_item.h"

#include <qpushbutton.h>
#include <qfont.h>

/**
 * Constant used when the string or image item does not have a special
 * appearance mode.  This constant's value is specified by the <i>MIDP
 * 2.0 Specification</i>.
 */
#define PLAIN     0

/**
 * Constant used when the MIDP implementation should display the
 * string or image item as though it were a hyperlink. This constant's
 * value is specified by the <i>MIDP Specification</i>.
 */
#define HYPERLINK 1

/**
 * Constant used when the MIDP implementation should display the
 * string or image item as though it were a button. This constant's
 * value is specified by the <i>MIDP Specification</i>.
 */
#define BUTTON    2

/**
 * Body widget of StringItem.
 */
class StringBody : public QPushButton {

private:

  /** text */
  QString text;

protected:
  /**
   * height, in pixels, of the text string
   */
  int textLineHeight;


  /**
   * Appearance mode of the string item (the constants are created in
   * this file)
   */
  int appearance;


  /**
   * Number of pixels of space between the item and its border.
   */
  int PAD;

   /**
    * Makes this item have focus, enabling any item-specific commands; in
    * addition, if the given event was caused by a user action, notifies the
    * Java platform of the change in focus.
    *
    * @param event pointer to the device event corresponding to the change in
    *        focus.
    */
  void focusInEvent(QFocusEvent *event);

  /**
   * Determines and caches whether this item can have focus: if its appearance
   * mode is <tt>PLAIN</tt>, the item cannot have focus; otherwise it can.
   */
  virtual void initFocus();

public:
  /**
   * StringBody constructor.
   *
   * @param textStr text
   * @param appearance PLAIN, BUTTON or HYPERLINK
   * @param f font for showing the text
   * @param parent StringItem main widget
   */
  StringBody(const QString &textStr, int appearance = 0, QFont *f = NULL, 
	     QWidget *parent = 0);

  /**
   * StringBody destructor.
   */
  virtual ~StringBody();

  /**
   * Override QButton to draw button in hyperlink mode.
   */
  virtual void drawButton(QPainter *);

  /**
   * Override QButton to draw long text with word wrap.
   */
  virtual void drawButtonLabel(QPainter *);

  /**
   * Calculate height if its width is limited to a given value.
   *
   * @param takenWidth return value of the real used width
   * @param w maximum width
   * @return height
   */
  virtual int  heightForWidth(int *takenWidth, int w);

  /**
   * Calculate width if its height is limited to a given value.
   * 
   * @param takenHeight return value of the real used height
   * @param h maximum height
   * @return width
   */
  virtual int  widthForHeight(int *takenHeight, int h);

  /**
   * Test whether the text can fit on a single text line if its body height is
   * a given value.
   */
  virtual bool isSingleLine(int bodyHeight);

  /**
   * Sets the appearance mode for this string item to the given appearance
   * mode.
   *
   * @param appearance appearance mode (<tt>PLAIN</tt>, <tt>HYPERLINK</tt>,
   *        or <tt>BUTTON</tt>) of this string item.
   */
  virtual void setAppearance(int appearance);

  /**
   * Update contents with new text and appearance mode.
   */
  void setContent(const QString &textStr, int appearanceMode);

  /**
   * Change text font.
   */
  void setTextFont(const QFont &font);
};


//**************************************************************
/**
 * Body widget of ImageItem.
 */
class ImageBody : public StringBody {

 protected:
  /**
   * Determines and caches whether this item can have focus: if its appearance
   * mode is <tt>PLAIN</tt>, the item cannot have focus; otherwise it can.
   */
    virtual void initFocus();

public:
    /**
     * ImageBody's constructor.
     *
     * @param pixmap image
     * @param textStr the text that may be used in place of the image
     * @param appearance appears in PLAIN, BUTTON or HYPERLINK mode
     * @param parent ImageItem's main widget
     */
    ImageBody(QPixmap *pixmap, const QString &textStr, int appearance = 0,
              QWidget *parent = 0);

    /**
     * Destructor.
     */
    virtual ~ImageBody();

    /**
     * Override QPushButton to draw focus highlight in HYPERLINK mode.
     */
    virtual void drawButton(QPainter *);

    /**
     * Override QPushButton to draw the image.
     */
    virtual void drawButtonLabel(QPainter *);

    /**
     * Set appearance mode to PLAIN, BUTTON or HYPERLINK.
     */
    virtual void setAppearance(int appearance);

    /**
     * Calculate height if its width is limited to a given value.
     *
     * @param takenWidth return value of the real used width
     * @param w maximum width
     * @return height
     */
    virtual int  heightForWidth(int *takenWidth, int w);

    /**
     * Calculate width if its height is limited to a given value.
     * 
     * @param takenHeight return value of the real used height
     * @param h maximum height
     * @return width
     */
    virtual int  widthForHeight(int *takenHeight, int h);

    /**
     * Test to whether a given body height is enough for this widget.
     */
    virtual bool isSingleLine(int bodyHeight);
};

// **********************************************************************

/**
 * Main widget of both StringItem and ImageItem.
 */
class StringImageItem : public Item {
    Q_OBJECT

    /**
     * The string or image associated with this string item or image
     * item.
     */
    StringBody *body;

public : 
    /**
     * Constructor for StringItem.
     *
     * @param parent owner screen's widget
     * @param label label text
     * @param layout layout directive associated with this string item
     * @param text text of the StringItem
     * @param f text font
     * @param appearance appearance mode
     */
    StringImageItem (QWidget *parent, const QString &label, int layout,
		     const QString &text, 
		     QFont *f,
                     int appearance);

    /**
     * Constructor for ImageItem.
     *
     * @param parent owner screen's widget
     * @param label label text
     * @param layout layout directive associated with this image item
     * @param image image of the ImageItem
     * @param text the text that may be used in place of the image
     * @param appearance appearance mode
     */
    StringImageItem (QWidget *parent, const QString &label,
		     int layout,
		     QPixmap *pixmap,
                     const QString &altText,
                     int appearance);

    /**
     * StringImageItem's destructor.
     */
    ~StringImageItem();

    /**
     * Moves body widget.
     *
     * @param x the horizontal coordinate of the upper-left corner of this main widget.
     * @param y the vertical coordinate of the upper-left corner of this main widget.
     */
    void bodyRelocate(int x, int y);

    /**
     * Resize body widget.
     * 
     * @param w new width
     * @param h new height
     */
    void bodyResize(int w, int h);

    /**
     * Calculate body widget height when width is limited to a given value.
     *
     * @param takenWidth return value of the real width used
     * @param w maximum width
     * @return body widget height
     */
    int  bodyHeightForWidth(int *takenWidth, int w);

    /**
     * Calculate body widget width when height is limited to a given value.
     *
     * @param takenHeight return value of the real height used
     * @param h maximum height
     * @return body widget width
     */
    int  bodyWidthForHeight(int *takenHeight, int h);

    /**
     * Test if the body widget can fit in the given height.
     */
    bool bodyCanBeOnSameLine(int bodyHeight);

    /**
     * Change font used to display text.
     */
    MidpError setTextFont(const QFont &font);

    /**
     * Change contents of a StringItem.
     * 
     * @param text new text
     * @param appearanceMode new appearance
     */
    MidpError setContent(const QString &text, int appearanceMode);

    /**
     * Change contents of a StringItem.
     * 
     * @param pixmap new image
     * @param altText new alternative text
     * @param appearanceMode new appearance
     */
    MidpError setContent(QPixmap *pixmap, const QString &altText, 
                         int appearanceMode);

public slots:
   
    /**
     * Notify Java to call activate a default command associated with
     * this StringItem or ImageItem
     */
    void activateDefaultCommand();
};

#endif /* _LFPPORT_QTE_STRINQ_IMAGE_ITEM_H_ */

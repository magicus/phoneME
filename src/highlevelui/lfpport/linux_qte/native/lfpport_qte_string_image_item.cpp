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

#include <kni.h>
#include <qfontmetrics.h>

#include "lfpport_qte_mscreen.h"
#include "lfpport_qte_util.h"

#include <gxpportqt_image.h>

#include <lfpport_stringitem.h>
#include <lfpport_imageitem.h>
#include "lfpport_qte_string_image_item.h"
#include <moc_lfpport_qte_string_image_item.cpp>


StringImageItem::StringImageItem(QWidget *parent, const QString &labelStr,
                                 int layout,
                                 const QString &textStr, 
                                 QFont *f,
                                 int appearance) 
        : Item( parent, labelStr, layout) { 

  body = new StringBody(textStr, appearance, f, this);

  // Delegate focus to QPushButton
  setFocusProxy(body);


  connect(body, SIGNAL(clicked()), this, SLOT(activateDefaultCommand()));
}

StringImageItem::StringImageItem(QWidget *parent, 
                                 const QString &labelStr,
                                 int layout,
                                 QPixmap *pixmap,
                                 const QString &altTextStr, 
                                 int appearance) 
  : Item( parent, labelStr, layout) {

  body = new ImageBody(pixmap, altTextStr, appearance, this);

  // Delegate focus to QPushButton
  setFocusProxy(body);

  connect(body, SIGNAL(clicked()), this, SLOT(activateDefaultCommand()));
}

StringImageItem::~StringImageItem() {
  // Item's destructor will be called since it is StringImageItem's base class
  delete body;
}

void StringImageItem::bodyRelocate(int x, int y) {
  body->move(x, y);
}

void StringImageItem::bodyResize(int w, int h) {    
  body->resize(w, h);
}

int  StringImageItem::bodyHeightForWidth(int *takenWidth, int w) {
  return body->heightForWidth(takenWidth, w);
}

int  StringImageItem::bodyWidthForHeight(int *takenHeight, int h) {
  return body->widthForHeight(takenHeight, h);
}

bool StringImageItem::bodyCanBeOnSameLine(int bodyHeight) {
  return Item::bodyCanBeOnSameLine(bodyHeight) && 
         body->isSingleLine(bodyHeight);
}

MidpError StringImageItem::setTextFont(const QFont &font) {
  body->setTextFont(font);  
  body->repaint();
  return KNI_OK;
}

MidpError StringImageItem::setContent(const QString &textStr, 
                                      int appearanceMode) {
  // The order of calling setContent and setAppearance is important;
  // setContent() depends on the appearanceMode to be set;
  // initFocus() is called as the last thing in setContent since
  // it depends on the content set
  body->setAppearance(appearanceMode);
  body->setContent(textStr, appearanceMode);
  body->repaint();

  return KNI_OK;
}

MidpError StringImageItem::setContent(QPixmap *pixmap, 
                                      const QString &altTextStr,
                                      int appearanceMode) {
  /* Suppress unused-parameter warning */
  (void)altTextStr;

  // The order of calling setPixmap and setAppearance is important;
  // setAppearance() calls initFocus() which could do different things
  // depending on the content 
  body->setPixmap(pixmap == NULL ? 0 : *pixmap);
  body->setAppearance(appearanceMode);
  body->repaint();

  return KNI_OK;
}

/**
 * Notify Java to call activate a default command associated with
 * this StringItem or ImageItem
 */
void StringImageItem::activateDefaultCommand() {
  MidpFormItemPeerStateChanged(this, -1);
}

//*************************************************************************
StringBody::StringBody(const QString &textStr, int appearance,
                       QFont *f,
                       QWidget *parent)
  : QPushButton(parent) {

  if (f != NULL) {
    setFont(*f);
  }

  textLineHeight = QFontMetrics(font()).height();

  setAppearance(appearance);
  setContent(textStr, appearance);
}

StringBody::~StringBody() {
}


void StringBody::drawButtonLabel(QPainter *painter) {
  if (!text.isEmpty()) {
    painter->setFont(font());
    painter->drawText(PAD, PAD, width() - PAD - PAD, height() - PAD - PAD, 
                      WordBreak, text);
  }
}

void StringBody::drawButton(QPainter *painter) {
    if (appearance == HYPERLINK) {
      drawButtonLabel(painter);
      if (hasFocus()) {
        style().drawFocusRect(
            painter, rect(), colorGroup(), &colorGroup().button());
      }
    } else {
      QPushButton::drawButton(painter);
    }
}

int StringBody::heightForWidth(int *takenWidth, int w) {
    if (text.isEmpty()) {
      *takenWidth = 0;
      return 0;
    }

    QRect textRect = QFontMetrics(font()).boundingRect(PAD, PAD, 
                                                       w - PAD - PAD,
                                                       qteapp_get_mscreen()->getScreenHeight(),
                                                       WordBreak, text);
    *takenWidth = PAD + textRect.width() + PAD;

    return (PAD + textRect.height() + PAD);
}

int StringBody::widthForHeight(int *takenHeight, int h) {
  /* Suppress unused-parameter warning */
  (void)takenHeight;
  (void)h;

  return 0;
}

bool StringBody::isSingleLine(int bodyHeight) {
  if (text.isEmpty()) {
    return true;
  }
  return (bodyHeight <= (PAD +textLineHeight + PAD));
}

void StringBody::setAppearance(int appearance) {

  this->appearance = appearance;

  switch (appearance) {
  case PLAIN:
    setFlat(true);
    setEnabled(false);
    {
      // Use the same background color as Form's qwidget
      setBackgroundColor(parentWidget()->palette().active().background());

      // Make sure that text is displayed with active color values
      QPalette curPalette = palette();
      QColorGroup curActive = curPalette.active();
      setPalette(QPalette(curActive, curActive, curActive));
    }

    PAD = 0;
    break;

  case HYPERLINK:
    setFlat(true);
    setEnabled(true);
    PAD = 2;
    break;

  case BUTTON:
    setFlat(false);
    setEnabled(true);
    PAD = 5;
    break;
  }
}

void StringBody::setContent(const QString &textStr, int appearance) {

  text = textStr.copy();

  QFont f = font();
  f.setUnderline(appearance == HYPERLINK);
  setFont(f);
 
  initFocus();
}

void StringBody::setTextFont(const QFont &font) {

  setFont(font);
  textLineHeight = QFontMetrics(font).height();
}

/** Override to notify Form focus change */
void StringBody::focusInEvent(QFocusEvent *event) {

  // Notify Java if this is caused by user action
  if (event->reason() != QFocusEvent::Other) {
    MidpFormFocusChanged(parent());
  }
  
  // Continue with focus activation
  QPushButton::focusInEvent(event);
}

void StringBody::initFocus() {
  // should be called after content is set
  if (!text.isEmpty() && appearance != PLAIN) {
    setFocusPolicy(QWidget::StrongFocus);
  } else {
    setFocusPolicy(QWidget::NoFocus);
  }
}

//*************************************************************************
ImageBody::ImageBody(QPixmap *pixmap, const QString &altText,
                 int appearance,
                 QWidget *parent)
  : StringBody(altText, appearance, NULL, parent) {

  QPushButton::setPixmap(pixmap == NULL ? 0 : *pixmap);
  setAppearance(appearance);
}

ImageBody::~ImageBody() {
}

void ImageBody::drawButtonLabel(QPainter *painter) {
  const QPixmap *p = pixmap();
  if (p != 0) {
    painter->drawPixmap(PAD, PAD, *p, 0, 0, p->width(), p->height());
    return;
  }

  StringBody::drawButtonLabel(painter);
}

void ImageBody::drawButton(QPainter *painter) {
  if (appearance == HYPERLINK) {
    drawButtonLabel(painter); 
    // by default focus is drawn on top of the button;
    // hilight might not be visible if image background is black.
    // To make sure that hilight is visible we do not call 
    // QPushButton::drawButton() and draw hilight outside of the image
    if (qteapp_get_application()->focusWidget() == this) { // draw focus
      painter->drawWinFocusRect(2, 2, width() - 4, height() - 4);
    }
  } else {
    QPushButton::drawButton(painter);
  }
}

int ImageBody::heightForWidth(int *takenWidth, int w) {
  const QPixmap *p = pixmap();
  if (p != 0) {
    *takenWidth = PAD + p->width() + PAD;
    return PAD + p->height() + PAD;
  }

  return StringBody::heightForWidth(takenWidth, w);
}

int ImageBody::widthForHeight(int *takenHeight, int h) {
  /* Suppress unused-parameter warning */
  (void)takenHeight;
  (void)h;

  return 0;
}

bool ImageBody::isSingleLine(int bodyHeight) {
  return StringBody::isSingleLine(bodyHeight);
}

void ImageBody::setAppearance(int appearance) {
  StringBody::setAppearance(appearance);
  if (appearance == HYPERLINK) {
    PAD = 5;
  }
  initFocus();
}

void ImageBody::initFocus() {
  const QPixmap *p = pixmap();
  if (p != NULL && p->width() > 0 && p->height() > 0 && 
      appearance != PLAIN) {
    setFocusPolicy(QWidget::StrongFocus);
  } else {
    setFocusPolicy(QWidget::NoFocus);
  }
}

// **************************************************************************

extern "C" {

/**
 * Creates a StringImageItem native peer without showing it yet.
 * Upon successful return, fields in *itemPtr should be set properly.
 */
  MidpError lfpport_stringitem_create(MidpItem* itemPtr, 
				      MidpDisplayable* formPtr,
				      const pcsl_string* label,
				      int layout,
				      const pcsl_string* text,
				      PlatformFontPtr fontPtr, 
				      int appearanceMode) {

    /* Suppress unused-parameter warning */
    (void)fontPtr;

    QString labelStr;
    QString textStr;
   
    pcsl_string2QString(*label, labelStr);
    pcsl_string2QString(*text, textStr);


    itemPtr->widgetPtr =
      new StringImageItem((formPtr == INVALID_NATIVE_ID ? 
                           0 : (QWidget *)formPtr->frame.widgetPtr),
                          labelStr,
                          layout,
                          textStr, 
                          (QFont *)fontPtr,
                          appearanceMode); 
    
    initItemPtr(itemPtr, formPtr);
   

    return KNI_OK;
  }

/**
 * Notifies a content change in the corresponding StringImageItem.
 * @param text - the new string set in the StringImageItem
 */
  MidpError lfpport_stringitem_set_content(MidpItem* itemPtr, const pcsl_string* text,
					   int appearanceMode) {
    QString textStr;

    pcsl_string2QString(*text, textStr);

    return ((StringImageItem *)itemPtr->widgetPtr)->setContent(textStr, 
                                                               appearanceMode);
  }
  
  /**
   * Notifies a font change in the corresponding StringImageItem.
   * @param font - the new font set in the StringImageItem
   */
  MidpError lfpport_stringitem_set_font(MidpItem* itemPtr, 
					PlatformFontPtr fontPtr) {
    QFont font(*(QFont *)fontPtr);
    return ((StringImageItem *)itemPtr->widgetPtr)->setTextFont(font);
  }

/**
 * Creates a StringImageItem native peer without showing it yet.
 * Upon successful return, fields in *itemPtr should be set properly.
 */
MidpError lfpport_imageitem_create(MidpItem* itemPtr,
				   MidpDisplayable* formPtr,
				   const pcsl_string* label,
				   int layout,
				   unsigned char* imgPtr, 
				   const pcsl_string* altText,
				   int appearanceMode) {
    QString labelStr;
    QString altTextStr;
   
    pcsl_string2QString(*label, labelStr);
    pcsl_string2QString(*altText, altTextStr);

    itemPtr->widgetPtr = 
        new StringImageItem((formPtr == INVALID_NATIVE_ID ? 
                           0 : (QWidget *)formPtr->frame.widgetPtr),
                          labelStr,
                          layout,
                          gxpportqt_get_immutableimage_pixmap(imgPtr),
                          altTextStr, 
                          appearanceMode);

    initItemPtr(itemPtr, formPtr);

    return KNI_OK;
}

/**
 * Notifies a content change in the corresponding StringImageItem.
 */
MidpError lfpport_imageitem_set_content(MidpItem* itemPtr, 
					unsigned char* imgPtr,
					const pcsl_string* altText,
					int appearanceMode) {
    QString altTextStr;

    pcsl_string2QString(*altText, altTextStr);

    return ((StringImageItem *)itemPtr->widgetPtr)->setContent(
                             gxpportqt_get_immutableimage_pixmap(imgPtr),
                             altTextStr, appearanceMode);
  }
} /* extern C */

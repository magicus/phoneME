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


#include <midp_logging.h>

#include "lfpport_qte_util.h"
#include "lfpport_qte_mscreen.h"

#include <lfpport_form.h>
#include "lfpport_qte_customitem.h"

#include <gxpportqt_image.h>

// ************************************************************
// ************************************************************
// *** Class DrawableArea implementation
// ************************************************************
// ************************************************************

DrawableArea::DrawableArea(QWidget *parent)
  : QWidget(parent)
{ 
    mouseIsPressed = FALSE;
    setMouseTracking(TRUE);

    bodyPix = NULL;
}

/** Repaint current screen upon system notification */

void DrawableArea::paintEvent(QPaintEvent *e)
{
#if REPORT_LEVEL <= LOG_INFORMATION
  QRect r(e->rect());
  reportToLog(LOG_INFORMATION, LC_HIGHUI, 
 	      "DrawableArea::paintEvent (%d,%d,%d,%d)\n",
 	      r.left(), r.top(), r.right(), r.bottom());
#endif

  if (bodyPix != NULL) {
    const QRect r = e->rect();
    bitBlt( this, r.topLeft(), bodyPix, r);
  }
}

void DrawableArea::mousePressEvent(QMouseEvent *mouse)
{
  PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
  mouseIsPressed = TRUE;
  mscreen->viewportMousePressEvent(mouse);
}

void DrawableArea::mouseMoveEvent( QMouseEvent *mouse)
{
  PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
  if (mouseIsPressed)
    mscreen->viewportMouseMoveEvent(mouse);
}


void DrawableArea::mouseReleaseEvent( QMouseEvent *mouse)
{
  PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
  mouseIsPressed = FALSE;
  mscreen->viewportMouseReleaseEvent(mouse);
}

void DrawableArea::setContentBuffer(QPixmap *p) {
  bodyPix = p;
}

void DrawableArea::refresh(int x, int y, int w, int h) {
  if (bodyPix != NULL) {
    bitBlt(this, x, y, bodyPix, x, y, w, h);
  }
}

// ************************************************************
// ************************************************************
// *** Class CustomItem implementation
// ************************************************************
// ************************************************************

/**
 * Construct a TextField native peer with label and body.
 */
CustomItem::CustomItem(QWidget *parent, 
		       const QString &labelSt, 
		       int layout)
  : Item(parent, labelSt, layout, true)
{

  drawable = new DrawableArea(this);

  setFocusPolicy(StrongFocus);
  drawable->setFocusPolicy(StrongFocus);

  // Delegate focus from DrawableArea to CustomItem
  drawable->setFocusProxy(this);
}

/**
 * Construct a TextField native peer with label and body.
 */
CustomItem::~CustomItem() {
  delete drawable;
}

/**
 * Sets the offscreen buffer for the content area of this custom item
 */
void
CustomItem::setContentBuffer(QPixmap *buffer) 
{
  drawable->setContentBuffer(buffer);

  if (buffer != NULL) {
    drawable->resize(buffer->width(), buffer->height());
  }
}

/** Implement virtual function (defined in lfpport_qte_item.h) */
void CustomItem::bodyResize(int w, int h)
{

  REPORT_INFO1(LC_HIGHUI, "[lfpport_qte_customitem.cpp:%d] "
	         "CustomItem::bodyResize " , __LINE__);


  REPORT_INFO2(LC_HIGHUI, " @@@@@ \tw & h = %d & %d @@@@@\n",w, h);
  
  drawable->resize(w, h);
}

void CustomItem::bodyRelocate(int x, int y) {
  drawable->move(x, y);
}

int CustomItem::bodyHeightForWidth(int *takenWidth, int) {
  
  REPORT_INFO1(LC_HIGHUI, "[lfpport_qte_customitem.cpp:%d] "
	       "***CustomItem::bodyHeightForWidth "
	       , __LINE__);

  *takenWidth = drawable->width();
  return drawable->height();
}

int CustomItem::bodyWidthForHeight(int *takenHeight, int ) {
  
  REPORT_INFO1(LC_HIGHUI, "[lfpport_qte_customitem.cpp:%d] "
	       "***CustomItem::bodyWidthForHeight ", 
	       __LINE__);

  *takenHeight = drawable->height();
  return drawable->width();
}

/** Override to notify Form focus change */
void 
CustomItem::focusInEvent(QFocusEvent *event) {
  
  REPORT_INFO1(LC_HIGHUI, "[lfpport_qte_customitem.cpp:%d] "
	       "CustomItem::focusInEvent ", __LINE__);

  // Notify Java if this is caused by user action
  if (event->reason() != QFocusEvent::Other) {
    MidpFormFocusChanged(this);
  }
  
  // Continue with focus activation
  QWidget::focusInEvent(event);

}

/** Repaint custom item upon system notification */
void CustomItem::paintEvent(QPaintEvent *e)
{
#if REPORT_LEVEL <= LOG_INFORMATION
  QRect r(e->rect());
  reportToLog(LOG_INFORMATION, LC_HIGHUI, 
 	      "CustomItem::paintEvent (%d,%d,%d,%d)\n",
 	      r.left(), r.top(), r.right(), r.bottom());
#endif

  Item::paintEvent(e);

    if (hasFocus()) {
      QPainter painter(this);
      QRect focusRect = drawable->geometry();
      
      // focusRect.setWidth(focusRect.width() + 3);
      // focusRect.setHeight(focusRect.height() + 3);

      focusRect.moveBy(-2, -2);
      focusRect.rRight() += 4;
      focusRect.rBottom() += 4;

      style().drawFocusRect(&painter, focusRect, colorGroup());
    }
}

/** 
 * Refresh the area specified.
 * @param x relative to drawable
 * @param y relative to drawable
 * @param width width of the area to be updated
 * @param height height of the area to be updated 
 */
MidpError
CustomItem::refresh(int x,
		    int y,
		    int width,
		    int height)
{
#if REPORT_LEVEL <= LOG_INFORMATION
  PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
  reportToLog(LOG_INFORMATION, LC_HIGHUI, 
	      "+ CustomItem::refresh: (%d,%d) (%dx%d)\n",
	      x, y, width, height);
  
  reportToLog(LOG_INFORMATION, LC_HIGHUI, 
	      "+       QWidget::pos() = (%d,%d)  drawable->pos() = (%d,%d)\n",
	      QWidget::pos().x(), QWidget::pos().y(),
	      drawable->pos().x(), drawable->pos().y());
  
  reportToLog(LOG_INFORMATION, LC_HIGHUI, 
	      "+           scroll position = %d\n",
	      mscreen->scrollPosition());

  reportToLog(LOG_INFORMATION, LC_HIGHUI, 
	      "+\t\tbitBlt(\tdst, %d, %d,\n\t\t\tsrc, %d, %d, %d, %d); ",
	      x, // dx
	      y, // dy

	      QWidget::pos().x() + drawable->pos().x() + x,
	      QWidget::pos().y() + drawable->pos().y() + y -
	      mscreen->scrollPosition(),
	      
	      width,
	      height);
#endif
   
  drawable->refresh(x, y, width, height);

  REPORT_INFO(LC_HIGHUI, "bitBlt complete.\n\n");

  return KNI_OK;  
}

void CustomItem::keyPressEvent(QKeyEvent *key)
{
  PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
  mscreen->keyPressEvent(key);
}

void CustomItem::keyReleaseEvent(QKeyEvent *key)
{
  PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
  mscreen->keyReleaseEvent(key);
}

// ************************************************************
// * accessed from lfp_customitem.c
// ************************************************************

/**
 * Creates a custom item's native peer, but does not display it.
 * When this function returns successfully, *itemPtr will be filled.
 *
 * @param itemPtr pointer to the custom item's MidpItem structure.
 * @param ownerPtr pointer to the item's owner(form)'s MidpDisplayable 
 *                 structure.
 * @param label the item label.
 * @param layout the item layout directive.
 *
 * @return an indication of success or the reason for failure
 */
extern "C" MidpError
lfpport_customitem_create(MidpItem* customitemPtr, 
			  MidpDisplayable* formPtr,
			  const pcsl_string* label, int layout)
{
  QString qlabel;

  pcsl_string2QString(*label, qlabel);

  // Fill in MidpItem structure
  customitemPtr->widgetPtr = 
    new CustomItem(formPtr == INVALID_NATIVE_ID ? 
		   0 : (QWidget *)formPtr->frame.widgetPtr,
		   qlabel, layout);

  initItemPtr(customitemPtr, formPtr);

  return KNI_OK;
}
/**
 * Causes an immediate bitblt on the specified rectangle.
 * The given x and y  values are relative to the custom item's 
 * content's co-ordinate system.
 *
 * @param itemPtr pointer to the custom item's MidpItem structure.
 * @param x x-coordinate of the refresh area.
 * @param y y-coordinate of the refresh area.
 * @param width width of area to be refreshed.
 * @param height height of area to be refreshed.
 *
 * @return an indication of success or the reason for failure
 */
extern "C" MidpError 
lfpport_customitem_refresh(MidpItem* customitemPtr, 
			   int x, int y,
			   int width, int height)
{


  REPORT_INFO1(LC_HIGHUI, "[lfpport_qte_customitem.cpp:%d] "
	       "lfpport_customitem_Refresh", __LINE__);

  MidpError err =
    ((CustomItem *)customitemPtr->widgetPtr)->refresh(x,
						      y,
						      width,
						      height);
  return err;
}

/**
 * Gets the width of the custom item's native label.
 *
 * @param widthRet the calculated label width, based on tentative 
 *                 width passed in.
 * @param width the tentative width for the label (without padding)
 * @param ciPtr pointer to the custom item's MidpItem structure.
 *
 * @return an indication of success or the reason for failure
 */
extern "C" MidpError
lfpport_customitem_get_label_width(int *widthRet,
				   int width,
				   MidpItem* ciPtr)
{

  REPORT_INFO1(LC_HIGHUI, "[lfpport_qte_customitem.cpp:%d] "
	       "lfpport_customitem_GetLabelWidth", __LINE__);

  // NOT TESTED

  if (width < 0) {
    width = qteapp_get_mscreen()->getScreenWidth() - ITEM_BOUND_PAD - ITEM_BOUND_PAD;
  }
  *widthRet = ((Item *)ciPtr->widgetPtr)->getLabelWidth(width);

  return KNI_OK;
}


/**
 * Gets the height of the custom item's native label. 
 *
 * @param width the tentative width for the label (without padding)
 * @param heightRet the calculated label height, based on tentative 
 *                  width passed in.
 * @param ciPtr pointer to the custom item's MidpItem structure.
 *
 * @return an indication of success or the reason for failure
 */
extern "C" MidpError
lfpport_customitem_get_label_height(int width,
				    int *heightRet,
				    MidpItem* ciPtr)
{

  REPORT_INFO1(LC_HIGHUI, "[lfpport_qte_customitem.cpp:%d] "
	       "lfpport_customitem_GetLabelHeight" , __LINE__);

  if (width < 0) {
    width = qteapp_get_mscreen()->getScreenWidth() - ITEM_BOUND_PAD - ITEM_BOUND_PAD;
  }
  *heightRet = ((Item *)ciPtr->widgetPtr)->getLabelHeight(width);

  REPORT_INFO2(LC_HIGHUI, 
	      "\t lfpport_customitem_GetLabelHeight(%d): labelHeight=%d\n",
	      width, heightRet);


  return KNI_OK;
}

/**
 * Gets the padding for the custom item.
 *
 * @param pad the padding for the custom item.
 * @param ciPtr pointer to the custom item's MidpItem structure.
 *
 * @return an indication of success or the reason for failure
 */
extern "C" MidpError
lfpport_customitem_get_item_pad(int *pad, MidpItem* ciPtr)
{
  REPORT_INFO1(LC_HIGHUI, "[lfpport_qte_customitem.cpp:%d] "
	       "lfpport_customitem_GetItemPad", __LINE__);
  (void)ciPtr; // Suppress unused parameter warning

  *pad = ITEM_BOUND_PAD;

  return KNI_OK;
}

/**
 * Sets the content buffer. All paints are done to that buffer.
 * When paint is processed snapshot of the buffer is flushed to
 * the native resource content area.
 * 
 * @param ciPtr pointer to the custom item's MidpItem structure.
 * @param imgPtr pointer to the native resource corresponding
 *        to the Java offcreen buffer for CustomItem content
 */
extern "C" MidpError
lfpport_customitem_set_content_buffer(MidpItem* ciPtr, unsigned char* imgPtr) {
  REPORT_INFO1(LC_HIGHUI, "[lfpport_qte_customitem.cpp:%d] "
	       "lfpport_customitem_SetContentBuffer", __LINE__);
  
  ((CustomItem *)ciPtr->widgetPtr)->
    setContentBuffer(gxpportqt_get_mutableimage_pixmap(imgPtr));

  return KNI_OK;
}

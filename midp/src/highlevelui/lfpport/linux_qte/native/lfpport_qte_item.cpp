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

#include <lfpport_component.h>
#include <lfpport_form.h>
#include "lfpport_qte_item.h"
#include <moc_lfpport_qte_item.cpp>

#include "lfpport_qte_util.h"
#include "lfpport_qte_mscreen.h"

Item::Item(QWidget *parent, const QString &labelStr, int layout,
	   bool alwaysVertLayout) 
  : QWidget(parent) {

  TRACE_ITM(Item::Item..);

  init();

  this->alwaysVertLayout = alwaysVertLayout;

  
  label = labelStr.copy();

  this->layout = layout;

  TRACE_ITM(..Item::Item);

}

Item::~Item() {
}

MidpError Item::setLabel(const QString &labelStr) {
  TRACE_ITM(Item::setLabel);

  label = labelStr.copy();

  QWidget::repaint();

  return KNI_OK;
}

bool Item::labelCanBeOnSameLine(int labelHeight) {
  return label.isEmpty() || labelHeight <= labelLineHeight;
}

bool Item::bodyCanBeOnSameLine(int bodyHeight) {
  /* Suppress unused-parameter warning */
  (void)bodyHeight;
  
  return !alwaysVertLayout;
}

void Item::drawLabel(QPainter *painter, int x, int y, int w, int h) {
  TRACE_ITM(Item::drawLabel);
  if (!label.isEmpty()) {
    painter->setFont(labelFont);
    painter->drawText(x, y, w, h, WordBreak, label);
  }
}

MidpError Item::relocate(int x, int y) {
  TRACE_ITM(Item::relocate);
  int oldX = this->x();
  int oldY = this->y();

  move(x, y);

  // Patch Qt 2.3.2 feature that move() does not properly repaint widget that was
  // originally covered by other widget before this move.
  if (y > oldY || x > oldX) {
    repaint(); // force a repaint of the previously covered area
  }

  return KNI_OK;
}

MidpError Item::setSize(int w, int h) {
  
  QWidget::resize(w, h);
  
  w -= (ITEM_BOUND_PAD + ITEM_BOUND_PAD);
  h -= (ITEM_BOUND_PAD + ITEM_BOUND_PAD);
  
  QRect labelRect = QFontMetrics(labelFont).boundingRect(ITEM_BOUND_PAD,
							 ITEM_BOUND_PAD, 
							 w, h,
							 WordBreak, label);

  int bodyTakenWidth;
  int bodyHeight = bodyHeightForWidth(&bodyTakenWidth, w);

  // take care of the simple cases:
  // 1. both label and body are empty
  // 2. only label is empty
  // 3. only body is empty
  // Note that label or text do not take any space if its height is 0,
  // width cannot be checked since for end of line width will be 0
  // but such string has to be displayed
  if (label.isEmpty() || labelRect.height() == 0) {
    if (bodyHeight == 0) {
      bodyResize(0, 0);
    } else {
      // label is empty => allocate all the space to the body
      bodyResize(w, h);
      bodyRelocate(ITEM_BOUND_PAD, ITEM_BOUND_PAD);
    }
    return KNI_OK;
  } else if (bodyHeight == 0) {
    bodyResize(0, 0);
    return KNI_OK;
  }

  // allocate label area and set the rest for the body
  // if possible label and body will be put on the same line

  // single line case
  if (labelCanBeOnSameLine(labelRect.height()) && 
      bodyCanBeOnSameLine(bodyHeight) &&
      labelRect.width() + LABEL_BODY_HRZN_PAD + bodyTakenWidth <= w) {

      bodyResize(w - labelRect.width() - LABEL_BODY_HRZN_PAD, h);
      bodyRelocate(ITEM_BOUND_PAD + labelRect.width() + LABEL_BODY_HRZN_PAD,
		   ITEM_BOUND_PAD);

      return KNI_OK;
  }

  // multiline case
  bodyResize(w, h - labelRect.height() - LABEL_BODY_VERT_PAD);
  bodyRelocate(ITEM_BOUND_PAD,
	       ITEM_BOUND_PAD + labelRect.height() + LABEL_BODY_VERT_PAD);

  return KNI_OK;
}

MidpError Item::preferredWidth(int *w, int h) {


  if (h == -1) {
    
    int maxWidth = qteapp_get_mscreen()->getScreenWidth() - ITEM_BOUND_PAD - ITEM_BOUND_PAD;
    int bodyTakenWidth;
    int bodyHeight = bodyHeightForWidth(&bodyTakenWidth, maxWidth);

    QRect labelRect = 
      QFontMetrics(labelFont).boundingRect(ITEM_BOUND_PAD, ITEM_BOUND_PAD, 
					   maxWidth,
					   PREF_HEIGHT_LIMIT,
					   WordBreak, label);
    // take care of the simple cases:
    // 1. both label and body are empty
    // 2. only label is empty
    // 3. only body is empty
    // Note that label or text do not take any space if its height is 0,
    // width cannot be checked since for end of line width will be 0
    // but such string has to be displayed
    if (label.isEmpty() || labelRect.height() == 0) {
      if (bodyHeight == 0) {
	*w = 0;
      } else {
	*w = ITEM_BOUND_PAD + bodyTakenWidth + ITEM_BOUND_PAD;
      }
      return KNI_OK;

    } else if (bodyHeight == 0) {
      *w = ITEM_BOUND_PAD + labelRect.width() + ITEM_BOUND_PAD;
      return KNI_OK;
    }
    
    // single line case 
    if (labelCanBeOnSameLine(labelRect.height()) && 
	bodyCanBeOnSameLine(bodyHeight) &&
	labelRect.width() + LABEL_BODY_HRZN_PAD + bodyTakenWidth <= maxWidth) {

	*w = ITEM_BOUND_PAD + labelRect.width()
	   + LABEL_BODY_HRZN_PAD + bodyTakenWidth + ITEM_BOUND_PAD;
    
    // multiline case
    } else {
      // Take the wider one of label and body
      *w = ITEM_BOUND_PAD + (bodyTakenWidth > labelRect.width()
			     ? bodyTakenWidth : 
			     labelRect.width()) + ITEM_BOUND_PAD;
	// Sometimes the label or subclass returns taken width that is larger
	// than the tentative width. Patch it here.
	if (*w > qteapp_get_mscreen()->getScreenWidth()) {
	    *w  = qteapp_get_mscreen()->getScreenWidth();
	}
    }
  } else {
    *w = qteapp_get_mscreen()->getScreenWidth();
  }
  
  return KNI_OK;
}

MidpError Item::preferredHeight(int *h, int w) {

  // item cannot be laid out in a 0 width
  if (w == 0) {
    *h = 0;
    return KNI_OK;
  }

  // default preferred width is the whole screen width
  if (w == -1) {
    w = qteapp_get_mscreen()->getScreenWidth();
  }

  // now lets calculate height based on given tentative width
  w -= (ITEM_BOUND_PAD + ITEM_BOUND_PAD);


  int bodyTakenWidth;
  int bodyHeight  = bodyHeightForWidth(&bodyTakenWidth, w);

  QRect labelRect = 
    QFontMetrics(labelFont).boundingRect(ITEM_BOUND_PAD, ITEM_BOUND_PAD, 
					 w, PREF_HEIGHT_LIMIT,
					 WordBreak, label);

  // take care of the simple cases:
  // 1. both label and body are empty
  // 2. only label is empty
  // 3. only body is empty
  // Note that label or text do not take any space if its height is 0,
  // width cannot be checked since for end of line width will be 0
  // but such string has to be displayed
  if (label.isEmpty() || labelRect.height() == 0) {
    if (bodyHeight == 0) {
      *h = 0;
    } else {
      *h = ITEM_BOUND_PAD + bodyHeight + ITEM_BOUND_PAD;
    }
    return KNI_OK;
  } else if (bodyHeight == 0) {
    *h = ITEM_BOUND_PAD + labelRect.height() + ITEM_BOUND_PAD;
    return KNI_OK;
  }

  // single line case
  if (labelCanBeOnSameLine(labelRect.height()) && 
      bodyCanBeOnSameLine(bodyHeight) &&
      labelRect.width() + LABEL_BODY_HRZN_PAD + bodyTakenWidth <= w) {
      *h = ITEM_BOUND_PAD
	 + (labelRect.height() > bodyHeight ? labelRect.height() : bodyHeight)
	 + ITEM_BOUND_PAD;
      return KNI_OK;
  }

  // Multiline case
  *h = ITEM_BOUND_PAD + labelRect.height()
     + LABEL_BODY_VERT_PAD + bodyHeight + ITEM_BOUND_PAD;

  return KNI_OK;
}

MidpError Item::minimumWidth(int *w) {
  // actual width could be smaller than the default minimum width
   preferredWidth(w, -1);

   // problem: there is no arbitrary value to suit all item types
   if (*w > MIN_WIDTH_LIMIT) {
     *w = MIN_WIDTH_LIMIT;
   }

   return KNI_OK;
}

MidpError Item::minimumHeight(int *h) {
    // The smallest height could be if we use all the width
    // available or should we allow clipping 
    // if clipping is allowed setSize will be complex
    preferredHeight(h, -1);

    return KNI_OK;
}


void Item::paintEvent(QPaintEvent *e)
{
  QPainter painter(this);

  //draw label
  drawLabel(&painter, ITEM_BOUND_PAD, ITEM_BOUND_PAD, 
	    width()- ITEM_BOUND_PAD - ITEM_BOUND_PAD,
	    height() - ITEM_BOUND_PAD - ITEM_BOUND_PAD);

  QWidget::paintEvent(e);
}

void Item::init() {
    TRACE_ITM(Item::init..);
    labelFont = QFont("Times", 10, QFont::Bold);
    QFontMetrics labelFontMetrics(labelFont);
    labelLineHeight = labelFontMetrics.height();
    TRACE_ITM(..Item::init);
}

/**
 * Gets actual width used by the label based on a passed content width.
 */
int Item::getLabelWidth(int contentWidth) {
  return QFontMetrics(labelFont).boundingRect(ITEM_BOUND_PAD, ITEM_BOUND_PAD,
				contentWidth - ITEM_BOUND_PAD - ITEM_BOUND_PAD,
					      PREF_HEIGHT_LIMIT,
					      WordBreak, label).width();
}

/**
 * Gets label height used based on the passed in width
 */
int Item::getLabelHeight(int w) {
  return QFontMetrics(labelFont).boundingRect(ITEM_BOUND_PAD, ITEM_BOUND_PAD, 
					w - ITEM_BOUND_PAD - ITEM_BOUND_PAD,
					      PREF_HEIGHT_LIMIT,
					      WordBreak, label).height();
}

/* *********************************************************************** */

extern "C" {

  MidpError set_label(MidpItem *itemPtr, const pcsl_string *label) {
    QString labelStr;   

    pcsl_string2QString(*label, labelStr);
    return ((Item *)itemPtr->widgetPtr)->setLabel(labelStr);
  }
  
  MidpError show(MidpItem *itemPtr){
    ((Item *)itemPtr->widgetPtr)->show();

    /* showNotify() if inside the viewport */
    /* (Item *)itemPtr->widgetPtr)->notifyShowHide(); */

    return KNI_OK;
  }

  MidpError hide(MidpItem *itemPtr){ 
    ((Item *)itemPtr->widgetPtr)->hide();
    return KNI_OK;
  }

  MidpError destroy(MidpItem *itemPtr){ 
    delete (Item *)itemPtr->widgetPtr;
    return KNI_OK;
  }

  MidpError relocate(MidpItem *itemPtr, int x, int y) {
    return ((Item *)itemPtr->widgetPtr)->relocate(x, y);
  }

  MidpError resize(MidpItem *itemPtr, int w, int h) {
    return ((Item *)itemPtr->widgetPtr)->setSize(w, h);
  }


  MidpError get_minimum_width(int *w, MidpItem *itemPtr){ 
    return ((Item *)itemPtr->widgetPtr)->minimumWidth(w);
  }
  
  MidpError get_minimum_height(int *h, MidpItem *itemPtr){ 
    return ((Item *)itemPtr->widgetPtr)->minimumHeight(h);
  }
  
  MidpError get_preferred_height(int *h, MidpItem *itemPtr, int w){
    return ((Item *)itemPtr->widgetPtr)->preferredHeight(h, w); 
  }
  
  MidpError get_preferred_width(int *w, MidpItem *itemPtr, int h){ 
    return ((Item *)itemPtr->widgetPtr)->preferredWidth(w, h);
  }

} /* extern "C" */

/**
 */
MidpError initItemPtr(MidpItem* itemPtr, 
		      MidpDisplayable* ownerPtr) {

  /* widgetPtr initialized in the caller function */
  itemPtr->ownerPtr           = ownerPtr;

  /* type and layout are not initialized */

  itemPtr->show               = show;
  itemPtr->hide		      = hide;
  itemPtr->destroy 	      = destroy;
  itemPtr->relocate           = relocate;
  itemPtr->setLabel           = set_label;

  itemPtr->resize             = resize;
  itemPtr->getMinimumWidth    = get_minimum_width;
  itemPtr->getMinimumHeight   = get_minimum_height;
  itemPtr->getPreferredWidth  = get_preferred_width;
  itemPtr->getPreferredHeight = get_preferred_height;
  
  return KNI_OK;
}

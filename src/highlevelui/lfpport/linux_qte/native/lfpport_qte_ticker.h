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
 * Qt port of Ticker.
 */

#ifndef _LFPPORT_QTE_TICKER_H_
#define _LFPPORT_QTE_TICKER_H_

#include <qapplication.h>
#include <qframe.h>
#include <qlabel.h>
#include <qmainwindow.h>
#include <qwidget.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qrect.h>
#include <qmetaobject.h>
#include <qpushbutton.h>
#include <qtimer.h>


/**
 * milliseconds between ticker text is moved.
 */
#define QT_TICKER_SPEED 30

/**
 * Number of pixels that two ticker texts should be spaced by.
 */
#define TICKER_SPACE    80

/**
 * Native platform Ticker class 
 */
class Ticker : public QFrame
{
  Q_OBJECT

  /** Speed property that can be read by currentSpeed()
      and change by setSpeed() */
  Q_PROPERTY( int speed READ currentSpeed WRITE setSpeed )
    
public:
  /**
   * Construct a native widget for Ticker.
   *
   * @param parent parent widget
   * @param Qt widget name
   */
  Ticker( QWidget* parent = 0, const char* name = 0 );

  /**
   * Set ticker text.
   */
  void setTickerString(const QString& incomeText);

  /**
   * Override to calculate size.
   */
  QSize sizeHint() const;

  /**
   * Return the current delay time between moving text.
   *
   * @return number of milliseconds
   */
  int currentSpeed() const { return speed; }

  /**
   * Set delay time between moving text.
   *
   * @param newSpeed number of milliseconds
   */
  void setSpeed( int newSpeed );
 
protected:

  /**
   * Override QFrame to paint ticker text.
   */
  void drawContents( QPainter * p );

  /**
   * Update ticker text.
   */
  void setText( const QString& text ) { txt = text; update(); }
 
 private:
  /**
   * X coordinate from which the text should be shown.
   */
  int offset;

  /**
   * Milliseconds between moving text.
   */
  int speed;

  /**
   * Ticker text.
   */
  QString txt; 

  /**
   * Timer to move the text.
   */
  QTimer timer;

  /**
   * enables cache
   */
  bool isCached;

  /**
   * cached text width
   */
  int cachedTextWidth;

  /**
   * cached ticker width available for text
   */
  int cachedWidth;

  /**
   * cached ticker height available for text
   */
  int cachedHeight;

  /**
   * cached ticker maximum frame width 
   * (frame width can be different for active and inactive states)
   */
  int maxFrameWidth;

private slots:

    /**
     * Move the text forward.
     */
    void progress();

};

#endif /* _LFPPORT_QTE_TICKER_H */

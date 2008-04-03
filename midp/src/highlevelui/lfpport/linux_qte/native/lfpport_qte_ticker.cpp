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

#include <stdio.h>
#include <kni.h>

#include <qapplication.h>
#include <qmainwindow.h>
#include <qpainter.h>
#include <qpaintdevice.h>
#include <qpaintdevicemetrics.h>
#include <qaction.h> 
#include <qstatusbar.h> 

#include <midpServices.h>
#include <midpMalloc.h>
#include <jvm.h>
#include <jvmspi.h>
#include <sni.h>
#include <lfpport_displayable.h>
#include <midpString.h>

#include "lfpport_qte_ticker.h"
#include "lfpport_qte_util.h"
#include "lfpport_qte_mscreen.h"
#include "lfpport_qte_mainwindow.h"
#include <moc_lfpport_qte_ticker.cpp>

/**
 * @file
 *
 * This file contains the full implementation of ticker
 * for native platform widget subsystem
 */

/**
 * Native platform Ticker class
 *
 */
Ticker::Ticker( QWidget* parent, const char* name )
  : QFrame( parent, name ), offset( 5 ), speed( 0 ), txt( "Ready" )
{
  setFrameStyle( WinPanel | Sunken );
  maxFrameWidth = frameWidth();
  setFrameStyle( Panel | Sunken );
  if (maxFrameWidth < frameWidth()) {
    maxFrameWidth = frameWidth();
  }

  setBackgroundMode( PaletteBase );

  connect( &timer, SIGNAL( timeout() ), this, SLOT( progress() ) );
  isCached = FALSE;
}

QSize Ticker::sizeHint() const {
  return QSize( 40*fontMetrics().width('x'),
		fontMetrics().height()+2*maxFrameWidth);
}

void Ticker::setTickerString(const QString& incomeText) { 

  isCached = FALSE;

  // We test to see if they are null or empty string
  if (!incomeText.isNull() && !incomeText.isEmpty()) {

    if (isVisible() && (incomeText != txt) ) {

      setText(incomeText);
      setSpeed(QT_TICKER_SPEED);

    } // else same ticker text - do nothing.

  } else { // string is null or empty

    if (isVisible()) {

      // When speed is 0 the ticker is actually a status bar
      setSpeed(0);

      setText("Ready");
    } // if it is already off, do nothing
  }

}

void Ticker::setSpeed( int newSpeed ) {
 if (newSpeed > 0) {
   if ( !timer.isActive() ) {
     setFrameStyle( WinPanel | Sunken );
     timer.start( newSpeed );
   }
 } else {
   timer.stop();
   setFrameStyle( Panel | Sunken );
 }

  speed = newSpeed;

  if ( timer.isActive() )
    timer.changeInterval(speed);
}

/**
 * Override QFrame to paint ticker text.
 */
void Ticker::drawContents( QPainter * p ) {

  if (isCached == FALSE) {
    cachedTextWidth = fontMetrics().width( txt );
    cachedWidth = width();
    cachedHeight = height();
    isCached = TRUE;
  }


  if (speed < 1) {
      offset = 5;

      p->drawText( 2, 0, INT_MAX, height(), AlignVCenter, txt );
      return;
  }

  int fw = cachedTextWidth + TICKER_SPACE;

  for ( int x = 0; x < cachedWidth - offset; x += fw )
      p->drawText( x + offset, 0, INT_MAX, cachedHeight, AlignVCenter, txt );

}

void Ticker::progress() {
  if (isCached == FALSE) {
    cachedTextWidth = fontMetrics().width( txt );
    cachedWidth = width();
    cachedHeight = height();
    isCached = TRUE;
  }


  if ( --offset < -( cachedTextWidth + TICKER_SPACE) )
    offset = 0;
  scroll( -1, 0, contentsRect() );
}


extern "C" {

MidpError displayable_set_ticker(MidpDisplayable* dispPtr, const pcsl_string* text) {
  (void)dispPtr; // Suppress warning

    if (!pcsl_string_is_null(text)) {
      QString textStr;
      
      pcsl_string2QString(*text, textStr);
      PlatformMIDPMainWindow::getMainWindow()->setTickerString(textStr);

    } else {
      PlatformMIDPMainWindow::getMainWindow()->setTickerString(QString::null);
    }
    return KNI_OK;
}
    
}



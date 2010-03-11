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
 *
 * This is the main window of QTE version. It is the
 * container of every other widgets.
 */

#ifndef _LFPPORT_QTE_MAINWINDOW_H_
#define _LFPPORT_QTE_MAINWINDOW_H_

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
#include <qwidgetstack.h>
#include <qscrollview.h>
#include <qlayout.h>

#include <qteapp_export.h>

#include "lfpport_qte_ticker.h"
#include "lfpport_qte_command.h"
#include "lfpport_qte_mscreen.h"

/**
 * MIDP main window class.
 *
 * It includes the functionality of maintain ticker,
 * fullscreen mode, and menu command.
 */
class PlatformMIDPMainWindow : public MIDPMainWindow
{

  Q_OBJECT

public:
  /**
   * Constructor.
   */
  PlatformMIDPMainWindow( QWidget *parent=0, const char* name=0);

   /**
    * MIDPMainWindow destructor.
    */
  ~PlatformMIDPMainWindow();

  /**
   * Set main window title bar
   */
  void setTitleBar(const QString &title);

  /**
   * Gives the ticker of the main window the given string contents.
   *
   * @param label the content that the ticker should display.
   */
  void setTickerString(const QString& textStr);

  /**
   * Returns a pointer to the ticker object of the main window.
   *
   * @return pointer to this window's ticker.
   */
  Ticker* getTicker() { return ticker; }

  // MScreen

  /**
   * Returns any mscreen associated with this main window.
   *
   * @return the mscreen associated with this main window, or
   * null if there is none.
   */
  MScreen* getMScreen() { return mscreen; }

  /**
   * Sets the main window to full screen. If the given
   * request asks for the current screen mode, this function
   * does nothing.
   *
   * Under normal screen mode, title, displayable widget and ticker are all
   * visible.
   *
   * Under full screen mode, title and ticker are hidden. The displayable
   * widget expands to take over the space left.
   * Qtopia status bar at the bottom remains visible as usual.
   *
   * @param fullscn 0 to request a &quote;normal&quote;
   * screen mode; a non-zero value to request the
   * full-screen.
   */
  void setFullScreen(const int fullscn);

  /**
   * Request title and ticker been shown.
   */
  void showNormalScreen();

  /**
   * Request title and ticker give space to displayable widget.
   */
  void showFullScreen();

  /**
   * Refresh screen after width or height was changed 
   */
  void resizeScreen();

  /**
   * Return the abstract command widget.
   *
   * @return the menu bar widget that holds all the abstract commands
   */
  CommandManager* getMenu();

  /**
   * Returns a pointer to the single PlatformMIDPMainWindow instance,
   * or NULL if it has not been created yet.
   */
  static PlatformMIDPMainWindow * getMainWindow() {
    return (PlatformMIDPMainWindow*)qteapp_get_main_window();
  }

   /**
    * Override to notify Java of key press.
    */
   void keyPressEvent(QKeyEvent *key);

protected:
  /**
   * Filter special system level Qt events, like Close event,
   * to notify Java layer.
   */
  bool eventFilter(QObject *obj, QEvent *e);

private:

  /**
   * The main child of this main widget. The main child is the big widget around
   * which the tool bars are arranged.
   */
  QWidget *mwindow;

  /**
   * Layout for displayable widget and ticker.
   */
  QVBoxLayout *box;

  /**
   * A reference needed by this class
   * to delegate command to Ticker object.
   */
  Ticker *ticker;

  /**
   * Whether the ticker is on: true
   * if it is; false otherwise.
   */
  bool isTickerOn;

  /**
   * A reference needed by this class
   * to delegate command to MScreen.
   */
  PlatformMScreen *mscreen; 

  /**
   * Whether the main window is in full-screen mode: true
   * if it is; false otherwise.
   */
  bool isFullScreen;


  /**
   * A reference needed by this class
   * to delegate command to CommandManager.
   */
  CommandManager *cmd;
};

MIDPMainWindow* lfpport_create_main_window(
  QWidget* parent, const char* name);

#endif /* _LFPPORT_QTE_MAINWINDOW_H_ */

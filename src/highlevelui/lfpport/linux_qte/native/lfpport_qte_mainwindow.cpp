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
#include <qlayout.h> 
#include <qtoolbar.h> 
#include <qaction.h> 
#include <qmessagebox.h> 
#include <qstatusbar.h> 
#include <qsizepolicy.h>

#include <midpServices.h>
#include <midpMalloc.h>
#include <jvm.h>
#include <jvmspi.h>
#include <sni.h>
#include <midpEventUtil.h>
#include <midpString.h>

#include "lfpport_qte_mainwindow.h"
#include "lfpport_qte_ticker.h"
#include "lfpport_qte_mscreen.h"
#include "lfpport_qte_util.h"

#if ENABLE_MULTIPLE_DISPLAYS
#include <lcdlf_export.h>
#endif /* ENABLE_MULTIPLE_DISPLAYS */

#if (!defined(QT_NO_QWS_KEYBOARD) && defined(QT_KEYPAD_MODE))
#include "lfpport_qte_inputmode.h"
#include <qteapp_export.h>
#endif

#include <moc_qteapp_mainwindow.cpp>
#include <moc_lfpport_qte_mainwindow.cpp>

/**
 * @file
 *
 * Main MIDP window intelligence. Contains controls for
 * things like event filtering and window sizing.
 */

/** Indicates that system is in the process of shutting 
 down, therefore "window close" events should be
 passed on to QT. */
extern jboolean shuttingDown;

/**
 * @class MIDPMainWindow lfpport_qte_mainwindow.h
 * @brief The main top window. A base for MScreen, ticker, etc.
 *
 * This is the main widget.
 */
PlatformMIDPMainWindow::PlatformMIDPMainWindow(QWidget *parent, 
                                               const char* name) :
  MIDPMainWindow(parent, name) {

  TRACE_MSC(PlatformMIDPMainWindow::PlatformMIDPMainWindow...);

  // Initial screen is always normal
  isFullScreen = FALSE;

  //  Top widget
  mwindow = new QWidget(this);

  ticker  = new Ticker(mwindow);
  mscreen = new PlatformMScreen(mwindow);
  mscreen->init();
  cmd     = new CommandManager(this);

  box = new QVBoxLayout(mwindow, 1 /* border */, 1 /* space */);

  // Default adding direction: from top to bottom
  box->addWidget(mscreen);
  box->addWidget(ticker);

  setCentralWidget(mwindow);

  // Set initial title before any Displayable is shown
  setTitleBar(MAINWINDOW_TITLE);

  // Fix main window geometry
  int WINDOW_HEIGHT = mscreen->getDisplayFullHeight() + MENUBAR_HEIGHT;
  setFixedSize(mscreen->getDisplayFullWidth(), WINDOW_HEIGHT);

  // Misc set-up
  ticker->setTickerString(QString::null);
  mscreen->setFocus();
  this->installEventFilter(this);

  // init input mode
#if (!defined(QT_NO_QWS_KEYBOARD) && defined(QT_KEYPAD_MODE))
  QWSServer * server = qteapp_get_server();
  server->setKeyboardFilter(new AbcKeyboardFilter());
#endif
  
  
  TRACE_MSC(...PlatformMIDPMainWindow::PlatformMIDPMainWindow);
}

/**
 * Deconstructor for MIDPMainWindow.
 * It has to remove any timer left.
 */
PlatformMIDPMainWindow::~PlatformMIDPMainWindow() {
    finalizeMenus();
    killTimers();
}

/**
 * Set main window title
 */
void PlatformMIDPMainWindow::setTitleBar(const QString &title) {
    static WFlags flags = 0;
    QWidget *parent = this->parentWidget();
    QPoint pos = this->pos();
    WFlags f = WStyle_Customize;
    setCaption(title);
    if (!title.isNull() && !title.isEmpty()) {
        f |= (WStyle_Title | WStyle_DialogBorder);
    }
    if (flags != f) {
        flags = f;
        reparent(parent, f, pos);
    }
}

/**
 * Return the menu command object.
 */
CommandManager* PlatformMIDPMainWindow::getMenu()
{
    return cmd;
}

/**
 * Override to notify Java of key press.
 */
void PlatformMIDPMainWindow::keyPressEvent(QKeyEvent *key)
{
    // return event to PlatformMScreen, wich is responsible
    // for notifying java
    mscreen->keyPressEvent(key);
}

/**
 * Ask the ticker associate with this mainwindow
 * to set the text. It is not main window responsible
 * to do so.
 *
 * @param textStr the string to be set to the ticker
 */
void
PlatformMIDPMainWindow::setTickerString(const QString& textStr)
{
    if (ticker)
       ticker->setTickerString(textStr);
}

/**
 * Returns TRUE if this event has been handled
 */
bool 
PlatformMIDPMainWindow::eventFilter(QObject *obj, QEvent *e) {
    // Filter out Close event to allow VM properly shuts down
    if (((e->type() == QEvent::Close) && !shuttingDown) ||
        ((e->type() == QEvent::KeyPress || e->type() == QEvent::Accel) &&
#ifdef QT_KEYPAD_MODE
         ((QKeyEvent *)e)->key() == Qt::Key_Hangup)) {  
#else
        (((QKeyEvent *)e)->key() == Qt::Key_End ||
         ((QKeyEvent *)e)->key() == Qt::Key_F8 ))) {
#endif
        // Pressing the (x) button means to destroy the
        // foreground MIDlet.
        MidpEvent evt;

        MIDP_EVENT_INITIALIZE(evt);
        
#if ENABLE_MULTIPLE_ISOLATES
        evt.type = MIDLET_DESTROY_REQUEST_EVENT;
#if ENABLE_MULTIPLE_DISPLAYS  
            evt.DISPLAY = gForegroundDisplayIds[lcdlf_get_current_hardwareId()];  
#else  
            evt.DISPLAY = gForegroundDisplayId;  
#endif /* ENABLE_MULTIPLE_DISPLAYS */  
        evt.intParam1 = gForegroundIsolateId;
        midpStoreEventAndSignalAms(evt);
#else
        evt.type = DESTROY_MIDLET_EVENT;
        midpStoreEventAndSignalForeground(evt);
#endif

        if (e->type() == QEvent::Close) {
            ((QCloseEvent*)e)->ignore();
        }
        return TRUE;
    }

    if (e->type() == QEvent::KeyPress || e->type() == QEvent::Accel) {
        QKeyEvent *ke = (QKeyEvent *) e;
        if (ke->key() == Qt::Key_Home ||
            ke->key() == Qt::Key_F7) {
            mscreen->keyPressEvent(ke);
            ke->ignore();
            return TRUE;
        }
    }

    return QWidget::eventFilter(obj, e );
}

/**
 * Setup full screen mode
 *
 * @param fullscn true if fullscreen mode is wanted, false
 * when normal screen mode is needed.
 */
void PlatformMIDPMainWindow::setFullScreen(int fullscn) {
 
     if (fullscn && !isFullScreen) {
        showFullScreen();
     }

     if (!fullscn && isFullScreen) {
        showNormalScreen();
     }
}

/**
 * Refresh screen after width or height was changed
 *
 * @param fullscn true if fullscreen mode is active, false
 * when normal screen mode is active.
 */
void PlatformMIDPMainWindow::resizeScreen() {

     if (isFullScreen) {
        showFullScreen();
     } else {
        showNormalScreen();
     }
}

/**
 * Show full screen more - in our case, it means no ticker.
 */
void PlatformMIDPMainWindow::showFullScreen(void) {
          
    isFullScreen = TRUE;

    ticker->hide();

    // Delete the current layout, a must.
    // otherwise, won't work.
    delete box;

    box = new QVBoxLayout(mwindow, 0 /* border */, 0 /* space */);

    mscreen->setVScrollBarMode(QScrollView::AlwaysOff);
    mscreen->setBufferSize(MScreen::fullScreenSize);
    box->addWidget(mscreen);

    setCentralWidget(mwindow);
    setFixedSize(mscreen->getDisplayFullWidth(), mscreen->getDisplayFullHeight() + MENUBAR_HEIGHT);
    mscreen->setFocus();

}

/**
 * Restructure the whole layout of this app to
 * show normal screen mode.
 */
void PlatformMIDPMainWindow::showNormalScreen(void) {

    isFullScreen = FALSE;

    // Delete the current layout, a must.
    // otherwise, won't work.
    delete box;

    box = new QVBoxLayout(mwindow, 1 /* border */, 1 /* space */);

    mscreen->setVScrollBarMode(QScrollView::Auto);
    mscreen->setBufferSize(MScreen::normalScreenSize);
    box->addWidget(mscreen);
    box->addWidget(ticker);
    ticker->show();

    setCentralWidget(mwindow);
    setFixedSize(mscreen->getDisplayWidth(), mscreen->getDisplayFullHeight() + MENUBAR_HEIGHT);
    mscreen->setFocus();
}

MIDPMainWindow* lfpport_create_main_window(
  QWidget* parent, const char* name) {
  return new PlatformMIDPMainWindow(parent, name);
}



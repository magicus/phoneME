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
#include <qapplication.h>
#include <qmainwindow.h>
#include <qpainter.h>
#include <qpaintdevice.h>
#include <qpaintdevicemetrics.h>
#include <qlayout.h>
#include <qtoolbar.h>
#include <qmenubar.h>
#include <qaction.h>
#include <qmessagebox.h>
extern "C" {
#include <midpServices.h>
#include <midpMalloc.h>
#include <jvm.h>
#include <jvmspi.h>
#include <sni.h>
#include <midpEventUtil.h>
}

#if ENABLE_MULTIPLE_DISPLAYS
#include <lcdlf_export.h>
#endif /* ENABLE_MULTIPLE_DISPLAYS */

#include <qteapp_key.h>
#include "lfjport_qte_mainwindow.h"
#include "lfjport_qte_mscreen.h"
#include <moc_qteapp_mainwindow.cpp>
#include <moc_lfjport_qte_mainwindow.cpp>

/**
 * @file
 *
 * Main MIDP window intelligence. Contains controls for
 * things like event filtering and window sizing.
 */

/** Indicates that system is in the process of shutting
 down, therefore "window close" events should be passed
 on to QT. Defined in
 core/common/native/linux_qte/cldc_vm/UI_linux.cpp */
extern jboolean shuttingDown;


ChameleonMIDPMainWindow::ChameleonMIDPMainWindow(
  QWidget *parent, const char* name) : MIDPMainWindow(parent, name) {
    // Initial screen is always normal
    isFullScreen = FALSE;

    //  Top widget
    mwindow = new QWidget(this);

    indicatorBar = IndicatorBar::createSingleton(mwindow);

    mscreen = new ChameleonMScreen(mwindow);

    box = new QVBoxLayout(mwindow, 0 /* border */, 0 /* space */);

    // Layout indicator bar on top of mscreen
    box->addWidget(indicatorBar);
    box->addWidget(mscreen);

    setCentralWidget( mwindow );

    //
    // Misc set-up
    //
    int indicatorHeight = indicatorBar->height();
    setFixedSize(CHAM_WIDTH, CHAM_HEIGHT + indicatorHeight);
    setCaption(MAINWINDOW_TITLE);
    mscreen->setFocus();
    this->installEventFilter(this);
}

ChameleonMIDPMainWindow::~ChameleonMIDPMainWindow() {

    killTimers();
}

// Returns TRUE if this event has been handled
bool ChameleonMIDPMainWindow::eventFilter(QObject *obj, QEvent *e) {
    if (((e->type() == QEvent::Close) && !shuttingDown) ||
        ((e->type() == QEvent::KeyPress || e->type() == QEvent::Accel) &&
#ifdef QT_KEYPAD_MODE
         ((QKeyEvent *)e)->key() == Qt::Key_Hangup)) {  
#else
        (((QKeyEvent *)e)->key() == Qt::Key_End ||
         ((QKeyEvent *)e)->key() == Qt::Key_F8))) {
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

    // Forward Home key presses to mscreen to resume apps
    if (e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = (QKeyEvent *) e;
        
        if (ke->key() == Qt::Key_Home ||
            ke->key() == Qt::Key_F7 ) {
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
void ChameleonMIDPMainWindow::setFullScreen(int fullscn) {
 
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
void ChameleonMIDPMainWindow::resizeScreen() {

     if (isFullScreen) {
        showFullScreen();
     } else {
        showNormalScreen();
     }
}

/**
 * Show full screen more - in our case, it means no indicator bar.
 */
void ChameleonMIDPMainWindow::showFullScreen(void) {

    isFullScreen = TRUE;

    indicatorBar->hide();

    // Delete the current layout, a must.
    // otherwise, won't work.
    delete box;

    box = new QVBoxLayout(mwindow, 0 /* border */, 0 /* space */);

    mscreen->setBufferSize(MScreen::fullScreenSize);
    box->addWidget(mscreen);

    setCentralWidget(mwindow);
    setFixedSize(mscreen->getDisplayFullWidth(), mscreen->getDisplayFullHeight());
    mscreen->setFocus();

}

/**
 * Restructure the whole layout of this app to
 * show normal screen mode.
 */
void ChameleonMIDPMainWindow::showNormalScreen() {

    isFullScreen = FALSE;
    int screenWidth = mscreen->getDisplayWidth();

    // Delete the current layout, a must.
    // otherwise, won't work.
    delete box;

    box = new QVBoxLayout(mwindow, 0 /* border */, 0 /* space */);

    mscreen->setBufferSize(MScreen::normalScreenSize);
    box->addWidget(indicatorBar);
    box->addWidget(mscreen);
    indicatorBar->setFixedWidth(screenWidth);
    indicatorBar->show();

    setCentralWidget(mwindow);
    int indicatorHeight = indicatorBar->height();
    setFixedSize(screenWidth, mscreen->getDisplayHeight() + indicatorHeight);
    mscreen->setFocus();
}


void ChameleonMIDPMainWindow::fileExit() {
    MidpEvent evt;

    MIDP_EVENT_INITIALIZE(evt);

    evt.type = SHUTDOWN_EVENT;

    midpStoreEventAndSignalAms(evt);
}

MIDPMainWindow* lfjport_create_main_window(
  QWidget* parent, const char* name) {
  return new ChameleonMIDPMainWindow(parent, name);
}


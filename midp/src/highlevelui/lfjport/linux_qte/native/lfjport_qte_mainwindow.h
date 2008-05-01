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
 * IMPL_NOTE:Definition of the MIDPMainWindow widget
 * Definition of the MIDPMainWindow widget
 */
#ifndef _LFJPORT_MAINWINDOW_QTE_H
#define _LFJPORT_MAINWINDOW_QTE_H

#include <qteapp_mainwindow.h>
#include <anc_qte_indicator.h>
#include "lfjport_qte_mscreen.h"
#include <qteapp_export.h>

/**
 * IMPL_NOTE:Document MIDPMainWindow
 */
class ChameleonMIDPMainWindow : public MIDPMainWindow
{
    Q_OBJECT

public:
    /**
     * IMPL_NOTE:Document MIDPMainWindow constructor
     */
    ChameleonMIDPMainWindow( QWidget *parent=0, const char* name=0 );

    /**
     * IMPL_NOTE:Document the MIDPMainWindow destructor
     */
    virtual ~ChameleonMIDPMainWindow();

    /**
     * IMPL_NOTE:Document *getMScreen
     */
    MScreen *getMScreen() { return mscreen; }

   /**
    * Sets the main window to full screen. If the given
    * request asks for the current screen mode, this function
    * does nothing.
    *
    * Under normal screen mode, displayable widget and indicator bar
    * are all visible.
    *
    * Under full screen mode, indicator is hidden. The displayable
    * widget expands to take over the space left.
    *
    * @param fullscn 0 to request a &quote;normal&quote;
    * screen mode; a non-zero value to request the
    * full-screen.
    */
    void setFullScreen(const int fullscn);

   /**
    * Request indicator bar been shown.
    */
    void showNormalScreen();

   /**
    * Request indicator bar give space to displayable widget.
    */
    void showFullScreen();

    /**
     * Refresh screen after width or height was changed
     */
    void resizeScreen();

   /**
    * Returns a pointer to the single ChameleonMIDPMainWindow instance,
    * or NULL if it has not been created yet.
    */
    static ChameleonMIDPMainWindow * getMainWindow() {
        return (ChameleonMIDPMainWindow*)qteapp_get_main_window();
    }

    
public slots:
    /**
     * IMPL_NOTE:Document fileExit
     */
    void fileExit();

protected:
    /**
     * Checks whether this object or the given object has handled the
     * given event.
     *
     * @returns true if the given event has been handled; false otherwise.
     */
    bool eventFilter(QObject *obj, QEvent *e);

private:

    /**
    * The main child of this main widget.
    */
    QWidget *mwindow;

    /**
    * Layout for displayable widget and indicator bar.
    */
    QVBoxLayout *box;

    /**
     * IMPL_NOTE:Document mscreen
     */
    ChameleonMScreen  *mscreen;

    /**
     * Indicator bar widget that hosts Home, Networking and Trusted MIDlet
     * icons.
     */
    IndicatorBar *indicatorBar;

    /**
    * Whether the main window is in full-screen mode: true
    * if it is; false otherwise.
    */
    bool isFullScreen;

};


MIDPMainWindow* lfjport_create_main_window(
  QWidget* parent, const char* name);

#endif /* _LFJPORT_MAINWINDOW_QTE_H */

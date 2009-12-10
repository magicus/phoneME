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

#ifndef _QTEAPP_MAINWINDOW_H_
#define _QTEAPP_MAINWINDOW_H_


/**
 * @file
 * @ingroup highui_qteapp
 *
 * @brief Linux/Qte main window interface
 */

#include <qmainwindow.h>
#include <qteapp_mscreen.h>

/**
 * Main application window.
 * Platform-specific main window classes inherit from this class.
 * @see ChameleonMIDPMainWindow, PlatformMIDPMainWindow
 */
class MIDPMainWindow : public QMainWindow {
  Q_OBJECT

protected:
/**
 * Constructs a MIDPMainWindow object.
 *
 * @param parent the parent widget, may be null
 * @param name a name for the new window, may be null
 */
  MIDPMainWindow(QWidget *parent, const char* name);

public:
  /**
   * Returns any mscreen associated with this main window.
   *
   * @return the mscreen associated with this main window, or
   * null if there is none.
   */
  virtual MScreen* getMScreen() = 0;
};

/**
 * Defines a pointer to the main window creation function,
 * for passing such functions as parameters.
 *
 * @param parent the parent widget for the new window
 * @param name name for the new window
 */
typedef MIDPMainWindow* (*MIDPMainWindowCreateProc)(QWidget* parent, 
						    const char* name);

/* @} */

#endif /* _QTEAPP_MAINWINDOW_H_ */

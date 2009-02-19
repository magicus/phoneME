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

#ifndef _QTEAPP_EXPORT_H_
#define _QTEAPP_EXPORT_H_


/**
 * @defgroup highui_qteapp Linux/Qte Application External Interface
 * @ingroup highui
 */

/**
 * @file
 * @ingroup highui_qteapp
 *
 * @brief Linux/Qte application exported native interface
 */

#include <qapplication.h>
#include <qteapp_mainwindow.h>
#include <qwindowsystem_qws.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes the Qtopia application and associated resources.
 */
extern void qteapp_init(MIDPMainWindowCreateProc p);

/**
 * Finalize the Qtopia application and associated resources.
 */
extern void qteapp_finalize();

/**
 * Returns a pointer to the QApplication instance of MIDP application.
 */
extern QApplication * qteapp_get_application();

/**
 * Returns a pointer to the QWSServer of MIDP application.
 */
extern QWSServer * qteapp_get_server();

/**
 * Returns a pointer to the main window of MIDP application.
 */
extern MIDPMainWindow * qteapp_get_main_window();

/**
 * Returns a pointer to the MScreen instance of MIDP application.
 */
extern MScreen * qteapp_get_mscreen();

/**
 * Requests that the slave mode VM control code schedule a time slice 
 * as soon as possible, since Java platform threads are waiting to be run.
 */
extern void qteapp_schedule_time_slice();

/**
 * Runs the Qte event loop.
 */
extern void qteapp_event_loop();

#ifdef __cplusplus
}
#endif

/* @} */

#endif /* _QTEAPP_EXPORT_H_ */

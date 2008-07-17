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
 * Displayable title handling.
 * Look at lfpport_qte_ticker.cpp for ticker handling.
 */
#include "lfpport_qte_displayable.h"
#include "lfpport_qte_util.h"
#include "lfpport_qte_mainwindow.h"

#include <qwidget.h>
#include <qstring.h>
#include <midpUtilKni.h>

PCSL_DEFINE_ASCII_STRING_LITERAL_START(truncmark)
{ 0x2026, 0 }
PCSL_DEFINE_ASCII_STRING_LITERAL_END(truncmark);

/** Title handling function pointer */
extern "C" MidpError
displayable_set_title(MidpDisplayable* screenPtr, const pcsl_string* title) {
  /* Suppress unused-parameter warning */
    (void)screenPtr;
    QString qtitle;
    pcsl_string2QString(*title, qtitle);
    truncateQString(qtitle,
            qteapp_get_main_window()->font(),
            calculateCaptionWidth(qteapp_get_application()->mainWidget()));
    qteapp_get_main_window()->setCaption(qtitle);

    return KNI_OK;
}


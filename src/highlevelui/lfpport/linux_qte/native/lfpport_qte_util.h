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
 * Macros and functions used by the files and functions that define LCDUI's
 * platform widgets.
 */

#ifndef _LFPPORT_QTE_UTIL_H_
#define _LFPPORT_QTE_UTIL_H_

#include <lfpport_error.h>
#include <midpString.h>
#include <stdio.h>
#include <midp_logging.h>

#include <qstring.h>


#if REPORT_LEVEL <= LOG_INFORMATION

/**
 * Macro to report ToDo comments at runtime.
 */
#define TODO(msg) reportToLog(LOG_INFORMATION, LC_HIGHUI, \
"IMPL_NOTE: "#msg" ("__FILE__":%d)", __LINE__)

/**
 * Macro to trace execution of date editor.
 */
#define TRACE_DE(msg) reportToLog(LOG_INFORMATION, LC_HIGHUI, \
"[lfpport_qte_dateeditor.cpp:%d]" #msg, __LINE__)

/**
 * Macro to trace execution of date field.
 */
#define TRACE_DF(msg) reportToLog(LOG_INFORMATION, LC_HIGHUI, \
"[lfpport_qte_datefield.cpp:%d]" #msg, __LINE__)

/**
 * Macro to trace execution of item base class.
 */
#define TRACE_ITM(msg) reportToLog(LOG_INFORMATION, LC_HIGHUI, \
"[lfpport_qte_item.cpp:%d]" #msg, __LINE__)

/** 
 * Macro to trace execution of custom item.
 */  
#define TRACE_CI(msg) reportToLog(LOG_INFORMATION, LC_HIGHUI, \
"[lfpport_qte_customitem.cpp:%d]" #msg, __LINE__)

/**
 * Macro to trace execution of MScreen.
 */
#define TRACE_MSC(msg) reportToLog(LOG_INFORMATION, LC_HIGHUI, \
"[lfpport_qte_mscreen.cpp:%d]" #msg, __LINE__)

#else

/**
 * Empty definition.
 */
#define TRACE_CI(msg) ;

/**
 * Empty definition.
 */
#define TODO(msg) ;

/**
 * Empty definition.
 */
#define TRACE_DE(msg) ;

/**
 * Empty definition.
 */
#define TRACE_DF(msg) ;

/**
 * Empty definition.
 */
#define TRACE_MSC(msg) ;

/**
 * Empty definition.
 */
#define TRACE_ITM(msg) ;

#endif


/**
 * Convert a pcsl_string to a QString.
 *
 * @param pstring source pcsl_string
 * @param qstring to be set on return to the converted qstring
 */
extern "C" void
pcsl_string2QString(const pcsl_string &pstring, QString &qstring);

/**
 * Convert a QString to pcsl_string.
 *
 * @param qstring QString
 * @param mstring pcsl_string to be set on return
 * @return error code
 */
extern "C" MidpError
QString2pcsl_string(QString &qstring, pcsl_string &pstring);

/**
 * Truncate str so that, if printed using given font, it would
 * fit in the width limit.
 * If the original string does not fit, the string is truncated,
 * and a truncation mark character is appended to it.
 *
 * @param str string to be truncated, if necessary
 * @param font font
 * @param widthLimit width limit
 */
void truncateQString(QString & str, class QFont font, int widthLimit);

/**
 * Calculate width available for the text in the title bar of the widget.
 *
 * @param widget widget whose title bar is of interest
 * @return width available for the caption text
 */
int calculateCaptionWidth(class QWidget* widget);

#endif /* _LFPPORT_QTE_UTIL_H_ */

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
 */

#ifndef _MIDP_DEBUG_H_
#define _MIDP_DEBUG_H_

#include <midpString.h>

#include <midp_logging.h>
#include <midp_constants_data.h>

/**
 * @file
 *
 * @brief Helper interface for the logging and tracing service
 */


#if REPORT_LEVEL <= LOG_INFORMATION
#define printMidpString(x) printMidpStringImpl((x))
#define printMidpStringWithMessage(x,y) printMidpStringWithMessageImpl((x),(y))
#define printPcslString(x) printPcslStringImpl((x))
#define printPcslStringWithMessage(x,y) printPcslStringWithMessageImpl((x),(y))
#else
#define printMidpString(x)
#define printMidpStringWithMessage(x,y)
#define printPcslString(x)
#define printPcslStringWithMessage(x,y)
#endif /* if REPORT_LEVEL <= LOG_INFORMATION */

/**
 * Helper function.
 * Prints a MidpString and a new line at the end.
 * This output goes to the log if the logging report level is
 * set to include <code>INFORMATION</code> severity level output.
 * @param mstr   MidpString to print
 */ 
/* To be removed when migration from MidpString to pcsl_string is complete */
void printMidpStringImpl(MidpString mstr);

/**
 * Helper function.
 * Prints a pcsl_string and a new line at the end.
 * This output goes to the log if the logging report level is
 * set to include <code>INFORMATION</code> severity level output.
 * @param pstr   points to pcsl_string to print
 */
void printPcslStringImpl(const pcsl_string* pstr);

/**
 * Helper function.
 * Prints a MidpString with the message.
 * This output goes to the log if the logging report level is
 * set to include <code>INFORMATION</code> severity level outputs
 * @param message   attached message to print 
 * @param mstr   MidpString to print
 */
/* To be removed when migration from MidpString to pcsl_string is complete */
void printMidpStringWithMessageImpl(char* message, MidpString mstr);

/**
 * Helper function.
 * Prints a pcsl_string with the message.
 * This output goes to the log if the logging report level is
 * set to include <code>INFORMATION</code> severity level outputs
 * @param message   attached message to print
 * @param pstr      points to pcsl_string to print
 */
void printPcslStringWithMessageImpl(char* message, const pcsl_string* pstr);

#endif /* _MIDP_DEBUG_H_ */


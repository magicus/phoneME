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

#ifndef _MIDP_LOGGING_H_
#define _MIDP_LOGGING_H_

#include <midp_constants_data.h>


/**
 * @defgroup core_log Logging External Interface
 * @ingroup core
 */

/**
 * @file
 * @ingroup core_log
 *
 * @brief Functions and definitions that provide a standard way to report
 * runtime
 * information from within Java class files or native code. Use this service
 * instead of making ad hoc calls to <tt>println()</tt>, <tt>printf()</tt>,
 * <tt>putc()</tt>, and so on. It will make porting easier.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Provides the given severity to the given channel of the Logging service,
 * along with the given message. The message string should describe any issues
 * well enough that someone reading the log will be able to diagnose and solve
 * any problems.  The severity level should accurately reflect the severity
 * of the message, and can be either:
 *
 * <ul>
 *  <li><tt>LOG_INFORMATION</tt>,</li>
 *  <li><tt>LOG_WARNING</tt>,</li>
 *  <li><tt>LOG_ERROR</tt>, or</li>
 *  <li><tt>LOG_CRITICAL</tt>.</li>
 * </ul>
 *
 * <p>The channel identifier should be one of the channels defined in
 * <tt>midp_constants_data.h</tt>.
 *
 * <p>The following example shows how to use this function:
 * <pre>#if REPORT_LEVEL <= severity
 *          reportToLog(severity, channel, <i>meaningfulMessage</i>);
 * #endif</pre>
 *
 * The <code>message</code> parameter is treated as a format
 * string to the standard C library call printf would be, with
 * conversion specifications (%s, %d, %c, etc) causing the
 * conversion and output of each successive argument after
 * <code>message</code>  As with printf, having a conversion
 * character in <code>message</code> without an associated argument
 * following it is an error.
 *
 * To ensure that no character in <code>message</code> is
 * interpreted as requiring conversion, a safe way to call
 * this method is:
 * <code> reportToLog(severity, chanID, "%s", message); </code>
 *
 * @param severity severity level of report.
 * @param channelID topic of the report, from <tt>midp_constants_data.h</tt>.
 * @param message detail message to record; it should not be NULL.
 */
void reportToLog(int severity, int channelID, char* message, ...);

/**
 * Log the native thread ID for debugging on multi-thread platforms.
 *
 * @param message message to prefix the thread ID
 */
void midp_logThreadId(char* message);

/**
 * Initializes the logging subsystem with the list of channels to log.
 *
 * @param pStrChannelList comma-separated list of channel numbers to log
 */
void setupChannelsToLog(const char* pStrChannelList);

/**
 * @note - Reporting Macros
 * These macros wrap reportToLog() for various reporting levels
 * and are empty for the cases when the current REPORT_LEVEL
 * disables their output.
 *
 * For example, the call
 *    REPORT_INFO1(LC_RMS, "unable to open file %s", filename);
 * is equivalent to
 *    if (REPORT_LEVEL <= LOG_INFORMATION) {
 *        reportToLog(LOG_INFORMATION, LC_RMS, "unable to open file %s",
 *                    filename);
 *    }
 */

/**
 * The PRINT_TO_LOG macro may be used in case of printf
 * in the code, and default to being set at an <code>
 * LOG_INFORMTION </code> severity level and using the
 * channel <code>LC_NONE</code>
 *
 * PRINT_TO_LOG() macro is defined if <code> REPORT_LEVEL <=
 * LOG_INFORMATION</code>, and is otherwise empty.
 */
#if REPORT_LEVEL <= LOG_INFORMATION
#define PRINT_TO_LOG(msg) reportToLog(LOG_INFORMATION, LC_NONE, "%s", msg)
#else
#define PRINT_TO_LOG(msg)
#endif

/**
 * @name REPORT_INFO*() macros
 * REPORT_INFO*() macros are defined if <code>REPORT_LEVEL
 * <= LOG_INFORMATION</code>, and are empty otherwise.
 * @see reportToLog
 * @{
 */
#if REPORT_LEVEL <= LOG_INFORMATION
#define REPORT_INFO(ch, msg) reportToLog(LOG_INFORMATION, ch, msg)
#define REPORT_INFO1(ch, msg, a1) reportToLog(LOG_INFORMATION, ch, msg, a1)
#define REPORT_INFO2(ch, msg, a1, a2) \
  reportToLog(LOG_INFORMATION, ch, msg, a1, a2)
#define REPORT_INFO3(ch, msg, a1, a2, a3) \
  reportToLog(LOG_INFORMATION, ch, msg, a1, a2, a3)
#define REPORT_INFO4(ch, msg, a1, a2, a3, a4) \
  reportToLog(LOG_INFORMATION, ch, msg, a1, a2, a3, a4)
#define REPORT_INFO5(ch, msg, a1, a2, a3, a4, a5) \
  reportToLog(LOG_INFORMATION, ch, msg, a1, a2, a3, a4, a5)
#define REPORT_INFO6(ch, msg, a1, a2, a3, a4, a5, a6) \
  reportToLog(LOG_INFORMATION, ch, msg, a1, a2, a3, a4, a5, a6)
#define REPORT_INFO7(ch, msg, a1, a2, a3, a4, a5, a6, a7) \
  reportToLog(LOG_INFORMATION, ch, msg, a1, a2, a3, a4, a5, a6, a7)
#define REPORT_INFO8(ch, msg, a1, a2, a3, a4, a5, a6, a7, a8) \
  reportToLog(LOG_INFORMATION, ch, msg, a1, a2, a3, a4, a5, a6, a7, a8)
#else
#define REPORT_INFO(ch, msg)
#define REPORT_INFO1(ch, msg, a1)
#define REPORT_INFO2(ch, msg, a1, a2)
#define REPORT_INFO3(ch, msg, a1, a2, a3)
#define REPORT_INFO4(ch, msg, a1, a2, a3, a4)
#define REPORT_INFO5(ch, msg, a1, a2, a3, a4, a5)
#define REPORT_INFO6(ch, msg, a1, a2, a3, a4, a5, a6)
#define REPORT_INFO7(ch, msg, a1, a2, a3, a4, a5, a6, a7)
#define REPORT_INFO8(ch, msg, a1, a2, a3, a4, a5, a6, a7, a8)
#endif
/** @} */

/**
 * @name REPORT_WARN*() macros
 * REPORT_WARN*() macros are defined if <code> REPORT_LEVEL
 * <= LOG_WARNING</code> and are empty otherwise.
 * @see reportToLog
 * @{
 */
#if REPORT_LEVEL <= LOG_WARNING
#define REPORT_WARN(ch, msg) reportToLog(LOG_WARNING, ch, msg)
#define REPORT_WARN1(ch, msg, a1) reportToLog(LOG_WARNING, ch, msg, a1)
#define REPORT_WARN2(ch, msg, a1, a2) reportToLog(LOG_WARNING, ch, msg, a1, a2)
#define REPORT_WARN3(ch, msg, a1, a2, a3) \
  reportToLog(LOG_WARNING, ch, msg, a1, a2, a3)
#define REPORT_WARN4(ch, msg, a1, a2, a3, a4) \
  reportToLog(LOG_WARNING, ch, msg, a1, a2, a3, a4)
#else
#define REPORT_WARN(ch, msg)
#define REPORT_WARN1(ch, msg, a1)
#define REPORT_WARN2(ch, msg, a1, a2)
#define REPORT_WARN3(ch, msg, a1, a2, a3)
#define REPORT_WARN4(ch, msg, a1, a2, a3, a4)
#endif
/** @} */

/**
 * @name REPORT_ERROR*() macros
 * REPORT_ERROR*() macros are defined if <code> REPORT_LEVEL
 * <= LOG_ERROR</code> and are empty otherwise.
 * @see reportToLog
 * @{
 */
#if REPORT_LEVEL <= LOG_ERROR
#define REPORT_ERROR(ch, msg) reportToLog(LOG_ERROR, ch, msg)
#define REPORT_ERROR1(ch, msg, a1) reportToLog(LOG_ERROR, ch, msg, a1)
#define REPORT_ERROR2(ch, msg, a1, a2) \
  reportToLog(LOG_ERROR, ch, msg, a1, a2)
#define REPORT_ERROR3(ch, msg, a1, a2, a3) \
  reportToLog(LOG_ERROR, ch, msg, a1, a2, a3)
#else
#define REPORT_ERROR(ch, msg)
#define REPORT_ERROR1(ch, msg, a1)
#define REPORT_ERROR2(ch, msg, a1, a2)
#define REPORT_ERROR3(ch, msg, a1, a2, a3)
#endif
/** @} */

/**
 * @name REPORT_CRIT*() macros
 * REPORT_CRIT*() macros are defined if <code> REPORT_LEVEL
 * <= LOG_CRITICAL</code> and are empty otherwise.
 * @see reportToLog
 * @{
 */
#if REPORT_LEVEL <= LOG_CRITICAL
#define REPORT_CRIT(ch, msg) reportToLog(LOG_CRITICAL, ch, msg)
#define REPORT_CRIT1(ch, msg, a1) reportToLog(LOG_CRITICAL, ch, msg, a1)
#define REPORT_CRIT2(ch, msg, a1, a2) \
  reportToLog(LOG_CRITICAL, ch, msg, a1, a2)
#define REPORT_CRIT3(ch, msg, a1, a2, a3) \
  reportToLog(LOG_CRITICAL, ch, msg, a1, a2, a3)
#else
#define REPORT_CRIT(ch, msg)
#define REPORT_CRIT1(ch, msg, a1)
#define REPORT_CRIT2(ch, msg, a1, a2)
#define REPORT_CRIT3(ch, msg, a1, a2, a3)
#endif
/** @} */

/**
 * @name REPORT_CALL_TRACE*() macros
 * REPORT_CALL_TRACE*() macros are defined to trace native function calls.
 * @see reportToLog
 * @{
 */
#define REPORT_CALL_TRACE(ch, msg) REPORT_WARN(ch, msg)
#define REPORT_CALL_TRACE1(ch, msg, a1) REPORT_WARN1(ch, msg, a1)
#define REPORT_CALL_TRACE2(ch, msg, a1, a2) REPORT_WARN2(ch, msg, a1, a2)
#define REPORT_CALL_TRACE3(ch, msg, a1, a2, a3) \
  REPORT_WARN3(ch, msg, a1, a2, a3)
#define REPORT_CALL_TRACE4(ch, msg, a1, a2, a3, a4) \
  REPORT_WARN4(ch, msg, a1, a2, a3, a4)

#ifdef __cplusplus
}
#endif
/** @} */

#endif /* _MIDP_LOGGING_H_ */

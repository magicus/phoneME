/*
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

#include <kni.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <midpMalloc.h>
#include <midp_properties_port.h>
#include <midp_logging.h>
#include <pcsl_print.h>
#include <midp_libc_ext.h>
#include <midpNativeThread.h>
#include "javautil_printf.h"
#include "jvm.h"

/**
 * @file
 *
 * Logging and tracing API implementation used to output text to the
 * default display device. In the VM, terminal output for Java methods
 * <tt>System.out.println()</tt> and
 * <tt>Java.lang.Throwable.printStackTrace()</tt> happen via a call to
 * the <tt>JVMSPI_PrintRaw()</tt> method. This implementation emulates
 * the mechanism used there.
 *
 * To redirect the logging output to a file or a stream other than
 * stdout modify the behavior of the <tt>JVMSPI_PrintRaw()</tt> method.
 */

/**
 * Argument for runMidlet to set the numbers of channels
 * for wich logging will be enabled.
 * If this argument is not used, logging will be enabled
 * for all channels.
 *
 * <p>NOTE: option '-Dlog.channels' is used to set up the
 *       corresponding system property that will be available
 *       in Java through System.getProperty("log.channels")
 *       and in native code through getSystemProperty().
 *
 * <p>Examples:
 * <pre>    -Dlog.channels=5000,10000
 *     -Dlog.channels=1000
 *     -Dlog.channels=    - disables logging</pre>
 */
#define LOG_CHANNELS_ARG "log.channels"

/** Buffer used by logging facility */
#define LOGGING_BUFFER_SIZE 400

/**
 * Report a message to the Logging service.  On the linux emulator
 * this will end up going to stdout.  On the Zaurus device it will
 * be written to a file.
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

 * @param severity severity level of report
 * @param channelID area report relates to, from midp_constants_data.h
 * @param message detail message to go with the report
 *                should not be NULL
 */
void reportToLog(int severity, int channelID, char* message, ...) {
    va_list ap;
    
    if(message == NULL) {
        return;
    }

    if(get_allowed_severity_c(channelID) <= severity) {
        va_start(ap, message);
        javautil_vprintf(severity, channelID, GET_ISOLATE_ID, message, ap);
        va_end(ap);
    }
}
    /**
    * Gets the severity per channel using the property mechanism.
    */
int get_allowed_severity_c(int channelID) {
    
    switch(channelID) {
        case LC_NONE:
            return(getInternalPropertyInt("NONE"));  
        case LC_AMS:
            return(getInternalPropertyInt("AMS"));
        case LC_CORE:
            return(getInternalPropertyInt("CORE"));
        case LC_LOWUI:
            return(getInternalPropertyInt("LOWUI"));
        case LC_HIGHUI:
            return(getInternalPropertyInt("HIGHUI"));
        case LC_PROTOCOL:
            return(getInternalPropertyInt("PROTOCOL"));
        case LC_RMS:
            return(getInternalPropertyInt("RMS"));
        case LC_SECURITY:
            return(getInternalPropertyInt("SECURITY"));
        case LC_SERVICES:
            return(getInternalPropertyInt("SERVICES"));
        case LC_STORAGE:
            return(getInternalPropertyInt("STORAGE"));
        case LC_PUSH:
            return(getInternalPropertyInt("PUSH"));
        case LC_MMAPI:
            return(getInternalPropertyInt("MMAPI"));
        case LC_LIFECYCLE:
            return(getInternalPropertyInt("LIFECYCLE"));
        case LC_MIDPSTRING:
            return(getInternalPropertyInt("MIDPSTRING"));
        case LC_MALLOC:
            return(getInternalPropertyInt("MALLOC"));
        case LC_CORE_STACK:
            return(getInternalPropertyInt("CORE_STACK"));   
        case LC_I18N:
            return(getInternalPropertyInt("I18N"));
        case LC_HIGHUI_ITEM_LAYOUT:
            return(getInternalPropertyInt("HIGHUI_ITEM_LAYOUT"));
        case LC_HIGHUI_ITEM_REPAINT:
            return(getInternalPropertyInt("HIGHUI_ITEM_REPAINT"));
        case LC_HIGHUI_FORM_LAYOUT:
            return(getInternalPropertyInt("HIGHUI_FORM_LAYOUT"));
        case LC_HIGHUI_ITEM_PAINT:
            return(getInternalPropertyInt("HIGHUI_ITEM_PAINT"));      
        case LC_TOOL:
            return(getInternalPropertyInt("TOOL"));
        case LC_JSR180:
            return(getInternalPropertyInt("JSR180"));
        case LC_JSR290:
            return(getInternalPropertyInt("JSR290"));
        case LC_JSR257:
            return(getInternalPropertyInt("JSR257"));
        case LC_JSR258:
            return(getInternalPropertyInt("JSR258"));
        case LC_EVENTS:
            return(getInternalPropertyInt("EVENTS"));

        default: 
            return(LOG_DISABLED);
    }

}


/**
 * Log the native thread ID for debugging on multi-thread platforms.
 *
 * @param message message to prefix the thread ID
 */
void midp_logThreadId(char* message) {
#if REPORT_LEVEL <= LOG_INFORMATION
    char temp[80];

    REPORT_WARN(LC_EVENTS, "midp_logThreadId: Stubbed out.");
    //put code here to get the thread id

    sprintf(temp, "%s: ThreadID = %d\n", message,
            (int)midp_getCurrentThreadId());
    REPORT_INFO(LC_EVENTS, temp);
#else
    (void)message; // avoid a compiler warning
#endif

}



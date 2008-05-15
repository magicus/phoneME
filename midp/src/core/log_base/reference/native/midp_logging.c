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

/** Maximum number of channels selected for logging */
#define MAX_LOG_CHANNELS 10

/** Array containing the numbers of channels selected for logging */
static int piChannelList[MAX_LOG_CHANNELS];

/** Buffer used by logging facility */
#define LOGGING_BUFFER_SIZE 400

/**
 * Global buffers definitions
 */
static char gLoggingBuffer[LOGGING_BUFFER_SIZE];

/**
 * The number of channels selected for logging.
 * Initial value of -1 means that logging will be enabled
 * for all channels. If LOG_CHANNELS_ARG argument is given
 * to the midlet runner, createLogChannelsList() will be called
 * setting iChannelsNum to 0 if the logging is disabled or to
 * the number of channels for which logging is enabled.
 */
static int iChannelsNum = -1;

/** A flag indicating if the channel list was already parsed */
static int fChannelSetupDone = 0;

/** Forward declaration */
static int channelInList(int channelId);

/** Forward declaration */
static void createLogChannelsList(const char* pStrChannelList);

/**
 * Initializes the logging subsystem with the list of channels to log.
 *
 * @param pStrChannelList comma-separated list of channel numbers to log
 */
void
setupChannelsToLog(const char* pStrChannelList) {
    iChannelsNum = -1;
    createLogChannelsList(pStrChannelList);
    fChannelSetupDone = 1;
}

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
void
reportToLog(int severity, int channelID, char* message, ...) {
    va_list ap;

    if (!fChannelSetupDone) {
        /*
         * Get the system property to check if the specific
         * channels were set for logging.
         */
        const char* pChannelsArg = getSystemProperty(LOG_CHANNELS_ARG);

        if (pChannelsArg) {
            createLogChannelsList(pChannelsArg);
        }

        fChannelSetupDone = 1;
    }

    if (message != NULL && channelInList(channelID)) {
        midp_snprintf(gLoggingBuffer, LOGGING_BUFFER_SIZE,
                "REPORT: <level:%d> <channel:%d> ",
                severity,  channelID);
        pcsl_print(gLoggingBuffer);

        va_start(ap, message);

        midp_vsnprintf(gLoggingBuffer, LOGGING_BUFFER_SIZE, message, ap);
        pcsl_print(gLoggingBuffer);

        va_end(ap);

        pcsl_print("\n");
    }
}

/**
 * Checks if logging is enabled for the specified channel or not.
 *
 * @param channelId number of the channel to check.
 * @return 1 if logging is enabled for the channel, 0 otherwise.
 */
static int channelInList(int channelId) {
    int i;

    /* assert(iChannelsNum <= MAX_LOG_CHANNELS); */

    if (iChannelsNum == -1) {
        /* -1 means that all channels must be logged. */
        return 1; /* TRUE */
    }

    for (i = 0; i < iChannelsNum; i++) {
        if (piChannelList[i] == channelId) {
            return 1; /* TRUE */
        }
    }

    return 0; /* FALSE */
}

/**
 * Sets the numbers of channels that should be logged.
 *
 * @param pStrChannelList comma-separated list of channel numbers to log.
 */
static void createLogChannelsList(const char* pStrChannelList) {
    int  isLastEntry = 0;
    int  iChannelId = 0;
    const char *pChannelStart;
    char *pChannelEnd;

    iChannelsNum = 0;

    pChannelStart = pStrChannelList;

    do {
        /*
         * IMPL_NOTE: remove.
         * pChannelEnd = strchr(pChannelStart, ',');
         * if (pChannelEnd) {
         *     *pChannelEnd = 0;
         * }
         */

        iChannelId = (int)strtol(pChannelStart, &pChannelEnd, 0);

        if (*pChannelEnd != ',' && *pChannelEnd != '\0') {
            if (pChannelEnd == pChannelStart) {
                /* Error: invalid channel number! */
                break;
            } else {
                isLastEntry = 1;
            }
        }

        pChannelStart = pChannelEnd;
        pChannelStart++;

        if (!channelInList(iChannelId)) {
            /* printf(">>> Adding channel %d...\n", iChannelId); */
            piChannelList[iChannelsNum++] = iChannelId;

            if (iChannelsNum == MAX_LOG_CHANNELS) {
                /* Other channel values will be ignored. */
                break;
            }
        }
    } while (*pChannelEnd && !isLastEntry);
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

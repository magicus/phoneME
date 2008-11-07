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

/**
 * @file
 * The glue code between the Java interface to the
 * logging/tracing service and the native provider code
 * defined in midp_logging.h and implemented in logging.c
 */

#include <kni.h>
#include <midpError.h>
#include <midpString.h>
#include <midpMalloc.h>
#include <midp_logging.h>
#include <midpUtilKni.h>

/**========================================================================
 * FUNCTION:      report(II,java/lang/String)V
 * CLASS:         com.sun.midp.services.Logging
 * TYPE:          virtual native function
 * OVERVIEW:      calls a system specific report function
 * INTERFACE (operand stack manipulation):
 *   parameters:  severity   constant shows severity of the error message
 *                channelID  allows log to be parsed by area/module
 *                message    String to be appended to the report log
 *   returns:     <nothing>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_log_LoggingBase_report) {
    reportToLog(LOG_ERROR, 0, "%s", "LoggingBase.report was called directly\n");
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_log_Logging_report0) {
    KNI_StartHandles(1);
    GET_PARAMETER_AS_PCSL_STRING(3,message_str) {
        const jbyte * tmpmsg = pcsl_string_get_utf8_data(&message_str);
        int channelID = KNI_GetParameterAsInt(2);
        int severity = KNI_GetParameterAsInt(1);

        /*
         * This call format is  needed because if s contains %,
         * printf(s) fails, but
         * printf("%s", s) is safe
         */
        reportToLog(severity, channelID, "%s", tmpmsg);

        pcsl_string_release_utf8_data(tmpmsg,&message_str);
    } RELEASE_PCSL_STRING_PARAMETER
    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_log_Logging_getAllowedSeverity) {
    int severity;
    int channelID; 

    channelID = KNI_GetParameterAsInt(1);
    severity = get_allowed_severity_c(channelID);

    KNI_ReturnInt(severity);
}

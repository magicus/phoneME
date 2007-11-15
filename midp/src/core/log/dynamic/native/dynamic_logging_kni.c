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

/**
 * @file
 * Implementation of dynamic logging.
 */

#if ENABLE_CONTROL_ARGS_FROM_JAD

#include <stdlib.h>
#include <kni.h>

#include <midp_logging.h>
#include <suitestore_common.h>
#include <suitestore_intern.h>

static int g_newReportLevel, g_newTraceEnabled, g_newAssertEnabled;

/**
 * Native implementation for parseMidpArg(int suiteId).
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_com_sun_midp_log_Logging_parseMidpControlArg0() {
    jboolean res = 0;
    const char* pTmp;
    MIDP_JAD_CONTROL_ARGS pJadArgs[MAX_JAD_CONTROL_ARGS];
    MIDPError status, found;
    MidpProperties jadProps;
    SuiteIdType suiteId = (SuiteIdType) KNI_GetParameterAsInt(1);

    status = load_install_properties(suiteId, &jadProps, NULL);
    if (status != ALL_OK) {
        KNI_ReturnBoolean(0);
    }

    parse_control_args_from_jad(&jadProps, pJadArgs, MAX_JAD_CONTROL_ARGS);

    found = get_jad_control_arg_value(pJadArgs, "log_channels", &pTmp);
    if (found == ALL_OK && pTmp) {
        setupChannelsToLog(pTmp);
    }

    found = get_jad_control_arg_value(pJadArgs, "report_level", &pTmp);
    res |= (found == ALL_OK);
    g_newReportLevel = (found == ALL_OK && pTmp) ? atoi(pTmp) : LOG_CURRENT;

    found = get_jad_control_arg_value(pJadArgs, "enable_trace", &pTmp);
    res |= (found == ALL_OK);
    g_newTraceEnabled = (found == ALL_OK && pTmp) ? atoi(pTmp) : -1;

    found = get_jad_control_arg_value(pJadArgs, "enable_assert", &pTmp);
    res |= (found == ALL_OK);
    g_newAssertEnabled = (found == ALL_OK && pTmp) ? atoi(pTmp) : -1;

    free_pcsl_string_list(jadProps.pStringArr,
                          jadProps.numberOfProperties << 1);

    KNI_ReturnBoolean(res);
}

/**
 * Native implementation for loadReportLevel0().
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_log_Logging_loadReportLevel0() {
    KNI_ReturnInt(g_newReportLevel);
}

/**
 * Native implementation for loadTraceEnabled0().
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_log_Logging_loadTraceEnabled0() {
    KNI_ReturnInt(g_newTraceEnabled);
}

/**
 * Native implementation for loadAssertsEnabled0().
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_log_Logging_loadAssertEnabled0() {
    KNI_ReturnInt(g_newAssertEnabled);
}

#endif /* ENABLE_CONTROL_ARGS_FROM_JAD */

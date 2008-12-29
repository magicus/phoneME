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

#include <stdio.h>

#include <java_profiler.h>
#include <jvm.h>
#include <midp_logging.h>
#include <midp_constants_data.h>
#include <commonKNIMacros.h>

void midp_profiler_start() {
    if (!JVM_SendProfilerCommand(JAVA_PROFILER_COMMAND_START, NULL))
        REPORT_ERROR(LC_CORE, "Cannot start java profiler.\n");
}

void midp_profiler_stop() {
    if (!JVM_SendProfilerCommand(JAVA_PROFILER_COMMAND_STOP, NULL)) {
        REPORT_ERROR(LC_CORE, "Cannot stop java profiler.\n");
    }
    if (!JVM_SendProfilerCommand(JAVA_PROFILER_COMMAND_DUMP_AND_CLEAR, NULL))
        REPORT_ERROR(LC_CORE, "Cannot dump java profiler data.\n");
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_profilercontrol_JavaProfilerControl_control0) {
    jint command = KNI_GetParameterAsInt(0);

    if (command)
        midp_profiler_start();
    else
        midp_profiler_stop();

    KNI_ReturnVoid();
}

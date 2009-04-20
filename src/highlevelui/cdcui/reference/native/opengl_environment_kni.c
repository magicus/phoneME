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
 *
 * All native functions related to com.sun.midp.lcdui.OpenGLEnvironment class
 */

#include <sni.h>
#include <jvm.h>
#include <commonKNIMacros.h>
#include <midpEventUtil.h>

extern void midpGL_flush(int dirtyRegions[], int numRegions);

/**
 *
 * Calls openGL function to prepare for switching from lcdui rendering
 * to rendering with some external API
 * <p>
 * Java declaration:
 * <pre>
 *    gainedForeground0(I)V
 * </pre>
 * Java parameters:
 * <pre>
 *    externalAPI the external API which is preparing to render
 *    dirtyRegions regions of the screen which need to be flushed
 *    numberOfRegions number of dirty regions to be processed
 *    displayId the display ID associated with the Display object
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_lcdui_OpenGLEnvironment_flushOpenGL0) {
    jint numRegions = KNI_GetParameterAsInt(2);
    jint displayId = KNI_GetParameterAsInt(3);

    if (midpHasForeground(displayId)) {
        // do processing
        KNI_StartHandles(1);
        KNI_DeclareHandle(dirtyRegions);

        KNI_GetParameterAsObject(1, dirtyRegions);
        if (KNI_IsNullHandle(dirtyRegions)) {
            KNI_ReturnVoid();
        }
        /* here we need to call midpGL_flush() */
        midpGL_flush(dirtyRegions, numRegions);
        KNI_EndHandles();
    }
    KNI_ReturnVoid();
}
/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

#include "JSR239-KNIInterface.h"

#include <gxapi_graphics.h>

#undef DEBUG

#ifdef DEBUG
#include <stdio.h>
#endif

/*
 * Return the width of the window accessed by the given Graphics
 * object.
 */
jint
JSR239_getGraphicsWidth(jobject graphicsHandle) {

#ifdef DEBUG
    printf("JSR239_getGraphicsWidth\n");
#endif

    return GXAPI_GET_GRAPHICS_PTR(graphicsHandle)->maxWidth;
}


/*
 * Return the height of the window accessed by the given Graphics
 * object.
 */
jint
JSR239_getGraphicsHeight(jobject graphicsHandle) {

#ifdef DEBUG
    printf("JSR239_getGraphicsHeight\n");
#endif

    return GXAPI_GET_GRAPHICS_PTR(graphicsHandle)->maxHeight;
}

void
JSR239_getGraphicsSource(jobject graphicsHandle, jobject resultHandle) {

#ifdef DEBUG
    printf("JSR239_getGraphicsSource\n");
#endif

    (void)graphicsHandle;

    KNI_StartHandles(1);
    KNI_DeclareHandle(sourceHandle);

    // IMPL_NOTE: retrieve real source handle
    KNI_ReleaseHandle(sourceHandle);

    KNI_SetObjectArrayElement(resultHandle, 0, sourceHandle);

#ifdef DEBUG
    printf("getGraphicsWidth: source = 0x%x\n", sourceHandle);
#endif

    KNI_EndHandles();
}

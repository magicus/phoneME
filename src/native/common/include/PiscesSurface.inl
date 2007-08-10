/*
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


#include "PiscesSurface.h"

#include <PiscesUtil.h>

#include <PiscesSysutils.h>

static INLINE void surface_dispose(Surface* surface);

static INLINE void surface_setRGB(Surface* dstSurface, jint x, jint y,
                                  jint width, jint height, jint* data,
                                  jint scanLength);


static void setRGB(jint* src, jint srcScanLength, jint* dst, jint dstScanLength,
                   jint width, jint height);


static INLINE void
surface_dispose(Surface* surface) {
    my_free(surface);
}

static INLINE void
surface_setRGB(Surface* dstSurface, jint x, jint y,
               jint width, jint height, jint* data, jint scanLength) {
    setRGB((jint*)dstSurface->data + y * dstSurface->width + x, 
           dstSurface->width, data, scanLength, width, height);
}


static void
setRGB(jint* dst, jint dstScanLength, jint* src, jint srcScanLength,
       jint width, jint height) {
    jint srcScanRest = srcScanLength - width;
    jint dstScanRest = dstScanLength - width;

    for (; height > 0; --height) {
        jint w2 = width;
        for (; w2 > 0; --w2) {
            *dst++ = *src++;
        }
        src += srcScanRest;
        dst += dstScanRest;
    }
}


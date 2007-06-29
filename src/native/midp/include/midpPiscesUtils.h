/*
 * 
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


#ifndef MIDP_PISCES_UTILS_H
#define MIDP_PISCES_UTILS_H
                                  
#   ifdef PISCES_USE_JWC_OLD_IMPLEMENTATION //es-gate-1 implementation
#       define PISCES_GET_DATA_POINTER(array) &array->elements[0]
#       include <midpGraphics.h>
#       include <midpLCDUI.h>
#       include <images.h>

extern VDC screenBuffer;

VDC *
setupImageVDC(jobject img, VDC *vdc);

#   else // irbis implementation
#       define PISCES_GET_DATA_POINTER(array) array
#       include <imgapi_image.h>
#       include <gxapi_graphics.h>
#       include <gxj_putpixel.h>
#       include <gx_image.h>


#define getScreenBuffer(sbuf) \
    ((sbuf == NULL) ? (&gxj_system_screen_buffer) : sbuf)

#   endif //PISCES_USE_JWC_OLD_IMPLEMENTATION                                 

#endif //MIDP_PISCES_UTILS_H

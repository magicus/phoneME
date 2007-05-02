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


#ifndef JABSTRACT_SURFACE_H
#define JABSTRACT_SURFACE_H

#include <PiscesSurface.h>

typedef struct _AbstractSurface {
    Surface super;
    void (*acquire)(struct _AbstractSurface* surface, jobject surfaceHandle);
    void (*release)(struct _AbstractSurface* surface, jobject surfaceHandle);
    void (*cleanup)();
}
AbstractSurface;

#ifdef PISCES_JAVA_SURFACE_SUPPORT

#define ACQUIRE_SURFACE(surface, surfaceHandle)                              \
        SNI_BEGIN_RAW_POINTERS;                                              \
        ((AbstractSurface*)(surface))->acquire((AbstractSurface*)(surface),  \
                                               (surfaceHandle));

// no need to call release from RELEASE_SURFACE on MIDP
#define RELEASE_SURFACE(surface, surfaceHandle)                              \
        SNI_END_RAW_POINTERS;

#else // PISCES_JAVA_SURFACE_SUPPORT

#define ACQUIRE_SURFACE(surface, surfaceHandle) ;

#define RELEASE_SURFACE(surface, surfaceHandle) ;

#endif // PISCES_JAVA_SURFACE_SUPPORT

AbstractSurface* surface_get(jobject surfaceHandle);
jboolean surface_initialize(jobject surfaceHandle);
void surface_finalize(jobject objectHandle);

#endif

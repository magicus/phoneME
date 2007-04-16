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


#include <PiscesUtil.h>

#include <PiscesSysutils.h>

#define POINTER_32

// Depth conversion

jint* _Pisces_convert8To5 = NULL;
jint* _Pisces_convert8To6 = NULL;

// Stroke bias

jint PISCES_STROKE_X_BIAS;
jint PISCES_STROKE_Y_BIAS;

jboolean
piscesutil_moduleInitialize() {
    if (_Pisces_convert8To5 == NULL) {
        jint i;
        jint* convert8To5;
        jint* convert8To6;

        convert8To5 = (jint*)PISCESmalloc(256*sizeof(jint));
        ASSERT_ALLOC_BOOLEAN(convert8To5);

        convert8To6 = (jint*)PISCESmalloc(256*sizeof(jint));
        ASSERT_ALLOC_BOOLEAN(convert8To6);

        if ((convert8To5 == NULL) ||
                (convert8To6 == NULL)) {
            if (convert8To5 != NULL) {
                PISCESfree(convert8To5);
            }
            if (convert8To6 != NULL) {
                PISCESfree(convert8To6);
            }
            return XNI_FALSE;
        }
        for (i = 0; i < 256; i++) {
            convert8To5[i] = (i*31 + 127)/255;
            convert8To6[i] = (i*63 + 127)/255;
        }

        _Pisces_convert8To5 = convert8To5;
        _Pisces_convert8To6 = convert8To6;
    }

    return XNI_TRUE;
}

void
piscesutil_moduleFinalize() {
    my_free(_Pisces_convert8To5);
    my_free(_Pisces_convert8To6);
    _Pisces_convert8To5 = NULL;
    _Pisces_convert8To6 = NULL;
}

void
piscesutil_setStrokeBias(jint xbias, jint ybias) {
    PISCES_STROKE_X_BIAS = xbias;
    PISCES_STROKE_Y_BIAS = ybias;
}

jlong
PointerToJLong(void *ptr) {
#ifdef POINTER_32
    // Avoid a compiler warning
    return (jlong)((jint) ptr);
#else
    return (jlong) ptr;
#endif
}

void*
JLongToPointer(jlong ptr) {
#ifdef POINTER_32
    // Avoid a compiler warning
    return (void *)((jint) ptr);
#else
    return (void *) ptr;
#endif
}

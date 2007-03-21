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


#include <PiscesLibrary.h>
#include <PiscesUtil.h>

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesStaticInitializerBase_staticInitialize() {
    jint xbias = KNI_GetParameterAsInt(1);
    jint ybias = KNI_GetParameterAsInt(2);

    piscesutil_setStrokeBias(xbias, ybias);

    if (KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
                     "Allocation of internal renderer buffer failed.");
    } else {
        if (!pisces_moduleInitialize()) {
            KNI_ThrowNew("java/lang/IllegalStateException", "");
        }
    }

    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesStaticInitializerBase_staticFinalize() {
    pisces_moduleFinalize();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_PiscesStaticInitializer_finalize() {
    pisces_moduleFinalize();
    KNI_ReturnVoid();
}

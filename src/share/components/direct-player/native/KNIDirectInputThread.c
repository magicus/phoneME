/*
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

#include "KNICommon.h"
#include "sni.h"

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_mmedia_DirectInputThread_nWriteData) {
    jlong posToRead = 0;
    jlong curPos = 0;
    jint sizeToRead = 0;
    jint addrToWrite = 0;
    
    KNI_StartHandles(2);
    KNI_DeclareHandle(instance);
    KNI_DeclareHandle(clazz);
    /* Get this object instance and clazz */
    KNI_GetThisPointer(instance);
    KNI_GetObjectClass(instance, clazz);
    
    /* Get field of this object */
    addrToWrite = KNI_GetIntField(instance, KNI_GetFieldID(clazz, "nativePtr", "I"));
    

    KNI_EndHandles();
    KNI_ReturnVoid();
}

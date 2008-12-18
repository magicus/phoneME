/*
 * @(#)sni_impl.c	1.2 06/10/10 10:06:46
 *
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

#include "javavm/export/sni.h"

/* Macros needed by sni implementation */
#undef CVMjniGcUnsafeRef2Class
#define CVMjniGcUnsafeRef2Class(ee, cl) CVMgcUnsafeClassRef2ClassBlock(ee, cl)

KNIEXPORT void
SNI_NewArrayImpl(CVMExecEnv* ee,
                 jint type, jint size, jarray arrayHandle)
{
    CVMClassBlock* arrayCb;

    CVMD_gcSafeExec(ee, {
        if (type == SNI_OBJECT_ARRAY) {
            arrayCb = CVMclassGetArrayOf(ee,
                          CVMsystemClass(java_lang_Object));
            type = CVM_T_CLASS;
        } else if (type == SNI_STRING_ARRAY) {
            arrayCb = CVMclassGetArrayOf(ee,
                          CVMsystemClass(java_lang_String));
            type = CVM_T_CLASS;
        } else {
            CVMassert(SNI_BOOLEAN_ARRAY <= type && type <= SNI_LONG_ARRAY);
            arrayCb = (CVMClassBlock*)CVMbasicTypeArrayClassblocks[type];
        }
        CVMID_allocNewArray(ee, type, arrayCb,
                            size, arrayHandle);
    });
}

KNIEXPORT void
SNI_NewObjectArrayImpl(CVMExecEnv* ee,
                       jclass elementType, jint size,
                       jarray arrayHandle)
{
    CVMClassBlock* elementCb = CVMjniGcUnsafeRef2Class(ee, elementType);
    CVMD_gcSafeExec(ee, {
        CVMClassBlock* arrayCb = CVMclassGetArrayOf(ee, elementCb);
        if (arrayCb == NULL) {
            return; /* exception already throw */
        }
        CVMID_allocNewArray(ee, CVM_T_CLASS, arrayCb,
                            size, arrayHandle);
    });
}

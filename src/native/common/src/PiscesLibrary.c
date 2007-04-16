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


#include <PiscesLibrary.h>

#include <PiscesMath.h>
#include <PiscesUtil.h>

static jboolean alreadyInitialized = XNI_FALSE;
static jboolean initializationResult;

jboolean
pisces_moduleInitialize() {
    if (alreadyInitialized) {
        return initializationResult;
    }
    alreadyInitialized = XNI_TRUE;
    if (!piscesmath_moduleInitialize() ||
            !piscesutil_moduleInitialize()) {
        piscesmath_moduleFinalize();
        piscesutil_moduleFinalize();
        initializationResult = XNI_FALSE;
        return XNI_FALSE;
    }

    initializationResult = XNI_TRUE;
    return XNI_TRUE;
}

void
pisces_moduleFinalize() {
    if (alreadyInitialized) {
        piscesmath_moduleFinalize();
        piscesutil_moduleFinalize();
        alreadyInitialized = XNI_FALSE;
    }
}

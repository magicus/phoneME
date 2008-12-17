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

#include <string.h>

#include <kni.h>

#include <midpError.h>
#include <midpMalloc.h>
#include <midpEvents.h>
#include <midpResourceLimit.h>

/**
 * Native cleanup code, called when this isolate is done,
 * even if killed.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_main_AppIsolateMIDletSuiteLoader_finalize) {
    midpFreeReservedResources();
    KNI_ReturnVoid();
}

/**
 * allocate the reserved resources for a new isolate
 *
 * @return true if the resources are available, otherwise return false
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_main_AppIsolateMIDletSuiteLoader_allocateReservedResources0) {
    KNI_ReturnBoolean(midpAllocateReservedResources());
}

/**
 * Reports a fatal error that cannot be handled in Java. 
 *
 * handleFatalError(Ljava/lang/Throwable;)V
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_main_AppIsolateMIDletSuiteLoader_handleFatalError) {
    handleFatalError();
    KNI_ReturnVoid();
}

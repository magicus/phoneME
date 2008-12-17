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

#include <kni.h>
#include <midpNativeAppManager.h>

/*
 * Declared here (not in midpNamsTestService.c) because this source is
 * used for building several tools and some of them (listMidlets, removeMidlet)
 * don't use midpNamsTestService.c.
 */
int g_namsTestServiceIsolateId = MIDP_INVALID_ISOLATE_ID;

extern void nams_test_service_setup_listeners();
extern void nams_test_service_remove_listeners();

/**
 * Initializes the native portion of the NAMS Test Service.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NamsTestService_initialize(void) {
    g_namsTestServiceIsolateId = KNI_GetParameterAsInt(1);
    nams_test_service_setup_listeners();
}

/**
 * Cleans up the native portion of the NAMS Test Service.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NamsTestService_cleanup(void) {
    g_namsTestServiceIsolateId = MIDP_INVALID_ISOLATE_ID;
    nams_test_service_remove_listeners();
}

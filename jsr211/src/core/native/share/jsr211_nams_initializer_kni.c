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
#include <midpUtilKni.h>
#include <suitestore_listeners.h>
#include <jsr211_nams_installer.h>

SUITESORE_DECLARE_LISTENER(jsr211_remove_listener);

/**
 * Native method void init() of
 * com.sun.midp.jsr211.Initializer.
 * <p>
 * Do the basic initialization of the JSR211.
 * Registers a callback on a midlet suite removal.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_jsr211_Initializer_init() {
    (void) midp_suite_add_listener(jsr211_remove_listener,
                                   SUITESTORE_LISTENER_TYPE_REMOVE,
                                   SUITESTORE_OPERATION_END);
    KNI_ReturnVoid();
}

/**
 * Native method void cleanup() of
 * com.sun.midp.jsr211.Initializer.
 * <p>
 * Finalizes the JSR211 subsystem.
 * Unregisters the callback on a midlet suite removal.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_jsr211_Initializer_cleanup() {
    (void) midp_suite_remove_listener(jsr211_remove_listener,
                                      SUITESTORE_LISTENER_TYPE_REMOVE);
    KNI_ReturnVoid();
}

/**
 * Callback function that is called on a midlet suite removal.
 *
 * @param listenerType type of listeners that should be notified:
 * <pre>
 *     SUITESTORE_LISTENER_TYPE_INSTALL - the listener will be called before
 *         and after the installation of a midlet suite;
 *     SUITESTORE_LISTENER_TYPE_REMOVE - the listener will be called before
 *         and after removing of a midlet suite.
 * </pre>
 * @param when defines at what stage of the operation this function being called
 * @param status status of the operation. Has a meaning if 'when' parameter
 * has SUITESTORE_OPERATION_END value.
 * @param pSuiteData information about the midlet suite on which the operation
 * is performed
 */
SUITESORE_DEFINE_LISTENER(jsr211_remove_listener) {
    (void) listenerType;
    (void) when;

    if (status == ALL_OK) {
        jsr211_remove_handlers(pSuiteData->suiteId);
    }
}

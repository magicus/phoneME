/*
 *
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

#include <pcsl_string.h>
#include <rms_registry.h>

/**
 * Starts listening of asynchronous changes of record store
 * @param suiteId suite ID of record store to start listen for
 * @param storeName name of record store to start listen for
 */
void rms_registry_start_record_store_listening(int suiteId, pcsl_string *storeName) {
    /* Suppress unused parameters warning */
    (void)suiteId;
    (void)storeName;
}

/**
 * Stops listening of asynchronous changes of record store
 * @param suiteId suite ID of record store to stop listen for
 * @param storeName name of record store to stop listen for
 */
void rms_registry_stop_record_store_listening(int suiteId, pcsl_string *storeName) {
    /* Suppress unused parameters warning */
    (void)suiteId;
    (void)storeName;
}

/**
 * Sends asynchronous notification about change of record store done
 * in the current execution context of method caller
 *
 * @param suiteId suite ID of changed record store
 * @param storeName name of changed record store
 * @param changeType type of record change: ADDED, DELETED or CHANGED
 * @param recordId ID of changed record
 */
void rms_registry_notify_record_store_change(
        int suiteId, pcsl_string *storeName, int changeType, int recordId) {

    /* Suppress unused parameters warning */
    (void)suiteId;
    (void)storeName;
    (void)changeType;
    (void)recordId;
}

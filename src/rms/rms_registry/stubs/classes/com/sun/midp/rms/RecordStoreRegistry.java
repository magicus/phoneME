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

package com.sun.midp.rms;

import com.sun.midp.security.SecurityToken;
import com.sun.midp.security.Permissions;


/** Stubbed implementation of record store registry API */
public class RecordStoreRegistry {

    /**
     * Registers listener and consumer of record store change events
     * @param token security token to restrict usage of the method
     * @param consumer record store events consumer
     */
    public static void registerRecordStoreEventConsumer(
            SecurityToken token, RecordStoreEventConsumer consumer) {

    }
    
    /**
     * Starts listening of asynchronous changes of record store
     *
     * @param token security token to restrict usage of the method
     * @param suiteId suite ID of record store to start listen for
     * @param storeName name of record store to start listen for
     */
    public static void startRecordStoreListening(
            SecurityToken token, int suiteId, String storeName) {
    }

    /**
     * Stops listening of asynchronous changes of record store
     *
     * @param token security token to restrict usage of the method
     * @param suiteId suite ID of record store to stop listen for
     * @param storeName name of record store to stop listen for
     */
    public static void stopRecordStoreListening(
            SecurityToken token, int suiteId, String storeName) {
    }

    /**
     * Sends asynchronous notification about change of record store done
     * in the current execution context of method caller
     *
     * @param token security token to restrict usage of the method
     * @param suiteId suite ID of changed record store
     * @param storeName name of changed record store
     * @param changeType type of record change: ADDED, DELETED or CHANGED
     * @param recordId ID of changed record 
     */
    public static void notifyRecordStoreChange(
            SecurityToken token, int suiteId, String storeName,
            int changeType, int recordId) {
    }

    /**
     * Acknowledges delivery of record store notifications
     * @param token security token to restrict usage of the method
     */
    public static void acknowledgeRecordStoreNotifications(SecurityToken token) {
    }

    /**
     * Shutdowns record store registry for this VM task
     * @param token security token to restrict usage of the method
     */
    public static void shutdown(SecurityToken token) {
    }

}

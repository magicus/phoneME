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

/**
 * A class represents registry of record stores withing current execution context.
 * Execution context can be understood e.g. as VM task in multitasking environment.
 * In the case record store is accessed concurrently from a few execution contexts,
 * listeners registered to be called on record store changing must be notified on
 * changes done to the record store in any of these execution contexts.
 * RecordStoreRegistry is responsible for system wide notifications about
 * changes of record stores done in a different execution contexts.
 */
public class RecordStoreRegistry {

    /**
     * Starts listening of asynchronous changes of record store
     *
     * @param token security token to restrict usage of the method
     * @param suiteId suite ID of record store to start listen for
     * @param storeName name of record store to start listen for
     */
    public static void startRecordStoreListening(
            SecurityToken token, int suiteId, String storeName) {
        token.checkIfPermissionAllowed(Permissions.MIDP);
        startRecordStoreListening(suiteId, storeName);
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
        token.checkIfPermissionAllowed(Permissions.MIDP);
        stopRecordStoreListening(suiteId, storeName);
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
        token.checkIfPermissionAllowed(Permissions.MIDP);
        notifyRecordStoreChange(suiteId, storeName, changeType, recordId);    
    }

    /**
     * Native implementation of #startRecordStoreListening
     * @param suiteId suite ID of record store to start listen for
     * @param storeName name of record store to start listen for
     */
    private static native void startRecordStoreListening(
        int suiteId, String storeName);

    /**
     * Native implementation of #stopRecordStoreListening
     * @param suiteId suite ID of record store to stop listen for
     * @param storeName name of record store to stop listen for
     */
    private static native void stopRecordStoreListening(
        int suiteId, String storeName);

    /**
     * Native implementation of #notifyRecordStoreChange
     * @param suiteId suite ID of changed record store
     * @param storeName name of changed record store
     * @param changeType type of record change: ADDED, DELETED or CHANGED
     * @param recordId ID of changed record
     */
    private static native void notifyRecordStoreChange(
        int suiteId, String storeName, int changeType, int recordId);
}

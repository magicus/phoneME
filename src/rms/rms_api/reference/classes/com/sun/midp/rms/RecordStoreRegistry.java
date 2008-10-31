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

public class RecordStoreRegistry {

    public static void startRecordStoreListening(
            SecurityToken token, int suiteId, String storeName) {
        token.checkIfPermissionAllowed(Permissions.MIDP);
        startRecordStoreListening(suiteId, storeName);
    }

    public static void stopRecordStoreListening(
            SecurityToken token, int suiteId, String storeName) {
        token.checkIfPermissionAllowed(Permissions.MIDP);
        stopRecordStoreListening(suiteId, storeName);
    }

    public static void notifyRecordStoreChange(
            SecurityToken token, int suiteId, String storeName,
            int changeType, int recordId) {
        token.checkIfPermissionAllowed(Permissions.MIDP);
        notifyRecordStoreChange(suiteId, storeName, changeType, recordId);    
    }

    private static native void startRecordStoreListening(
        int suiteId, String storeName);

    private static native void stopRecordStoreListening(
        int suiteId, String storeName);

    private static native void notifyRecordStoreChange(
        int suiteId, String storeName, int changeType, int recordId);
}

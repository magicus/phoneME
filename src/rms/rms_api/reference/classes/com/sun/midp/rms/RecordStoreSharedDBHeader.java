/*
 *
 *
 * Portions Copyright  2000-2007 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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
 *
 * Copyright 2000 Motorola, Inc. All Rights Reserved.
 * This notice does not imply publication.
 */

package com.sun.midp.rms;

import java.io.IOException;
import javax.microedition.rms.*;

import com.sun.midp.security.Permissions;
import com.sun.midp.security.SecurityToken;
import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

/**
 * Class for sharing record store DB header between MIDlets. 
 * The reason why this class is needed is because we don't want 
 * to re-read the header form disk every time the record store
 * has been changed, so we share it in native instead.
 */
class RecordStoreSharedDBHeader {
    /** Cached header data, may be out of date */
    private byte[] cachedHeaderData;

    /** ID used for lookup in native code */
    private int lookupId;

    RecordStoreSharedDBHeader(int suiteId, String storeName, 
            byte[] theHeaderData) {

        cachedHeaderData = theHeaderData;
        lookupId = getLookupId0(suiteId, storeName, theHeaderData.length);
    }

    synchronized void headerUpdated(byte[] theHeaderData) {
        cachedHeaderData = theHeaderData;
        headerUpdated0(lookupId, cachedHeaderData);
    }

    synchronized byte[] getHeaderData() {
        getHeaderData0(lookupId, cachedHeaderData);
        return cachedHeaderData;
    }

    /**
     * Gets lookup ID 
     */
    private static native int getLookupId0(int suiteId, String storeName, 
            int headerDataSize);

    private static native void headerUpdated0(int lookupId, byte[] headerData);
    private static native void getHeaderData0(int lookupId, byte[] headerData);
}

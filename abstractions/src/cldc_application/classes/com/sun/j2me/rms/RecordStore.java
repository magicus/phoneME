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

package com.sun.j2me.rms;

/**
 * A class representing a record store. 
 */

public class RecordStore {

    javax.microedition.rms.RecordStore rs;

    public RecordStore(javax.microedition.rms.RecordStore rs) {
        this.rs = rs;
    }

    public static RecordStore openRecordStore(String recordStoreName,
                                              boolean createIfNecessary)
        throws RecordStoreException, RecordStoreFullException,
        RecordStoreNotFoundException {
        
        try {
            return new RecordStore(javax.microedition.rms.RecordStore.openRecordStore(recordStoreName,
                                                          createIfNecessary));
        } catch (javax.microedition.rms.RecordStoreFullException rsfe) {
            throw new RecordStoreFullException(rsfe.getMessage());
        } catch (javax.microedition.rms.RecordStoreNotFoundException rsnfe) {
            throw new RecordStoreNotFoundException(rsnfe.getMessage());
        } catch (javax.microedition.rms.RecordStoreException rse) {
            throw new RecordStoreException(rse.getMessage());
        }

    }

    public RecordEnumeration enumerateRecords(RecordFilter filter,
                                              RecordComparator comparator,
                                              boolean keepUpdated)
        throws RecordStoreNotOpenException {

        try {
            return (RecordEnumeration)(new RecordEnumerationImpl(rs.enumerateRecords(
                                       (javax.microedition.rms.RecordFilter)filter,
                                       (javax.microedition.rms.RecordComparator)comparator,
                                       keepUpdated)));
        } catch (javax.microedition.rms.RecordStoreNotOpenException rse) {
            throw new RecordStoreNotOpenException(rse.getMessage());
        }
    }

    public byte[] getRecord(int recordId)
        throws RecordStoreNotOpenException, InvalidRecordIDException,
            RecordStoreException {

        try {
            return rs.getRecord(recordId);
        } catch (javax.microedition.rms.RecordStoreNotOpenException rsnoe) {
            throw new RecordStoreNotOpenException(rsnoe.getMessage());
        } catch (javax.microedition.rms.InvalidRecordIDException irie) {
            throw new InvalidRecordIDException(irie.getMessage());
        } catch (javax.microedition.rms.RecordStoreException rse) {
            throw new RecordStoreException(rse.getMessage());
        }
    }
}

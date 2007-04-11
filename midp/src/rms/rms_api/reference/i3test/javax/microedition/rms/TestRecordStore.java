/*
 *   
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
package javax.microedition.rms;

import com.sun.midp.i3test.*;

public class TestRecordStore extends TestCase {
    final String RECORD_STORE_NAME = "testrms";
    byte[] smallData;
    byte[] largeData;
    int recordId1, recordId2;

    /**
     * Add a small record then a large record.
     */
    private void setupRSTest() throws RecordStoreException {

	smallData = new byte[5]; // 5 Bytes
	// Contents are 0,1,...,n
	for (int i = 0; i < smallData.length; i++) {
	    smallData[i] = (byte)i;
	}

	largeData = new byte[10*1024]; // 10K Bytes
	// Contents are 0,1,...,n
	for (int i = 0; i < largeData.length; i++) {
	    largeData[i] = (byte)i;
	}

	// Remove old record store
	try {
	    RecordStore.deleteRecordStore(RECORD_STORE_NAME);
	} catch (RecordStoreNotFoundException rnfe) {}

        RecordStore store = RecordStore.openRecordStore(RECORD_STORE_NAME, true);
        recordId1 = store.addRecord(smallData, 0, smallData.length);
        recordId2 = store.addRecord(largeData, 0, largeData.length);
        store.closeRecordStore();
    }

    private void testSequentialRMS() throws RecordStoreException {
        RecordStore store1 = null;
        RecordStore store2 = null;
        boolean exceptionThrown = false;

        declare("Sequential access");

        store1 = RecordStore.openForLockTesting(RECORD_STORE_NAME);

        try {

            try {
                store2 = RecordStore.openForLockTesting(RECORD_STORE_NAME);
            } catch (RecordStoreException re) {
                exceptionThrown = true;
            } 
            
            if (store2 != null) {    
                store2.closeRecordStore();
            }
        } finally {
            store1.closeRecordStore();
        }

        assertTrue(exceptionThrown);
    }

    private void testEnumeration() throws RecordStoreException {

        declare("getNumRecords");
        RecordStore store = RecordStore.openRecordStore(RECORD_STORE_NAME, false);
	assertTrue(store.getNumRecords() == 2);

        declare("Enumeration nextRecordId");
	RecordEnumeration recordenumeration = store.enumerateRecords(null, null,
	true);
	assertTrue("recordId1", recordenumeration.nextRecordId() == recordId1);
	assertTrue("recordId2", recordenumeration.nextRecordId() == recordId2);

        declare("Enumeration nextRecord");
	byte record[];
	int i;

	recordenumeration.reset();

	record = recordenumeration.nextRecord();
	for (i = 0; i < record.length; i++)
	    if (record[i] != smallData[i])
		break;
	assertTrue("small", record.length == smallData.length && i == record.length);

	record = recordenumeration.nextRecord();
	for (i = 0; i < record.length; i++)
	    if (record[i] != largeData[i])
		break;

	assertTrue("large", record.length == largeData.length && i == record.length);

        store.closeRecordStore();
    }

    private void testCompactRecords() throws RecordStoreException {

        declare("testCompactRecords");
        RecordStore store = RecordStore.openRecordStore(RECORD_STORE_NAME, false);

	// Delete the small record
	store.deleteRecord(recordId1);

	// Close the store to exercise compacting code
	store.closeRecordStore();

	// Reopen and check for the large record
        store = RecordStore.openRecordStore(RECORD_STORE_NAME, false);
        
	byte record[] = store.getRecord(recordId2);
	int i;

	for (i = 0; i < record.length; i++)
	    if (record[i] != largeData[i])
		break;

	assertTrue(record.length == largeData.length && i == record.length);

        store.closeRecordStore();
    }

    private void cleanup() throws RecordStoreException {
        RecordStore.deleteRecordStore(RECORD_STORE_NAME);
    }

    public void runTests() throws Throwable {

        setupRSTest();

        try {
            testSequentialRMS();
	    testEnumeration();
	    testCompactRecords();
	} catch (Throwable t) {
	    t.printStackTrace();
        } finally {
            cleanup();
        }

    }
}

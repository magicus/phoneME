/*
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
package com.sun.midp.jump.push.executive.persistence;

import junit.framework.*;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;


public final class AppSuiteDataStoreTest extends TestCase {

    public AppSuiteDataStoreTest(String testName) {
        super(testName);
    }

    /** Dir for AppSuiteDataStore. */
    private static final String DIR = "./dir";

    /**
     * Data converter.
     *
     * <p>As plaing <code>String</code>s are used, it quite simple</p>
     */
    private static final AppSuiteDataStore.DataConverter DATA_CONVERTER =
            new AppSuiteDataStore.DataConverter() {
                public String dataToString(final Object obj) {
                    return (String) obj;
                }

                public Object stringToData(final String s) {
                    return s;
                }
    };

    /**
     * Creates a handle for <code>JUMPStore</code>.
     *
     * @returns a handle
     */
    private static StoreOperationManager createStoreManager()
	throws IOException {
        return StoreUtils.createInMemoryManager(new String [] {DIR});
    }

    private AppSuiteDataStore createAppSuiteDataStore(
            final StoreOperationManager storeManager) throws IOException {
        return new AppSuiteDataStore(storeManager, DIR, DATA_CONVERTER);
    }

    private void _testCtorThrowsIllegalArgumentException(
            final StoreOperationManager storeManager,
            final String dir,
            final AppSuiteDataStore.DataConverter dataConverter,
            final String paramName) throws IOException {
        try {
            new AppSuiteDataStore(storeManager, dir, dataConverter);
            fail("AppSuiteDataStore.<init> doesn't throw"
                    + " IllegalArgumentException for null "
                    + paramName);
        } catch (IllegalArgumentException _) {
        }
    }

    /**
     * Tests that AppSuiteDataStore constructor throws an exception
     * when fed with <code>null</code> as store handler.
     */
    public void testCtorNullStoreHandler() throws IOException {
        _testCtorThrowsIllegalArgumentException(
                null, DIR, DATA_CONVERTER, "store handle");
    }

    /**
     * Tests that AppSuiteDataStore constructor throws an exception
     * when fed with <code>null</code> as store handler.
     */
    public void testCtorNullDir() throws IOException {
        _testCtorThrowsIllegalArgumentException(
                createStoreManager(), null, DATA_CONVERTER, "dir");
    }

    /**
     * Tests that AppSuiteDataStore constructor throws an exception
     * when fed with <code>null</code> as store handler.
     */
    public void testCtorNullDataConverter() throws IOException {
        _testCtorThrowsIllegalArgumentException(
                createStoreManager(), DIR, null, "data converter");
    }

    /**
     * Tests that <code>updateSuiteData</code> throws
     * <code>IllegalArgumentException</code> when fed with <code>null</code>.
     */
    public void testUpdateSuiteDataThrows() throws IOException {
        try {
            createAppSuiteDataStore(createStoreManager())
                .updateSuiteData(0, null);
            fail("should throw IllegalArgumentException");
        } catch (IllegalArgumentException _) {
        }
    }

    /** Bundle of suite id and data. */
    private static final class SuiteData {
        private final int suiteId;
        private final String data;

        SuiteData(final int suiteId, final String data) {
            this.suiteId = suiteId;
            this.data = data;
        }

        void updateStore(final AppSuiteDataStore store) throws IOException {
            if (data != null) {
                store.updateSuiteData(suiteId, data);
            } else {
                store.removeSuiteData(suiteId);
            }
        }

        static void updateStore(
                final AppSuiteDataStore store,
                final SuiteData [] dataToUpdate) throws IOException {
            for (int i = 0; i < dataToUpdate.length; i++) {
                dataToUpdate[i].updateStore(store);
            }
        }
    }

    private void checkTestData(
            final AppSuiteDataStore store,
            final SuiteData [] expected) {
        /*
         * NB: it's ok for expected to have several records
         * for the same suite: only the last one will be picked
         * (following AppSuiteDataStore semantics)
         */
        final Map e = new HashMap();
        for (int i = 0; i < expected.length; i++) {
            final SuiteData d = expected[i];
            e.put(new Integer(d.suiteId), d.data);
        }

        final Map actual = new HashMap();

        store.listData(new AppSuiteDataStore.DataConsumer() {
            public void consume(final int suiteId, final Object data) {
                assertNull("reported twice: " + suiteId,
                        actual.put(new Integer(suiteId), data));
            }
        });

        assertEquals(e, actual);
    }

    private void checkTestData(
            final SuiteData [] dataToUpdate,
            final SuiteData [] expected) throws IOException {
        final StoreOperationManager storeManager = createStoreManager();

        final AppSuiteDataStore store = createAppSuiteDataStore(storeManager);
        SuiteData.updateStore(store, dataToUpdate);
        checkTestData(store, expected);
        // And reread it afresh (to emulate real persistence)
        checkTestData(createAppSuiteDataStore(storeManager), expected);
    }

    private void checkGetSuiteData(final AppSuiteDataStore store)
            throws IOException {
        store.listData(new AppSuiteDataStore.DataConsumer() {
            public void consume(final int suiteId, final Object data) {
                assertEquals(data, store.getSuiteData(suiteId));
            }
        });
    }

    private void checkGetSuiteData(final SuiteData [] dataToUpdate)
            throws IOException {
        final StoreOperationManager storeManager = createStoreManager();

        final AppSuiteDataStore store = createAppSuiteDataStore(storeManager);
        SuiteData.updateStore(store, dataToUpdate);
        checkGetSuiteData(store);
        // And reread it afresh (to emulate real persistence)
        checkGetSuiteData(createAppSuiteDataStore(storeManager));
    }

    private final static SuiteData [] NO_UPDATES = new SuiteData [] {
    };

    private final static SuiteData [] ADD_AND_REMOVE = new SuiteData [] {
        new SuiteData(0, "foo"),
        new SuiteData(0, null),
    };

    private final static SuiteData [] SEVERAL_SUITES = new SuiteData [] {
        new SuiteData(0, "foo"),
        new SuiteData(239, "bar"),
        new SuiteData(1024, "qux"),
    };

    private final static SuiteData [] OVERWRITTES = new SuiteData [] {
        new SuiteData(17, "foo"),
        new SuiteData(17, "bar")
    };

    public void testGetSuiteDataNO_UPDATES() throws IOException {
        checkGetSuiteData(NO_UPDATES);
    }

    public void testGetSuiteDataADD_AND_REMOVE() throws IOException {
        checkGetSuiteData(ADD_AND_REMOVE);
    }

    public void testGetSuiteDataSEVERAL_SUITES() throws IOException {
        checkGetSuiteData(SEVERAL_SUITES);
    }

    public void testGetSuiteDataOVERWRITTES() throws IOException {
        checkGetSuiteData(OVERWRITTES);
    }

    public void testNO_UPDATES() throws IOException {
        checkTestData(NO_UPDATES, NO_UPDATES);
    }

    public void testADD_AND_REMOVE() throws IOException {
        checkTestData(ADD_AND_REMOVE, new SuiteData [] {});
    }

    public void testSEVERAL_SUITES() throws IOException {
        checkTestData(SEVERAL_SUITES, SEVERAL_SUITES);
    }

    public void testOVERWRITTES() throws IOException {
        checkTestData(OVERWRITTES, OVERWRITTES);
    }
}

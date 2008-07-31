/*
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

package com.sun.midp.jump.push.executive.persistence;

import com.sun.jump.module.contentstore.JUMPData;
import com.sun.jump.module.contentstore.JUMPNode;
import java.io.IOException;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

/**
 * Manages storing information associated with an application suite.
 *
 * <p>The class is <em>not</em> thread-safe.</p>
 *
 * <p>
 * IMPORTANT_NOTE:  The clients of this class should ensure that content store
 *  manager passed into the constructor doesn't have exclusive lock when
 *  methods are invoked.  Otherwise we'll face deadlocks.
 * <p>
 */
final class AppSuiteDataStore {
    /*
     * Typical memory footprint vs. performance choice:
     * one can keep all the data in the content store only and reparse
     * them on each invocation or keep data cache (the latter option
     * was picked)
     */

    /**
     * Abstarcts data to string and back conversion.
     */
    static interface DataConverter {
        /**
         * Converts data into a string.
         *
         * @param data data to convert
         *
         * @return string representation
         */
        String dataToString(Object data);

        /**
         * Converts a string into data.
         *
         * @param s string to convert
         *
         * @return data
         */
        Object stringToData(String s);
    }

    /** JUMP content store manager to use. */
    private final StoreOperationManager storeManager;

    /** Dir to store data in. */
    private final String dir;

    /** Data converter to use. */
    private final DataConverter dataConverter;

    /** Data for all app suites. */
    private Map data;

    /**
     * Creats an app data store and reads the data.
     *
     * @param storeManager JUMP content store manager to use
     * @param dir dir to store data in
     * @param dataConverter data converter to use
     *
     * @throws IOException if IO fails
     */
    AppSuiteDataStore(
            final StoreOperationManager storeManager,
            final String dir,
            final DataConverter dataConverter) throws IOException {
        if (storeManager == null) {
            throw new IllegalArgumentException("storeManager is null");
        }
        this.storeManager = storeManager;

        if (dataConverter == null) {
            throw new IllegalArgumentException("dataConverter is null");
        }
        this.dataConverter = dataConverter;

        if (dir == null) {
            throw new IllegalArgumentException("dir is null");
        }
        this.dir = dir;

        this.data = null;

        readData();
    }

    /**
     * Reads all the data for each app.
     *
     * @throws IOException if the content store failed
     */
    private void readData() throws IOException {
        data = new HashMap();

        final JUMPNode.List dataDir = (JUMPNode.List) storeManager.getNode(dir);
        /*
         * IMPL_NOTE: the presence of the corresponding dir must be ensured
         * before
         */
        // assert dataDir != null : "application suite data dir is missing";
        for (Iterator it = dataDir.getChildren(); it.hasNext(); ) {
            final JUMPNode.Data elem = (JUMPNode.Data) it.next();
            final int suiteId = convertNameToId(elem.getName());
            final Object d =
                    dataConverter.stringToData(elem.getData().getStringValue());

            final Integer key = new Integer(suiteId);
            // assert !data.containsKey(key);
            data.put(key, d);
        }
    }

    /** Callback for app suite data listing. */
    static interface DataConsumer {
        /**
         * Consumes app suite data.
         *
         * @param suiteId app suite id
         * @param data app suite data
         */
        void consume(int suiteId, Object data);
    }

    /**
     * Lists all the data.
     *
     * @param dataLister data lister
     */
    void listData(final DataConsumer dataLister) {
        for (Iterator it = data.entrySet().iterator(); it.hasNext(); ) {
            final Map.Entry entry = (Map.Entry) it.next();
            final int suiteId = ((Integer) entry.getKey()).intValue();
            final Object suiteData = entry.getValue();
            dataLister.consume(suiteId, suiteData);
        }
    }

    /**
     * Gets data associated with the app suite.
     *
     * @param suiteId app suite to fetch data for
     *
     * @return data or <code>null</code>
     */
    Object getSuiteData(final int suiteId) {
        return data.get(new Integer(suiteId));
    }

    /**
     * Updates data for the given suite.
     *
     * @param suiteId app suite to update data for
     * @param suiteData data to store
     *
     * @throws IOException if the content store failed
     */
    void updateSuiteData(
            final int suiteId,
            final Object suiteData)
            throws IOException {
        if (suiteData == null) {
            throw new IllegalArgumentException("suiteData is null");
        }

        final String nodeUri = makeURI(suiteId);
        final Integer key = new Integer(suiteId);
        final JUMPData jumpData = new JUMPData(
                dataConverter.dataToString(suiteData));

        storeManager.safelyUpdateDataNode(nodeUri, jumpData);
        data.put(key, suiteData);
    }

    /**
     * Removes data for the given suite.
     *
     * @param suiteId app suite to remove data for
     *
     * @throws IOException if the content store failed
     */
    void removeSuiteData(final int suiteId) throws IOException {
        final String nodeUri = makeURI(suiteId);
        storeManager.safelyDeleteDataNode(nodeUri);
        data.remove(new Integer(suiteId));
    }

    /** Radix to encode suite IDs. */
    private static final int RADIX = 16;

    /**
     * Forms an URI for the suite.
     *
     * @param suiteId app suite ID to form URI for
     *
     * @return URI
     */
    private String makeURI(final int suiteId) {
        return dir + "/" + Integer.toString(suiteId, RADIX);
    }

    /**
     * Converts a name into app suite ID.
     *
     * @param name Name to convert
     *
     * @return app suite ID
     */
    private static int convertNameToId(final String name) {
        return Integer.valueOf(name, RADIX).intValue();
    }
}

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

import com.sun.midp.push.controller.ConnectionInfo;
import com.sun.midp.push.persistence.Store;
import com.sun.midp.push.persistence.Store.AlarmsConsumer;
import com.sun.midp.push.persistence.Store.ConnectionsConsumer;

import com.sun.jump.module.contentstore.JUMPNode;
import com.sun.jump.module.contentstore.JUMPStoreHandle;
import java.io.IOException;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Vector;

/**
 * JUMP specific implementation of <code>Store</code> interface.
 *
 * <p>
 * IMPORTANT_NOTE:  The clients of this class should ensure that content store
 *  manager passed into the constructor doesn't have exclusive lock when
 *  methods are invoked.  Otherwise we'll face deadlocks.
 * <p>
 *
 * <p><strong>Implementation notice</strong>: as for now if <code>MIDlet
 * suite</code> removes all the connections, the file with suite connections
 * is not removed and the suite is filtered in {@line getConnections} method.
 * Another option might be to remove the file.</p>
 */
public final class JUMPStoreImpl implements Store {
    /** PushRegistry root dir. */
    private static final String ROOT_DIR = "./push/";

    /** Dir to store connections. */
    static final String CONNECTIONS_DIR = ROOT_DIR + "connections";

    /** Dir to store alarms. */
    static final String ALARMS_DIR = ROOT_DIR + "alarms";

    /** PushRegistry connections. */
    private final AppSuiteDataStore connectionsStore;

    /** PushRegistry alarms. */
    private final AppSuiteDataStore alarmsStore;

    /**
     * Constructs a JUMPStoreImpl and reads the data.
     *
     * @param storeManager JUMP content store manager to use
     *
     * @throws IOException if IO fails
     */
    public JUMPStoreImpl(final StoreOperationManager storeManager)
            throws IOException {
        if (storeManager == null) {
            throw new IllegalArgumentException("storeManager is null");
        }

        ensureStoreStructure(storeManager);

        connectionsStore = new AppSuiteDataStore(
                storeManager, CONNECTIONS_DIR, CONNECTIONS_CONVERTER);

        alarmsStore = new AppSuiteDataStore(
                storeManager, ALARMS_DIR, ALARMS_CONVERTER);
    }

    /**
     * Ensures presence of store layout requested for the
     *  store to function correctly.
     *
     * @param storeManager JUMP content store manager to use
     *
     * @throws IOException in case of IO errors
     */
    private static void ensureStoreStructure(
            final StoreOperationManager storeManager)
            throws IOException {
        storeManager.doOperation(true, new StoreOperationManager.Operation() {
            public Object perform(final JUMPStoreHandle storeHandle)
                    throws IOException {
                ensureDir(storeHandle, CONNECTIONS_DIR);
                ensureDir(storeHandle, ALARMS_DIR);
                return null;
            }
        });
    }

    /**
     * Ensures presence of the given dir.
     *
     * @param storeHandle handle to content store to use
     * @param dir name of directory to check
     *
     * @throws IOException in case of IO troubles
     */
    private static void ensureDir(
            final JUMPStoreHandle storeHandle,
            final String dir) throws IOException {
        final JUMPNode node = storeHandle.getNode(dir);
        if (node == null) {
            logWarning("Directory " + dir + " is missing: recreating");
            storeHandle.createNode(dir);
            return;
        }
        if (!(node instanceof JUMPNode.List)) {
            logWarning("Node " + dir
                    + " is a data node: trying to erase and recreate");
            storeHandle.deleteNode(dir);
            storeHandle.createNode(dir);
            return;
        }
    }

    /**
     * Logs warning message.
     *
     * @param msg warning to log
     */
    private static void logWarning(final String msg) {
        // TBD: use common logging
        System.out.println("[warning] " + JUMPStoreImpl.class + ": " + msg);
    }

    /** {@inheritDoc} */
    public synchronized void listConnections(
            final ConnectionsConsumer connectionsLister) {
        connectionsStore.listData(new AppSuiteDataStore.DataConsumer() {
           public void consume(final int suiteId, final Object suiteData) {
               final Vector v = (Vector) suiteData;
               final ConnectionInfo [] cns
                       = new ConnectionInfo[v.size()];
               v.toArray(cns);
               connectionsLister.consume(suiteId, cns);
           }
        });
    }

    /** {@inheritDoc} */
    public synchronized void addConnection(
            final int midletSuiteID,
            final ConnectionInfo connection)
            throws IOException {
        Vector cns = (Vector) connectionsStore.getSuiteData(midletSuiteID);
        if (cns == null) {
            cns = new Vector();
        }
        cns.add(connection);
        connectionsStore.updateSuiteData(midletSuiteID, cns);
    }

    /** {@inheritDoc} */
    public synchronized void addConnections(
            final int midletSuiteID,
            final ConnectionInfo[] connections)
            throws IOException {
        final Vector cns = new Vector(Arrays.asList(connections));
        connectionsStore.updateSuiteData(midletSuiteID, cns);
    }

    /** {@inheritDoc} */
    public synchronized void removeConnection(
            final int midletSuiteID,
            final ConnectionInfo connection)
            throws IOException {
        Vector cns = (Vector) connectionsStore.getSuiteData(midletSuiteID);
        // assert cns != null : "cannot be null";
        cns.remove(connection);
        if (!cns.isEmpty()) {
            connectionsStore.updateSuiteData(midletSuiteID, cns);
        } else {
            connectionsStore.removeSuiteData(midletSuiteID);
        }
    }


    /** {@inheritDoc} */
    public synchronized void removeConnections(final int midletSuiteID)
            throws IOException {
        connectionsStore.removeSuiteData(midletSuiteID);
    }

    /** {@inheritDoc} */
    public synchronized void listAlarms(final AlarmsConsumer alarmsLister) {
        alarmsStore.listData(new AppSuiteDataStore.DataConsumer() {
           public void consume(final int suiteId, final Object suiteData) {
               alarmsLister.consume(suiteId, (Map) suiteData);
           }
        });
    }

    /** {@inheritDoc} */
    public synchronized void addAlarm(
            final int midletSuiteID,
            final String midlet,
            final long time)
            throws IOException {
        HashMap as = (HashMap) alarmsStore.getSuiteData(midletSuiteID);
        if (as == null) {
            as = new HashMap();
        }
        as.put(midlet, new Long(time));
        alarmsStore.updateSuiteData(midletSuiteID, as);
    }

    /** {@inheritDoc} */
    public synchronized void removeAlarm(
            final int midletSuiteID,
            final String midlet)
            throws IOException {
        final HashMap as = (HashMap) alarmsStore.getSuiteData(midletSuiteID);
        // assert as != null;
        as.remove(midlet);
        if (!as.isEmpty()) {
            alarmsStore.updateSuiteData(midletSuiteID, as);
        } else {
            alarmsStore.removeSuiteData(midletSuiteID);
        }
    }

    /** Connections converter. */
    private static final AppSuiteDataStore.DataConverter CONNECTIONS_CONVERTER
            = new ConnectionConverter();

    /** Implements conversion interface for connections. */
    private static final class ConnectionConverter
            implements AppSuiteDataStore.DataConverter {
        /**
         * Separator to use.
         *
         * <p>
         * <strong>NB</strong>: Separator shouldn't be valid character
         * for <code>connection</code>, <code>midlet</code> or
         * <code>filter</code> fields of <code>JUMPConnectionInfo</code>
         */
        static final char SEPARATOR = '\n';

        /**
         * Number of strings per record.
         */
        static final int N_STRINGS_PER_RECORD = 3;

        /**
         * Converts a <code>Vector</code> of <code>JUMPConnectionInfo</code>
         *  into a string.
         *
         * @param data data to convert
         * @return string with all connections
         */
        public String dataToString(final Object data) {
            final Vector connections = (Vector) data;
            if (connections == null) {
                throw new
                        IllegalArgumentException("connections vector is null");
            }

            final StringBuffer sb = new StringBuffer();

            for (Iterator it = connections.iterator(); it.hasNext(); ) {
                ConnectionInfo connection = (ConnectionInfo) it.next();
                sb.append(connection.connection);   sb.append(SEPARATOR);
                sb.append(connection.midlet);       sb.append(SEPARATOR);
                sb.append(connection.filter);       sb.append(SEPARATOR);
            }

            return sb.toString();
        }

        /**
         * Converts a string into a <code>Vector</code> of
         *  <code>JUMPConnectionInfo</code>.
         *
         * @param string string to convert
         * @return <code>Vector</code> of connections
         */
        public Object stringToData(final String string) {
            if (string == null) {
                throw new IllegalArgumentException("string is null");
            }

            Vector split = splitString(string);
            String [] ss = new String [split.size()];
            split.toArray(ss);

            // assert (strings.length % N_STRINGS_PER_RECORD == 0)
            //  : "Broken data";

            Vector v = new Vector();
            for (int i = 0; i < ss.length; i += N_STRINGS_PER_RECORD) {
                v.add(new ConnectionInfo(ss[i], ss[i + 1], ss[i + 2]));
            }
            return v;
        }

        /**
         * Splits a string into <code>Vector</code> of strings.
         *
         * @param string string to split
         *
         * @return <code>Vector</code> of strings splitted by
         *  <code>SEPARATOR</code>
         */
        private Vector splitString(final String string) {
            Vector v = new Vector();
            int start = 0;
            while (true) {
                int i = string.indexOf(SEPARATOR, start);
                if (i == -1) {
                    // assert start == string.length();
                    return v;
                }
                v.add(string.substring(start, i));
                start = i + 1;
            }
        }
    };

    /** Alarms converter. */
    private static final AppSuiteDataStore.DataConverter ALARMS_CONVERTER
            = new AlarmsConverter();

    /** Implements conversion interface for alarms. */
    private static final class AlarmsConverter
            implements AppSuiteDataStore.DataConverter {
        /** Char to seprate midlet class name from time. */
        private static final char FIELD_SEP = ':';

        /**
         * Converts data into a string.
         *
         * @param data data to convert
         *
         * @return string representation
         */
        public String dataToString(final Object data) {
            final Map m = (Map) data;

            final StringBuffer sb = new StringBuffer();
            for (Iterator it = m.entrySet().iterator(); it.hasNext(); ) {
                final Map.Entry entry = (Map.Entry) it.next();
                final String midlet = (String) entry.getKey();
                final Long time = (Long) entry.getValue();
                sb.append(midlet);
                sb.append(FIELD_SEP);
                sb.append(time);
                sb.append('\n');
            }

            return sb.toString();
        }

        /**
         * Converts a string into data.
         *
         * @param s string to convert
         *
         * @return data
         */
        public Object stringToData(final String s) {
            final HashMap data = new HashMap();
            int pos = 0;
            while (true) {
                final int p = s.indexOf('\n', pos);
                if (p == -1) {
                    // assert pos == s.length() - 1 : "unprocessed chars!";
                    return data;
                }
                final String record = s.substring(pos, p);

                // Parse record
                final int sepPos = record.indexOf(FIELD_SEP);
                // assert sepPos != -1 : "wrong record";
                final String midlet = record.substring(0, sepPos);
                final String timeString = record.substring(sepPos + 1);
                data.put(midlet, Long.valueOf(timeString));

                pos = p + 1;
            }
        }
    }
}

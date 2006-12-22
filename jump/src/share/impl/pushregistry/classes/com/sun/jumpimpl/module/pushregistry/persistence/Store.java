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
package com.sun.jumpimpl.module.pushregistry.persistence;

import com.sun.jump.module.contentstore.JUMPStoreHandle;
import com.sun.jump.module.pushregistry.JUMPConnectionInfo;
import java.io.IOException;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Vector;

/**
 * Persistent store class for <code>PushRegistry</code> module.
 *
 * <p><strong>NB</strong>: method <code>getConnections</code> guarantees
 * that <code>MIDlet suite</code> without connections won't be listed,
 * i.e. <code>MIDletSuiteConnections.connections</code>
 * array is not empty</p>
 *
 * <p><strong>NB</strong>: this class has no intellegence of connection
 * semantics, e.g. it doesn't handle connection conflicts.  It's a simplistic
 * database.</p>
 *
 * <p><strong>Implementation notice</strong>: as for now if <code>MIDlet
 * suite</code> removes all the connections, the file with suite connections
 * is not removed and the suite is filtered in {@line getConnections} method.
 * Another option might be to remove the file.</p>
 */
public final class Store {
    /** PushRegistry root dir. */
    private static final String ROOT_DIR = "./PushRegistry/";

    /** Dir to store connections. */
    static final String CONNECTIONS_DIR = ROOT_DIR + "connections";

    /** Dir to store alarms. */
    static final String ALARMS_DIR = ROOT_DIR + "alarms";

    /** PushRegistry connections. */
    private final AppSuiteDataStore connectionsStore;

    /** PushRegistry alarms. */
    private final AppSuiteDataStore alarmsStore;

    /**
     * Constructor.
     *
     * @param storeHandle JUMP store to use
     */
    public Store(final JUMPStoreHandle storeHandle) {
        connectionsStore = new AppSuiteDataStore(
                storeHandle, CONNECTIONS_DIR, CONNECTIONS_CONVERTER);

        alarmsStore = new AppSuiteDataStore(
                storeHandle, ALARMS_DIR, ALARMS_CONVERTER);
    }

    /**
     * Reads all the data.
     *
     * @throws IOException if the content store failed
     */
    public void readData() throws IOException {
        connectionsStore.readData();
        alarmsStore.readData();
    }

    /** Connections consumer. */
    public static interface ConnectionsConsumer {
        /**
         * COnsumes app suite connections.
         *
         * @param suiteId app suite ID
         * @param connections app suite connections
         */
        void consume(int suiteId, JUMPConnectionInfo [] connections);
    }

    /**
     * Lists all registered connections.
     *
     * @param connectionsLister connection lister
     */
    public synchronized void listConnections(
            final ConnectionsConsumer connectionsLister) {
        connectionsStore.listData(new AppSuiteDataStore.DataConsumer() {
           public void consume(final int suiteId, final Object suiteData) {
               final Vector v = (Vector) suiteData;
               final JUMPConnectionInfo [] cns
                       = new JUMPConnectionInfo[v.size()];
               v.toArray(cns);
               connectionsLister.consume(suiteId, cns);
           }
        });
    }

    /**
     * Adds new registered connection.
     *
     * <p><strong>Precondition</strong>: <code>connection</code> MUST not be
     * already registered (the method doesn't check it)</p>
     *
     * @param midletSuiteID ID of <code>MIDlet suite</code> to register
     *   connection for
     *
     * @param connection Connection to register
     *
     * @throws IOException if the content store failed to perform operation
     */
    public synchronized void addConnection(
            final int midletSuiteID,
            final JUMPConnectionInfo connection)
            throws IOException {
        Vector cns = (Vector) connectionsStore.getSuiteData(midletSuiteID);
        if (cns == null) {
            cns = new Vector();
        }
        cns.add(connection);
        connectionsStore.updateSuiteData(midletSuiteID, cns);
    }

    /**
     * Adds connections for the suite being installed.
     *
     * <p><strong>Preconditin</strong>: <code>connection</code> MUST not be
     * already registered (the method doesn't check it)</p>
     *
     * @param midletSuiteID ID of <code>MIDlet suite</code> to register
     *   connection for
     *
     * @param connections Connections to register
     *
     * @throws IOException if the content store failed to perform operation
     */
    public synchronized void addConnections(
            final int midletSuiteID,
            final JUMPConnectionInfo[] connections)
            throws IOException {
        final Vector cns = new Vector(Arrays.asList(connections));
        connectionsStore.updateSuiteData(midletSuiteID, cns);
    }

    /**
     * Removes registered connection.
     *
     * <p><strong>Preconditin</strong>: <code>connection</code> MUST have been
     * already registered (the method doesn't check it)</p>
     *
     * <p><strong>NB</strong>: <code>throws IOException</code> was intentionally
     * removed from the signature: it's resonsibility of <code>Store</code>
     * to ensure removal of all connections.</p>
     *
     * @param midletSuiteID ID of <code>MIDlet suite</code> to remove
     *   connection for
     *
     * @param connection Connection to remove
     *
     * @throws IOException if the content store failed to perform operation
     */
    public synchronized void removeConnection(
            final int midletSuiteID,
            final JUMPConnectionInfo connection)
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


    /**
     * Removes all registered connections.
     *
     * <p><strong>Preconditin</strong>: <code>connection</code> MUST have been
     * already registered (the method doesn't check it)</p>
     *
     * <p><strong>NB</strong>: <code>throws IOException</code> was intentionally
     * removed from the signature: it's resonsibility of <code>Store</code>
     * to ensure removal of all connections.</p>
     *
     * @param midletSuiteID ID of <code>MIDlet suite</code> to remove
     *   connections for
     *
     * @throws IOException if the content store failed
     */
    public synchronized void removeConnections(final int midletSuiteID)
            throws IOException {
        connectionsStore.removeSuiteData(midletSuiteID);
    }

    /** Alarms lister. */
    public static interface AlarmsConsumer {
        /**
         * Lists app suite alarms.
         *
         * @param suiteId app suite ID
         * @param alarms app suite alatms
         */
        void consume(int suiteId, Map alarms);
    }

    /**
     * Lists all alarms.
     *
     * @param alarmsLister connection lister
     */
    public synchronized void listAlarms(final AlarmsConsumer alarmsLister) {
        alarmsStore.listData(new AppSuiteDataStore.DataConsumer() {
           public void consume(final int suiteId, final Object suiteData) {
               alarmsLister.consume(suiteId, (Map) suiteData);
           }
        });
    }

    /**
     * Adds an alarm.
     *
     * @param midletSuiteID <code>MIDlet suite</code> to add alarm for
     * @param midlet <code>MIDlet</code> class name
     * @param time alarm time
     *
     * @throws IOException if the content store failed
     */
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

    /**
     * Removes an alarm.
     *
     * @param midletSuiteID <code>MIDlet suite</code> to remove alarm for
     * @param midlet <code>MIDlet</code> class name
     *
     * @throws IOException if the content store failed
     */
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

            for (Iterator it = connections.iterator(); it.hasNext();) {
                JUMPConnectionInfo connection = (JUMPConnectionInfo) it.next();
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
                v.add(new JUMPConnectionInfo(ss[i], ss[i + 1], ss[i + 2]));
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
            for (Iterator it = m.entrySet().iterator(); it.hasNext();) {
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
            for (;;) {
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

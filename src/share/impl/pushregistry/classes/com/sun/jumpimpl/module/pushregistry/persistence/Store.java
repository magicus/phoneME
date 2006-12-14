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

import com.sun.jump.module.contentstore.JUMPData;
import com.sun.jump.module.contentstore.JUMPNode;
import com.sun.jump.module.contentstore.JUMPStoreHandle;
import com.sun.jump.module.pushregistry.JUMPConnectionInfo;
import java.io.IOException;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
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
    /**
     * Representation of all connections registered for
     * a <code>MIDlet suite</code>.
     */
    public static class MIDletSuiteConnections {
        /**
         * ID of MIDlet suite.
         */
        public final int midletSuiteID;

        /**
         * Connections for the given <code>MIDlet suite</code>.
         */
        public final JUMPConnectionInfo[] connections;

        /**
         * Creates a suite connection info.
         *
         * @param midletSuiteID <code>MIDlet suite</code> id
         * @param connections suite connections
         */
        MIDletSuiteConnections(
                final int midletSuiteID,
                final JUMPConnectionInfo[] connections) {
            this.midletSuiteID = midletSuiteID;
            this.connections = connections;
        }
    }

    /**
     * Reference to the JUMP store to use.
     */
    private final JUMPStoreHandle store;

    /**
     * Root URI for <code>PushRegistry</code> persistent data.
     */
    private static final String ROOT_URI = "./PushRegistry";

    /**
     * Radix to encode MIDlet suite IDs.
     */
    private static final int RADIX = 16;

    /**
     * Constructor.
     *
     * @param storeHandle JUMP store to use
     */
    public Store(final JUMPStoreHandle storeHandle) {
        this.store = storeHandle;
    }

    /**
     * Gets all registered connections.
     *
     * @return Array with registered connections (this value is
     * <strong>guaranteed</strong> to be not <code>null</code>).
     *
     * @throws IOException if internal structure of data is broken
     */
    public MIDletSuiteConnections [] getConnections() throws IOException {
        final Vector connections = new Vector();

        final JUMPNode.List connectionNodes
                = (JUMPNode.List) store.getNode(ROOT_URI);

        // TBD: if it's impossible to ensure at deployment time existance of
        //  root dir, I should create one lazily
        // assert connectionNodes != null : "PushRegistry root dir is missing";
        for (Iterator it = connectionNodes.getChildren(); it.hasNext();) {
            final JUMPNode.Data elem = (JUMPNode.Data) it.next();
            final Vector suiteConnections = getNodeConnections(elem);
            if (suiteConnections.isEmpty()) {
                continue;
            }
            final int suiteId = convertNameToId(elem.getName());

            final JUMPConnectionInfo [] cns = new
                    JUMPConnectionInfo[suiteConnections.size()];
            suiteConnections.toArray(cns);

            connections.add(new MIDletSuiteConnections(suiteId, cns));
        }

        final MIDletSuiteConnections [] result = new
                MIDletSuiteConnections[connections.size()];
        connections.toArray(result);
        return result;
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
    public void addConnection(
            final int midletSuiteID,
            final JUMPConnectionInfo connection) throws IOException {
        updateNode(midletSuiteID,
                new Updater() {
                    public Vector update(final JUMPNode.Data node) {
                        final Vector connections = (node == null)
                            ? new Vector() : getNodeConnections(node);
                        connections.add(connection);
                        return connections;
                    }
        });
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
    public void addFreshConnections(
            final int midletSuiteID,
            final JUMPConnectionInfo[] connections)
            throws IOException {
        final String nodeUri = makeURI(midletSuiteID);
        final JUMPData data = new
                JUMPData(ToString.listToString(Arrays.asList(connections)));

        store.createDataNode(nodeUri, data);
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
    public void removeConnection(
            final int midletSuiteID,
            final JUMPConnectionInfo connection) throws IOException {
        updateNode(midletSuiteID,
                new Updater() {
            public Vector update(final JUMPNode.Data node) {
                // assert node != null :
                // "Removing connection from the suite without connections";
                final Vector connections = getNodeConnections(node);
                // assert connections != null : "Internal invariant broken";
                connections.remove(connection);
                return connections;
            }
        });
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
     * @throws IOException if the content store failed to perform operation
     */
    public void removeSuiteConnections(final int midletSuiteID)
            throws IOException {
        store.deleteNode(makeURI(midletSuiteID));
    }

    /**
     * Forms an URI for the suite.
     *
     * @param suiteId <code>MIDlet suite</code> ID to form URI for
     *
     * @return URI
     */
    private static String makeURI(final int suiteId) {
        return ROOT_URI + "/" + Integer.toString(suiteId, RADIX);
    }

    /**
     * Converts a name into <code>MIDlet suite</code> ID.
     *
     * @param name Name to convert
     *
     * @return <code>MIDlet suite</code> ID
     */
    private static int convertNameToId(final String name) {
        return Integer.valueOf(name, RADIX).intValue();
    }

    /**
     * Gets all the connections for the node.
     *
     * @param elem Node to fetch connections for
     *
     * @return vector of connections
     */
    private static Vector getNodeConnections(final JUMPNode.Data elem) {
        return ToString.stringToVector(elem.getData().getStringValue());
    }

    /**
     * General node mutator interface.
     */
    private static interface Updater {
        /**
         * Updates a node.
         *
         * @param node Node to update
         *
         * @return new data for this node
         */
        Vector update(JUMPNode.Data node);
    }

    /**
     * Updates a node.
     *
     * @param midletSuiteID <code>MIDlet suite</code> ID of suite to update
     * @param updater An object that would update the node
     *
     * @throws IOException if content store operation fails
     */
    private void updateNode(
            final int midletSuiteID,
            final Updater updater) throws IOException {
        final String nodeUri = makeURI(midletSuiteID);
        final JUMPNode.Data node = (JUMPNode.Data) store.getNode(nodeUri);

        final boolean doesNodeExist = (node != null);
        final JUMPData data = new
                JUMPData(ToString.listToString(updater.update(node)));

        if (doesNodeExist) {
            store.updateDataNode(nodeUri, data);
        } else {
            store.createDataNode(nodeUri, data);
        }
    }

    /**
     * Utility class to convert a <code>Vector</code> of
     * <code>JUMPConnectionInfo</code> into a string and vice verse.
     */
    private static final class ToString {
        /**
         * Hides a constructor.
         */
        private ToString() { }

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
         * Converts a <code>List</code> of <code>JUMPConnectionInfo</code>
         *  into a string.
         *
         * @param connections list to convert
         * @return string with all connections
         *
         * <p>TBD: unittests</p>
         */
        static String listToString(final List connections) {
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
        static Vector stringToVector(final String string) {
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
        private static Vector splitString(final String string) {
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
    }
}

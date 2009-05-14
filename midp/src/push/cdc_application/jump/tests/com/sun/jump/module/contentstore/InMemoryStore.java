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
package com.sun.jump.module.contentstore;

import java.io.IOException;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Vector;

/** Simple in-memory store. */
public final class InMemoryStore extends JUMPStore {

    /** In memory version of content store Node. */
    private abstract static class Node implements JUMPNode {
        /** Node uri. */
        private final String uri;
        /**  Node name. */
        private final String name;
        /** <code>true</code> if is a data node. */
        private final boolean isDataNode;

        /**
         * Creats a node.
         *
         * @param uri node's uri
         * @param name node's name
         * @param isDataNode <code>true</code> if is a data node
         */
        protected  Node(final String uri, final String name,
                final boolean isDataNode) {
            this.uri = uri;
            this.name = name;
            this.isDataNode = isDataNode;
        }

        /**
         * Gets node's uri.
         *
         * @return node's uri
         */
        public String getURI() {
            return uri;
        }

        /**
         * Gets node's name.
         *
         * @return node's name
         */
        public String getName() {
            return name;
        }

        /**
         * Returns <code>true</code> if this node contains data.
         *
         * @return <code>true</code> if this node contains data.
         */
        public boolean containsData() {
            return isDataNode;
        }
    }

    /** In memory version of <code>JUMPNode.List</code>. */
    private static final class ListNode extends Node implements JUMPNode.List {
        /** Node's children. */
        private final HashMap children;

        /**
         * Creates a list node.
         *
         * @param uri node's uri
         * @param name node's name
         */
        private ListNode(final String uri, final String name) {
            super(uri, name, false);
            children = new HashMap();
        }

        /**
         * Iterates through node's children.
         *
         * @return children iterator
         */
        public Iterator getChildren() {
            return children.values().iterator();
        }

        /**
         * Fetches a node by the name.
         *
         * @param name child node name
         * @return child node if any
         */
        private Node get(final String name) {
            return (Node) children.get(name);
        }

        /**
         * Adds a child node.
         *
         * @param name child's name
         * @param node child node
         */
        private void put(final String name, final Node node) {
            children.put(name, node);
        }

        /**
         * Removes a child node.
         *
         * @param name child's name
         */
        private void remove(final String name) {
            children.remove(name);
        }
    }

    /** In memory version of <code>JUMPNode.Data</code>. */
    private static final class DataNode extends Node implements JUMPNode.Data {
        /** Node's data. */
        private final JUMPData data;

        /**
         * Creates a data node.
         *
         * @param uri node's uri
         * @param name node's name
         * @param data node's data
         */
        private DataNode(final String uri, final String name,
                final JUMPData data) {
            super(uri, name, true);
            this.data = data;
        }

        /**
         * Fetches node's data.
         *
         * @return data
         */
        public JUMPData getData() {
            return data;
        }
    }

    /** Store root. */
    private final ListNode root = new ListNode("", "");

    /**
     * Creates an in memory store.
     *
     * @throws IOException if it's impossible to create a store
     */
    public InMemoryStore() throws IOException {
        createNode(".");
    }

    /**
     * Loads this module.
     *
     * @param map parameters
     */
    public void load(final Map map) { }

    /**
     * Unloads this module.
     */
    public void unload() { }

    /**
     * Creates a data node.
     *
     * @param uri node's uri
     * @param data node's data
     *
     * @throws IOException if creation failed
     */
    public void createDataNode(final String uri, final JUMPData data)
            throws IOException {
        process(uri, new Processor() {
           public Node process(final ListNode location, final String name)
                    throws IOException {
               if (location.get(name) != null) { // If a node already exists
                   reportURIProblem(uri, "node already exists");
               }
               location.put(name, new DataNode(uri, name, data));
               return null;
           }
        });
    }

    /**
     * Creates a list node.
     *
     * @param uri list node uri
     *
     * @throws IOException if operation fails
     */
    public void createNode(final String uri) throws IOException {
        final String [] path = splitUri(uri);
        ListNode location = root;
        for (int i = 0; i < path.length - 1; i++) {
            final Node node = location.get(path[i]);
            if (node == null) {
                if (i == 0) {
                    throw new IllegalStateException("No root (.)");
                }
                // Should create a rest of it manually
                // Assemble new URI as we go
                StringBuffer newUri = new StringBuffer(path[0]); // MUST be "."
                for (int j = 1; j < i; j++) {
                    newUri.append(URI_SEPARATOR);
                    newUri.append(path[j]);
                }
                for (int j = i; j < path.length; j++) {
                    final String name = path[j];
                    newUri.append(URI_SEPARATOR);
                    newUri.append(name);
                    ListNode listNode = new ListNode(new String(newUri), name);
                    location.put(name, listNode);
                    location = listNode;
                }
                return;
            }

            if (!(node instanceof ListNode)) {
                reportURIProblem(uri, "no such node");
            }
            location = (ListNode) node;
        }

        final String name = path[path.length - 1];
        if (location.get(name) != null) {
            // Node already exists
            reportURIProblem(uri, "node already exists");
        }
        location.put(name, new ListNode(uri, name));
    }

    /**
     * Removes a node.
     *
     * @param uri node's uri
     *
     * @throws IOException if operation fails
     */
    public void deleteNode(final String uri) throws IOException {
        process(uri, new Processor() {
           public Node process(final ListNode location, final String name) {
               location.remove(name);
               return null;
           }
        });
    }

    /**
     * Fetches a node.
     *
     * @param uri node's uri
     *
     * @return node
     *
     * @throws IOException if operation fails
     */
    public JUMPNode getNode(final String uri)
            throws IOException {
        return process(uri, new Processor() {
            public Node process(final ListNode location, final String name) {
                return location.get(name);
            }
        });
    }

    /**
     * Updates a node with new data.
     *
     * @param uri node's uri
     * @param data new data
     *
     * @throws IOException if operation fails
     */
    public void updateDataNode(final String uri, final JUMPData data)
            throws IOException {
        process(uri, new Processor() {
            public Node process(final ListNode location, final String name)
                    throws IOException {
                // When updating, a node should already exist
                final Node node = location.get(name);
                if ((node == null) || !(node instanceof DataNode)) {
                    reportURIProblem(uri, "no such node");
                }
                location.put(name, new DataNode(uri, name, data));
                return null;
            }
        });
    }

    /** Internal node processor interface. */
    private static interface Processor {
        /**
         * Processes a node.
         *
         * @param location parent node
         * @param name node name
         *
         * @return new node
         *
         * @throws IOException if processing fails
         */
        Node process(ListNode location, String name) throws IOException;
    }

    /**
     * Performs an operation.
     *
     * @param uri uri to perform operation at
     * @param processor processor which performs an operation
     *
     * @return new node
     *
     * @throws IOException if the operation fails
     */
    private Node process(final String uri, final Processor processor)
            throws IOException {
        final String [] path = splitUri(uri);
        ListNode location = root;
        for (int i = 0; i < path.length - 1; i++) {
            final Node node = location.get(path[i]);
            if ((node == null) || !(node instanceof ListNode)) {
                reportURIProblem(uri, "no such node");
            }
            location = (ListNode) node;
        }
        return processor.process(location, path[path.length - 1]);
    }

    /** Path elements separator. */
    private static final char URI_SEPARATOR = '/';

    /**
     * Validates a path.
     *
     * @param path path to validate
     * @param uri original uri
     */
    private static void validateElements(
            final String [] path,
            final String uri) {
        // TBD: more validation
        if (path.length == 0) {
            reportURIProblem(uri, "no elements in the URI");
        }
        if (!path[0].equals(".")) {
            reportURIProblem(uri, "1st element must be '.'");
        }
        for (int i = 1; i < path.length; i++) {
            final String e = path[i];
            if (e.length() == 0) {
                reportURIProblem(uri, "empty elements are not allowed");
            }
            if (e.equals(".")) {
                reportURIProblem(uri, "'.' is only allowed as 1st element");
            }
        }
    }

    /**
     * Reports a problematic uri.
     *
     * @param uri problematic uri
     * @param message message
     */
    private static void reportURIProblem(
            final String uri,
            final String message) {
        throw new JUMPStoreRuntimeException(message + " [uri = " + uri + "]");
    }

    /**
     * Splits an URI into path elements.
     *
     * @param uri <code>URI</code> to split
     *
     * @return array of path elements
     */
    private static String [] splitUri(final String uri) {
        // TBD: reuse splitting
        final Vector elements = new Vector();
        int pos = 0;
        while (true) {
            final int p = uri.indexOf(URI_SEPARATOR, pos);
            if (p == -1) {
                elements.add(uri.substring(pos));
                final String [] path = new String[elements.size()];
                elements.toArray(path);
                validateElements(path, uri);
                return path;
            }
            elements.add(uri.substring(pos, p));
            pos = p + 1;
        }
    }
}

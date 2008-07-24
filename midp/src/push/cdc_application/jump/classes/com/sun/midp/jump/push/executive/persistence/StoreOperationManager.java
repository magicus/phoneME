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
import com.sun.jump.module.contentstore.JUMPStoreHandle;
import java.io.IOException;

/**
 * Abstracts transaction-like operations on the content
 * store which need store handle.
 */
public final class StoreOperationManager {
    /**
     * Abstracts an operation that should be perfomed with
     *  exclusive lock on the content store.
     */
    public interface Operation {
        /**
         * Performs an operation with exclusive lock on the content store.
         *
         * Implementations shouldn't keep the <code>storeHandle</code>
         *  passed out of <code>perform</code> method: the very moment
         *  <code>perform</code> quits, store handle gets invalidated.
         *
         * @param storeHandle exclusively locked store handle
         *
         * @return abstract result of operation
         *
         * @throws IOException if IO fails
         */
        Object perform(JUMPStoreHandle storeHandle) throws IOException;
    }

    /**
     * Adapter to <code>JUMPContentStore</code>.
     *
     * Actually, this adapter just grants necessary visibility to the methods.
     */
    public interface ContentStore {
        /**
         * Opens the content store in exclusive mode.
         *
         * @param accessExclusive controls exclusiveness of access
         *
         * @return store handle
         */
        JUMPStoreHandle open(boolean accessExclusive);

        /**
         * Closes a handle.
         *
         * @param storeHandle handle to close
         */
        void close(JUMPStoreHandle storeHandle);
    }

    /** Content store adapter to operate on. */
    private final ContentStore contentStore;

    /**
     * Constructs an instance of manager.
     *
     * @param contentStore content store to use
     */
    public StoreOperationManager(final ContentStore contentStore) {
        this.contentStore = contentStore;
    }

    /**
     * Performs an operation on the content store keeping an exclsuive lock.
     *
     * Whatever outcome of the operation is, the exclusive lock is released.
     *
     * @param operation operation to perform
     * @param accessExclusive controls exclusiveness of access
     *
     * @return abstract result of the operation
     *
     * @throws IOException if IO fails
     */
    public Object doOperation(
             final boolean accessExclusive, final Operation operation)
                throws IOException {
        final JUMPStoreHandle storeHandle = contentStore.open(accessExclusive);
        try {
            return operation.perform(storeHandle);
        } finally {
            contentStore.close(storeHandle);
        }
    }

    /**
     * Gets a node.
     *
     * This method is just a wrapper around the corresponding content store API.
     *
     * @param uri URI of the node to get
     *
     * @return reference to a node
     *
     * @throws IOException if IO fails
     */
    public JUMPNode getNode(final String uri) throws IOException {
        return (JUMPNode) doOperation(false,
            new Operation() {
                public Object perform(final JUMPStoreHandle storeHandle)
                        throws IOException {
                    return storeHandle.getNode(uri);
                }
        });
    }

    /**
     * Safely updates a data node.
     *
     * Safely update means that node is either updated (if it exists already)
     *  or created afresh.
     *
     * The exclusive lock is obtained and held while operation is running and
     *  thus the content store should be in unlocked state.
     *
     * @param uri URI of the node to get
     * @param data data to put
     *
     * @throws IOException if IO fails
     */
    public void safelyUpdateDataNode(final String uri, final JUMPData data)
            throws IOException {
        doOperation(true,
            new Operation() {
                public Object perform(final JUMPStoreHandle storeHandle)
                        throws IOException {
                    if (doesNodeExist(storeHandle, uri)) {
                        storeHandle.updateDataNode(uri, data);
                    } else {
                        storeHandle.createDataNode(uri, data);
                    }
                    return null;
                }
        });
    }

    /**
     * Safely deletes a data node.
     *
     * Safely deletion means that node is deleted if it is present.
     *
     * The exclusive lock is obtained and held while operation is running and
     *  thus the content store should be in unlocked state.
     *
     * @param uri URI of the node to delete
     *
     * @throws IOException if IO fails
     */
    public void safelyDeleteDataNode(final String uri)
            throws IOException {
        doOperation(true,
            new Operation() {
                public Object perform(final JUMPStoreHandle storeHandle)
                        throws IOException {
                    if (doesNodeExist(storeHandle, uri)) {
                        storeHandle.deleteNode(uri);
                    }
                    return null;
                }
        });
    }

    /**
     * Checks if the data node with the given name already exists.
     *
     * @param storeHandle content store handle
     * @param uri node's URI
     *
     * @return <code>true</code> iff the node exists
     *
     * @throws IOException if IO fails.
     */
    private static boolean doesNodeExist(
            final JUMPStoreHandle storeHandle, final String uri)
                throws IOException {
        final JUMPNode.Data node = (JUMPNode.Data) storeHandle.getNode(uri);
        return (node != null);
    }
}

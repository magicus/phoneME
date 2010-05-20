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

import com.sun.midp.jump.push.executive.persistence.StoreOperationManager;
import java.io.IOException;
import java.util.Map;

/** In memory version of <code>JUMPContentStore</code>. */
public final class InMemoryContentStore
        extends JUMPContentStore
        implements StoreOperationManager.ContentStore {
    /** Store. */
    private final InMemoryStore store;

    /**
     * Creates a content store.
     *
     * @throws IOException if operation fails
     */
    public InMemoryContentStore() throws IOException {
        store = new InMemoryStore();
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
     * Fetches the store.
     *
     * @return store
     */
    protected JUMPStore getStore() {
        return store;
    }

    /**
     * Opens content store.
     *
     * @param accessExclusive access type
     *
     * @return store handle
     */
    public JUMPStoreHandle openStore(final boolean accessExclusive) {
        return new JUMPStoreHandle(getStore(), accessExclusive);
    }

    /**
     * Opens content store.
     *
     * As requested by #StoreOperationManager.ContentStore interface
     *
     * @param accessExclusive access type
     *
     * @return store handle
     *
     * @thorws IOException if IO fails
     */
    public JUMPStoreHandle open(final boolean accessExclusive) {
        return openStore(accessExclusive);
    }

    /**
     * Closes opened content store handle.
     *
     * As requested by #StoreOperationManager.ContentStore interface
     *
     * @param storeHandle handle to close
     */
    public void close(final JUMPStoreHandle storeHandle) {
        closeStore(storeHandle);
    }

    /**
     * Populates a store handle with some prebuild dirs.
     *
     * @param storeHandle handle to the store to operate on
     * @param dirs directories to create
     *
     * @throws IOException if IO failed
     */
    public static void initStore(
            final JUMPStoreHandle storeHandle, final String [] dirs)
                throws IOException {
        for (int i = 0; i < dirs.length; i++) {
            storeHandle.createNode(dirs[i]);
        }
    }
}

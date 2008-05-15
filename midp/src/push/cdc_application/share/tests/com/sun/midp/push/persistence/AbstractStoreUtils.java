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

package com.sun.midp.push.persistence;

import java.io.IOException;

/**
 * Provides functionality needed for unittesting.
 *
 * <p>
 * Each stack MUST define
 * <code>com.sun.midp.push.persistence.StoreUtils</code> class
 * which extends this class.
 * </p>
 */
public abstract class AbstractStoreUtils {
    /**
     * Manages refreshable stores.
     *
     * <p>
     * Some <code>Store</code> implementations might read all the data
     * at some desugnated time, e.g. construction.  Therefore to allow testing
     * correct reading of data from underlying persistent store one might use
     * this interface.
     */
    public static interface Refresher {
        /**
         * Gets a refreshed store.
         * 
         * <p> 
         * Returns a store maximally close to one freshly read from
         * the actual persistent storage.  However, might return the same
         * instance of the store on different invocations.
         * </p>
         * 
         * @return store to use
         * 
         * @throws IOException if creation of store failed
         */
        Store getStore() throws IOException;
    }

    /**
     * Gets a refresher.
     * 
     * @return an instance of <code>Refresher</code>
     * 
     * @throws IOException if creation of refresher failed
     */
    public abstract Refresher getRefresher() throws IOException;


    /**
     * Gets an unique store.
     * 
     * @return store to use
     * 
     * @throws IOException if creation of store failed
     */
    public final Store getStore() throws IOException {
        return getRefresher().getStore();
    }

    /** Lazy singleton utility class. */
    private static class Singleton {
        /** Singleton instance. */
        static final AbstractStoreUtils STORE_UTILS = new StoreUtils();
    }

    /**
     * Implements singleton pattern.
     *
     * @return an instance of store utils
     */
    public static AbstractStoreUtils getInstance() {
        return Singleton.STORE_UTILS;
    }
}

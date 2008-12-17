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

import com.sun.midp.push.persistence.Store;
import com.sun.midp.push.persistence.AbstractStoreUtils;

import com.sun.jump.module.contentstore.InMemoryContentStore;
import com.sun.jump.module.contentstore.JUMPStoreHandle;
import java.io.IOException;

/** Jump variant of <code>StoreUtils</code>. */
public final class StoreUtils extends AbstractStoreUtils {
    /** Hides a constructor. */
    protected StoreUtils() { }

    /** {@inheritDoc} */
    public static StoreOperationManager createInMemoryManager(
            final String [] dirs) throws IOException {
        final StoreOperationManager storeManager =
                new StoreOperationManager(new InMemoryContentStore());
        storeManager.doOperation(true, new StoreOperationManager.Operation() {
            public Object perform(final JUMPStoreHandle storeHandle)
                    throws IOException {
                InMemoryContentStore.initStore(storeHandle, dirs);
                return null;
            }
        });
        return storeManager;
    }

    /** Jump-specific impl of <code>Refresher</code>. */
    private static class MyRefresher implements Refresher {
        /** Content-store dirs. */
        private static final String [] DIRS = {
                JUMPStoreImpl.CONNECTIONS_DIR, JUMPStoreImpl.ALARMS_DIR
        };

        /** <code>StoreOperationManager</code> to use. */
        private final StoreOperationManager som;

        /**
         * Default ctor.
         *
         * @throws IOException if creation fails
         */
        MyRefresher() throws IOException {
            som = createInMemoryManager(DIRS);
        }

        /** {@inheritDoc} */
        public Store getStore() throws IOException {
            return new JUMPStoreImpl(som);
        }
    }

    /** {@inheritDoc} */
    public Refresher getRefresher() throws IOException {
        return new MyRefresher();
    }
}

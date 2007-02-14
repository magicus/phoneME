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

package com.sun.jumpimpl.module.pushregistry;

import com.sun.jump.module.JUMPModule;
import com.sun.jump.module.JUMPModuleFactory;
import com.sun.jump.module.contentstore.JUMPContentStore;
import com.sun.jump.module.contentstore.JUMPStore;
import com.sun.jump.module.contentstore.JUMPStoreHandle;
import com.sun.jump.module.contentstore.JUMPStoreFactory;
import com.sun.jumpimpl.module.pushregistry.MIDP.MIDPPushRegistry;
import com.sun.jumpimpl.module.pushregistry.persistence.Store;
import com.sun.jumpimpl.module.pushregistry.persistence.StoreOperationManager;
import java.io.IOException;
import java.util.Map;

final class PushSystem {
    /** Push-specific view of persistent store. */
    final Store store;

    /** MIDP container service provider instance. */
    final MIDPPushRegistry midpPushRegistry;

    PushSystem(final StoreOperationManager storeManager) throws IOException {
        store = new Store(storeManager);
        store.ensureStoreStructure(storeManager);
        store.readData();

        midpPushRegistry = new MIDPPushRegistry(store);
        // TBD: register midpPushRegistry IXC
        // TBD: installation interface
    }

    void close() {
    }
}

final class PushContentStore
        extends JUMPContentStore
        implements StoreOperationManager.ContentStore {

    protected JUMPStore getStore() {
        return JUMPStoreFactory.getInstance().getModule(JUMPStoreFactory.TYPE_FILE);
    }

    public JUMPStoreHandle open(final boolean accessExclusive) {
        return openStore(accessExclusive);
    }

    public void close(final JUMPStoreHandle storeHandle) {
        closeStore(storeHandle);
    }

    public void load(final Map map) {
    }

    public void unload() {
    }
}

final class PushModule implements JUMPModule {

    private PushSystem pushSystem = null;

    public void load(final Map map) {
        // assert pushSystem == null; // I hope we cannot get to load's without unload
        final PushContentStore contentStore = new PushContentStore();
        final StoreOperationManager storeOperationManager =
                new StoreOperationManager(contentStore);
        try {
            pushSystem = new PushSystem(storeOperationManager);
        } catch (IOException ioex) {
            // TBD: common logging mechanism
            System.err.println("Failed to read push persistent data: " + ioex);
        }
    }

    public void unload() {
        pushSystem.close();
        pushSystem = null;
    }
}

/**
 * Factory to initialize Push subsystem in the Executive.
 *
 * IMPL_NOTE: as currently PushRegistry module has no public
 *  API the factory is used for initialization only.
 */
public class PushRegistryModuleFactoryImpl extends JUMPModuleFactory {
    PushModule pushModule = new PushModule();

    public void load(final Map map) {
        pushModule.load(map);
    }

    public void unload() {
        pushModule.unload();
    }
}

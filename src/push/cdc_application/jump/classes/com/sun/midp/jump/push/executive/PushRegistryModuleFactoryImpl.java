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

package com.sun.midp.jump.push.executive;

import com.sun.jump.common.JUMPAppModel;
import com.sun.jump.common.JUMPApplication;
import com.sun.jump.module.contentstore.JUMPContentStore;
import com.sun.jump.module.contentstore.JUMPStore;
import com.sun.jump.module.contentstore.JUMPStoreFactory;
import com.sun.jump.module.contentstore.JUMPStoreHandle;
import com.sun.jump.module.installer.JUMPInstallerModuleFactory;
import com.sun.jump.module.lifecycle.JUMPApplicationLifecycleModule;
import com.sun.jump.module.lifecycle.JUMPApplicationLifecycleModuleFactory;
import com.sun.jump.module.serviceregistry.JUMPServiceRegistryModule;
import com.sun.jump.module.serviceregistry.JUMPServiceRegistryModuleFactory;
import com.sun.midp.jump.installer.MIDLETInstallerImpl;
import com.sun.midp.push.ota.InstallerInterface;
import com.sun.midp.push.persistence.Store;
import com.sun.midp.push.controller.PushController;
import com.sun.midp.push.controller.LifecycleAdapter;
import com.sun.midp.push.controller.InstallerInterfaceImpl;
import com.sun.midp.jump.push.executive.persistence.JUMPStoreImpl;
import com.sun.midp.jump.push.executive.persistence.StoreOperationManager;
import com.sun.midp.jump.push.share.Configuration;
import com.sun.midp.push.reservation.impl.ReservationDescriptorFactory;
import java.io.IOException;
import java.rmi.AccessException;
import java.rmi.AlreadyBoundException;
import java.rmi.NotBoundException;
import java.rmi.RemoteException;
import java.util.Map;

final class PushContentStore
        extends JUMPContentStore
        implements StoreOperationManager.ContentStore {

    protected JUMPStore getStore() {
        return JUMPStoreFactory.getInstance()
	    .getModule(JUMPStoreFactory.TYPE_FILE);
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

final class PushModule implements JUMPPushModule {

    static final class PushSystem {
        /** Installer-to-Push interface impl. */
        private final InstallerInterface installerInterfaceImpl;

        /** Push controller instance. */
        private final PushController pushController;

        PushSystem(final StoreOperationManager storeManager)
                throws IOException, RemoteException, AlreadyBoundException {
            final Store store = new JUMPStoreImpl(storeManager);

            final ReservationDescriptorFactory reservationDescriptorFactory =
                    Configuration.getReservationDescriptorFactory();

            pushController = new PushController(
                    store,
                    createLifecycleAdapter(),
                    reservationDescriptorFactory);

            final MIDPContainerInterfaceImpl midpContainerInterfaceImpl =
                    new MIDPContainerInterfaceImpl(pushController);
            getJUMPServiceRegistry().registerService(
                    Configuration.MIDP_CONTAINER_INTERFACE_IXC_URI,
                    midpContainerInterfaceImpl);

            installerInterfaceImpl = new InstallerInterfaceImpl(pushController,
                    reservationDescriptorFactory);
        }

        /**
         * Creates a lifecycle adapter for Push system.
         *
         * @return an instance of LifecycleAdapter (cannot be <code>null</code>)
         */
        private LifecycleAdapter createLifecycleAdapter() {
            /*
             * IMPL_NOTE: not an ideal solution as it introduces
             * implementation dependencies.  However pragmatically good enough
             */
            final MIDLETInstallerImpl midletInstaller = (MIDLETInstallerImpl)
                        JUMPInstallerModuleFactory
                            .getInstance()
                            .getModule(JUMPAppModel.MIDLET);
            if (midletInstaller == null) {
                throw new RuntimeException("failed to obtain midlet installer");
            }

            final String policy = JUMPApplicationLifecycleModuleFactory
                    .POLICY_ONE_LIVE_INSTANCE_ONLY;

            final JUMPApplicationLifecycleModule lifecycleModule =
                    JUMPApplicationLifecycleModuleFactory
                        .getInstance()
                        .getModule(policy);
            if (lifecycleModule == null) {
                throw new RuntimeException("failed to obtain lifecycle module");
            }

            return new LifecycleAdapter() {
                public void launchMidlet(final int midletSuiteID,
                        final String midlet) {
                    final JUMPApplication app = midletInstaller
                            .getMIDletApplication(midletSuiteID, midlet);
                    if (app == null) {
                        logError("failed to convert to MIDP application");
                    }

                    // TBD: push-launch dialogs

                    lifecycleModule.launchApplication(app, new String [0]);
                }
            };
        }

        /**
         * Gets a reference to service registry for IXC.
         *
         * @return <code>JUMPServiceRegistryModule</code> instance
         */
        private JUMPServiceRegistryModule getJUMPServiceRegistry() {
            return JUMPServiceRegistryModuleFactory
                    .getInstance()
                    .getModule();
        }

        void close() {
            // Best effort service unregistration...
            try {
                getJUMPServiceRegistry().unregisterService(
                        Configuration.MIDP_CONTAINER_INTERFACE_IXC_URI);
            } catch (AccessException ex) {
                logError("failed to unbound IXC MIDP interface");
            } catch (NotBoundException ex) {
                logError("failed to unbound IXC MIDP interface");
            }

            pushController.dispose();
        }
    }

    private PushSystem pushSystem = null;

    /** {@inheritDoc} */
    public void load(final Map map) {
        // assert pushSystem == null;
	// I hope we cannot get to load's without unload
        final PushContentStore contentStore = new PushContentStore();
        final StoreOperationManager storeOperationManager =
                new StoreOperationManager(contentStore);
        try {
            pushSystem = new PushSystem(storeOperationManager);
        } catch (AlreadyBoundException ex) {
            logError("Failed to read push persistent data: " + ex);
            throw new RuntimeException("Failed to load Push module", ex);
        } catch (IOException ex) {
            logError("Failed to read push persistent data: " + ex);
            throw new RuntimeException("Failed to load Push module", ex);
        }
    }

    /** {@inheritDoc} */
    public void unload() {
        pushSystem.close();
        pushSystem = null;
    }

    /**
     * Logs an error message.
     *
     * TBD: common logging mechanism.
     *
     * @param msg message to log
     */
    private static void logError(final String msg) {
        System.err.println("[error in " + PushModule.class + "]: " + msg);
    }

    /** {@inheritDoc} */
    public InstallerInterface getInstallerInterfaceImpl() {
        return pushSystem.installerInterfaceImpl;
    }
}

/**
 * Factory to initialize Push subsystem in the Executive.
 *
 * IMPL_NOTE: as currently PushRegistry module has no public
 *  API the factory is used for initialization only.
 */
public final class PushRegistryModuleFactoryImpl extends JUMPPushModuleFactory {
    private final JUMPPushModule pushModule = new PushModule();

    /** {@inheritDoc} */
    public void load(final Map map) {
        pushModule.load(map);
    }

    /** {@inheritDoc} */
    public void unload() {
        pushModule.unload();
    }

    /** {@inheritDoc} */
    public JUMPPushModule getPushModule() {
        return pushModule;
    }
}

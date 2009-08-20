/*
 *   
 *
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

package com.sun.midp.amsservices;

import com.sun.midp.installer.DynamicComponentInstaller;

import com.sun.midp.midlet.MIDletStateHandler;
import com.sun.midp.midlet.MIDletSuite;

import com.sun.midp.midletsuite.MIDletSuiteLockedException;
import com.sun.midp.midletsuite.DynamicComponentStorage;

import com.sun.midp.configurator.Constants;
import com.sun.midp.events.EventTypes;
import com.sun.midp.events.EventQueue;
import com.sun.midp.events.NativeEvent;
import com.sun.midp.main.MIDletSuiteUtils;
import com.sun.cldchi.jvm.JVM;

import java.io.IOException;

/**
 * Implementation of the AMSServices interface providing some AMS services.
 */
final public class AMSServicesImpl implements AMSServices {
    /**
     * Creates a new instance of AMSServicesImpl class.
     *
     * @throws SecurityException if the calling midlet has no rights to
     *                           access this API 
     */
    public AMSServicesImpl() throws SecurityException {
        MIDletSuite currSuite =
                MIDletStateHandler.getMidletStateHandler().getMIDletSuite();

        /*
         * Even if the suite is trusted, it doesn't guarantee that it will
         * be able to use this API. Later the installer checks that
         * Permissions.AMS_PERMISSION_NAME permission is allowed for the
         * calling suite. It is true when both following conditions are met:
         *
         * - the calling midlet belong to a suite from "manufacturer" domain;
         * - this suite asked for "com.sun.midp.ams" permission through
         *   MIDlet-Permissions atribute given in its descriptor file.
         */
        if (!currSuite.isTrusted()) {
            throw new SecurityException(
                    "MIDlet is not authorized to access this API.");
        }
    }

    /**
     * Installs the component pointed by the given URL.
     *
     * @param url HTTP URL pointing to the application descriptor
     *            or to the jar file of the component that must
     *            be installed
     * @param name user-friendly name of the component

     * @return unique component identifier; currently is not used as far
     *         as the calling midlet is restarted
     *
     * @throws IOException if the installation failed
     * @throws SecurityException if the caller does not have permission
     *         to install components
     */
    public int installComponent(String url, String name)
            throws IOException, SecurityException {
        int componentId;
        DynamicComponentInstaller installer = new DynamicComponentInstaller();
        MIDletStateHandler msh = MIDletStateHandler.getMidletStateHandler();
        MIDletSuite currSuite = msh.getMIDletSuite();

        try {
            // unlock the component if it exists and belongs to this suite
            JVM.flushJarCaches();
            componentId = installer.installComponent(currSuite.getID(),
                                                     url, name);
        } catch (MIDletSuiteLockedException msle) {
            throw new IOException("The component is being used now.");
        }

        // restarting the calling midlet
        
        EventQueue eq = EventQueue.getEventQueue();
        NativeEvent event = new NativeEvent(EventTypes.RESTART_MIDLET_EVENT);

        event.intParam1    = 0; // external application ID: unused in JAMS
        event.intParam2    = currSuite.getID();
        event.stringParam1 = msh.getFirstRunningMidlet();
        event.stringParam2 = name;

        eq.sendNativeEventToIsolate(event, MIDletSuiteUtils.getAmsIsolateId());

        return componentId;
    }

    /**
     * Removes the specified component belonging to the calling midlet.
     *
     * @param componentId ID of the component to remove
     *
     * @throws IllegalArgumentException if the component with the given ID
     *                                  does not exist
     * @throws IOException if the component is used now and can't be removed, or
     *                     other I/O error occured when removing the component
     * @throws SecurityException if the component with the given ID doesn't
     *                           belong to the calling midlet suite
     */
    public void removeComponent(int componentId)
            throws IllegalArgumentException, IOException, SecurityException {
        DynamicComponentStorage dcs =
                DynamicComponentStorage.getComponentStorage();
        try {
            // unlock the component if it exists and belongs to this suite
            JVM.flushJarCaches();
            dcs.removeComponent(componentId);
        } catch (MIDletSuiteLockedException msle) {
            throw new IOException("Component is in use: " + msle.getMessage());
        }
    }

    /**
     * Removes all installed components belonging to the calling midlet.
     * 
     * @throws IllegalArgumentException if there is no suite with
     *                                  the specified ID
     * @throws IOException is thrown, if any component is locked
     * @throws SecurityException if the calling midlet suite has no rights
     *                           to access this API
     */
    public void removeAllComponents()
            throws IllegalArgumentException, IOException, SecurityException {
        MIDletStateHandler msh = MIDletStateHandler.getMidletStateHandler();
        MIDletSuite currSuite = msh.getMIDletSuite();
        DynamicComponentStorage dcs =
                DynamicComponentStorage.getComponentStorage();

        try {
            // unlock the component if it exists and belongs to this suite
            JVM.flushJarCaches();
            dcs.removeAllComponents(currSuite.getID());
        } catch (MIDletSuiteLockedException msle) {
            throw new IOException("One or more components are in use: "
                    + msle.getMessage());
        }
    }

    /**
     * Returns description of the components belonging to the calling midlet.
     *
     * @return an array of classes describing the components belonging to
     *         the calling midlet, an empty array if there are no such
     *         components
     *
     * @throws SecurityException if the calling midlet suite has no rights
     *                           to access this API
     * @throws IOException if an the information cannot be read
     */
    public ComponentInfo[] getAllComponentsInfo()
            throws SecurityException, IOException {
        MIDletSuite currSuite =
                MIDletStateHandler.getMidletStateHandler().getMIDletSuite();
        DynamicComponentStorage dcs =
                DynamicComponentStorage.getComponentStorage();

        ComponentInfo[] ci;

        try {
            ci = dcs.getListOfSuiteComponents(currSuite.getID());
        } catch (IllegalArgumentException iae) {
            iae.printStackTrace();
            ci = null;
        }

        return (ci != null ? ci : new ComponentInfo[0]);
    }

    /**
     * Retrieves information about the component having the given ID belonging
     * to the calling suite.
     *
     * @param componentId ID of the component
     *
     * @return a class describing the component with the given ID
     *
     * @throws IllegalArgumentException if the component with the given ID
     *                                  does not exist
     * @throws SecurityException if the component with the given ID doesn't
     *                           belong to the calling midlet suite
     * @throws IOException if an the information cannot be read
     */
    public ComponentInfo getComponentInfo(int componentId)
            throws IllegalArgumentException, SecurityException, IOException {
        DynamicComponentStorage dcs =
                DynamicComponentStorage.getComponentStorage();

        ComponentInfo ci = new ComponentInfoImpl();
        dcs.getComponentInfo(componentId, ci);

        return ci;
    }
}

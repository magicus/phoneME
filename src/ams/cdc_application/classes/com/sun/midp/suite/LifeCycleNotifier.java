/*
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.suite;

import com.sun.midp.installer.InvalidJadException;
import com.sun.midp.midlet.MIDletSuite;
import com.sun.midp.util.Properties;

/**
 * SPI interface to be implemented by classes which want to receive lifecycle
 * notifications.
 */
public interface LifeCycleNotifier {

    /**
     * Adds a listener to the list of listeners to be notified.
     * 
     * @param listener the listener to be notified.
     */
    public void addListener(LifeCycleListener lifecycleListener);

    /**
     * Removes the listener from the list of listeners to be notified.
     * 
     * @param listener the listener to be notified.
     */
    public void removeListener(LifeCycleListener lifecycleListener);

    /**
     * Called when a MIDlet is to be destroyed.
     *
     * @param suite the MIDlet suite to be destroyed.
     * @param className the class name of the MIDlet to destroy.
     */
    public void midletDestroyed(MIDletSuite midletSuite, String className);

    /**
     * Called when a MIDlet is to be paused.
     *
     * @param suite the MIDlet suite to be paused.
     * @param className the class name of the MIDlet to pause.
     */
    public void midletPaused(MIDletSuite midletSuite, String className);

    /**
     * Called when a MIDlet is to be resumed.
     *
     * @param suite the MIDlet suite to be resumed.
     * @param className the class name of the MIDlet to resume.
     */
    public void midletResumed(MIDletSuite midletSuite, String className);

    /**
     * Called when a MIDlet is to be started.
     *
     * @param suite the MIDlet suite to be started.
     * @param className the class name of the MIDlet to start.
     */
    public void midletToBeStarted(MIDletSuite midletSuite, String className);

    /**
     * Called when a MIDlet JAR is to be installed.
     *
     * @param id the MIDlet suite ID of the of the suite to be installed.
     * @param jarPathName the path name to the MIDlet JAR.
     * @param appProps the application properties.
     * @throws InvalidJadException if there is an error parsing the JAD.
     */
    public void toBeInstalled(int id, String jarPathname, Properties appProps)
	throws InvalidJadException;

    /**
     * Called when a MIDlet JAR is to be un-installed.
     *
     * @param id the MIDlet suite ID of the of the suite to be uninstalled.
     * @param jarPathName the path name to the MIDlet JAR.
     * @param appProps the application properties.
     * @throws InvalidJadException if there is an error parsing the JAD.
     */
    public void toBeUninstalled(int id, String jarPathname,
				Properties appProps);
}

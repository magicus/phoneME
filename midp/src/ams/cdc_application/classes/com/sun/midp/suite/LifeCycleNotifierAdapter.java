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

import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;

import com.sun.midp.midlet.MIDletSuite;

/**
 * Manages a simple vector of lifecycle listeners.
 */
public class LifeCycleNotifierAdapter implements LifeCycleNotifier,
                                                 LifeCycleListener {
    /**
     * Vector of LifeCycleListener's
     */
    protected Vector lifecycleListeners = new Vector();

    /**
     * Called when a MIDlet is to be destroyed.
     *
     * @param suite the MIDlet suite to be destroyed.
     * @param className the class name of the MIDlet to destroy.
     */
    public void midletDestroyed(MIDletSuite suite, String className) {
        Enumeration listenerEnum = lifecycleListeners.elements();

        while (listenerEnum.hasMoreElements()) {
            try {
                LifeCycleListener lifecycleListener = 
                    (LifeCycleListener)listenerEnum.nextElement();
                lifecycleListener.midletDestroyed(suite, className);
            } catch (Throwable t) {
                // move onto next listener
            }
        }
    }

    /**
     * Called when a MIDlet is to be paused.
     *
     * @param suite the MIDlet suite to be paused.
     * @param className the class name of the MIDlet to pause.
     */
    public void midletPaused(MIDletSuite suite, String className) {
        Enumeration listenerEnum = lifecycleListeners.elements();

        while (listenerEnum.hasMoreElements()) {
            try {
                LifeCycleListener lifecycleListener =
                    (LifeCycleListener)listenerEnum.nextElement();
                lifecycleListener.midletPaused(suite, className);
            } catch (Throwable t) {
                // move onto next listener
            }
        }
    }

    /**
     * Called when a MIDlet is to be resumed.
     *
     * @param suite the MIDlet suite to be resumed.
     * @param className the class name of the MIDlet to resume.
     */
    public void midletResumed(MIDletSuite suite, String className) {
        Enumeration listenerEnum = lifecycleListeners.elements();

        while (listenerEnum.hasMoreElements()) {
            try {
                LifeCycleListener lifecycleListener =
                    (LifeCycleListener)listenerEnum.nextElement();
                lifecycleListener.midletResumed(suite, className);
            } catch (Throwable t) {
                // move onto next listener
            }
        }
    }

    /**
     * Called when a MIDlet is to be created and started.
     *
     * @param suite the MIDlet suite to be started
     * @param className the class name of the MIDlet
     */
    public void midletToBeStarted(MIDletSuite suite, String className) {
        Enumeration listenerEnum = lifecycleListeners.elements();

        while (listenerEnum.hasMoreElements()) {
            try {
                LifeCycleListener lifecycleListener =
                    (LifeCycleListener)listenerEnum.nextElement();
                lifecycleListener.midletToBeStarted(suite, className);
            } catch (Throwable t) {
                // move onto next listener
            }
        }
    }

    /**
     * This method must be called when a MIDlet suite is installed.
     *
     * @param suite the MIDlet suite that was installed
     * @param jarPathName the path name to the MIDlet JAR.
     */
    public void installed(MIDletSuite suite, String jarPathname) {
        Enumeration listenerEnum = lifecycleListeners.elements();

        while (listenerEnum.hasMoreElements()) {
            try {
                LifeCycleListener lifecycleListener =
                    (LifeCycleListener)listenerEnum.nextElement();
                lifecycleListener.installed(suite, jarPathname);
            } catch (Throwable t) {
                // move onto next listener
            }
        }
    }

    /**
     * This method must be called when a MIDlet suite is updated.
     *
     * @param newSuite the new MIDlet suite that was installed
     * @param jarPathName the path name to the new MIDlet's JAR
     * @param oldSuite the old version of the MIDlet suite
     * @param keepOldData true if user wants to keep the previous suite's data
     */
    public void updated(MIDletSuite newSuite, String jarPathname,
                        MIDletSuite oldSuite, boolean keepOldData) {
        Enumeration listenerEnum = lifecycleListeners.elements();

        while (listenerEnum.hasMoreElements()) {
            try {
                LifeCycleListener lifecycleListener =
                    (LifeCycleListener)listenerEnum.nextElement();
                lifecycleListener.updated(newSuite, jarPathname,
                                          oldSuite, keepOldData);
            } catch (Throwable t) {
                // move onto next listener
            }
        }
    }

    /**
     * This method must be called just before a MIDlet suite is to be
     * uninstalled.
     *
     * @param suite the MIDlet suite that was installed
     */
    public void toBeUninstalled(MIDletSuite suite) {
        Enumeration listenerEnum = lifecycleListeners.elements();

        while (listenerEnum.hasMoreElements()) {
            try {
                LifeCycleListener lifecycleListener =
                    (LifeCycleListener)listenerEnum.nextElement();
                lifecycleListener.toBeUninstalled(suite);
            } catch (Throwable t) {
                // move onto next listener
            }
        }
    }

    /**
     * Adds a listener to the list of listeners to be notified.
     *
     * @param listener the listener to be notified.
     */
    public void addListener(LifeCycleListener listener) {
        lifecycleListeners.add(listener);
    }

    /**
     * Removes the listener from the list of listeners to be notified.
     * 
     * @param listener the listener to be notified.
     */
    public void removeListener(LifeCycleListener lifecycleListener) {
        lifecycleListeners.remove(lifecycleListener);
    }
}

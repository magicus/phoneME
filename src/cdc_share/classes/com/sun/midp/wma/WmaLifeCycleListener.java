/*
 *
 *  Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
 *  DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License version
 *  2 only, as published by the Free Software Foundation. 
 *  
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  General Public License version 2 for more details (a copy is
 *  included at /legal/license.txt). 
 *  
 *  You should have received a copy of the GNU General Public License
 *  version 2 along with this work; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 *  02110-1301 USA 
 *  
 *  Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 *  Clara, CA 95054 or visit www.sun.com if you need additional
 *  information or have any questions. 
 */

package com.sun.midp.wma;

import com.sun.j2me.proxy.suite.*;
import com.sun.j2me.proxy.midlet.*;
import java.security.*;
import com.sun.midp.io.j2me.ProtocolBase;
import java.util.*;

public class WmaLifeCycleListener implements LifeCycleListener {

    static WmaLifeCycleListener singletoneThis;

    /**
     * Called when a MIDlet is to be destroyed.
     *
     * @param suite the MIDlet suite to be destroyed.
     * @param className the class name of the MIDlet to destroy.
     */
    public void midletDestroyed(MIDletSuite suite, String className) {

        ProtocolBase protocol;
        while ((protocol = getRemoveNextProtocol(suite)) != null) {
            try {
                protocol.close();
            } catch (java.io.IOException ioe) {}
        }
    }

    /**
     * Called when a MIDlet is to be paused.
     *
     * @param suite the MIDlet suite to be paused.
     * @param className the class name of the MIDlet to pause.
     */
    public void midletPaused(MIDletSuite suite, String className) {
        // do nothing
    }

    /**
     * Called when a MIDlet is to be resumed.
     *
     * @param suite the MIDlet suite to be resumed.
     * @param className the class name of the MIDlet to resume.
     */
    public void midletResumed(MIDletSuite suite, String className) {
        // do nothing
    }

    /**
     * Called when a MIDlet is to be started.
     *
     * @param suite the MIDlet suite to be started.
     * @param className the class name of the MIDlet to start.
     */
    public void midletToBeStarted(MIDletSuite suite, String className) {
        // do nothing
    }


    /**
     * Called when a MIDlet suite has just been installed. Listeners should
     * not have a UI and should just set up optional optimation data for
     * a suite, such as image caching.
     * <p>
     * If feature needs user input or can fail because a property is missing
     * or invalid, the suite installer for that AMS it should extended,
     * instead of using this callback.
     *
     * @param suite the MIDlet suite that was installed
     * @param jarPathName the path name to the MIDlet JAR.
     */
    public void installed(MIDletSuite suite, String jarPathname) {
        // do nothing
    }
    
    /**
     * Called when a MIDlet suite has just been updated. Listeners should
     * not have a UI and should just set up optional optimation data for
     * a suite, such as image caching.
     * <p>
     * If feature needs user input or can fail because a property is missing
     * or invalid, the suite installer for that AMS it should extended,
     * instead of using this callback.
     *
     * @param newSuite the new MIDlet suite that was installed
     * @param jarPathName the path name to the new MIDlet's JAR
     * @param oldSuite the old version of the MIDlet suite
     * @param keepOldData true if user wants to keep the previous suite's data
     */
    public void updated(MIDletSuite newSuite, String jarPathname,
        MIDletSuite oldSuite, boolean keepOldData) {
        // do nothing
    }

    /**
     * Called when a MIDlet JAR is to be un-installed.
     *
     * @param suite the MIDlet suite that was installed
     */
    public void toBeUninstalled(MIDletSuite suite) {
        // do nothing
    }

    public static void registerProtocol(ProtocolBase protocol) {
        MIDletStateHandler msh = MIDletStateHandler.getMidletStateHandler();
        if (msh == null) return;
        MIDletSuite suite = msh.getMIDletSuite();
        init();

        singletoneThis.addProtocol(protocol, suite);
    }
   
    /**  
     * Contains (midletSuite, Vector(Protocols)) pairs
     */
    private Hashtable table = new Hashtable();

    private void addProtocol(ProtocolBase protocol, MIDletSuite suite) {
        Object obj = table.get(suite);
        if (obj == null) {
            Vector vec = new Vector();
            vec.add(protocol);
            table.put(suite, vec);
        } else {
            Vector vec = (Vector)obj;
            vec.add(protocol);
        }
    }

    /**
     *  returns protocols for given midlet suite 
     *  removes entry from the list
     *  removes midlet suite entry finally
     */
    private ProtocolBase getRemoveNextProtocol(MIDletSuite suite) {
        Object obj = table.get(suite);
        if (obj != null) {
            Vector vec = (Vector)obj;
            if (vec.isEmpty()) {
                table.remove(suite);
                return null;
            } else { 
                return (ProtocolBase)vec.remove(0);
            }
        }

        return null;
    }

    private static boolean initialized = false;
    private static Object initSync = new Object();
    public synchronized static void init() {
        if (!initialized) {
            AccessController.doPrivileged(new PrivilegedAction() {
                public Object run() {
                    LifeCycleNotifier notifier = LifeCycleNotifierProvider.getInstance();
                    singletoneThis = new WmaLifeCycleListener();
                    notifier.addListener(singletoneThis);
                    return null;
                }
            });
            initialized = true;
        }
    }
}


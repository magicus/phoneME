/*
 *
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

package com.sun.midp.main;

import com.sun.midp.midletsuite.MIDletSuiteStorage;

/** Implements utilities that are different for SVM and MVM modes. */
public class AmsUtil {
    /** Cached reference to the MIDletProxyList. */
    private static MIDletProxyList midletProxyList;
        
    /** The unique ID of the next MIDlet suite to run. */
    static String nextMidletSuiteToRun;

    /** The class of the next MIDlet to run. */
    static String nextMidletToRun; 

    /**
     * If not null, this will be available to the MIDlet to run as
     * application property arg-0.
     */
    static String arg0ForNextMidlet;

    /**
     * If not null, this will be available to the MIDlet to run as
     * application property arg-1.
     */
    static String arg1ForNextMidlet;

    /**
     * If not null, this will be available to the MIDlet to run as
     * application property arg-2.
     */
    static String arg2ForNextMidlet;

    /**
     * Initializes AmsUtil class. shall only be called from 
     * MIDletSuiteLoader's main() in MVM AMS isolate
     * or in SVM main isolate.
     * No need in security checks since it is package private method.
     *
     * @param theMIDletProxyList MIDletController's container
     * @param theMidletControllerEventProducer utility to send events
     */
    static void initClass(MIDletProxyList theMIDletProxyList,
            MIDletControllerEventProducer theMidletControllerEventProducer) {
            
        midletProxyList = theMIDletProxyList;
    }
    
    /**
     * Queues the execution of the named Application suite to run.
     * The current application suite should terminate itself normally
     * to make resources available to the new application suite. Only
     * one package and set of MIDlets can be queued in this manner.
     * If multiple calls to execute are made, the package and MIDlets
     * specified during the <em>last</em> invokation will be executed
     * when the current application is terminated.
     *
     * @param midletSuiteStorage reference to a MIDletStorage object
     * @param externalAppId ID of MIDlet to invoke, given by an external
     *                      application manager (MVM only)
     * @param id ID of an installed suite
     * @param midlet class name of MIDlet to invoke
     * @param displayName name to display to the user
     * @param arg0 if not null, this parameter will be available to the
     *             MIDlet as application property arg-0
     * @param arg1 if not null, this parameter will be available to the
     *             MIDlet as application property arg-1
     * @param arg2 if not null, this parameter will be available to the
     *             MIDlet as application property arg-2
     *
     * @return true to signal that the MIDlet suite MUST first exit before the
     * MIDlet is run
     */
    static boolean executeWithArgs(MIDletSuiteStorage midletSuiteStorage,
            int externalAppId, String id, String midlet,
            String displayName, String arg0, String arg1, String arg2) {

        if (id != null) {
            if (midletProxyList.isMidletInList(id, midlet)) {
                // No need to exit, MIDlet already loaded
                return false;
            }
        }

        nextMidletSuiteToRun = id;
        nextMidletToRun = midlet; 
        arg0ForNextMidlet = arg0;
        arg1ForNextMidlet = arg1;
        arg2ForNextMidlet = arg2;

        return true;
    }

    /**
     * Does nothing in SVM mode
     *
     * @param id Isolate Id
     */
    static void terminateIsolate(int id) {
    }
}



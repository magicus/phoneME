/*
 * @(#)CDCInit.java	1.6 06/09/15 @(#)
 *
 * Copyright 2004 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package com.sun.midp.main;

/**
 * Initialize the CDC environment for MIDlet execution.
 */
public class CDCInit {
    static void init() {
        try {
            String n = System.getProperty("sun.midp.library.name", "midp");
            // DEBUG: System.err.println("Loading DLL \"" + n + "\" ...");
            System.loadLibrary(n);
            // DEBUG: System.err.println("done");
        } catch (UnsatisfiedLinkError err) {
        }

	/** Path to MIDP working directory. */
        String home = System.getProperty("sun.midp.home.path", "");

        initMidpNativeStates(home);
        // DEBUG: System.err.println("MIDP states initialized");
    }

    /**
     * Performs native subsystem initialization.
     * @param home path to the MIDP working directory.
     */
    static native void initMidpNativeStates(String home);
}

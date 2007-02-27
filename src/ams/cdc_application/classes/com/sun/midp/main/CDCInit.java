/*
 * @(#)CDCInit.java	1.6 06/09/15 @(#)
 *
 * Copyright 2004 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package com.sun.midp.main;

import java.io.File;

/**
 * Initialize the CDC environment for MIDlet execution.
 */
public class CDCInit {
    /**
     * Performs CDC API initialization.
     *
     * @param midpHome root directory of the MIDP working files
     * @param nativeLib name of the native shared library, only applies to
     * non-rommized build
     */
    public static void init(String midpHome, String nativeLib) {
        try {
            if (nativeLib != null) {
                System.loadLibrary(nativeLib);
            }
        } catch (UnsatisfiedLinkError err) {
            /*
             * Since there is currenly no to determine if the build rommized
             * the MIDP native methods or not, it is customary to pass in a
             * default library name even if there is no library to load,
             * which will cause this exception, so the exception has to be
             * ignored here. If this is a non-rommized build and library is
             * not found, the first native method call below will throw an
             * error.
             */
        }

        /*
         * In case the normal system properites configration did not set
         * the profile set it here.
         */
        String profile = System.getProperty("microedition.profiles");
	if (profile == null) {
            System.setProperty("microedition.profiles", "MIDP-2.1");
        }

        initMidpNativeStates(midpHome);
    }

    /** Performs CDC API initialization. */
    public static void init() {
	/*
         * Path to MIDP working directory. 
         * Default is the property "sun.midp.home.path",
         * the fallback is user.dir.
         */
        String userdir = System.getProperty("user.dir", ".");
        userdir+= File.separator + "midp" + File.separator + "midp_fb";
        String home = System.getProperty("sun.midp.home.path", userdir);
        String lib = System.getProperty("sun.midp.library.name", "midp");

        init(home, lib);
    }

    /**
     * Performs native subsystem initialization.
     * @param home path to the MIDP working directory.
     */
    static native void initMidpNativeStates(String home);
}

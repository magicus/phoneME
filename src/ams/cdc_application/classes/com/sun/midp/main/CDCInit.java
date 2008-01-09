/*
 * Copyright 1990-2007 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package com.sun.midp.main;

import java.io.File;

/**
 * Initialize the CDC environment for MIDlet execution.
 */
public class CDCInit {
    /** True, if the system is initialized. */
    private static boolean initialized;

    /**
     * Performs CDC API initialization.
     *
     * @param midpHome root directory of the MIDP working files
     * @param storageHome root directory for MIDP storage files
     * @param nativeLib name of the native shared library, only applies to
     * non-rommized build
     */
    public static void init(String midpHome, String storageHome,
			    String nativeLib) {
        if (initialized) {
            return;
        }

        initialized = true;

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

        initMidpNativeStates(midpHome, storageHome);
    }

    /** Performs CDC API initialization. */
    public static void init() {
	/*
         * Path to MIDP working directory. 
         * Default is the property "sun.midp.home.path",
         * the fallback is user.dir.
         */
        String userdir = System.getProperty("user.dir", ".");
        userdir += File.separator + "midp" + File.separator + "midp_linux_gci";
        String home = System.getProperty("sun.midp.home.path", userdir);

        init(home);
    }

    public static void init(String midpHome) {
        String storagePath = System.getProperty("sun.midp.storage.path", null);
        if (storagePath == null) {
            storagePath = midpHome;
        } 
        
        init(midpHome, storagePath);
    }
    
    public static void init(String midpHome, String storageHome) {
        init(midpHome, storageHome, 
	     System.getProperty("sun.midp.library.name", "midp"));
    }

    /**
     * Performs native subsystem initialization.
     * @param home path to the MIDP working directory.
     */
    static native void initMidpNativeStates(String config, String storage);
}

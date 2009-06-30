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
     * @param storageHome MIDP application database directory to be used
     *                    instead of "midpHome/appdb", directory must be secure
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

        initMidpNativeStates(midpHome + File.separator + "lib", storageHome);
    }

    /**
     * Performs CDC API initialization.
     * <p>
     * Uses the sun.midp.library.name property for the native library name or
     * "midp" if not found.
     *
     * @param midpHome root directory of the MIDP working files
     * @param storageHome MIDP application database directory to be used
     *                    instead of "midpHome/appdb", directory must be secure
     */
    public static void init(String midpHome, String storageHome) {
        // Only System.getProperty(String) looks in the MIDP property list.
        String nativeLib = System.getProperty("sun.midp.library.name");

        if (nativeLib == null) {
            nativeLib = "midp";
        }

        init(midpHome, storageHome, nativeLib);
    }

    /**
     * Performs CDC API initialization.
     * <p>
     * Uses the sun.midp.library.name property for the native library name or
     * "midp" if not found.
     * <p>
     * Uses the sun.midp.storage.path property for the storage directory or
     * "midpHome/appdb" if not found.
     *
     * @param midpHome root directory of the MIDP working files
     */
    public static void init(String midpHome) {
        // Only System.getProperty(String) looks in the MIDP property list.
        String storagePath = System.getProperty("sun.midp.storage.path");

        if (storagePath == null) {
            storagePath = midpHome + File.separator + "appdb";
        } 
        
        init(midpHome, storagePath);
    }
    
    
    /**
     * Performs CDC API initialization.
     * <p>
     * Uses the sun.midp.library.name property for the native library name or
     * "midp" if not found.
     * <p>
     * Uses the sun.midp.storage.path property for the storage directory or
     * "midpHome/appdb" if not found.
     * <p>
     * Uses the sun.midp.home.path property for the MIDP home directory or
     * the user.dir property if not found, and if user.dir is not found,
     * "./midp/midp_linux_gci" is used.
     */
    public static void init() {
        // Only System.getProperty(String) looks in the MIDP property list.
        String home = System.getProperty("sun.midp.home.path");

        if (home == null) {
            home = System.getProperty("user.dir");
            if (home == null) {
                home = "." + File.separator + "midp" + File.separator +
                    "midp_linux_gci";
            }
        }

        init(home);
    }

    /**
     * Performs native subsystem initialization.
     *
     * @param config directory of the MIDP system resources and config files
     * @param storage MIDlet storage directory
     */
    static native void initMidpNativeStates(String config, String storage);
}

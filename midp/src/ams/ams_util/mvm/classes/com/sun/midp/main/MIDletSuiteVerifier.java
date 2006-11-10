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

import com.sun.cldc.isolate.Util;
import com.sun.cldc.isolate.Isolate;
import java.io.IOException;

/**
 * This class is designed to provide the functionality
 * needed for MIDlet suite classes verification and the
 * methods to control that the verified suite hasn't been
 * changed since the verification moment.
 */
public class MIDletSuiteVerifier {
    /** Size of the verification portion */
    final static int CHUNK_SIZE = 10240;

    /**
     * Verify classes within JAR package
     * @param jarPath path to the JAR package within file system
     * @return true if all classes were verified successfully, false otherwise
     */
    static boolean verifyJar(String jarPath) {
        return Util.verify(jarPath, CHUNK_SIZE);
    }

    /**
     * Verify suite classes and return hash value of the suite JAR
     * @param jarPath path to the JAR package within file system
     * @return hash value of the successfully verified JAR package,
     * or null if verification failed
     */
    public static byte[] verifyJarClasses(String jarPath) throws IOException {
        if (verifyJar(jarPath))
            return getJarHash(jarPath);
        else return null;
    }

    /**
     * Disable or enable verifier for the current isolate
     * @param verifier true to enable, false to disable verifier
     */
    static void setUseVerifier(boolean verifier) {
        Isolate.currentIsolate().setUseVerifier(verifier);
    }

    /**
     * Evaluate hash value for the JAR  package
     *
     * @param jarPath JAR package path
     * @return hash value for JAR package
     */
    public native static byte[] getJarHash(String jarPath) throws IOException;
}

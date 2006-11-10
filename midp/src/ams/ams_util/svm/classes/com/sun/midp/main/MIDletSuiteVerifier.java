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

import java.io.IOException;

/**
 * This is a stub class at present.
 * The implementation of "verify once" optimization
 * for SVM will be provided later for release.
 */

public class MIDletSuiteVerifier {
    /**
     * Verify classes within JAR package
     * @param jarPath path to the JAR package within file system
     * @return true if all classes were verified successfully, false otherwise
     */
    static boolean verifyJar(String jarPath) {
        return false;
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
     * Evaluate hash value for the JAR  package
     *
     * @param jarPath JAR package path
     * @return hash value for JAR package
     */
    public native static byte[] getJarHash(String jarPath) throws IOException;
}

/*
 *   
 *
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

import com.sun.cldc.isolate.Util;
import com.sun.midp.midletsuite.MIDletSuiteStorage;

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
     * Verify suite classes and return hash value of the suite JAR,
     * throws Error in the case of unsuccessful class verification
     *
     * @param suiteId id of the suite whose classes are to be verified
     * @param suiteStorage suite storage instance
     * @return hash value of the successfully verified JAR package,
     *   or null if class verification wasn't done by some reason,
     *   e.g. it was scheduled to be started later
     * @throws Error, IOException
     */
    public static byte[] verifySuiteClasses(int suiteId,
            MIDletSuiteStorage suiteStorage) throws IOException {

        String jarPath = suiteStorage.getMidletSuiteJarPath(suiteId);
        if (verifyJar(jarPath)) {
            return getJarHash(jarPath);
        } else {
            /**
             * Class verification failure is a serious problem, it's
             * up to caller how to handle it and proceed with further
             * execution.
             */
            throw new java.lang.Error();
        }
    }

    /**
     * Disable or enable class verifier for the current Isolate
     * @param verifier true to enable, false to disable verifier
     */
    static native void useClassVerifier(boolean verifier);

    /**
     * Evaluate hash value for the JAR  package
     *
     * @param jarPath JAR package path
     * @return hash value for JAR package
     */
    public native static byte[] getJarHash(String jarPath) throws IOException;

    /**
     * Compare hash value of the JAR with provided hash value.
     *
     * @param jarPath path to JAR file
     * @param hashValue hash value to compare with
     * @return true if JAR has hash value equal to the provided one,
     *   otherwise false
     * @throws IOException
     */
    public native static boolean checkJarHash(String jarPath,
            byte[] hashValue) throws IOException;
}

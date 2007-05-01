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

import com.sun.midp.midletsuite.MIDletSuiteStorage;

import java.io.IOException;

/**
 * This a class stub class to enable the com.sun.midp.installer.Installer
 * to compile.
 */
public class MIDletSuiteVerifier {
    /**
     * Schedule suite classes verification to be done by a new VM started
     * as soon as the current VM will be terminated.
     *
     * @param suiteId id of the suite whose classes are to be verified
     * @param suiteStorage suite storage instance
     * @return null verify hash value, since no verification is done,
     *   the verification is only scheduled to be done in the future
     * @throws IOException
     */
    public static byte[] verifySuiteClasses(int suiteId,
        MIDletSuiteStorage suiteStorage) throws IOException {
        throw new IOException("stub version");
    }

    /**
     * Evaluate hash value for the JAR  package
     *
     * @param jarPath JAR package path
     * @return hash value for JAR package
     */
    public static byte[] getJarHash(String jarPath) throws IOException {
        throw new IOException("stub version");
    }

    /**
     * Compare hash value of the JAR with provided hash value.
     *
     * @param jarPath path to JAR file
     * @param hashValue hash value to compare with
     * @return true if JAR has hash value equal to the provided one,
     *   otherwise false
     * @throws IOException
     */
    public static boolean checkJarHash(String jarPath,
            byte[] hashValue) throws IOException {
        throw new IOException("stub version");
    }
}

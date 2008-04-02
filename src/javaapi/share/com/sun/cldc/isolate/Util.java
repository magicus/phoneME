/*
 *   
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.cldc.isolate;

import com.sun.cldchi.jvm.JVM;

public final class Util {
    /**
     * Use VM verifier to make sure that supplied jar is correct.
     *
     * <p><i>Note:</i> for this API to function, compilation time flag
     * ENABLE_VERIFY_ONLY=true must be specified
     *
     * @param jarPath path to JAR file to verify 
     * @return true if supplied JAR is correct, false otherwise
     **/
    public static boolean verify(String jarPath) {
      return verify(jarPath, Integer.MAX_VALUE);
    }

    /**
     * Use VM verifier to make sure that supplied jar is correct.
     * <p>
     * The verification of a JAR is performed in chunks. 
     * During verification of a chunk no other isolates can execute.
     * Each chunk consists of one or more class files.
     * The size of a chunk is the total compressed size of JAR entries
     * in the chunk.  Verification of a chunk stops when either the
     * size of a chunk exceeds the specified <code>chunkSize</code> or
     * there are no more entries in the JAR.  Larger values of
     * <code>chunkSize</code> ensure faster verification of the JAR,
     * but other active isolates will have less chances to execute
     * while the verification is in progress.
     * <p>
     * This method is multitask-safe, i.e. it can be invoked from multiple
     * tasks at the same time.
     * <p>
     * The method returns after the whole JAR is verified and the return value
     * indicates if the JAR is correct.
     *
     * <p><i>Note:</i> for this API to function, compilation time flag
     * ENABLE_VERIFY_ONLY=true must be specified
     *
     * @param jarPath path to JAR file to verify 
     * @param chunkSize size of the JAR file chunk
     * @return true if supplied JAR is correct, false otherwise
     * @throws IllegalArgumentException if <code>chunkSize</code> is not
     *         positive
     */
    public static boolean verify(String jarPath, int chunkSize) {
        if (chunkSize <= 0) {
            throw new IllegalArgumentException();
        }
    String[] classpath  = new String[1];
    String[] mainArgs   = new String[2];
    classpath[0] = jarPath;
    mainArgs[0] = jarPath;
        mainArgs[1] = new Integer(chunkSize).toString();
    int status = 0;

    try {
        Isolate iso = new Isolate("com.sun.cldc.isolate.Verifier",
                       mainArgs,
                       classpath);
            iso.setAPIAccess(true);
        iso.start();
        iso.waitForExit();

        status = iso.exitCode();
    }
    catch (IsolateStartupException ise) {
        ise.printStackTrace();
        return false;
    }
    return status == JVM.STATUS_VERIFY_SUCCEEDED;
    }
}

final class Verifier {
    public static void main(String[] args) {
        int chunkSize = Integer.parseInt(args[1]);
	    Isolate.currentIsolate().exit(
            JVM.verifyJar(args[0], chunkSize));
    }
    
}

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

import com.sun.cldc.isolate.Isolate;

import com.sun.cldc.isolate.IsolateStartupException;

import com.sun.cldchi.jvm.JVM;

import com.sun.midp.installer.InstallState;
import com.sun.midp.midletsuite.MIDletSuiteStorage;
import com.sun.midp.installer.InstallListener;
import com.sun.midp.configurator.Constants;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

import java.io.IOException;

/**
 * This class is designed to provide the functionality
 * needed for generating the binary image from the MIDlet suite 
 * classes.
 */
public class MIDletAppImageGeneratorBase {

    /**
     * Creates an application image file. It loads the Java classes
     * from the <code>jarFile</code> into the heap, verify the class
     * contents, and write the classes to an Application Image file as
     * specified by <code>binFile</code>.
     *
     * @param jarFile - source JAR file to create binary image from
     * @param binFile - output file, where binary image should be stored
     * @param flags - flags, can be JVM.REMOVE_CLASSES_FROM_JAR
     * @return true if application image was generated successfully, 
     *            false otherwise
     */
    protected static boolean generateAppImage(String jarFile, String binFile, 
            int flags) {
        int code = 0;        
        String[] classpath  = new String[0]; // IMPL NOTE: specify the set of
                                             // dynamic libs as well
        String[] mainArgs   = new String[3];
        mainArgs[0] = jarFile;
        mainArgs[1] = binFile;
        mainArgs[2] = String.valueOf(flags);
          	        
        try {
            // IMPL NOTE: eliminate the hardcoded string constants
            Isolate iso =  new Isolate("com.sun.midp.main.AppImageWriter",
  			            mainArgs, classpath);
            iso.setAPIAccess(true);
            iso.start();
            iso.waitForExit();

            code = iso.exitCode();
        }
        catch (IsolateStartupException ise) {
            if (Logging.REPORT_LEVEL <= Logging.ERROR) {
                Logging.report(Logging.ERROR, LogChannels.LC_AMS,
                "Cannot startup isolate: " + ise);
            }            
        }
        return code == 0;
    }
}

final class AppImageWriter {
    public static void main(String[] args) {
        int status = 1;
        try {
            int flags = Integer.parseInt(args[2]);
            JVM.createAppImage(args[0], args[1], flags);
            status = 0;
        } catch (Throwable t) {
            if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                t.printStackTrace();
            }
            if (Logging.REPORT_LEVEL <= Logging.ERROR) {
                Logging.report(Logging.ERROR, LogChannels.LC_AMS,
                "Error during conversion: " + t);
            }
        }

        Isolate.currentIsolate().exit(status);
    }
}



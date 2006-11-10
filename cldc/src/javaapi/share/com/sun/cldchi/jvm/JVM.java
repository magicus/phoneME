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

package com.sun.cldchi.jvm;

public class JVM {
    /**
     * If this flag is defined and the romization is successful, class
     * files are removed from the JAR file(s) after the romization
     * process. This parameter is ignored for source romization.
     */
    public static final int REMOVE_CLASSES_FROM_JAR = (1 << 1);

    /**
     * Returned by getAppImageProgress() to indicate that the last image
     * creation process has was cancelled before it was completed.
     */
     public static final int STATUS_CANCELLED =  -3;

    /**
     * Returned by getAppImageProgress() to indicate that the last image
     * creation process has failed before it was completed.
     */
    public static final int STATUS_FAILED    =  -2;

    /**
     * Returned by getAppImageProgress() to indicate that no image
     * creation process has ever been started since the VM was bootstraped.
     */
    public static final int STATUS_VIRGIN    =  -1;

    /**
     * Any value returned by getAppImageProgress() that.s greater or equal to
     * STATUS_START, but lower than STATUS_SUCCEEDED, means that the
     * image creation is still taking place.
     */
    public static final int STATUS_START     =   0;

    /**
     * Returned by getAppImageProgress() to indicate that the last image
     * creation process has succeeded.
     */
    public static final int STATUS_SUCCEEDED = 100;

    /**
     * Creates an application image file. It loads the Java classes
     * from the <code>jarFile</code> into the heap, verify the class
     * contents, and write the classes to an Application Image file as
     * specified by <code>binFile</code>. This function is typically
     * executed by the Application Management Software (AMS)
     * immediately after a JAR file is downloaded to the device. <p>
     *
     * This function must be called with a clean VM state -- i.e., if a
     * Java application is executing, you must exit the Java application
     * before running the Converter. <p>
     *
     * In MVM mode, this method should not be called only from within
     * a clean Isolate. <p>
     *
     * <b>Interaction with classpath and shared libraries: </b>
     * In the context of the VM (or current Isolate), the classpath
     * may be specified to additional shared libraries. These shared 
     * libraries are loaded first, before jarFile is loaded. All shared
     * libraries specified on the classpath must be binary image files
     * and must be be JAR files.
     *
     * Note that if the image creation process was cancelled, no exception
     * is thrown. A subsequent call to getAppImageProgress() will return
     * STATUS_CANCELLED.
     *
     * @param jarFile specifies the JAR file to be converted.
     *
     * @param binFile specifies the name of the app image file to be
     *                written into
     *
     * @exception Error if another instance of the converter is 
     *            already running.
     *
     * @exception OutOfMemoryError if the VM ran out of memory during
     *            the image creation process.
     */
    private static void createAppImage(char jarFile[], char binFile[],
                                       int flags) throws Error {
        startAppImage(jarFile, binFile, flags);
        for (;;) {
            if (!createAppImage0()) {
                break;
            }
        }
    }

    public static void createAppImage(String jarFile, String binFile,
                                      int flags) throws Error
    {
        createAppImage(jarFile.toCharArray(), binFile.toCharArray(),
                       flags);
    }

    public native static int getAppImageProgress();

    /**
     * If an image creation process is underway, cancel it. This will
     * force createAppImage() to delete all temporary files, as well as
     * the output image file, and return immediately. A future call to
     * getAppImageProgress() will return STATUS_CANCELLED.
     *
     * If an image creation process is not underway, this method has no
     * effect.
     */
    public native static void cancelImageCreation();

    private static native void startAppImage(char jarFile[], char binFile[],
                                             int flags) throws Error;

    /**
     * Returns true if the image creation process has completed or
     * been concelled.
     */
    private native static boolean createAppImage0();

    /**
     * This method is used by the source romizer to create ROMImage.cp.
     *
     * @exception Error if the romization process fails for any reason.
     */
    private native static void createSysImage()
         throws Error;

    public native static void loadLibrary(String libName)
         throws Error;
}

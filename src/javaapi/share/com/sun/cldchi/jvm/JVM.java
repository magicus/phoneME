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
     * Returned by verifyJar() to indicate no classes verification
     * has ever been started since VM didn't find any classes in JAR.
     */
    public static final int STATUS_VERIFY_NOTHING = 1;

    /**
     * Returned by verifyJar() to indicate all JAR classes were
     * successfully verified.
     */
    public static final int STATUS_VERIFY_SUCCEEDED = 2;

    /**
     * Returned by verifyJar() to indicate JAR classes verification
     * failed by some reason.
     */
    public static final int STATUS_VERIFY_FAILED = 3;

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
     * This method is used by the source romizer to create ROMImage.cpp.
     *
     * @exception Error if the romization process fails for any reason.
     */
    private native static void createSysImage()
         throws Error;

    /**
     * This method is used to load binary library into the VM. 
     * It allows to call native function implementations from this library.
     *
     * @param libName name of the library WITHOUT EXTENSION. It was made to make 
     *        java code platform-independent.
     *
     * @exception Error if the VM fails to load the library with this name.
     */
    public native static void loadLibrary(String libName)
         throws Error;


    /**
     * Copy an array from the specified source array, beginning at the
     * specified position, to the specified position of the destination array.
     * <p>
     * Impose the following restrictions on the input arguments:
     * <ul>
     * <li><code>dst</code> is not <code>null</code>.
     * <li><code>src</code> is not <code>null</code>.
     * <li>The <code>srcOffset</code> argument is not negative.
     * <li>The <code>dstOffset</code> argument is not negative.
     * <li>The <code>length</code> argument is not negative.
     * <li><code>srcOffset+length</code> is not greater than
     *     <code>src.length</code>, the length of the source array.
     * <li><code>dstOffset+length</code> is not greater than
     *     <code>dst.length</code>, the length of the destination array.
     * <li>any actual component of the source array from position 
     *     <code>srcOffset</code> through <code>srcOffset+length-1</code> 
     *     can be converted to the component type of the destination array
     * </ul>
     * <p>
     * The caller is responsible that these restrictions are not violated.
     * If any of the restrictions above is violated, the behavior is undefined.
     *
     * @param      src          the source array.
     * @param      srcOffset    start position in the source array.
     * @param      dst          the destination array.
     * @param      dstOffset    start position in the destination data.
     * @param      length       the number of array elements to be copied.
     */
    public static void unchecked_byte_arraycopy(byte[] src, 
                                                       int srcOffset,
                                                       byte[] dst, 
                                                       int dstOffset, 
                                                       int length) {
      System.arraycopy(src, srcOffset, dst, dstOffset, length);
    }

    public static void unchecked_char_arraycopy(char[] src, 
                                                       int srcOffset,
                                                       char[] dst, 
                                                       int dstOffset, 
                                                       int length) {
      System.arraycopy(src, srcOffset, dst, dstOffset, length);
    }

    public static void unchecked_int_arraycopy(int[] src, 
                                                      int srcOffset,
                                                      int[] dst, 
                                                      int dstOffset, 
                                                      int length) {
      System.arraycopy(src, srcOffset, dst, dstOffset, length);
    }

    public static void unchecked_long_arraycopy(long[] src, 
                                                int srcOffset,
                                                long[] dst, 
                                                int dstOffset, 
                                                int length) {
      System.arraycopy(src, srcOffset, dst, dstOffset, length);
    }

    public static void unchecked_obj_arraycopy(Object[] src, 
                                                      int srcOffset,
                                                      Object[] dst, 
                                                      int dstOffset, 
                                                      int length) {
      System.arraycopy(src, srcOffset, dst, dstOffset, length);
    }

    /**
     * Verifies all classes of the given JAR package within the current
     * VM instance. The JAR path should be included into classpath(s) of
     * the VM.
     *
     * @param jar specifies the JAR file to be verified.
     * @param chunkSize amount of bytecode to be verified with a single
     *          native call, however not less than one class will be
     *          verified with a single call.
     * @return status of the JAR classes verification, it can be one of the
     *   following values STATUS_VERIFY_NOTHING, STATUS_VERIFY_SUCCEEDED or
     *   STATUS_VERIFY_FAILED
     */
    public static int verifyJar(String jar, int chunkSize) {

        int nextChunkID = 0;
        int status = STATUS_VERIFY_NOTHING;
        try {
            do {
                nextChunkID = verifyNextChunk(jar, nextChunkID, chunkSize);
                Thread.yield();
            } while (nextChunkID > 0);
            // OK, just all files verified
            if (nextChunkID == 0) {
                status = STATUS_VERIFY_SUCCEEDED;
            }
        } catch (Throwable t) {
            //do we need it?
            t.printStackTrace();
            status = STATUS_VERIFY_FAILED;
        }

        return status;
    }

    private static native int verifyNextChunk(String jar, int nextChunkID,
                                              int chunkSize);

}

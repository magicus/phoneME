/*
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
package com.sun.mmedia;

/*
 * IMPL_NOTE:
 * - redesign to make it more platform-independent
 */

// package private class
class FileIO {
    static final int READ = 0;
    static final int WRITE = 1;
    static final String PREFIX = "/mmatemp_";
    static private byte[] tempDirArray = new byte[256];
    static private boolean cleanupDone = false;
    static private String tempDir = null;

    static final native int nOpen(byte[] filename, int mode);

    /*
     * Returns the actual number of bytes read. 0 is returned if there is an error
     */
    static final native int nRead(int handle, byte[] data, int offset, int length);

    /*
     * Returns the actual number of bytes written. 0 is returned if there is an error
     */
    static final native int nWrite(int handle, byte[] data, int offset, int length);

    static final native boolean nSeek(int handle, int offset);

    /** Deletes a file by the given name */
    static final native boolean nDelete(byte[] filename);

    static final String getTempFileName() {
	return getTempDirectory() + PREFIX + System.currentTimeMillis();
    }

    static final private String getTempDirectory() {
	if (tempDir != null)
	    return tempDir;
	int length = nGetTempDirectory(tempDirArray);
	if (length == 0)
	    tempDir = "/tmp"; // $$fb platform-dependent!
	else
	    tempDir = new String(tempDirArray, 0, length);
	removeTempFiles();
	return tempDir;
    }

    static final native boolean nClose(int handle);

    // $$fb this function is platform-dependent
    // Currently, this method will only do a clean-up once per VM lifetime
    // when requesting a temp file for the first time
    static final synchronized void removeTempFiles() {
	if (!cleanupDone) {
	    cleanupDone = true;
	    nRemoveTempFiles((tempDir + PREFIX + "*").getBytes());
	}
    }

    /*
     * Returns the length of the temporary directory string
     */
    static private native int nGetTempDirectory(byte[] tempDir);

    static private native void nRemoveTempFiles(byte[] tempDir);

}

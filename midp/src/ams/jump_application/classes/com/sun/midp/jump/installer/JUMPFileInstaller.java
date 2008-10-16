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

package com.sun.midp.jump.installer;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import javax.microedition.io.ConnectionNotFoundException;
import javax.microedition.io.Connector;
import com.sun.midp.configurator.Constants;
import com.sun.midp.installer.InvalidJadException;
import com.sun.midp.io.j2me.storage.RandomAccessStream;
import com.sun.midp.midletsuite.MIDletSuiteLockedException;
import com.sun.midp.security.SecurityToken;
import com.sun.midp.jump.installer.JUMPInstallerInterface;

/**
 * An Installer allowing to install a midlet suite from a file.
 * If the midlet suite is given by a descriptor file, the jar
 * URL specified in the descriptor must have a "file" scheme.
 */
public class JUMPFileInstaller extends JUMPInstaller 
    implements JUMPInstallerInterface {

    /** Number of bytes to read at one time when copying a file. */
    private static final int CHUNK_SIZE = 10 * 1024;

    /**
     * Downloads an application descriptor file from the given URL.
     *
     * @return a byte array representation of the file or null if not found
     *
     * @exception IOException is thrown if any error prevents the download
     *            of the JAD
     */
    protected byte[] downloadJAD() throws IOException {
        RandomAccessStream jadInputStream;
        ByteArrayOutputStream bos = new ByteArrayOutputStream(CHUNK_SIZE);
        String jadFilename = getUrlPath(state.localJadUrl);

        state.beginTransferDataStatus = DOWNLOADING_JAD;
        state.transferStatus = DOWNLOADED_1K_OF_JAD;

        jadInputStream = new RandomAccessStream();
        jadInputStream.connect(jadFilename, Connector.READ);

        transferData(jadInputStream.openInputStream(), bos, CHUNK_SIZE);

        jadInputStream.close();

        return bos.toByteArray();
    }

    /**
     * Downloads an application archive file from the given URL into the
     * given file. Automatically handle re-tries.
     *
     * @param filename name of the file to write. This file resides
     *          in the storage area of the given application
     *
     * @return size of the JAR
     *
     * @exception IOException is thrown if any error prevents the download
     *   of the JAR
     */
    protected int downloadJAR(String filename) throws IOException {
        int jarSize;
        RandomAccessStream jarInputStream, jarOutputStream;
        String jarFilename = getUrlPath(state.localJarUrl);

        // Open source (jar) file
        jarInputStream = new RandomAccessStream();
        jarInputStream.connect(jarFilename, Connector.READ);

        // Open destination (temporary) file
        jarOutputStream = new RandomAccessStream();
        jarOutputStream.connect(filename,
                                RandomAccessStream.READ_WRITE_TRUNCATE);

        // transfer data
        state.beginTransferDataStatus = DOWNLOADING_JAR;
        state.transferStatus = DOWNLOADED_1K_OF_JAR;

        jarSize = transferData(jarInputStream.openInputStream(),
                               jarOutputStream.openOutputStream(), CHUNK_SIZE);

        jarInputStream.close();

        return jarSize;
    }

    /**
     * Compares two URLs for equality in sense that they have the same
     * scheme, host and path.
     *
     * @param url1 the first URL for comparision
     * @param url2 the second URL for comparision
     *
     * @return true if the scheme, host and path of the first given url
     *              is identical to the scheme, host and path of the second
     *              given url; false otherwise
     */
    protected boolean isSameUrl(String url1, String url2) {
        try {
            String defaultScheme = "file";
            String scheme1 = getUrlScheme(url1, defaultScheme);
            String scheme2 = getUrlScheme(url2, defaultScheme);

            if (url1.equals(url2)) {
                return true;
            }
        } catch (NullPointerException npe) {
            // no match, fall through
        }

        return true;
    }

    /**
     * Stops the installation. If installer is not installing then this
     * method has no effect. This will cause the install method to
     * throw an IOException if the install is not writing the suite
     * to storage which is the point of no return.
     *
     * @return true if the install will stop, false if it is too late
     */
    public boolean stopInstalling() {

        boolean res = super.stopInstalling();
        if (!res) {
            return res;
        }

        /* some additional actions can be added here */

        return true;
    }

    public int verifyAndStoreSuite(String jadSourceUrl, String encoding,
        String tempJadFileName, String tempJarFileName, boolean isUpdate)
            throws IOException, InvalidJadException,
                   MIDletSuiteLockedException, SecurityException {

           return installJad(jadSourceUrl, tempJadFileName, tempJarFileName,
              encoding, Constants.INTERNAL_STORAGE_ID, isUpdate, false, null);
    }
    
    public int verifyAndStoreSuite(String jarSourceUrl,
        String tempJarFileName, String suiteName, boolean isUpdate)
                   throws IOException, InvalidJadException,
                   MIDletSuiteLockedException, SecurityException {
	
           return installJar(jarSourceUrl, tempJarFileName, suiteName,
                   Constants.INTERNAL_STORAGE_ID, isUpdate, false, null);
    }
}

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

package com.sun.j2me.io;

import java.io.DataOutputStream;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.File;
import javax.microedition.io.Connector;
import com.sun.j2me.main.Configuration;

import com.sun.j2me.security.Token;

/**
 * Provides abstraction for working with files
 */
public class FileAccess {
    
    private String name;
    private Object outputStream;
    private Object inputStream;
    private Token  securityToken;

    public static int INTERNAL_STORAGE_ID = 0;
    
    /**
     * Prevents of creating a new instance of FileAccess
     */
    private FileAccess() {
    }
    
    private FileAccess(String name, Token securityToken) {
        this.name = name;
        this.securityToken = securityToken;
    }

    /**
     * Returns the root to build storage filenames including an needed
     * file separators, abstracting difference of the file systems
     * of development and device platforms. Note the root is never null.
     *
     * @param storageId ID of the storage the root of which should be returned
     *
     * @return root of any filename for accessing device persistant
     *         storage.
     */
    public static String getStorageRoot(int storageId) {
        String storagePath = Configuration.getProperty("sun.storage.path" + storageId) + File.separator;
        if (storagePath == null || storagePath.equals("")) {
            throw new UnsupportedOperationException("\"sun.storage.path" + storageId + "\" property is not set");            
        }

        return storagePath;
    }

    public static FileAccess getInstance(String fileName, Token securityToken) {
        return new FileAccess(fileName, securityToken);
    }
    
    public void connect(int accessType) throws IOException {
        File file = new File(name);
        try {
            if ((accessType == Connector.READ)||(accessType == Connector.READ_WRITE)) {
                inputStream = new FileInputStream(file);
            }
            
            if ((accessType == Connector.WRITE)||(accessType == Connector.READ_WRITE)) {
                FileOutputStream outStream = new FileOutputStream(file);
                outputStream = new DataOutputStream(outStream);
            }
        }
        catch(FileNotFoundException fnfe) {
            throw new IOException();
        }
    }

    public void disconnect() throws IOException {
        if (inputStream instanceof InputStream) {
            ((InputStream)inputStream).close();
        }

        if (outputStream instanceof DataOutputStream) {
            ((DataOutputStream)outputStream).close();
        }
    }

    public InputStream openInputStream() throws IOException {
        return inputStream instanceof InputStream ? (InputStream)inputStream : null;
    }

    public DataOutputStream openDataOutputStream() throws IOException {
        return outputStream instanceof DataOutputStream ? (DataOutputStream)outputStream : null;
    }

    public void truncate(int i) throws IOException {
    }
    
}

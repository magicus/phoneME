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

package com.sun.j2me.io;

import java.io.DataOutputStream;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.File;
import javax.microedition.io.Connector;

/**
 * Provides abstraction for working with files
 */
public class FileAccess {
    
    private String name;
    private Object stream;

    public static int INTERNAL_STORAGE_ID = 0;
    
    /**
     * Prevents of creating a new instance of FileAccess
     */
    private FileAccess() {
    }
    
    private FileAccess(String name) {
        this.name = name;
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
        throw new UnsupportedOperationException("Not yet implemented");
    }

    public static FileAccess getInstance(String fileName) {
        return new FileAccess(fileName);
    }
    
    public void connect(int accessType) throws IOException {
        File file = new File(name);
        try {
            if (accessType == Connector.READ) {
                stream = new FileInputStream(file);
            }
            else if (accessType == Connector.WRITE) {
                FileOutputStream outStream = new FileOutputStream(file);
                stream = new DataOutputStream(outStream);
            }
        }
        catch(FileNotFoundException fnfe) {
            throw new IOException();
        }
    }

    public void disconnect() throws IOException {
        if (stream instanceof InputStream) {
            ((InputStream)stream).close();
        }
        else if (stream instanceof DataOutputStream) {
            ((DataOutputStream)stream).close();
        }
    }

    public InputStream openInputStream() {
        return stream instanceof InputStream ? (InputStream) stream : null;
    }

    public DataOutputStream openDataOutputStream() {
        return stream instanceof DataOutputStream ? (DataOutputStream) stream : null;
    }

    public void truncate(int i) {
    }
    
}

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

package com.sun.j2me.io;

import com.sun.midp.io.j2me.storage.File;
import com.sun.midp.io.j2me.storage.RandomAccessStream;
import com.sun.midp.configurator.Constants;

import com.sun.j2me.security.Token;

import java.io.DataOutputStream;
import java.io.InputStream;
import java.io.IOException;

/**
 * Provides abstraction for working with files
 */
public class FileAccess {

    private String name;
    private RandomAccessStream storage;

    public static int INTERNAL_STORAGE_ID = Constants.INTERNAL_STORAGE_ID;

    /**
     * Prevents of creating a new instance of FileAccess
     */
    private FileAccess() {
    }

    private FileAccess(String name, Token securityToken) {
        this.storage = new RandomAccessStream(securityToken.getSecurityToken());        
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
        return File.getStorageRoot(storageId);
    }

    public static FileAccess getInstance(String fileName, Token securityToken) {
        return new FileAccess(fileName, securityToken);
    }

    public void connect(int accessType) throws IOException {
        storage.connect(name, accessType);
    }

    public void disconnect() throws IOException {
        storage.disconnect();
    }

    public InputStream openInputStream() throws IOException {
        return storage.openInputStream();
    }

    public DataOutputStream openDataOutputStream() throws IOException {
        return storage.openDataOutputStream();
    }

    public void truncate(int i) throws IOException {
        storage.truncate(i);
    }
} 

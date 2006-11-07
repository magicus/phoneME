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
package com.sun.mmedia.protocol;

import com.sun.midp.security.*;
import com.sun.midp.io.j2me.storage.RandomAccessStream;
import com.sun.midp.io.j2me.storage.File;
import com.sun.midp.midlet.Scheduler;
import com.sun.midp.midlet.MIDletSuite;
import java.io.*;

public class FileConnectionSubstitute implements ImplicitlyTrustedClass
{
    //private SecurityToken _securityToken = null;
    private RandomAccessStream ras = null;
    private File file = null; 
    private static SecurityToken classSecurityToken = null;


    public FileConnectionSubstitute() {}

    public FileConnectionSubstitute(SecurityToken st, int permission) {
        if(st == null) {
            throw new SecurityException("No permissions for file connections");
        }
        // throws SecurityException if no permission
        st.checkIfPermissionAllowed(permission);
        
        ras = new RandomAccessStream(st);
        file = new File(st);
    }

    public FileConnectionSubstitute(int permission) {
        MIDletSuite midletSuite = Scheduler.getScheduler().getMIDletSuite();

        if(midletSuite == null) {
            throw new SecurityException("No permissions for file connections");
        }

        // throws SecurityException if no permission
        try {
            midletSuite.checkForPermission(permission, "");
        } catch (InterruptedException ie) {
            throw new SecurityException(
                "Failed to check user permission");
        }

        if(classSecurityToken == null) {
            throw new SecurityException("FileConnectionSubstitute is used before being securely initialized");
        }
        ras = new RandomAccessStream(classSecurityToken);
        file = new File(classSecurityToken);
    }
    
    /**
     * Initializes the security token for this class, so it can
     * perform actions that a normal MIDlet Suite cannot.
     *
     * @param token security token for this class.
     */
    public final void initSecurityToken(SecurityToken token) {
        if (classSecurityToken == null) {
            classSecurityToken = token;
        }
    }
    
    public void connect(String name, int mode) throws IOException {
        ras.connect(name, mode);
    }
    
    public int getSizeOf() throws IOException {
        return ras.getSizeOf();
    }

    public void disconnect() throws IOException {
        ras.disconnect();
    }

    public int readBytes(byte b[], int off, int len) throws IOException {
        return ras.readBytes(b, off, len);
    }
    
    public void setPosition(int absolutePosition) throws IOException {
        ras.setPosition(absolutePosition);
    }

    public void truncate(int size) throws IOException {
        ras.truncate(size);
    }
   

    public int writeBytes(byte b[], int off, int len) throws IOException {
        return ras.writeBytes(b, off, len);
    }

    public void commitWrite() throws IOException {
        ras.commitWrite();
    }

    /** Deletes a file by the given name */
    public boolean nDelete(byte[] filename)
    {
        try {
            file.delete(new String(filename));
        }
        catch(IOException e) {
            return false;
        }
        return true;
    }
    
}

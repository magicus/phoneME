/*
 * @(#)ConnectionBase.java	1.17 06/10/10
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
 *
 */


package com.sun.cdc.io;

import java.io.*;
import javax.microedition.io.*;

/**
 * Base class for Connection protocols.
 *
 * @version 1.1 2/3/2000
 */
abstract public class ConnectionBase extends GeneralBase implements Connection, ConnectionBaseInterface {

    /**
     * Open a connection to a target.
     *
     * @param string           The URL for the connection
     * @param mode             The access mode
     * @param timeouts         A flag to indicate that the called wants timeout exceptions
     *
     * @exception IllegalArgumentException If a parameter is invalid.
     * @exception ConnectionNotFoundException If the connection cannot be found.
     * @exception IOException  If some other kind of I/O error occurs.
     */
    abstract public void open(String name, int mode, boolean timeouts) throws IOException;

    /**
     * Open a connection to a target.
     *
     * @param string           The URL for the connection
     * @param mode             The access mode
     * @param timeouts         A flag to indicate that the called wants timeout exceptions
     * @return                 A new Connection object
     *
     * @exception IllegalArgumentException If a parameter is invalid.
     * @exception ConnectionNotFoundException If the connection cannot be found.
     * @exception IOException  If some other kind of I/O error occurs.
     */
    public Connection openPrim(String name, int mode, boolean timeouts) throws IOException {
        open(name, mode, timeouts);
        return this;
    }

    /**
     * Open and return an input stream for a connection.
     *
     * @return                 An input stream
     * @exception IOException  If an I/O error occurs
     */
    public InputStream openInputStream() throws IOException {
        throw new RuntimeException("No openInputStream");
    }

    /**
     * Open and return an output stream for a connection.
     *
     * @return                 An input stream
     * @exception IOException  If an I/O error occurs
     */
    public OutputStream openOutputStream() throws IOException {
        throw new RuntimeException("No openOutputStream");
    }

    /**
     * Open and return a data input stream for a connection.
     *
     * @return                 An input stream
     * @exception IOException  If an I/O error occurs
     */
    public DataInputStream openDataInputStream() throws IOException {
        InputStream is = openInputStream();
        if(is instanceof DataInputStream) {
            return (DataInputStream)is;
        } else {
            return new DataInputStream(is);
        }
    }

    /**
     * Open and return a data output stream for a connection.
     *
     * @return                 An input stream
     * @exception IOException  If an I/O error occurs
     */
    public DataOutputStream openDataOutputStream() throws IOException {
        OutputStream os = openOutputStream();
        if(os instanceof DataOutputStream) {
            return (DataOutputStream)os;
        } else {
            return new DataOutputStream(os);
        }
    }

}

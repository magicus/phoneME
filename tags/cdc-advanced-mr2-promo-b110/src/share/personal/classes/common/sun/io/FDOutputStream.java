/*
 * @(#)FDOutputStream.java	1.37 06/10/10
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

package sun.io;

import java.io.*;

/**
 * A file output stream is an output stream for writing data to a 
 * <code>File</code> or to a <code>FileDescriptor</code>. 
 *
 * @author  Arthur van Hoff
 * @version 1.33, 08/19/02
 * @see     java.io.File
 * @see     java.io.FileDescriptor
 * @see     java.io.FileInputStream
 * @since   JDK1.0
 */
public
class FDOutputStream extends OutputStream {
    static {
        java.security.AccessController.doPrivileged(
            new sun.security.action.LoadLibraryAction("javafile"));
    }
    /**
     * The system dependent file descriptor. The value is
     * 1 more than actual file descriptor. This means that
     * the default value 0 indicates that the file is not open.
     */
    private FileDescriptor fd;
    /**
     * Creates an output file stream to write to the file with the 
     * specified name. 
     *
     * @param      name   the system-dependent filename.
     * @exception  IOException  if the file could not be opened for writing.
     * @exception  SecurityException  if a security manager exists, its
     *               <code>checkWrite</code> method is called with the name
     *               argument to see if the application is allowed write access
     *               to the file.
     * @see        java.lang.SecurityManager#checkWrite(java.lang.String)
     * @since      JDK1.0
     */
    public FDOutputStream(String name) throws IOException {
        SecurityManager security = System.getSecurityManager();
        if (security != null) {
            security.checkWrite(name);
        }
        try {
            fd = new FileDescriptor();
            open(name);
        } catch (IOException e) {
            throw new IOException(name);
        }
    }

    /**
     * Creates an output file with the specified system dependent
     * file name.
     * @param name the system dependent file name 
     * @exception IOException If the file is not found.
     * @since     JDK1.1
     */
    public FDOutputStream(String name, boolean append) throws IOException {
        SecurityManager security = System.getSecurityManager();
        if (security != null) {
            security.checkWrite(name);
        }
        try {
            fd = new FileDescriptor();
            if (append)
                openAppend(name);
            else
                open(name);
        } catch (IOException e) {
            throw new IOException(name);
        }
    }
    
    /**
     * Creates an output file stream to write to the specified file descriptor.
     *
     * @param      fdObj   the file descriptor to be opened for writing.
     * @exception  SecurityException  if a security manager exists, its
     *               <code>checkWrite</code> method is called with the file
     *               descriptor to see if the application is allowed to write
     *               to the specified file descriptor.
     * @see        java.lang.SecurityManager#checkWrite(java.io.FileDescriptor)
     * @since      JDK1.0
     */
    public FDOutputStream(FileDescriptor fdObj) {
        SecurityManager security = System.getSecurityManager();
        if (fdObj == null) {
            throw new NullPointerException();
        }
        if (security != null) {
            security.checkWrite(fdObj);
        }
        fd = fdObj;
    }

    /**
     * Opens a file, with the specified name, for writing.
     * @param name name of file to be opened
     */
    public native void open(String name) throws IOException;

    /**
     * Opens a file, with the specified name, for appending.
     * @param name name of file to be opened
     */
    public native void openAppend(String name) throws IOException;

    /**
     * Writes the specified byte to this file output stream. 
     *
     * @param      b   the byte to be written.
     * @exception  IOException  if an I/O error occurs.
     * @since      JDK1.0
     */
    public native void write(int b) throws IOException;

    /**
     * Writes a sub array as a sequence of bytes.
     * @param b the data to be written
     * @param off the start offset in the data
     * @param len the number of bytes that are written
     * @exception IOException If an I/O error has occurred.
     */
    public native void writeBytes(byte b[], int off, int len) throws IOException;

    /**
     * Writes <code>b.length</code> bytes from the specified byte array 
     * to this file output stream. 
     *
     * @param      b   the data.
     * @exception  IOException  if an I/O error occurs.
     * @since      JDK1.0
     */
    public void write(byte b[]) throws IOException {
        writeBytes(b, 0, b.length);
    }

    /**
     * Writes <code>len</code> bytes from the specified byte array 
     * starting at offset <code>off</code> to this file output stream. 
     *
     * @param      b     the data.
     * @param      off   the start offset in the data.
     * @param      len   the number of bytes to write.
     * @exception  IOException  if an I/O error occurs.
     * @since      JDK1.0
     */
    public void write(byte b[], int off, int len) throws IOException {
        writeBytes(b, off, len);
    }

    /**
     * Closes this file output stream and releases any system resources 
     * associated with this stream. 
     *
     * @exception  IOException  if an I/O error occurs.
     * @since      JDK1.0
     */
    public native void close() throws IOException;

    /**
     * Returns the file descriptor associated with this stream.
     *
     * @return  the file descriptor object associated with this stream.
     * @exception  IOException  if an I/O error occurs.
     * @see        java.io.FileDescriptor
     * @since      JDK1.0
     */
    public final FileDescriptor getFD()  throws IOException {
        if (fd != null) return fd;
        throw new IOException();
    }
    
    /**
     * Ensures that the <code>close</code> method of this file output stream is
     * called when there are no more references to this stream. 
     *
     * @exception  IOException  if an I/O error occurs.
     * @see        java.io.FileInputStream#close()
     * @since      JDK1.0
     */
    protected void finalize() throws IOException {
        if (fd != null) {
            if (fd == fd.out || fd == fd.err) {
                flush();
            } else {
                close();
            }
        }
    }
}

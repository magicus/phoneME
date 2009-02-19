/*
 * @(#)FDInputStream.java	1.44 06/10/10
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
 * A file input stream is an input stream for reading data from a 
 * <code>File</code> or from a <code>FileDescriptor</code>. 
 *
 * @author  Arthur van Hoff
 * @version 1.40, 08/19/02
 * @see     java.io.File
 * @see     java.io.FileDescriptor
 * @see	    java.io.FileOutputStream
 * @since   JDK1.0
 */
public
class FDInputStream extends InputStream {
    static {
        java.security.AccessController.doPrivileged(
            new sun.security.action.LoadLibraryAction("javafile"));
    }
    /* File Descriptor - handle to the open file */
    private FileDescriptor fd;
    /**
     * Creates an input file stream to read from a file with the 
     * specified name. 
     *
     * @param      name   the system-dependent file name.
     * @exception  SecurityException      if a security manager exists, its
     *               <code>checkRead</code> method is called with the name
     *               argument to see if the application is allowed read access
     *               to the file.
     * @see        java.lang.SecurityManager#checkRead(java.lang.String)
     * @since      JDK1.0
     */
    public FDInputStream(String name) throws IOException {
        SecurityManager security = System.getSecurityManager();
        if (security != null) {
            security.checkRead(name);
        }
        try {
            fd = new FileDescriptor();
            open(name);
        } catch (IOException e) {
            throw new IOException(name);
        }
    }
    
    /**
     * Creates an input file stream to read from the specified file descriptor.
     *
     * @param      fdObj   the file descriptor to be opened for reading.
     * @exception  SecurityException  if a security manager exists, its
     *               <code>checkRead</code> method is called with the file
     *               descriptor to see if the application is allowed to read
     *               from the specified file descriptor.
     * @see        java.lang.SecurityManager#checkRead(java.io.FileDescriptor)
     * @since      JDK1.0
     */
    public FDInputStream(FileDescriptor fdObj) {
        SecurityManager security = System.getSecurityManager();
        if (fdObj == null) {
            throw new NullPointerException();
        }
        if (security != null) {
            security.checkRead(fdObj);
        }
        fd = fdObj;
    }

    /**
     * Opens the specified file for reading.
     * @param name the name of the file
     */
    public native void open(String name) throws IOException;

    /**
     * Reads a byte of data from this input stream. This method blocks 
     * if no input is yet available. 
     *
     * @return     the next byte of data, or <code>-1</code> if the end of the
     *             file is reached.
     * @exception  IOException  if an I/O error occurs.
     * @since      JDK1.0
     */
    public native int read() throws IOException;

    /** 
     * Reads a subarray as a sequence of bytes. 
     * @param b the data to be written
     * @param off the start offset in the data
     * @param len the number of bytes that are written
     * @exception IOException If an I/O error has occurred. 
     */ 
    public native int readBytes(byte b[], int off, int len) throws IOException;

    /**
     * Reads up to <code>b.length</code> bytes of data from this input 
     * stream into an array of bytes. This method blocks until some input 
     * is available. 
     *
     * @param      b   the buffer into which the data is read.
     * @return     the total number of bytes read into the buffer, or
     *             <code>-1</code> if there is no more data because the end of
     *             the file has been reached.
     * @exception  IOException  if an I/O error occurs.
     * @since      JDK1.0
     */
    public int read(byte b[]) throws IOException {
        return readBytes(b, 0, b.length);
    }

    /**
     * Reads up to <code>len</code> bytes of data from this input stream 
     * into an array of bytes. This method blocks until some input is 
     * available. 
     *
     * @param      b     the buffer into which the data is read.
     * @param      off   the start offset of the data.
     * @param      len   the maximum number of bytes read.
     * @return     the total number of bytes read into the buffer, or
     *             <code>-1</code> if there is no more data because the end of
     *             the file has been reached.
     * @exception  IOException  if an I/O error occurs.
     * @since      JDK1.0
     */
    public int read(byte b[], int off, int len) throws IOException {
        return readBytes(b, off, len);
    }

    /**
     * Skips over and discards <code>n</code> bytes of data from the 
     * input stream. The <code>skip</code> method may, for a variety of 
     * reasons, end up skipping over some smaller number of bytes, 
     * possibly <code>0</code>. The actual number of bytes skipped is returned.
     *
     * @param      n   the number of bytes to be skipped.
     * @return     the actual number of bytes skipped.
     * @exception  IOException  if an I/O error occurs.
     * @since      JDK1.0
     */
    public native long skip(long n) throws IOException;

    /**
     * Returns the number of bytes that can be read from this file input
     * stream without blocking.
     *
     * @return     the number of bytes that can be read from this file input
     *             stream without blocking.
     * @exception  IOException  if an I/O error occurs.
     * @since      JDK1.0
     */
    public native int available() throws IOException;

    /**
     * Closes this file input stream and releases any system resources 
     * associated with the stream. 
     *
     * @exception  IOException  if an I/O error occurs.
     * @since      JDK1.0
     */
    public native void close() throws IOException;

    /**
     * Returns the opaque file descriptor object associated with this stream.
     *
     * @return     the file descriptor object associated with this stream.
     * @exception  IOException  if an I/O error occurs.
     * @see        java.io.FileDescriptor
     * @since      JDK1.0
     */
    public final FileDescriptor getFD() throws IOException {
        if (fd != null) return fd;
        throw new IOException();
    }

    /**
     * Ensures that the <code>close</code> method of this file input stream is
     * called when there are no more references to it. 
     *
     * @exception  IOException  if an I/O error occurs.
     * @see        java.io.FileInputStream#close()
     * @since      JDK1.0
     */
    protected void finalize() throws IOException {
        if (fd != null) {
            if (fd != fd.in) {
                close();
            }
        }
    }
}

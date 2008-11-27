/*
 * @(#)ProtocolNative.java	1.8 06/10/10
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

/*
 * NOTE - This code is written a somewhat unusual, and not very object oriented, way.
 * The principal reason for this is that we intend to translate much of this
 * protocol into C and place it in the KVM kernel where is will be much smaller
 * and faster.
 */

package com.sun.cdc.io.j2me.file;

import java.io.*;
import javax.microedition.io.*;

/**
 * This implements the default "file:" protocol J2ME
 * <p>
 * This class represents the native functions that will be written in C for KVM.
 * For now this Java version is used instead.
 *
 * @version 2.0 2/21/2000
 */
public abstract class ProtocolNative extends ProtocolBase {

    /* Read mode */
    public boolean reading;

    /* Write mode */
    public boolean writing;

    /* Append mode */
    boolean append = false;

    /* Need to open flag */
    boolean needToOpen = true;

    /*
     * open0
     *
     * @param name the target of the connection
     * @param parms the target parameters
     * @param mode the access mode
     */
    public Connection open0(String name, String parms, int mode) throws IOException {

//System.out.println("open0 |"+name+"| parms |"+parms+"|");

        if(name.length() >= 2 && name.charAt(0) == '/' && name.charAt(1) == '/') {
            if(name.length() >= 3) {
                if(name.charAt(2) != '/') {
                    throw new IllegalArgumentException("Network format not implemented "+name);
                } else {
                    name = name.substring(2);
                }
            } else {
                throw new IllegalArgumentException("Invalid file name "+name);
            }
        }

        reading = (mode&Connector.READ) != 0;
        writing = (mode&Connector.WRITE) != 0;

        if(parms.length() > 0) {
            if(parms.equals("append=true")) {
                append = true;
            } else {
                throw new IllegalArgumentException("Unknown parameters "+parms);
            }
        }

        if(append && reading) {
            throw new IllegalArgumentException("Cannot open file for reading and appending");
        }

        return prim_openProtocol(name, parms, mode);
    }

    /*
     * close0
     */
    public void close0() throws IOException {
        prim_closeProtocol();
    }

    /*
     * ensurePrimOpen
     */
    void ensurePrimOpen() throws IOException {
        testSelection();
        if(needToOpen) {
            if(!prim_isDirectory()) {
                reallyOpen();
            }
            needToOpen = false;
        }
    }

    /*
     * reallyOpen
     */
    void reallyOpen()  throws IOException {
        boolean exists  = prim_exists();

        if(append) {
            if(reading) {
                throw new RuntimeException("Internal error - prim_open() append && reading");
            }
            if(!writing) {
                throw new RuntimeException("Internal error - prim_open() append && !writing");
            }
            /* Must be writing */
            prim_realOpen();
            prim_seek(prim_lengthOf());
            return;
        }

        if(reading && !writing) {
            if(!exists) {
                throw new RuntimeException("Internal error - prim_open() reading && !exists");
            }
            prim_realOpen();
            return;
        }

        if(writing && !reading) {
            if(exists) {
                prim_deleteItem();
            }
            prim_realOpen();
            return;
        }

        if(reading && writing) {
            prim_realOpen();
            return;
        }

        throw new RuntimeException("Internal error - prim_open() !writing && !reading");
    }


    /*
     * clearSelection
     */
    public void clearSelection() throws IOException {
        needToOpen = true;
        prim_clearSelection();
    }

    /*
     * getAvailableSpace0
     */
    public long getAvailableSpace0() throws IOException {
        return prim_availableSpace();
    }

    /*
     * getItemCount0
     */
    public int getItemCount0() throws IOException {
        return prim_countItems();
    }

    /*
     * selectFirstItem0
     */
    public boolean selectFirstItem0() throws IOException {
        clearSelection();
        prim_findFirstItemAndSelect();
        return isSelected0();
    }

    /*
     * selectNextItem0
     */
    public boolean selectNextItem0() throws IOException {

        // can't use clearSelection() here
        boolean res = false;

        try {
            prim_findItemAfterSelectionAndSelect();
            res = isSelected0();
        } finally {
            clearSelection();
        }
        return res;
    }

    /*
     * selectItem0
     */
    public boolean selectItem0(String name) throws IOException {
        clearSelection();
        prim_findItemAndSelect(name);
        return isSelected0();
    }

    /*
     * selectItemByInt0
     */
    public boolean selectItemByInt0(int i) throws IOException {
        clearSelection();
        prim_findItemAndSelectByInt(i);
        return isSelected0();
    }

    /*
     * deselectItem0
     */
    public void deselectItem0() throws IOException {
        clearSelection();
    }

    /*
     * isSelected0
     */
    public boolean isSelected0() throws IOException {
        return prim_isSelected();
    }

    /*
     * create0
     */
    public void create0() throws IOException {
        clearSelection();
        prim_createAndSelect();
        if(!isSelected0()) {
            throw new IOException("Internal error - exception not thrown");
        }
    }

    /*
     * createName0
     */
    public void createName0(String name) throws IOException {
        clearSelection();
        prim_createFileAndSelect(name);
        if(!isSelected0()) {
            throw new IOException("Internal error - exception not thrown");
        }
    }

    /*
     * createName0
     */
    public void createNameByInt0(int i) throws IOException {
        clearSelection();
        prim_createFileAndSelectByInt(i);
        if(!isSelected0()) {
            throw new IOException("Internal error - exception not thrown");
        }
    }

    /*
     * createDirectory0
     */
    public void createDirectory0(String name) throws IOException {
        clearSelection();
        prim_createDirectoryAndSelect(name);
        if(!isSelected0()) {
            throw new IOException("Internal error - exception not thrown");
        }
    }

    /*
     * testSelection
     */
    void testSelection() throws IOException {
        if(!isSelected0()) {
            throw new IOException("Internal error - nothing selected");
        }
    }

    /*
     * delete0
     */
    public void delete0() throws IOException {
        testSelection();
        prim_deleteItem();
    }

    /*
     * rename0
     */
    public void rename0(String name2) throws IOException {
        testSelection();
        prim_renameFile(name2);
    }

    /*
     * renameByInt0
     */
    public void renameByInt0(int i) throws IOException {
        testSelection();
        prim_renameFileByInt(i);
    }

    /*
     * renameDirectory0
     */
    public void renameDirectory0(String name2) throws IOException {
        testSelection();
        prim_renameDirectory(name2);
    }

    /*
     * getLength0
     */
    public long getLength0() throws IOException {
        testSelection();
        return prim_lengthOf();
    }

    /*
     * setLength0
     */
    public void setLength0(long len) throws IOException {
        testSelection();
        prim_setLength(len);
    }

    /*
     * getModificationDate0
     */
    public long getModificationDate0() throws IOException {
        testSelection();
        return prim_timeOf();
    }

    /*
     * getItemName0
     */
    public String getItemName0() throws IOException {
        testSelection();
        return prim_getSelectionName();
    }

    /*
     * getItemNumber0
     */
    public int getItemNumber0() throws IOException {
        testSelection();
        return prim_getSelectionNumber();
    }

    /*
     * isDirectory0
     */
    public boolean isDirectory0() throws IOException {
        testSelection();
        return prim_isDirectory();
    }

    /*
     * canRead0
     */
    public boolean canRead0() throws IOException {
        testSelection();
        return prim_canRead();
    }

    /*
     * canWrite0
     */
    public boolean canWrite0() throws IOException {
        testSelection();
        return prim_canWrite();
    }

    /*
     * setReadable0
     */
    public void setReadable0(boolean tf) throws IOException {
        testSelection();
        if(!prim_setReadable(tf)) {
            throw new SecurityException();
        }
    }

    /*
     * setWritable0
     */
    public void setWritable0(boolean tf) throws IOException {
        testSelection();
        if(!prim_setWritable(tf)) {
            throw new SecurityException();
        }
    }

    /*
     * available0
     */
    public int available0() throws IOException {
        ensurePrimOpen();
        return (int)(getLength0() - getPosition0());
    }

    /*
     * seek0
     */
    public void seek0(long pos) throws IOException {
        ensurePrimOpen();
        prim_seek(pos);
    }

    /*
     * getPosition0
     */
    public long getPosition0() throws IOException {
        ensurePrimOpen();
        return prim_getPosition();
    }

    /*
     * read0
     */
    public int read0() throws IOException {
        ensurePrimOpen();
        return prim_read();
    }

    /*
     * readBytes0
     */
    public int readBytes0(byte b[], int off, int len) throws IOException {
        ensurePrimOpen();
        return prim_readBytes(b, off, len);
    }

    /*
     * write0
     */
    public void write0(int b) throws IOException {
        ensurePrimOpen();
        prim_write(b);
    }

    /*
     * writeBytes0
     */
    public void writeBytes0(byte b[], int off, int len) throws IOException {
        ensurePrimOpen();
        prim_writeBytes(b, off, len);
    }

    /*
     * Real primitive methods
     */
    abstract public Connection prim_openProtocol(String name, String parms, int mode) throws IOException;
    abstract public void    prim_closeProtocol() throws IOException;
    abstract public void    prim_realOpen() throws IOException;
    abstract public void    prim_clearSelection() throws IOException;
    abstract public String  prim_getSelectionName() throws IOException;
    abstract public int     prim_getSelectionNumber() throws IOException;
    abstract public boolean prim_isSelected() throws IOException;
    abstract public void    prim_findFirstItemAndSelect() throws IOException;
    abstract public void    prim_findItemAndSelect(String name) throws IOException;
    abstract public void    prim_findItemAndSelectByInt(int i) throws IOException;
    abstract public void    prim_findItemAfterSelectionAndSelect() throws IOException;
    abstract public long    prim_availableSpace() throws IOException;
    abstract public int     prim_countItems() throws IOException;
    abstract public void    prim_createAndSelect() throws IOException;
    abstract public void    prim_createFileAndSelect(String name) throws IOException;
    abstract public void    prim_createFileAndSelectByInt(int i) throws IOException;
    abstract public void    prim_createDirectoryAndSelect(String name) throws IOException;
    abstract public boolean prim_deleteItem() throws IOException;
    abstract public boolean prim_renameFile(String name2) throws IOException;
    abstract public boolean prim_renameFileByInt(int i) throws IOException;
    abstract public boolean prim_renameDirectory(String name2) throws IOException;
    abstract public long    prim_lengthOf() throws IOException;
    abstract public void    prim_setLength(long len) throws IOException;
    abstract public long    prim_timeOf() throws IOException;
    abstract public boolean prim_exists() throws IOException;
    abstract public boolean prim_isDirectory() throws IOException;
    abstract public boolean prim_canRead() throws IOException;
    abstract public boolean prim_canWrite() throws IOException;
    abstract public boolean prim_setReadable(boolean tf) throws IOException;
    abstract public boolean prim_setWritable(boolean tf) throws IOException;
    abstract public void    prim_seek(long pos) throws IOException;
    abstract public long    prim_getPosition() throws IOException;
    abstract public int     prim_read() throws IOException;
    abstract public int     prim_readBytes(byte b[], int off, int len) throws IOException;
    abstract public void    prim_write(int b) throws IOException;
    abstract public void    prim_writeBytes(byte b[], int off, int len) throws IOException;

}

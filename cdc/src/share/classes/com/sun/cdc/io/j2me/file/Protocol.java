/*
 * @(#)Protocol.java	1.13 06/10/10
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
 * This implements the default "file:" protocol J2SE
 * <p>
 * This class represents the internal functions that are written in C for KVM
 * in the file storagePrim.c
 *
 * @version 2.0 2/21/2000
 */
public class Protocol extends ProtocolNative {

    /* File handle */
    private FileInputStream fis;

    /* The directory opened */
    private String _directory_;

    /* Currently selected file */
    private String _selection_;

    /*
     * setDirectory
     */
    private void setDirectory(String name) {
        if(name.length() == 0) {
            _directory_ = "";
        } else if(!name.endsWith("/")) {
            _directory_ = name + "/";
        } else {
            _directory_ = name;
        }
        setProtocolBaseDirectory(_directory_);
    }

    /*
     * getDirectory
     */
    protected String getDirectory() {
        return _directory_;
    }

    /*
     * formName
     */
    public String formName(String name) {
        return getDirectory()+name;
    }

    /*
     * prim_initialise
     */
    public Connection prim_openProtocol(String openName, String parms, int mode) throws IOException {

        setDirectory(""); /* Just so prim_exists() will work */

        if(prim_exists(openName) && prim_isDirectory(openName)) {
            setDirectory(openName);
            setSelection(null);
        } else {
            if(!prim_exists(openName) && !writing) {
                throw new ConnectionNotFoundException("No such file "+openName);
            }
            int slash = openName.lastIndexOf('/');
            if(slash == -1) {
                setDirectory("");
                setSelection(openName);
            } else {
                setDirectory(openName.substring(0, slash+1));
                String n = openName.substring(slash+1);
                if(n.length() > 0) {
                    setSelection(n);
                }
            }
            // Check for SecurityManager.checkWrite(filename) permission
            // only for WRITE and READ_WRITE mode.
            // NOTE: this will cause a java.lang.SecurityException if
            //       no permission for writing.
            if (mode != Connector.READ) {
                prim_canWrite();
                
                // Check for SecurityManager.checkRead(filename) permission
                // only for READ or READ_WRITE mode.
                // NOTE: this will cause a java.lang.SecurityException if
                //       no permission for reading.
            } else if ((mode == Connector.READ) || 
                       (mode == Connector.READ_WRITE)) {
	       	java.lang.SecurityManager sm = System.getSecurityManager();
		if ((sm != null) && (openName != null)) {
                        sm.checkRead(openName);
		}
            }
        }
        return this;
    }


    /*
     * prim_closeProtocol
     */
    public void prim_closeProtocol() throws IOException {
        prim_close();
        _directory_ = null;
        _selection_ = null;
    }

    /*
     * prim_realOpen
     */
    public void prim_realOpen() throws IOException {
        String name = formName(prim_getSelectionName());
        fis = new FileInputStream(name);
    }

    /*
     * prim_close
     */
    void prim_close() throws IOException {
        if(fis != null) {
            FileInputStream f = fis;
            fis = null;
            f.close();
        }
    }

    /*
     * setSelection
     */
    private void setSelection(String thing) throws IOException {
        _selection_ = thing;
        if(thing == null) {
            prim_close();
        }
    }

    /*
     * prim_clearSelection
     */
    public void prim_clearSelection() throws IOException {
        setSelection(null);
    }

    /*
     * prim_isSelected
     */
    public boolean prim_isSelected() {
        return _selection_ != null;
    }

    /*
     * getSelectionName
     */
    public String prim_getSelectionName() {
        return (String)_selection_;
    }

    /*
     * getSelectionNumber
     */
    public int prim_getSelectionNumber() throws IOException {
        try {
            return Integer.parseInt(prim_getSelectionName());
        } catch(NumberFormatException x) {
            throw new IOException("NumberFormatException");
        }
    }

    public void prim_findFirstItemAndSelect() throws IOException {
        setSelection(prim_findFirstItem());
    }

    public void prim_findItemAndSelect(String name) throws IOException {
        setSelection(prim_findItem(name));
    }

    public void prim_findItemAndSelectByInt(int i) throws IOException {
        setSelection(prim_findItem(String.valueOf(i)));
    }

    public void prim_findItemAfterSelectionAndSelect() throws IOException {
        setSelection(prim_findItemAfter(prim_getSelectionName()));
    }

    public void prim_createAndSelect() throws IOException {
        setSelection(prim_create());
    }

    public void prim_createFileAndSelect(String name) throws IOException {
        setSelection(prim_createFile(name));
    }

    public void prim_createFileAndSelectByInt(int i) throws IOException {
        setSelection(prim_createFile(String.valueOf(i)));
    }

    public void prim_createDirectoryAndSelect(String name) throws IOException {
        setSelection(prim_createDirectory(name));
    }

    public boolean prim_deleteItem() {
        return prim_deleteItem(prim_getSelectionName());
    }

    public boolean prim_renameFile(String name2) {
        return prim_renameItem(prim_getSelectionName(), name2);
    }

    public boolean prim_renameFileByInt(int i) {
        return prim_renameItem(prim_getSelectionName(), String.valueOf(i));
    }

    public boolean prim_renameDirectory(String name2) {
        return prim_renameItem(prim_getSelectionName(), name2);
    }

    public long prim_lengthOf() throws IOException {
        return prim_lengthOf(prim_getSelectionName());
    }

    public void prim_setLength(long len) throws IOException {
        throw new RuntimeException("prim_setLength not implemented");
    }

    public long prim_timeOf() throws IOException {
        return prim_timeOf(prim_getSelectionName());
    }

    public boolean prim_exists() {
        return prim_exists(prim_getSelectionName());
    }

    public boolean prim_isDirectory() {
        return prim_isDirectory(prim_getSelectionName());
    }

    public boolean prim_canRead() {
        return prim_canRead(prim_getSelectionName());
    }

    public boolean prim_canWrite() {
        return prim_canWrite(prim_getSelectionName());
    }

    public boolean prim_setReadable(boolean tf) {
        return prim_setReadable(prim_getSelectionName(), tf);
    }

    public boolean prim_setWritable(boolean tf) {
        return prim_setWritable(prim_getSelectionName(), tf);
    }


    /*
     * Real primitive methods
     */

   /*
     * prim_findFirstItem
     */
    String prim_findFirstItem() {
        File file = new File(getDirectory());
        if(!file.exists()) {
            return null;
        }
        String[] s = file.list();
        if(s.length == 0) {
            return null;
        }
        return s[0];
    }

    /*
     * prim_findItem
     */
    String prim_findItem(String name) {
        File file = new File(formName(name));
        if(!file.exists()) {
            return null;
        }
        return name;
    }

    /*
     * prim_findItemAfter
     */
    String prim_findItemAfter(String name) {
        File file = new File(getDirectory());
        if(!file.exists()) {
            return null;
        }
        String[] s = file.list();
        for(int i = 0 ; i < s.length-2 ; i++) {
            if(s[i].equals(name)) {
                return s[1+1];
            }
        }
        return null;
    }

    /*
     * prim_availableSpace
     */
    public long prim_availableSpace() {
        return Long.MAX_VALUE; //TEMP
    }

    /*
     * prim_countItems
     */
    public int prim_countItems() {
        File file = new File(getDirectory());
        if(!file.exists()) {
            return 0;
        }
        String[] s = file.list();
        return s.length;
    }

    /*
     * prim_create
     */
    String prim_create() throws IOException {
        File d = new File(getDirectory());
        File f = File.createTempFile("tmp", null, d);
        return f.getName();
    }

    /*
     * prim_createFile
     */
    String prim_createFile(String name)  throws IOException {
        FileOutputStream fis;
        if(prim_exists(name)) {
            throw new IOException("prim_createFile - file exists"+name);
        }
        fis = new FileOutputStream(formName(name));
        fis.close();
        return name;
    }

    /*
     * prim_createDirectory
     */
    String prim_createDirectory(String name)  throws IOException {
        if(new File(formName(name)).mkdir()) {
            return name;
        }
        throw new IOException("Could not prim_createDirectory()");
    }

    /*
     * prim_deleteItem
     */
    boolean prim_deleteItem(String name) {
        boolean res = new File(formName(name)).delete();

        if(res && prim_exists(name)) {
            throw new RuntimeException("Internal error - prim_deleteItem did not delete");
        }

        return res;
    }

    /*
     * prim_renameItem
     */
    boolean prim_renameItem(String name, String name2) {
        File to = new File(name2);
        return new File(formName(name)).renameTo(to);
    }

    /*
     * prim_lengthOf
     */
    long prim_lengthOf(String name) throws IOException {
        if(fis != null) {
            throw new IOException();
        } else {
            return new File(formName(name)).length();
        }
    }

    /*
     * prim_timeOf
     */
    long prim_timeOf(String name) {
        if(fis != null) {
            return System.currentTimeMillis();
        } else {
           return new File(formName(name)).lastModified();
        }
    }

    /*
     * prim_exists
     */
    boolean prim_exists(String name) {
        return new File(formName(name)).exists();
    }

    /*
     * prim_isDirectory
     */
    boolean prim_isDirectory(String name) {
        return new File(formName(name)).isDirectory();
    }

    /*
     * prim_canRead
     */
    boolean prim_canRead(String name) {
        return new File(formName(name)).canRead();
    }

    /*
     * prim_canWrite
     */
    boolean prim_canWrite(String name) {
        return new File(formName(name)).canWrite();
    }

    /*
     * prim_setReadable
     */
    boolean prim_setReadable(String name, boolean tf) {
        return false;
    }

    /*
     * prim_setWritable
     */
    boolean prim_setWritable(String name, boolean tf) {
        return false;
    }

    /*
     * seek0
     */
    public void prim_seek(long pos) throws IOException {
        throw new IOException();
    }

    /*
     * getPosition0
     */
    public long prim_getPosition() throws IOException {
        throw new IOException();
    }

    /*
     * prim_read
     */
    public int prim_read() throws IOException {
        return fis.read();
    }

    /*
     * prim_readBytes
     */
    public int prim_readBytes(byte b[], int off, int len) throws IOException {
        return fis.read(b, off, len);
    }

    /*
     * prim_writeBytes
     */
    public void prim_write(int b) throws IOException {
        throw new IOException();
    }

    /*
     * prim_write
     */
    public void prim_writeBytes(byte b[], int off, int len) throws IOException {
        throw new IOException();
    }

/*
    public  native long    prim_availableSpace();
    public  native int     prim_countItems();
    private native String  prim_create() throws IOException;
    private native String  prim_createFile(String name) throws IOException;
    private native String  prim_createDirectory(String name) throws IOException;
    private native boolean prim_deleteItem(String name);
    private native boolean prim_renameItem(String name, String name2);
    private native long    prim_lengthOf(String name) throws IOException;
    private native long    prim_timeOf(String name);
    private native boolean prim_exists(String name);
    private native boolean prim_isDirectory(String name);
    private native boolean prim_canRead(String name);
    private native boolean prim_canWrite(String name);
    private native boolean prim_setReadable(String name, boolean tf);
    private native boolean prim_setWritable(String name, boolean tf);
    public  native void    prim_seek(long pos) throws IOException;
    public  native long    prim_getPosition() throws IOException;
    public  native int     prim_read() throws IOException;
    public  native int     prim_readBytes(byte b[], int off, int len) throws IOException;
    public  native void    prim_write(int b) throws IOException;
    public  native void    prim_writeBytes(byte b[], int off, int len) throws IOException;
*/
}

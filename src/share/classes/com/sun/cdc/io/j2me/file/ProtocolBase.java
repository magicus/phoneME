/*
 * @(#)ProtocolBase.java	1.13 06/10/10
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
import com.sun.cdc.io.*;
import com.sun.cdc.io.j2me.file.*;
import java.lang.SecurityManager;

/**
 * This implements the default "file:" protocol J2ME
 *
 * @version 2.0 2/20/2000
 */
public abstract class ProtocolBase extends ConnectionBase implements InputConnection, OutputConnection, StreamConnection {

    /** The string used to open */
    private boolean isOpen = true;

    /** Keep track of the directory */
    private String protocolBaseDirectory = null;

    /** Open count */
    private int opens = 1;

    /** Open mode */
    protected int openMode;

    /** Operation mode */
    protected static final int O_RAND  = 0;
    protected static final int O_READ  = 1;
    protected static final int O_WRITE = 2;
    protected int operationMode = O_RAND;


/**********************************************************************\
 *                     Methods from Connection                        *
\**********************************************************************/

    /**
     * @param name the target of the connection
     * @param mode the access mode
     * @param timeouts A flag to indicate that the called wants timeout exceptions
     */
    public void open(String name, int mode, boolean timeouts) throws IOException {
       throw new RuntimeException();
    }

    /*
     * Check permissions and throw a SecurityException if the
     * permission check fails. The CDC prim_OpenProtocol method
     * will be invoked directly and does the proper SecurityManager
     * check, so the CDC version of this method can be empty.
     * Override in the MIDP protocol handler to make the proper
     * MIDP security check.
    */
    protected void checkPermission(String name, String params, int mode) {
        return;
    }    

    /*
     * Check permission when opening an OutputStream. MIDP
     * versions of the protocol handler should override this
     * with an empty method.
     */
    protected void outputStreamPermissionCheck() throws IOException {
        // Check for SecurityManager permission
        java.lang.SecurityManager sm = System.getSecurityManager();
        String itemName = getItemName();
        if (itemName != null){
            String tmpName = protocolBaseDirectory+itemName;
            String name = (new File(tmpName)).getAbsolutePath();
            if (sm != null) {
                sm.checkWrite(name);
            }
        }
        return;
    }

    /*
     * Check permission when opening an InputStream. MIDP
     * versions of the protocol handler should override this
     * with an empty method. A SecurityException will be
     * raised if the connection is not allowed. Currently the
     * file protocol handler does not make a permission
     * check at this point so this method is empty.
     */
    protected void inputStreamPermissionCheck() {
        return;
    }

    /**
     * @param name the target of the connection
     * @param mode the access mode
     * @param timeouts A flag to indicate that the called wants timeout exceptions
     */
    public Connection openPrim(String name, int mode, boolean timeouts) throws IOException {
        openMode = mode;
        int semi = name.indexOf(';');
        if(semi == -1) {
            checkPermission(name, null, mode);
             return open0(name, "", mode);
        } else {
             if(name.endsWith(";")) {
                 throw new IllegalArgumentException("Bad options "+name);
             }
             String params = name.substring(semi+1);
             checkPermission(name, params, mode);
             return open0(name.substring(0, semi), params, mode);
        }
    }

    protected void ensureOpen() throws IOException {
        if(!isOpen) {
            throw new IOException("Connection closed");
        }
        if(operationMode != O_RAND) {
            throw new IOException("Open mode conflict");
        }
    }

    protected void ensureOpenForReading() throws IOException {
        ensureOpen();
        if((openMode&Connector.READ) == 0) {
            throw new SecurityException("Connection not open for reading");
        }
    }

    protected void ensureOpenForWriting() throws IOException {
        ensureOpen();
        if((openMode&Connector.WRITE) == 0) {
            throw new SecurityException("Connection not open for writing");
        }
    }

    protected void ensureOpenAndSelected() throws IOException {
        ensureOpen();
        if(!isSelected0()) {
            throw new IOException("No selection");
        }
    }

    protected void ensureOpenForReadingAndSelected() throws IOException {
        ensureOpenForReading();
        if(!isSelected0()) {
            throw new IOException("No selection");
        }
    }

    protected void ensureOpenForWritingAndSelected() throws IOException {
        ensureOpenForWriting();
        if(!isSelected0()) {
            throw new IOException("No selection");
        }
    }

    protected void ensureDirectory() throws IOException {
        if(!isDirectory()) {
            throw new IOException("Selection not a directory");
        }
    }

    protected void ensureNotDirectory() throws IOException {
        if(isDirectory()) {
            throw new IOException("Selection is a directory");
        }
    }

    /**
     * Close the connection.
     *
     * @exception  IOException  if an I/O error occurs when closing the
     *                          connection.
     */
    public void close() throws IOException {
        if(isOpen) {
            isOpen = false;
            realClose();
         }
    }

    protected void setProtocolBaseDirectory(String name) {
	protocolBaseDirectory = name;
    }


    /**
     * Close the connection.
     *
     * @exception  IOException  if an I/O error occurs.
     */
    void realClose() throws IOException {
        if(--opens == 0) {
             close0();
        }
    }

/**********************************************************************\
 *                     Methods from InputConnection                   *
\**********************************************************************/

    /**
     * Returns an input stream for a database record
     *
     * @return     an input stream for reading bytes from this record.
     * @exception  IOException  if an I/O error occurs when creating the
     *                          input stream.
     */
    public InputStream openInputStream() throws IOException {
	inputStreamPermissionCheck();
        ensureOpenForReadingAndSelected();
        ensureNotDirectory();
        opens++;
        operationMode = O_READ;
        return new PrivateFileInputStream(this);
    }

/**********************************************************************\
 *                     Methods from OutputConnection                  *
\**********************************************************************/

    /**
     * Returns an output stream for this socket.
     *
     * @param      True if appending
     * @return     an output stream for writing bytes to this socket.
     * @exception  IOException  if an I/O error occurs when creating the
     *                          output stream.
     */
    public OutputStream openOutputStream() throws IOException {
	outputStreamPermissionCheck();
        ensureOpenForWritingAndSelected();
        ensureNotDirectory();
        opens++;
        operationMode = O_WRITE;
        return new PrivateFileOutputStream(this);
    }


/**********************************************************************\
 *                 Methods from RandomAccessConnection                *
\**********************************************************************/

//
// Selection
//

    /**
     * Test to see if a record in the collection is selected. Until this is done none of the
     * data access methods below will work. Some random access connection are always
     * selected, but other ones, such as a MetadataConncetion may require a specific
     * record to be selected before I/O can commence.
     *
     * @return true if the connection is selected, otherwise false.
     */
    public boolean isSelected() throws IOException {
        ensureOpen();
        return isSelected0();
    }

//
// Seeking
//

    /**
     * Sets the position pointer offset, measured from the beginning of the
     * data, at which the next read or write occurs.  The offset may be
     * set beyond the end of the data. Setting the offset beyond the end
     * of the data does not change the data length.  The data length will
     * change only by writing after the offset has been set beyond the end
     * of the data.
     *
     * @param      pos   the offset position, measured in bytes from the
     *                   beginning of the data, at which to set the position
     *                   pointer.
     * @exception  IOException  if <code>pos</code> is less than
     *                          <code>0</code>, if an I/O error occurs, or
     *                          there is an input or output stream open on the data.
     */
    public void seek(long pos) throws IOException {
        ensureOpenAndSelected();
        ensureNotDirectory();
        seek0(pos);
    }

    /**
     * Returns the current offset into the data.
     *
     * @return     the offset from the beginning of the data, in bytes,
     *             at which the next read or write occurs.
     * @exception  IOException  if an I/O error occurs.
     */
    public long getPosition() throws IOException {
        ensureOpenAndSelected();
        ensureNotDirectory();
        return getPosition0();
    }

//
// Size management
//

    /**
     * Get the length of the data
     *
     * @return the length of the data
     */
    public long getLength() throws IOException {
        ensureOpenForReadingAndSelected();
        ensureNotDirectory();
        return getLength0();
    }

    /**
     * Set the length of the data (for truncation).
     *
     * @param len the new length of the data
     */
    public void setLength(long len) throws IOException {
        ensureOpenForWritingAndSelected();
        ensureNotDirectory();
        setLength0(len);
    }

//
// I/O
//

    /**
     * Reads a byte of data.  The byte is returned as an
     * integer in the range 0 to 255 (<code>0x00-0x0ff</code>). This
     * method blocks if no input is yet available.
     * <p>
     * @return     the next byte of data, or <code>-1</code> if the end of the
     *             data has been reached.
     * @exception  IOException  if an I/O error occurs. Not thrown if
     *                          end-of-data has been reached.
     */
    public int read() throws IOException {
        ensureOpenForReadingAndSelected();
        ensureNotDirectory();
        return read0();
    }

    /**
     * Reads up to <code>len</code> bytes of data into an
     * array of bytes. This method blocks until at least one byte of input
     * is available.
     *
     * @param      b     the buffer into which the data is read.
     * @param      off   the start offset of the data.
     * @param      len   the maximum number of bytes read.
     * @return     the total number of bytes read into the buffer, or
     *             <code>-1</code> if there is no more data because the end of
     *             the data has been reached.
     * @exception  IOException  if an I/O error occurs.
     */
    public int read(byte b[], int off, int len) throws IOException {
        ensureOpenForReadingAndSelected();
        ensureNotDirectory();
        return read0(b, off, len);
    }

    /**
     * Reads up to <code>len</code> bytes of data into an
     * array of bytes. This method blocks until at least one byte of input
     * is available.
     *
     * @param      b     the buffer into which the data is read.
     * @param      off   the start offset of the data.
     * @param      len   the maximum number of bytes read.
     * @return     the total number of bytes read into the buffer, or
     *             <code>-1</code> if there is no more data because the end of
     *             the data has been reached.
     * @exception  IOException  if an I/O error occurs.
     */
    public int read0(byte b[], int off, int len) throws IOException {
        if (b == null) {
            throw new NullPointerException();
        } else if ((off < 0) || (off > b.length) || (len < 0) ||
                   ((off + len) > b.length) || ((off + len) < 0)) {
            throw new IndexOutOfBoundsException();
        }
        if (len == 0) {
            return 0;
        }
        return readBytes0(b, off, len);
    }

    /**
     * Writes the specified byte to this file. The write starts at
     * the current position pointer.
     *
     * @param      b   the <code>byte</code> to be written.
     * @exception  IOException  if an I/O error occurs.
     */
    public void write(int b) throws IOException {
        ensureOpenForWritingAndSelected();
        ensureNotDirectory();
        write0(b);
    }

    /**
     * Writes <code>len</code> bytes from the specified byte array
     * starting at offset <code>off</code> to the data
     *
     * @param      b     the data.
     * @param      off   the start offset in the data.
     * @param      len   the number of bytes to write.
     * @exception  IOException  if an I/O error occurs.
     */
    public void write(byte b[], int off, int len) throws IOException {
        ensureOpenForWritingAndSelected();
        ensureNotDirectory();
        write0(b, off, len);
    }

    /**
     * Writes <code>len</code> bytes from the specified byte array
     * starting at offset <code>off</code> to the data
     *
     * @param      b     the data.
     * @param      off   the start offset in the data.
     * @param      len   the number of bytes to write.
     * @exception  IOException  if an I/O error occurs.
     */
    void write0(byte b[], int off, int len) throws IOException {
        if ((off < 0) || (off > b.length) || (len < 0) ||
            ((off + len) > b.length) || ((off + len) < 0)) {
            throw new IndexOutOfBoundsException();
        }
        if (len == 0) {
            return;
        }
        writeBytes0(b, off, len);
    }


/**********************************************************************\
 *                   Methods from DirectoryConnection                 *
\**********************************************************************/


//
// Collection size functions
//

    /**
     * Return the size in bytes that the collection can grow to
     *
     * @return  size
     */
    public long getAvailableSpace() throws IOException {
        ensureOpen();
        return getAvailableSpace0();
    }

    /**
     * Return the number of items in the collection
     *
     * @return  size
     */
    public int getItemCount() throws IOException {
        ensureOpen();
        return getItemCount0();
    }

//
// Item selection
//

    /**
     * Select the first record int the database
     *
     * @return true if there was a record false if the collection was empty
     * @exception  IOException  if an I/O error occurs.
     */
    public boolean selectFirstItem() throws IOException {
        ensureOpen();
        return selectFirstItem0();
    }

    /**
     * Select the next record int the database
     *
     * @return true if there was another record false otherwise
     * @exception  IOException  if an I/O error occurs.
     */
    public boolean selectNextItem() throws IOException {
        ensureOpen();
        return selectNextItem0();
    }

    /**
     * Select an item in the collection
     * @param name the name of the item to select
     * @return true if the record exists
     * @exception  IOException  if an I/O error occurs.
     */
    public boolean selectItem(String name) throws IOException {
        ensureOpen();
        return selectItem0(name);
    }

    /**
     * Select an item in the collection
     * @param i the record number
     * @return true if the record exists
     * @exception  IOException  if an I/O error occurs.
     */
    public boolean selectItem(int i) throws IOException {
        ensureOpen();
        return selectItemByInt0(i);
    }

    /**
     * Unselect the current item. This is a way to unlock a record without locking another one.
     * @exception  IOException  if an I/O error occurs.
     */
    public void deselectItem() throws IOException {
        ensureOpen();
        deselectItem0();
    }

//
// Metadata management
//

    /**
     * Test to see if the current item a directory
     * @return true if it is, false if it is not
     */
    public boolean isDirectory() throws IOException {
        ensureOpen();
        return isDirectory0();
    }

    /**
     * Create a data item in the collection with a randomly chosen unique name and select it
     * @exception  IOException  if an I/O error occurs.
     */
    public void create() throws IOException {
        ensureOpenForWriting();
        create0();
    }

    /**
     * Create a data item with the supplied name and select it
     * @param name the name of the item to create
     * @exception  IOException  if an I/O error occurs.
     */
    public void create(String name) throws IOException {
        ensureOpenForWriting();
        createName0(name);
    }

    /**
     * Create an item with the supplied number
     * @param i the record number
     * @exception  IOException  if an I/O error occurs.
     */
    public void create(int i) throws IOException {
        ensureOpenForWriting();
        createNameByInt0(i);
    }

    /**
     * Create a directory with the supplied name
     * @param name the name of the directory to create
     * @exception  IOException  if an I/O error occurs.
     */
    public void createDirectory(String name) throws IOException {
        ensureOpenForWriting();
        createDirectory0(name);
    }

    /**
     * Delete the current data item from the collection
     * @exception  IOException  if an I/O error occurs.
     */
    public void delete() throws IOException {
        ensureOpenForWritingAndSelected();
        ensureNotDirectory();
        delete0();
    }

    /**
     * Delete the current data item from the collection
     * @exception  IOException  if an I/O error occurs.
     */
    public void deleteDirectory() throws IOException {
        ensureOpenForWriting();
        ensureDirectory();
        delete0();
    }

    /**
     * Rename the current data item
     * @param name the new name for the item
     * @exception  IOException  if an I/O error occurs.
     */
    public void rename(String newName) throws IOException {
        ensureOpenForWritingAndSelected();
        ensureNotDirectory();
        rename0(newName);
    }

    /**
     * Rename the current data item
     * @param name the new name for the item
     * @exception  IOException  if an I/O error occurs.
     */
    public void rename(int i) throws IOException {
        ensureOpenForWritingAndSelected();
        ensureNotDirectory();
        renameByInt0(i);
    }

    /**
     * Rename a directory with the supplied name
     * @param newName the new name for the directory
     * @exception  IOException  if an I/O error occurs.
     */
    public void renameDirectory(String newName) throws IOException {
        ensureOpenForWritingAndSelected();
        ensureDirectory();
        renameDirectory0(newName);
    }

    /**
     * Return the name of the currently selected item
     * @return  the name of the item or null if no item is selected
     * @exception  IOException  if an I/O error occurs.
     */
    public String getItemName() throws IOException {
        ensureOpenAndSelected();
        return getItemName0();
    }

    /**
     * Return the number of the current item
     * @return the current item number
     * @exception  IOException  if an I/O error occurs or if the item does not have a numeric name.
     */
    public int getItemNumber() throws IOException {
        ensureOpenAndSelected();
        return getItemNumber0();
    }

    /**
     * Return the date that the item was last modified
     * @return  date or -1 if the record was undated.
     * @exception  IOException  if an I/O error occurs.
     */
    public long getModificationDate() throws IOException {
        ensureOpenAndSelected();
        return getModificationDate0();
    }

//
// R/W bits
//

    /**
     * Test to see if the selected item can be read
     * @return  true if the item can be read, else false.
     * @exception  IOException  if an I/O error occurs.
     */
    public boolean canRead() throws IOException {
        ensureOpenAndSelected();
        return canRead0();
    }

     /**
     * Set or clear the read bit
     * @param tf the new value for the read bit
     * @exception  IOException  if an I/O error occurs.
     */
    public void setReadable(boolean tf) throws IOException {
        ensureOpenAndSelected();
        setReadable0(tf);
    }

    /**
     * Test to see if the selected item can be written
     * @return  true if the item can be written, else false.
     * @exception  IOException  if an I/O error occurs.
     */
    public boolean canWrite() throws IOException {
        ensureOpenAndSelected();
        return canWrite0();
    }

     /**
     * Set or clear the write bit
     * @param tf the new value for the write bit
     * @exception  IOException  if an I/O error occurs.
     */
    public void setWritable(boolean tf) throws IOException {
        ensureOpenAndSelected();
        setWritable0(tf);
    }



/**********************************************************************\
 *                            Native Methods                          *
\**********************************************************************/

    public abstract Connection open0(String openName, String parms, int mode) throws IOException;
    public abstract void    close0() throws IOException;
    public abstract long    getAvailableSpace0() throws IOException;
    public abstract int     getItemCount0() throws IOException;

    public abstract boolean selectFirstItem0() throws IOException;
    public abstract boolean selectNextItem0() throws IOException;
    public abstract boolean selectItem0(String name) throws IOException;
    public abstract boolean selectItemByInt0(int i) throws IOException;
    public abstract void    deselectItem0() throws IOException;
    public abstract boolean isSelected0() throws IOException;

    public abstract void    create0() throws IOException;
    public abstract void    createName0(String name) throws IOException;
    public abstract void    createNameByInt0(int i) throws IOException;
    public abstract void    createDirectory0(String name) throws IOException;
    public abstract void    delete0() throws IOException;
    public abstract void    rename0(String name2) throws IOException;
    public abstract void    renameByInt0(int i) throws IOException;
    public abstract void    renameDirectory0(String name2) throws IOException;

    public abstract long    getLength0() throws IOException;
    public abstract void    setLength0(long len) throws IOException;
    public abstract long    getModificationDate0() throws IOException;
    public abstract String  getItemName0() throws IOException;
    public abstract int     getItemNumber0() throws IOException;
    public abstract boolean isDirectory0() throws IOException;
    public abstract boolean canRead0() throws IOException;
    public abstract boolean canWrite0() throws IOException;
    public abstract void    setReadable0(boolean tf) throws IOException;
    public abstract void    setWritable0(boolean tf) throws IOException;

    public abstract int     available0() throws IOException;
    public abstract void    seek0(long pos) throws IOException;
    public abstract long    getPosition0() throws IOException;
    public abstract int     read0() throws IOException;
    public abstract int     readBytes0(byte b[], int off, int len) throws IOException;
    public abstract void    write0(int b) throws IOException;
    public abstract void    writeBytes0(byte b[], int off, int len) throws IOException;
}





/**********************************************************************\
 *                        PrivateFileInputStream                      *
\**********************************************************************/


class PrivateFileInputStream extends InputStream {

    ProtocolBase parent;

    public PrivateFileInputStream(ProtocolBase parent) throws IOException {
        this.parent = parent;
    }

    void ensureOpen() throws IOException {
        if(parent == null) {
            throw new IOException("Stream closed");
        }
    }

    public int read() throws IOException {
        ensureOpen();
        return parent.read0();
    }

    public int read(byte b[], int off, int len) throws IOException {
        ensureOpen();
        return parent.read0(b, off, len);
    }

    public int available() throws IOException {
        ensureOpen();
        return parent.available0();
    }

    public void close() throws IOException {
        ensureOpen();
        parent.operationMode = ProtocolBase.O_RAND;
        parent.realClose();
        parent = null;
    }
}




/**********************************************************************\
 *                        PrivateFileOutputStream                     *
\**********************************************************************/


class PrivateFileOutputStream extends OutputStream {

    ProtocolBase parent;

    public PrivateFileOutputStream(ProtocolBase parent) throws IOException {
        this.parent = parent;
    }

    void ensureOpen() throws IOException {
        if(parent == null) {
            throw new IOException("Stream closed");
        }
    }

    public void write(int b) throws IOException {
        ensureOpen();
        parent.write0(b);
    }

    public void write(byte b[], int off, int len) throws IOException {
        ensureOpen();
        parent.write0(b, off, len);
    }

    public void close() throws IOException {
        ensureOpen();
        parent.operationMode = ProtocolBase.O_RAND;
        parent.realClose();
        parent = null;
    }
}










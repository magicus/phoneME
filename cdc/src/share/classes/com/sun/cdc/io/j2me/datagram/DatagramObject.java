/*
 * @(#)DatagramObject.java	1.25 06/10/10
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

package com.sun.cdc.io.j2me.datagram;

import java.io.*;
import java.net.*;
import javax.microedition.io.*;
import com.sun.cdc.io.j2me.*;

/**
 * This class is required because the J2SE Datagram class is final.
 */
public class DatagramObject extends UniversalOutputStream implements Datagram {

    DatagramPacket dgram;
    String host;
    int port;

    boolean resetFlag = false;

    public DatagramObject(DatagramPacket dpkt) {
        dgram = dpkt;
    }

    public DatagramObject(int p) {
        port  = p;
        host  = "localhost";
    }

    public String getAddress() {
        if (dgram == null) {
            return null;
        }

        InetAddress addr = dgram.getAddress();

        if (addr == null) {
            return null;
        }

        if (host == null) {
            return "datagram://:" + port;
        }

        return "datagram://" + host + ":" + port;
    }

    public byte[] getData() {
        return dgram.getData();
    }

    public int getOffset() {
        return dgram.getOffset();
    }

    public int getLength() {
        return dgram.getLength();
    }

    public void setAddress(String addr) throws IOException,
        IllegalArgumentException {
        if (addr == null)
	    throw new IllegalArgumentException("NULL datagram address");
        if(!addr.startsWith("datagram://")) {
            throw new IllegalArgumentException("Invalid datagram address"+addr);
        }
        String address = addr.substring(11);
        try {
            host = Protocol.getAddress(address);
            port = Protocol.getPort(address);
            dgram.setAddress(InetAddress.getByName(host));
            dgram.setPort(port);
        } catch(NumberFormatException x) {
            throw new IllegalArgumentException("Invalid datagram address"+addr);
        } catch(UnknownHostException x) {
            throw new IllegalArgumentException("Unknown host "+addr);
        }
    }

    public void setAddress(Datagram reference) throws IllegalArgumentException
{
        if (reference == null)
	    throw new IllegalArgumentException("NULL datagram address");
        DatagramObject ref = (DatagramObject)reference;
	host = ref.host;
	port = ref.port;
        if (host == null) 
            throw new IllegalArgumentException("NULL datagram address");
	dgram.setAddress(ref.dgram.getAddress());
	dgram.setPort(port);
    }

    public void setData(byte[] buffer, int offset, int len)  throws 
	IllegalArgumentException {
        if (buffer == null) {
	    throw new IllegalArgumentException("NULL buffer");
	}
	if (offset > buffer.length || len > buffer.length ) {
	    throw new IllegalArgumentException("offset past length of buffer");
	}
        dgram.setData(buffer, offset, len);
        pointer = 0;
    }

    public void setLength(int len) {
        /* The length of the datagram should not exceed the length of the data buffer length */
        if (len > dgram.getData().length ) {
	    throw new IllegalArgumentException("length past length of buffer");
	}
        dgram.setLength(len);
    }

/*
    public void setOffset(int offset) {
        dgram.setOffset(offset);
    }
*/


//
// Read / write functions.
//

    int pointer = 0;

    public void reset() {
        byte[] b = dgram.getData();
        dgram.setData(b, 0, 0);
        pointer = 0;
        resetFlag = true;
    }

    public long skip(long n) throws EOFException {
        int len = dgram.getLength();
	if (n > (len-pointer)) {
	    throw new EOFException();
	}
        pointer += n;
        return n;
    }

    public int read() throws IOException {
        byte[] b = dgram.getData();
        if(pointer >= dgram.getLength()) {
            return -1;
        }
        return b[dgram.getOffset()+(pointer++)] & 0xFF;
    }

    public void write(int ch) throws IOException {
        byte[] b = dgram.getData();

        // Check if trying to move past end of datagram buffer
	// NOTE: only make this check if reset was not called
	if ( (!resetFlag) && (dgram.getLength() > 0) && 
	     ((pointer+1) > dgram.getLength())) {
	    throw new IOException();
	
	} else if ( (!resetFlag) && (dgram.getLength() == 0) ) {
	    throw new IOException();

	} else {
	    // Create a new buffer if dgram is now empty
	    if (dgram.getLength() == 0) {
		byte new_b[] = new byte[1];
		b = new_b;
		dgram.setData(b, 0, 1);
		dgram.setLength(1);
	    }
	}

	// Grow the datagram if needed	    
	if ((dgram.getOffset()+pointer) == b.length) {
	    byte new_b[] = new byte[b.length+1];
	    System.arraycopy(b, 0, new_b, 0, b.length);
	    b = new_b;
	    dgram.setData(b, 0, b.length);
	    dgram.setLength(b.length);
	}
        b[dgram.getOffset()+pointer++] = (byte)ch;
    }

//
// DataInput functions
//

    /**
     * See the general contract of the <code>readFully</code>
     * method of <code>DataInput</code>.
     * <p>
     * Bytes
     * for this operation are read from the contained
     * input stream.
     *
     * @param      b   the buffer into which the data is read.
     * @exception  EOFException  if this input stream reaches the end before
     *               reading all the bytes.
     * @exception  IOException   if an I/O error occurs.
     */
    public final void readFully(byte b[]) throws IOException {
        readFully(b, 0, b.length);
    }

    /**
     * See the general contract of the <code>readFully</code>
     * method of <code>DataInput</code>.
     * <p>
     * Bytes
     * for this operation are read from the contained
     * input stream.
     *
     * @param      b     the buffer into which the data is read.
     * @param      off   the start offset of the data.
     * @param      len   the number of bytes to read.
     * @exception  EOFException  if this input stream reaches the end before
     *               reading all the bytes.
     * @exception  IOException   if an I/O error occurs.
     */
    public final void readFully(byte b[], int off, int len) throws IOException {
        if ((len < 0) || (off < 0) || ((off+len) > b.length))
            throw new IndexOutOfBoundsException();

        int n = 0;
        while (n < len) {
            int ch = read();
            if (ch < 0) {
                throw new EOFException();
            }
            b[off+(n++)] = (byte)ch;
        }
    }

    /**
     * See the general contract of the <code>skipBytes</code>
     * method of <code>DataInput</code>.
     * <p>
     * Bytes
     * for this operation are read from the contained
     * input stream.
     *
     * @param      n   the number of bytes to be skipped.
     * @return     the actual number of bytes skipped.
     * @exception  IOException   if an I/O error occurs.
     */
    public final int skipBytes(int n) throws IOException {
        int len = dgram.getLength();

	// Do not skip past end of buffer
	if ((n > 0) && (n >= (len-pointer))) {
	    // Return the last position if trying to go past end of
	    //  buffer
	    n = len-pointer;
	}
        pointer += n;

	// Return the pointer position of next byte
        return n;
    }

    /**
     * See the general contract of the <code>readBoolean</code>
     * method of <code>DataInput</code>.
     * <p>
     * Bytes
     * for this operation are read from the contained
     * input stream.
     *
     * @return     the <code>boolean</code> value read.
     * @exception  EOFException  if this input stream has reached the end.
     */
    public final boolean readBoolean() throws IOException {
        int ch = read();
        if (ch < 0)
            throw new EOFException();
        return (ch != 0);
    }

    /**
     * See the general contract of the <code>readByte</code>
     * method of <code>DataInput</code>.
     * <p>
     * Bytes
     * for this operation are read from the contained
     * input stream.
     *
     * @return     the next byte of this input stream as a signed 8-bit
     *             <code>byte</code>.
     * @exception  EOFException  if this input stream has reached the end.
     * @exception  IOException   if an I/O error occurs.
     */
    public final byte readByte() throws IOException {
        int ch = read();
        if (ch < 0)
            throw new EOFException();
        return (byte)(ch);
    }

    /**
     * See the general contract of the <code>readUnsignedByte</code>
     * method of <code>DataInput</code>.
     * <p>
     * Bytes
     * for this operation are read from the contained
     * input stream.
     *
     * @return     the next byte of this input stream, interpreted as an
     *             unsigned 8-bit number.
     * @exception  EOFException  if this input stream has reached the end.
     * @exception  IOException   if an I/O error occurs.
     */
    public final int readUnsignedByte() throws IOException {
        int ch = read();
        if (ch < 0)
            throw new EOFException();
        return ch;
    }

    /**
     * See the general contract of the <code>readShort</code>
     * method of <code>DataInput</code>.
     * <p>
     * Bytes
     * for this operation are read from the contained
     * input stream.
     *
     * @return     the next two bytes of this input stream, interpreted as a
     *             signed 16-bit number.
     * @exception  EOFException  if this input stream reaches the end before
     *               reading two bytes.
     * @exception  IOException   if an I/O error occurs.
     */
    public final short readShort() throws IOException {
        int ch1 = read();
        int ch2 = read();
        if ((ch1 | ch2) < 0)
             throw new EOFException();
        return (short)((ch1 << 8) + (ch2 << 0));
    }

    /**
     * See the general contract of the <code>readUnsignedShort</code>
     * method of <code>DataInput</code>.
     * <p>
     * Bytes
     * for this operation are read from the contained
     * input stream.
     *
     * @return     the next two bytes of this input stream, interpreted as an
     *             unsigned 16-bit integer.
     * @exception  EOFException  if this input stream reaches the end before
     *               reading two bytes.
     * @exception  IOException   if an I/O error occurs.
     */
    public final int readUnsignedShort() throws IOException {
        int ch1 = read();
        int ch2 = read();
        if ((ch1 | ch2) < 0)
             throw new EOFException();
        return (ch1 << 8) + (ch2 << 0);
    }

    /**
     * See the general contract of the <code>readChar</code>
     * method of <code>DataInput</code>.
     * <p>
     * Bytes
     * for this operation are read from the contained
     * input stream.
     *
     * @return     the next two bytes of this input stream as a Unicode
     *             character.
     * @exception  EOFException  if this input stream reaches the end before
     *               reading two bytes.
     * @exception  IOException   if an I/O error occurs.
     */
    public final char readChar() throws IOException {
        int ch1 = read();
        int ch2 = read();
        if ((ch1 | ch2) < 0)
             throw new EOFException();
        return (char)((ch1 << 8) + (ch2 << 0));
    }

    /**
     * See the general contract of the <code>readInt</code>
     * method of <code>DataInput</code>.
     * <p>
     * Bytes
     * for this operation are read from the contained
     * input stream.
     *
     * @return     the next four bytes of this input stream, interpreted as an
     *             <code>int</code>.
     * @exception  EOFException  if this input stream reaches the end before
     *               reading four bytes.
     * @exception  IOException   if an I/O error occurs.
     */
    public final int readInt() throws IOException {
        int ch1 = read();
        int ch2 = read();
        int ch3 = read();
        int ch4 = read();
        if ((ch1 | ch2 | ch3 | ch4) < 0)
             throw new EOFException();
        return ((ch1 << 24) + (ch2 << 16) + (ch3 << 8) + (ch4 << 0));
    }

    public final float readFloat() throws IOException {
        int ch = readInt();
	float fl = Float.intBitsToFloat(ch);
	return(fl);
    }

    public final double readDouble() throws IOException {
        long ch = readLong();
	double db = Double.longBitsToDouble(ch);
	return(db);
    }

    /**
     * See the general contract of the <code>readLong</code>
     * method of <code>DataInput</code>.
     * <p>
     * Bytes
     * for this operation are read from the contained
     * input stream.
     *
     * @return     the next eight bytes of this input stream, interpreted as a
     *             <code>long</code>.
     * @exception  EOFException  if this input stream reaches the end before
     *               reading eight bytes.
     * @exception  IOException   if an I/O error occurs.
     */
    public final long readLong() throws IOException {
        return ((long)(readInt()) << 32) + (readInt() & 0xFFFFFFFFL);
    }

    /**
     * See the general contract of the <code>readUTF</code>
     * method of <code>DataInput</code>.
     * <p>
     * Bytes
     * for this operation are read from the contained
     * input stream.
     *
     * @return     a Unicode string.
     * @exception  EOFException  if this input stream reaches the end before
     *               reading all the bytes.
     * @exception  IOException   if an I/O error occurs.
     * @see        java.io.DataInputStream#readUTF(java.io.DataInput)
     */
    public final String readUTF() throws IOException {
        return DataInputStream.readUTF(this);
    }

    /**
     * See the general contract of the <code>readLine</code>
     * method of <code>DataInput</code>.
     * <p>
     * Bytes
     * for this operation are read from the contained
     * input stream.
     *
     * @return     a Unicode string.
     * @exception  EOFException  if this input stream reaches the end before
     *               reading all the bytes.
     * @exception  IOException   if an I/O error occurs.
     * @see        java.io.DataInputStream#readLine()
     */
    public final String readLine() throws IOException {
        return DataInputStream.readUTF(this);
    }


//
// DataOutput functions come from UniversalOutputStream
//


}

/*
 * @(#)RandomAccessROMFile.java	1.14 06/10/10
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
 * Instances of this class support reading of files in ROM.
 *
 * WARNING: This is not production code and is only meant as an example of
 * how to access files romized by JavaDataCompact. Among other things, no attempt
 * is made to syncronize any of the calls. This can result in errors when
 * two threads are trying to access the same file through the same
 * RandomAccessROMFile instance, or if 2 threads are trying
 * to open a romfile at the same time, even if each is opening a different
 * romfile.
 *  
 */
public
class RandomAccessROMFile  extends InputStream implements DataInput {
    private FileDescriptor fd;
    /**
     * Creates a random access file stream to read from with the specified name. 
     * <p>
     * The mode argument must either be equal to <code>"r"</code>. 
     *
     * @param      name   the system-dependent filename.
     * @param      mode   the access mode.
     * @exception  IllegalArgumentException  if the mode argument is not equal
     *               to <code>"r"</code>.
     * @exception  IOException               if an I/O error occurs.
     * @exception  SecurityException         if a security manager exists, its
     *               <code>checkRead</code> method is called with the name
     *               argument to see if the application is allowed read access
     *               to the file. this may result in a security exception.
     * @see        java.lang.SecurityException
     * @see        java.lang.SecurityManager#checkRead(java.lang.String)
     */
    public RandomAccessROMFile(String name, String mode) throws IOException {
        if (!mode.equals("r"))
            throw new IllegalArgumentException("mode must be r");
        SecurityManager security = System.getSecurityManager();
        if (security != null)
            security.checkRead(name);
        fd = new FileDescriptor();
        open(name);
    }
    
    /**
     * Creates a random access file stream to read from. 
     * <p>
     * The mode argument must either be equal to <code>"r"</code>. 
     *
     * @param      file   the file object.
     * @param      mode   the access mode.
     * @exception  IllegalArgumentException  if the mode argument is not equal
     *               to <code>"r"</code>.
     * @exception  IOException               if an I/O error occurs.
     * @exception  SecurityException         if a security manager exists, its
     *               <code>checkRead</code> method is called with the pathname
     *               of the <code>File</code> argument to see if the
     *               application is allowed read access to the file.
     * @see        java.io.File#getPath()
     * @see        java.lang.SecurityManager#checkRead(java.lang.String)
     */
    public RandomAccessROMFile(FileIO file, String mode) throws IOException {
        this(file.getPath(), mode);
    }

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
     * Opens a file and returns the file descriptor.  The file is 
     * opened  as read-only.
     * @param name the name of the file
     */
    private native void open(String name) throws IOException;

    // 'Read' primitives
    
    /**
     * Reads a byte of data from this file. This method blocks if no 
     * input is yet available. 
     *
     * @return     the next byte of data, or <code>-1</code> if the end of the
     *             file is reached.
     * @exception  IOException  if an I/O error occurs.
     * @since      JDK1.0
     */
    public native int read() throws IOException;

    /**
     * Reads a sub array as a sequence of bytes. 
     * @param b the buffer where data is to be written
     * @param off the start offset in the data
     * @param len the number of bytes that are to be read
     * @exception IOException If an I/O error has occurred.
     */
    private native int readBytes(byte b[], int off, int len) throws IOException;
 
    /**
     * Reads up to <code>len</code> bytes of data from this file into an 
     * array of bytes. This method blocks until at least one byte of input 
     * is available. 
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
     * Reads up to <code>b.length</code> bytes of data from this file 
     * into an array of bytes. This method blocks until at least one byte 
     * of input is available. 
     *
     * @param      b   the buffer into which the data is read.
     * @return     the total number of bytes read into the buffer, or
     *             <code>-1</code> if there is no more data because the end of
     *             this file has been reached.
     * @exception  IOException  if an I/O error occurs.
     * @since      JDK1.0 
     */
    public int read(byte b[]) throws IOException {
        return readBytes(b, 0, b.length);
    }
   
    /**
     * Reads <code>b.length</code> bytes from this file into the byte 
     * array. This method reads repeatedly from the file until all the 
     * bytes are read. This method blocks until all the bytes are read, 
     * the end of the stream is detected, or an exception is thrown. 
     *
     * @param      b   the buffer into which the data is read.
     * @exception  EOFException  if this file reaches the end before reading
     *               all the bytes.
     * @exception  IOException   if an I/O error occurs.
     * @since      JDK1.0
     */
    public final void readFully(byte b[]) throws IOException {
        readFully(b, 0, b.length);
    }

    /**
     * Reads exactly <code>len</code> bytes from this file into the byte 
     * array. This method reads repeatedly from the file until all the 
     * bytes are read. This method blocks until all the bytes are read, 
     * the end of the stream is detected, or an exception is thrown. 
     *
     * @param      b     the buffer into which the data is read.
     * @param      off   the start offset of the data.
     * @param      len   the number of bytes to read.
     * @exception  EOFException  if this file reaches the end before reading
     *               all the bytes.
     * @exception  IOException   if an I/O error occurs.
     * @since      JDK1.0
     */
    public final void readFully(byte b[], int off, int len) throws IOException {
        int n = 0;
        while (n < len) {
            int count = this.read(b, off + n, len - n);
            if (count < 0)
                throw new EOFException();
            n += count;
        }
    }

    /**
     * Skips exactly <code>n</code> bytes of input. 
     * <p>
     * This method blocks until all the bytes are skipped, the end of 
     * the stream is detected, or an exception is thrown. 
     *
     * @param      n   the number of bytes to be skipped.
     * @return     the number of bytes skipped, which is always <code>n</code>.
     * @exception  EOFException  if this file reaches the end before skipping
     *               all the bytes.
     * @exception  IOException  if an I/O error occurs.
     * @since      JDK1.0
     */
    public int skipBytes(int n) throws IOException {
        seek(getFilePointer() + n);
        return n;
    }

    /**
     * Returns the current offset in this file. 
     *
     * @return     the offset from the beginning of the file, in bytes,
     *             at which the next read occurs.
     * @exception  IOException  if an I/O error occurs.
     * @since      JDK1.0
     */
    public native long getFilePointer() throws IOException;

    /**
     * Sets the offset from the beginning of this file at which the next 
     * read occurs. 
     *
     * @param      pos   the absolute position.
     * @exception  IOException  if an I/O error occurs.
     * @since      JDK1.0
     */
    public native void seek(long pos) throws IOException;

    /**
     * Returns the length of this file.
     *
     * @return     the length of this file.
     * @exception  IOException  if an I/O error occurs.
     * @since      JDK1.0
     */
    public native long length() throws IOException;

    /**
     * Closes this random access file stream and releases any system 
     * resources associated with the stream. 
     *
     * @exception  IOException  if an I/O error occurs.
     * @since      JDK1.0
     */
    public native void close() throws IOException;

    //
    //  Some "reading Java data types" methods stolen from
    //  DataInputStream.
    //

    /**
     * Reads a <code>boolean</code> from this file. This method reads a 
     * single byte from the file. A value of <code>0</code> represents 
     * <code>false</code>. Any other value represents <code>true</code>. 
     * This method blocks until the byte is read, the end of the stream 
     * is detected, or an exception is thrown. 
     *
     * @return     the <code>boolean</code> value read.
     * @exception  EOFException  if this file has reached the end.
     * @exception  IOException   if an I/O error occurs.
     * @since      JDK1.0
     */
    public final boolean readBoolean() throws IOException {
        int ch = this.read();
        if (ch < 0)
            throw new EOFException();
        return (ch != 0);
    }

    /**
     * Reads a signed 8-bit value from this file. This method reads a 
     * byte from the file. If the byte read is <code>b</code>, where 
     * <code>0&nbsp;&lt;=&nbsp;b&nbsp;&lt;=&nbsp;255</code>, 
     * then the result is:
     * <ul><code>
     *     (byte)(b)
     *</code></ul>
     * <p>
     * This method blocks until the byte is read, the end of the stream 
     * is detected, or an exception is thrown. 
     *
     * @return     the next byte of this file as a signed 8-bit
     *             <code>byte</code>.
     * @exception  EOFException  if this file has reached the end.
     * @exception  IOException   if an I/O error occurs.
     * @since      JDK1.0
     */
    public final byte readByte() throws IOException {
        int ch = this.read();
        if (ch < 0)
            throw new EOFException();
        return (byte) (ch);
    }

    /**
     * Reads an unsigned 8-bit number from this file. This method reads 
     * a byte from this file and returns that byte. 
     * <p>
     * This method blocks until the byte is read, the end of the stream 
     * is detected, or an exception is thrown. 
     *
     * @return     the next byte of this file, interpreted as an unsigned
     *             8-bit number.
     * @exception  EOFException  if this file has reached the end.
     * @exception  IOException   if an I/O error occurs.
     * @since      JDK1.0
     */
    public final int readUnsignedByte() throws IOException {
        int ch = this.read();
        if (ch < 0)
            throw new EOFException();
        return ch;
    }

    /**
     * Reads a signed 16-bit number from this file. The method reads 2 
     * bytes from this file. If the two bytes read, in order, are 
     * <code>b1</code> and <code>b2</code>, where each of the two values is 
     * between <code>0</code> and <code>255</code>, inclusive, then the 
     * result is equal to:
     * <ul><code>
     *     (short)((b1 &lt;&lt; 8) | b2)
     * </code></ul>
     * <p>
     * This method blocks until the two bytes are read, the end of the 
     * stream is detected, or an exception is thrown. 
     *
     * @return     the next two bytes of this file, interpreted as a signed
     *             16-bit number.
     * @exception  EOFException  if this file reaches the end before reading
     *               two bytes.
     * @exception  IOException   if an I/O error occurs.
     * @since      JDK1.0
     */
    public final short readShort() throws IOException {
        int ch1 = this.read();
        int ch2 = this.read();
        if ((ch1 | ch2) < 0)
            throw new EOFException();
        return (short) ((ch1 << 8) + (ch2 << 0));
    }

    /**
     * Reads an unsigned 16-bit number from this file. This method reads 
     * two bytes from the file. If the bytes read, in order, are 
     * <code>b1</code> and <code>b2</code>, where 
     * <code>0&nbsp;&lt;=&nbsp;b1, b2&nbsp;&lt;=&nbsp;255</code>, 
     * then the result is equal to:
     * <ul><code>
     *     (b1 &lt;&lt; 8) | b2
     * </code></ul>
     * <p>
     * This method blocks until the two bytes are read, the end of the 
     * stream is detected, or an exception is thrown. 
     *
     * @return     the next two bytes of this file, interpreted as an unsigned
     *             16-bit integer.
     * @exception  EOFException  if this file reaches the end before reading
     *               two bytes.
     * @exception  IOException   if an I/O error occurs.
     * @since      JDK1.0
     */
    public final int readUnsignedShort() throws IOException {
        int ch1 = this.read();
        int ch2 = this.read();
        if ((ch1 | ch2) < 0)
            throw new EOFException();
        return (ch1 << 8) + (ch2 << 0);
    }

    /**
     * Reads a Unicode character from this file. This method reads two
     * bytes from the file. If the bytes read, in order, are 
     * <code>b1</code> and <code>b2</code>, where 
     * <code>0&nbsp;&lt;=&nbsp;b1,&nbsp;b2&nbsp;&lt;=&nbsp;255</code>, 
     * then the result is equal to:
     * <ul><code>
     *     (char)((b1 &lt;&lt; 8) | b2)
     * </code></ul>
     * <p>
     * This method blocks until the two bytes are read, the end of the 
     * stream is detected, or an exception is thrown. 
     *
     * @return     the next two bytes of this file as a Unicode character.
     * @exception  EOFException  if this file reaches the end before reading
     *               two bytes.
     * @exception  IOException   if an I/O error occurs.
     * @since      JDK1.0
     */
    public final char readChar() throws IOException {
        int ch1 = this.read();
        int ch2 = this.read();
        if ((ch1 | ch2) < 0)
            throw new EOFException();
        return (char) ((ch1 << 8) + (ch2 << 0));
    }

    /**
     * Reads a signed 32-bit integer from this file. This method reads 4 
     * bytes from the file. If the bytes read, in order, are <code>b1</code>,
     * <code>b2</code>, <code>b3</code>, and <code>b4</code>, where 
     * <code>0&nbsp;&lt;=&nbsp;b1, b2, b3, b4&nbsp;&lt;=&nbsp;255</code>, 
     * then the result is equal to:
     * <ul><code>
     *     (b1 &lt;&lt; 24) | (b2 &lt;&lt; 16) + (b3 &lt;&lt; 8) + b4
     * </code></ul>
     * <p>
     * This method blocks until the four bytes are read, the end of the 
     * stream is detected, or an exception is thrown. 
     *
     * @return     the next four bytes of this file, interpreted as an
     *             <code>int</code>.
     * @exception  EOFException  if this file reaches the end before reading
     *               four bytes.
     * @exception  IOException   if an I/O error occurs.
     * @since      JDK1.0
     */
    public final int readInt() throws IOException {
        int ch1 = this.read();
        int ch2 = this.read();
        int ch3 = this.read();
        int ch4 = this.read();
        if ((ch1 | ch2 | ch3 | ch4) < 0)
            throw new EOFException();
        return ((ch1 << 24) + (ch2 << 16) + (ch3 << 8) + (ch4 << 0));
    }

    /**
     * Reads a signed 64-bit integer from this file. This method reads eight
     * bytes from the file. If the bytes read, in order, are 
     * <code>b1</code>, <code>b2</code>, <code>b3</code>, 
     * <code>b4</code>, <code>b5</code>, <code>b6</code>, 
     * <code>b7</code>, and <code>b8,</code> where:
     * <ul><code>
     *     0 &lt;= b1, b2, b3, b4, b5, b6, b7, b8 &lt;=255,
     * </code></ul>
     * <p>
     * then the result is equal to:
     * <p><blockquote><pre>
     *     ((long)b1 &lt;&lt; 56) + ((long)b2 &lt;&lt; 48)
     *     + ((long)b3 &lt;&lt; 40) + ((long)b4 &lt;&lt; 32)
     *     + ((long)b5 &lt;&lt; 24) + ((long)b6 &lt;&lt; 16)
     *     + ((long)b7 &lt;&lt; 8) + b8
     * </pre></blockquote>
     * <p>
     * This method blocks until the eight bytes are read, the end of the 
     * stream is detected, or an exception is thrown. 
     *
     * @return     the next eight bytes of this file, interpreted as a
     *             <code>long</code>.
     * @exception  EOFException  if this file reaches the end before reading
     *               eight bytes.
     * @exception  IOException   if an I/O error occurs.
     * @since      JDK1.0
     */
    public final long readLong() throws IOException {
        return ((long) (readInt()) << 32) + (readInt() & 0xFFFFFFFFL);
    }

    /**
     * Reads a <code>float</code> from this file. This method reads an 
     * <code>int</code> value as if by the <code>readInt</code> method 
     * and then converts that <code>int</code> to a <code>float</code> 
     * using the <code>intBitsToFloat</code> method in class 
     * <code>Float</code>. 
     * <p>
     * This method blocks until the four bytes are read, the end of the 
     * stream is detected, or an exception is thrown. 
     *
     * @return     the next four bytes of this file, interpreted as a
     *             <code>float</code>.
     * @exception  EOFException  if this file reaches the end before reading
     *             four bytes.
     * @exception  IOException   if an I/O error occurs.
     * @see        java.io.RandomAccessFile#readInt()
     * @see        java.lang.Float#intBitsToFloat(int)
     * @since      JDK1.0
     */
    public final float readFloat() throws IOException {
        return Float.intBitsToFloat(readInt());
    }

    /**
     * Reads a <code>double</code> from this file. This method reads a 
     * <code>long</code> value as if by the <code>readLong</code> method 
     * and then converts that <code>long</code> to a <code>double</code> 
     * using the <code>longBitsToDouble</code> method in 
     * class <code>Double</code>.
     * <p>
     * This method blocks until the eight bytes are read, the end of the 
     * stream is detected, or an exception is thrown. 
     *
     * @return     the next eight bytes of this file, interpreted as a
     *             <code>double</code>.
     * @exception  EOFException  if this file reaches the end before reading
     *             eight bytes.
     * @exception  IOException   if an I/O error occurs.
     * @see        java.io.RandomAccessFile#readLong()
     * @see        java.lang.Double#longBitsToDouble(long)
     * @since      JDK1.0
     */
    public final double readDouble() throws IOException {
        return Double.longBitsToDouble(readLong());
    }

    /**
     * Reads the next line of text from this file. This method 
     * successively reads bytes from the file until it reaches the end of 
     * a line of text. 
     * <p>
     * A line of text is terminated by a carriage-return character 
     * (<code>'&#92;r'</code>), a newline character (<code>'&#92;n'</code>), a 
     * carriage-return character immediately followed by a newline 
     * character, or the end of the input stream. The line-terminating 
     * character(s), if any, are included as part of the string returned. 
     * <p>
     * This method blocks until a newline character is read, a carriage 
     * return and the byte following it are read (to see if it is a 
     * newline), the end of the stream is detected, or an exception is thrown.
     *
     * @return     the next line of text from this file.
     * @exception  IOException  if an I/O error occurs.
     * @since      JDK1.0
     */
    public final String readLine() throws IOException {
        StringBuffer input = new StringBuffer();
        int c;
        while (((c = read()) != -1) && (c != '\n')) {
            input.append((char) c);
        }
        if ((c == -1) && (input.length() == 0)) {
            return null;
        }
        return input.toString();
    }

    /**
     * Reads in a string from this file. The string has been encoded 
     * using a modified UTF-8 format. 
     * <p>
     * The first two bytes are read as if by 
     * <code>readUnsignedShort</code>. This value gives the number of 
     * following bytes that are in the encoded string, not
     * the length of the resulting string. The following bytes are then 
     * interpreted as bytes encoding characters in the UTF-8 format 
     * and are converted into characters. 
     * <p>
     * This method blocks until all the bytes are read, the end of the 
     * stream is detected, or an exception is thrown. 
     *
     * @return     a Unicode string.
     * @exception  EOFException            if this file reaches the end before
     *               reading all the bytes.
     * @exception  IOException             if an I/O error occurs.
     * @exception  UTFDataFormatException  if the bytes do not represent 
     *               valid UTF-8 encoding of a Unicode string.
     * @see        java.io.RandomAccessFile#readUnsignedShort()
     * @since      JDK1.0
     */
    public final String readUTF() throws IOException {
        return DataInputStream.readUTF(this);
    }
}

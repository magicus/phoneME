/*
 * @(#)UniversalInputStream.java	1.10 06/10/10
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

package com.sun.cdc.io.j2me;

import java.io.*;
import com.sun.cdc.i18n.*;

/**
 * This class is a combination of several J2SE stream classes. It implements
 * the InputStream and DataInput interfaces. It also implements a few simple text parsing
 * functions that are reminiscent of scanf. Lastly the stream may, in certain cases, be
 * 'seeked' and have a timeout value associated with it.
 *
 * @author  Nik Shaylor
 * @version 1.0 1/7/2000
 */
public abstract class UniversalInputStream extends InputStream implements DataInput {

//
// Binary input
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
    public void readFully(byte b[]) throws IOException {
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
    public void readFully(byte b[], int off, int len) throws IOException {
        if (len < 0)
            throw new IndexOutOfBoundsException();

        int n = 0;
        while (n < len) {
            int count = read(b, off + n, len - n);
            if (count < 0)
                throw new EOFException();
            n += count;
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
    public int skipBytes(int n) throws IOException {
        int total = 0;
        int cur = 0;

        while ((total<n) && ((cur = (int) skip(n-total)) > 0)) {
            total += cur;
        }

        return total;
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
     * @exception  IOException   if an I/O error occurs.
     */
    public boolean readBoolean() throws IOException {
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
    public byte readByte() throws IOException {
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
    public int readUnsignedByte() throws IOException {
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
    public short readShort() throws IOException {
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
    public int readUnsignedShort() throws IOException {
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
    public char readChar() throws IOException {
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
    public int readInt() throws IOException {
        int ch1 = read();
        int ch2 = read();
        int ch3 = read();
        int ch4 = read();
        if ((ch1 | ch2 | ch3 | ch4) < 0)
             throw new EOFException();
        return ((ch1 << 24) + (ch2 << 16) + (ch3 << 8) + (ch4 << 0));
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
    public long readLong() throws IOException {
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
     * @~~~see        java.io.DataInputStream#readUTF(java.io.DataInput)
     */
    public String readUTF() throws IOException {
        return readUTF(this);
    }

    /**
     * Reads from the
     * stream <code>in</code> a representation
     * of a Unicode  character string encoded in
     * Java modified UTF-8 format; this string
     * of characters  is then returned as a <code>String</code>.
     * The details of the modified UTF-8 representation
     * are  exactly the same as for the <code>readUTF</code>
     * method of <code>DataInput</code>.
     *
     * @param      in   a data input stream.
     * @return     a Unicode string.
     * @exception  EOFException            if the input stream reaches the end
     *               before all the bytes.
     * @exception  IOException             if an I/O error occurs.
     * @exception  UTFDataFormatException  if the bytes do not represent a
     *               valid UTF-8 encoding of a Unicode string.
     * @~~~see        java.io.DataInputStream#readUnsignedShort()
     */
    public static String readUTF(DataInput in) throws IOException {
        int utflen = in.readUnsignedShort();
        StringBuffer str = new StringBuffer(utflen);
        byte bytearr [] = new byte[utflen];
        int c, char2, char3;
        int count = 0;

        in.readFully(bytearr, 0, utflen);

        while (count < utflen) {
            c = (int) bytearr[count] & 0xff;
            switch (c >> 4) {
                case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
                    /* 0xxxxxxx*/
                    count++;
                    str.append((char)c);
                    break;
                case 12: case 13:
                    /* 110x xxxx   10xx xxxx*/
                    count += 2;
                    if (count > utflen)
                        throw new UTFDataFormatException();
                    char2 = (int) bytearr[count-1];
                    if ((char2 & 0xC0) != 0x80)
                        throw new UTFDataFormatException();
                    str.append((char)(((c & 0x1F) << 6) | (char2 & 0x3F)));
                    break;
                case 14:
                    /* 1110 xxxx  10xx xxxx  10xx xxxx */
                    count += 3;
                    if (count > utflen)
                        throw new UTFDataFormatException();
                    char2 = (int) bytearr[count-2];
                    char3 = (int) bytearr[count-1];
                    if (((char2 & 0xC0) != 0x80) || ((char3 & 0xC0) != 0x80))
                        throw new UTFDataFormatException();
                    str.append((char)(((c     & 0x0F) << 12) |
                                      ((char2 & 0x3F) << 6)  |
                                      ((char3 & 0x3F) << 0)));
                    break;
                default:
                    /* 10xx xxxx,  1111 xxxx */
                    throw new UTFDataFormatException();
                }
        }
        // The number of chars produced may be less than utflen
        return new String(str);
    }

    /**
     * Unsupported function
     */
    public float readFloat() throws IOException {
        throw new RuntimeException("Function not supported");
    }

    /**
     * Unsupported function
     */
    public double readDouble() throws IOException {
        throw new RuntimeException("Function not supported");
    }

    /**
     * Unsupported function
     */
    public String readLine() throws IOException {
        throw new RuntimeException("Function not supported");
    }


    /**
     * Reads from the
     * stream <code>in</code> a representation of a byte array.
     * The byte array data must be preceded in the stream by a
     * readInt's worth of data that is equal to the length of the
     * data.
     *
     * @param      in   a data input stream.
     * @return     a byte array.
     * @exception  EOFException            if the input stream reaches the end
     *               before all the bytes.
     * @exception  IOException             if an I/O error occurs.
     */
    public byte[] readByteArray() throws IOException {
        int len = readInt();
        byte[] byteArray = new byte[len];
        readFully(byteArray, 0, len);
        return byteArray;
    }


//
// Text input
//


    /**
     * The reader used for scanning input text
     */
    private Reader reader;

    /**
     * This is the internal buffer where scanned characters can be 'unread' into.
     * There are two special values:
     *
     *  -1   - This value indicates that there is no character waiting
     *  -2   - This indicated that there is no character and that reader
     *         has not been created.
     */
    private int lastch = -2;

    /**
     * Set the encoding for text input. All the scan... routines
     * read text through a Reader that it set, by default, to the default
     * character encoding. This function will change the encoding to another
     * type.
     *
     * @param enc the encoding
     *
     * @exception  UnsupportedEncodingException
     *             If the named encoding is not supported
     */
    public void setEncoding(String enc) throws java.io.UnsupportedEncodingException {
        reader = Helper.getStreamReader(this, enc);
        lastch = -1;
    }

    /**
     * Read a character from from the stream. The raw data in the stream is passed
     * through a Reader to perform the necessary character translation.
     *
     * @return     a Unicode character.
     *
     * @exception  IOException   if an I/O error occurs.
     */
    public int scanChar() throws IOException {
        if(lastch == -1) {
            return reader.read();
        }
        if(lastch == -2) {
            reader = Helper.getStreamReader(this);
            lastch = -1;
            return reader.read();
        }

        int ch = lastch;
        lastch = -1;
        return ch;
    }

    /**
     * Scan a number in from the stream. The number must be in decimal
     * format between the values between -2147483648 and 2147483647. The
     * number may start with a '-' or '+' character and the scan is terminated
     * when a character is read that is not a numeric character. The terminating
     * character is saved in the stream and may be read subsequentially using scanChar().
     * The input data is read from the stream through a character translation Reader.
     * <p>
     * Note: After this routine is called the binary interfaces API readXxx() will not
     * function correctly because there is a terminating character that is not accessible
     * to them. In general is not not advisable to mix binary and textual access to the same
     * stream.
     *
     * @return     an integer.
     *
     * @exception  EOFException  if this input stream reaches the end before
     *               reading the number.
     * @exception  IOException   if an I/O error occurs.
     */
    public int scanInt() throws IOException {
        int i = 0;
        boolean neg = false;
        int ch = scanChar();
        if(ch == '-') {
            neg = true;
            ch = scanChar();
        }
        if(ch == '+') {
            ch = scanChar();
        }
        if(ch == -1) {
            throw new EOFException();
        }
        while(true) {
            if(ch >= '0' && ch <='9') {
                i = (i * 10) + (ch - 48);
            } else {
                lastch = ch;
                break;
            }
            ch = scanChar();
        }
        return (neg) ? 0-i : i;
    }

    /**
     * Scan a textual sequence of characters. A textual sequence is defined here as a
     * set of characters that are not in the first 33 Unicode characters values. This
     * includes space, tab, form-feed-, carriage-return, line-feed, etc. The terminating
     * character is saved in the stream and may be read subsequentially using scanChar().
     * If the stream
     * is at EOF when the scan starts null is returned.
     * The input data is read from the stream through a character translation Reader.
     *
     * @return     a Unicode string or null if at EOF.
     *
     * @exception  IOException   if an I/O error occurs.
     */
    public String scanText() throws IOException {
        int ch = scanChar();
        if(ch == -1) {
             return null;
        }
        StringBuffer sb = new StringBuffer();
        while(true) {
            if(ch <= ' ') {
                lastch = ch;
                break;
            }
            sb.append((char)ch);
            ch = scanChar();
        }
        return sb.toString();
    }

    /**
     * Scan a line of text in from the stream. A line is defined as a character sequence
     * that ends with a "\n" character or EOF. These symbols are not returned with the
     * text string, and all "\r" characters are also discarded in this process. If the stream
     * is at EOF when the scan starts null is returned.
     * The input data is read from the stream through a character translation Reader.
     *
     * @return     a Unicode string or null if at EOF.
     *
     * @exception  IOException   if an I/O error occurs.
     */
    public String scanLine() throws IOException {
        int ch = scanChar();
        if(ch == -1) {
             return null;
        }
        StringBuffer sb = new StringBuffer();
        do {
            if(ch != '\r') {
                if(ch == '\n') {
                    break;
                }
                sb.append((char)ch);
            }
        } while((ch = scanChar()) != -1);

        return sb.toString();
    }

//
// Extra things
//

    /**
     * Sets the file-pointer offset, measured from the beginning of this
     * file, at which the next read or write occurs.  The offset may be
     * set beyond the end of the file. Setting the offset beyond the end
     * of the file does not change the file length.  The file length will
     * change only by writing after the offset has been set beyond the end
     * of the file.
     * <p>
     * Note: This function can be used only on random access streams.
     * other streams will result by throwing an exception.
     *
     * @param      pos   the offset position, measured in bytes from the
     *                   beginning of the file, at which to set the file
     *                   pointer.
     *
     * @exception  IllegalAccessException  if this input stream is sequential only
     *
     * @exception  IOException  if <code>pos</code> is less than
     *                          <code>0</code> or if an I/O error occurs.
     */
    public void seek(long pos) throws IOException, IllegalAccessException {
        throw new IllegalAccessException();
    }

    /**
     * Enable or disable the timeout value for I/O performed on the stream, in
     * milliseconds.  With this option set to a non-zero timeout,
     * any kind of read/scan call on the stream
     * will block for only this amount of time.  If the timeout expires,
     * a <B>java.io.InterruptedIOException</B> is raised, though the
     * Socket is still valid. The option <B>must</B> be enabled
     * prior to entering the blocking operation to have effect. The
     * timeout must be > 0.
     * A timeout of zero is interpreted as an infinite timeout.
     *
     * @param time the time in ms. 0 = forever.
     */
    public void setTimeout(int time) throws IOException {
    }

}

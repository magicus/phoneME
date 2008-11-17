/*
 * @(#)UniversalOutputStream.java	1.13 06/10/10
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
 * the OutputStream and DataOutput interfaces. It also implements the normal set of print() and
 * println() functions. Lastly the stream may, in certain cases, be
 * 'seeked' and have a timeout value associated with it.
 * <p>
 * Note: Printing a "\n" or calling println() will cause just a single line-feed character
 * to be placed in the output file.
 *
 * @version 1.0 1/7/2000
 */
public abstract class UniversalOutputStream extends OutputStream implements DataOutput {

//
// Binary output
//

    /**
     * Writes a <code>boolean</code> to the underlying output stream as
     * a 1-byte value. The value <code>true</code> is written out as the
     * value <code>(byte)1</code>; the value <code>false</code> is
     * written out as the value <code>(byte)0</code>. If no exception is
     * thrown, the counter <code>written</code> is incremented by
     * <code>1</code>.
     *
     * @param      v   a <code>boolean</code> value to be written.
     * @exception  IOException  if an I/O error occurs.
     */
    public void writeBoolean(boolean v) throws IOException {
        write(v ? 1 : 0);
    }

    /**
     * Writes out a <code>byte</code> to the underlying output stream as
     * a 1-byte value. If no exception is thrown, the counter
     * <code>written</code> is incremented by <code>1</code>.
     *
     * @param      v   a <code>byte</code> value to be written.
     * @exception  IOException  if an I/O error occurs.
     */
    public void writeByte(int v) throws IOException {
        write(v);
    }

    /**
     * Writes a <code>short</code> to the underlying output stream as two
     * bytes, high byte first. If no exception is thrown, the counter
     * <code>written</code> is incremented by <code>2</code>.
     *
     * @param      v   a <code>short</code> to be written.
     * @exception  IOException  if an I/O error occurs.
     */
    public void writeShort(int v) throws IOException {
        write((v >>> 8) & 0xFF);
        write((v >>> 0) & 0xFF);
    }

    /**
     * Writes a <code>char</code> to the underlying output stream as a
     * 2-byte value, high byte first. If no exception is thrown, the
     * counter <code>written</code> is incremented by <code>2</code>.
     *
     * @param      v   a <code>char</code> value to be written.
     * @exception  IOException  if an I/O error occurs.
     */
    public void writeChar(int v) throws IOException {
        write((v >>> 8) & 0xFF);
        write((v >>> 0) & 0xFF);
    }

    /**
     * Writes an <code>int</code> to the underlying output stream as four
     * bytes, high byte first. If no exception is thrown, the counter
     * <code>written</code> is incremented by <code>4</code>.
     *
     * @param      v   an <code>int</code> to be written.
     * @exception  IOException  if an I/O error occurs.
     */
    public void writeInt(int v) throws IOException {
        write((v >>> 24) & 0xFF);
        write((v >>> 16) & 0xFF);
        write((v >>>  8) & 0xFF);
        write((v >>>  0) & 0xFF);
    }

    /**
     * Writes a <code>long</code> to the underlying output stream as eight
     * bytes, high byte first. In no exception is thrown, the counter
     * <code>written</code> is incremented by <code>8</code>.
     *
     * @param      v   a <code>long</code> to be written.
     * @exception  IOException  if an I/O error occurs.
     */
    public void writeLong(long v) throws IOException {
        write((int)(v >>> 56) & 0xFF);
        write((int)(v >>> 48) & 0xFF);
        write((int)(v >>> 40) & 0xFF);
        write((int)(v >>> 32) & 0xFF);
        write((int)(v >>> 24) & 0xFF);
        write((int)(v >>> 16) & 0xFF);
        write((int)(v >>>  8) & 0xFF);
        write((int)(v >>>  0) & 0xFF);
    }

    /**
     * Writes a <code>float</code> to the underlying output stream as int
     * bits. In no exception is thrown, the counter
     * <code>written</code> is incremented by <code>8</code>.
     *
     * @param      v   a <code>float</code> to be written.
     * @exception  IOException  if an I/O error occurs.
     */
    public void writeFloat(float v) throws IOException {
        writeInt((int)(Float.floatToIntBits(v)));
    }

    public void writeDouble(double v) throws IOException {
        writeLong(Double.doubleToLongBits(v));
    }

    /**
     * Writes a string to the underlying output stream as a sequence of
     * characters. Each character is written to the data output stream as
     * if by the <code>writeChar</code> method. If no exception is
     * thrown, the counter <code>written</code> is incremented by twice
     * the length of <code>s</code>.
     *
     * @param      s   a <code>String</code> value to be written.
     * @exception  IOException  if an I/O error occurs.
     */
    public void writeChars(String s) throws IOException {
        int len = s.length();
        for (int i = 0 ; i < len ; i++) {
            int v = s.charAt(i);
            write((v >>> 8) & 0xFF);
            write((v >>> 0) & 0xFF);
        }
    }

    /**
     * Writes a string to the underlying output stream using UTF-8
     * encoding in a machine-independent manner.
     * <p>
     * First, two bytes are written to the output stream as if by the
     * <code>writeShort</code> method giving the number of bytes to
     * follow. This value is the number of bytes actually written out,
     * not the length of the string. Following the length, each character
     * of the string is output, in sequence, using the UTF-8 encoding
     * for the character. If no exception is thrown, the counter
     * <code>written</code> is incremented by the total number of
     * bytes written to the output stream. This will be at least two
     * plus the length of <code>str</code>, and at most two plus
     * thrice the length of <code>str</code>.
     *
     * @param      str   a string to be written.
     * @exception  IOException  if an I/O error occurs.
     */
    public void writeUTF(String str) throws IOException {
        writeUTF(str, this);
    }

    /**
     * Writes a string to the specified DataOutput using UTF-8 encoding in a
     * machine-independent manner.
     * <p>
     * First, two bytes are written to out as if by the <code>writeShort</code>
     * method giving the number of bytes to follow. This value is the number of
     * bytes actually written out, not the length of the string. Following the
     * length, each character of the string is output, in sequence, using the
     * UTF-8 encoding for the character. If no exception is thrown, the
     * counter <code>written</code> is incremented by the total number of
     * bytes written to the output stream. This will be at least two
     * plus the length of <code>str</code>, and at most two plus
     * thrice the length of <code>str</code>.
     *
     * @param      str   a string to be written.
     * @param      out   destination to write to
     * @return     The number of bytes written out.
     * @exception  IOException  if an I/O error occurs.
     */
    static int writeUTF(String str, DataOutput out) throws IOException {
        int strlen = str.length();
        int utflen = 0;
        char[] charr = new char[strlen];
        int c, count = 0;

        str.getChars(0, strlen, charr, 0);

        for (int i = 0; i < strlen; i++) {
            c = charr[i];
            if ((c >= 0x0001) && (c <= 0x007F)) {
                utflen++;
            } else if (c > 0x07FF) {
                utflen += 3;
            } else {
                utflen += 2;
            }
        }

        if (utflen > 65535)
            throw new UTFDataFormatException();

        byte[] bytearr = new byte[utflen+2];
        bytearr[count++] = (byte) ((utflen >>> 8) & 0xFF);
        bytearr[count++] = (byte) ((utflen >>> 0) & 0xFF);
        for (int i = 0; i < strlen; i++) {
            c = charr[i];
            if ((c >= 0x0001) && (c <= 0x007F)) {
                bytearr[count++] = (byte) c;
            } else if (c > 0x07FF) {
                bytearr[count++] = (byte) (0xE0 | ((c >> 12) & 0x0F));
                bytearr[count++] = (byte) (0x80 | ((c >>  6) & 0x3F));
                bytearr[count++] = (byte) (0x80 | ((c >>  0) & 0x3F));
            } else {
                bytearr[count++] = (byte) (0xC0 | ((c >>  6) & 0x1F));
                bytearr[count++] = (byte) (0x80 | ((c >>  0) & 0x3F));
            }
        }
        out.write(bytearr);

        return utflen + 2;
    }

    /**
     * writeBytes
     */
    public void writeBytes(String s) throws IOException {
	byte strbyte[] = s.getBytes();
        for (int c=0; c < strbyte.length; c++) {
	    writeByte((int)strbyte[c]);
	}
    }


//
// Text output
//

    /**
     * The writer used for printing output text
     */
    private Writer writer;

    /**
     * An error flag that can be enquired
     */
    private boolean trouble = false;

    /**
     * A flag to prevent infinitly recursive flushing
     */
    private boolean flushing = false;

    /**
     * Set the encoding for text output. All the print() routines
     * write text through a Writer that it set, by default, to the default
     * character encoding. This function will change the encoding to another
     * type.
     *
     * @param enc the encoding
     *
     * @exception  UnsupportedEncodingException
     *             If the named encoding is not supported
     */
    public void setEncoding(String enc) throws UnsupportedEncodingException {
        writer = Helper.getStreamWriter(this, enc);
    }

    /**
     * Flush the stream.
     * <p>
     * Note: If this method is also implemented in a subclass, this method should
     * be called before the body of the code is executed. E.G.
     * <pre>
     *    public void flush() throws IOException {
     *        super.flush();
     *        ... other things ...
     *    }
     * <\pre>
     */
    public void flush() throws IOException {
        if (writer != null && !flushing) {
            flushing = true;
            writer.flush();
            flushing = false;
        }
    }

    /**
     * Close the stream.
     * <p>
     * Note: If this method is also implemented in a subclass, this method should
     * be called before the body of the code is executed. E.G.
     * <pre>
     *    public void close() throws IOException {
     *        super.close();
     *        ... other things ...
     *    }
     * <\pre>
     */
    public void close() throws IOException {
        flush();
    }

    /**
     * Prints a String. All the data is passed through a "Writer" to ensure that the correct
     * character translation is performed.
     *
     * @param s the String to be printed
     */
    synchronized public void print(String s) {
        if(writer == null) {
            writer = Helper.getStreamWriter(this);
        }
        if (s == null) {
            s = "null";
        }
        try {
            writer.write(s);
            writer.flush();
        } catch(IOException x) {
            trouble = true;
        }
    }

    /**
     * Flush the stream and check its error state.  Errors are cumulative;
     * once the stream encounters an error, this routine will return true on
     * all successive calls.
     *
     * @return True if the print stream has encountered an error, either on the
     * underlying output stream or during a format conversion.
     */
    public boolean checkError() {
        try {
            flush();
        } catch(IOException x) {
            trouble = true;
        }
        return trouble;
    }

    /**
     * Prints an array of characters.
     * @param s the array of chars to be printed
     */
    synchronized public void print(char s[]) {
        print(new String(s));
    }

    /**
     * Prints a newline.
     */
    public void println() {
        print("\n");
    }

    /**
     * Prints an object.
     * @param obj the object to be printed
     */
    public void print(Object obj) {
        print(String.valueOf(obj));
    }

    /**
     * Prints an character.
     * @param c the character to be printed
     */
    public void print(char c) {
        print(String.valueOf(c));
    }

    /**
     * Prints an integer.
     * @param i the integer to be printed
     */
    public void print(int i) {
        print(String.valueOf(i));
    }

    /**
     * Prints a long.
     * @param l the long to be printed.
     */
    public void print(long l) {
        print(String.valueOf(l));
    }

//    /**
//     * Prints a float.
//     * @param f the float to be printed
//     */
//    public void print(float f) {
//        print(String.valueOf(f));
//    }

//    /**
//     * Prints a double.
//     * @param d the double to be printed
//     */
//    public void print(double d) {
//        print(String.valueOf(d));
//    }

    /**
     * Prints a boolean.
     * @param b the boolean to be printed
     */
    public void print(boolean b) {
        print(b ? "true" : "false");
    }

    /**
     * Prints an object followed by a newline.
     * @param obj the object to be printed
     */
    synchronized public void println(Object obj) {
        print(obj);
        println();
    }

    /**
     * Prints a string followed by a newline.
     * @param s the String to be printed
     */
    synchronized public void println(String s) {
        print(s);
        println();
    }

    /**
     * Prints an array of characters followed by a newline.
     * @param s the array of characters to be printed
     */
    synchronized public void println(char s[]) {
        print(s);
        println();
    }

    /**
     * Prints a character followed by a newline.
     * @param c the character to be printed
     */
    synchronized public void println(char c) {
        print(c);
        println();
    }

    /**
     * Prints an integer followed by a newline.
     * @param i the integer to be printed
     */
    synchronized public void println(int i) {
        print(i);
        println();
    }

    /**
     * Prints a long followed by a newline.
     * @param l the long to be printed
     */
    synchronized public void println(long l) {
        print(l);
        println();
    }

//    /**
//     * Prints a float followed by a newline.
//     * @param f the float to be printed
//     */
//    synchronized public void println(float f) {
//        print(f);
//        println();
//    }

//    /**
//     * Prints a double followed by a newline.
//     * @param d the double to be printed
//     */
//    synchronized public void println(double d) {
//        print(d);
//        println();
//    }

    /**
     * Prints a boolean followed by a newline.
     * @param b the boolean to be printed
     */
    synchronized public void println(boolean b) {
        print(b);
        println();
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
     * any kind of write/print call on the stream
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

/*
 * @(#)StringWriter.java	1.25 06/10/10
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

package java.io;


/**
 * A character stream that collects its output in a string buffer, which can
 * then be used to construct a string.
 * <p>
 * Closing a <tt>StringWriter</tt> has no effect. The methods in this class
 * can be called after the stream has been closed without generating an
 * <tt>IOException</tt>.
 *
 * @version 	1.18, 00/02/02
 * @author	Mark Reinhold
 * @since	JDK1.1
 */

public class StringWriter extends Writer {

    private StringBuffer buf;

    /**
     * Create a new string writer, using the default initial string-buffer
     * size.
     */
    public StringWriter() {
	buf = new StringBuffer();
	lock = buf;
    }

    /**
     * Create a new string writer, using the specified initial string-buffer
     * size.
     *
     * @param initialSize  an int specifying the initial size of the buffer.
     */
    public StringWriter(int initialSize) {
	if (initialSize < 0) {
	    throw new IllegalArgumentException("Negative buffer size");
	}
	buf = new StringBuffer(initialSize);
	lock = buf;
    }

    /**
     * Write a single character.
     */
    public void write(int c) {
	buf.append((char) c);
    }

    /**
     * Write a portion of an array of characters.
     *
     * @param  cbuf  Array of characters
     * @param  off   Offset from which to start writing characters
     * @param  len   Number of characters to write
     */
    public void write(char cbuf[], int off, int len) {
        if ((off < 0) || (off > cbuf.length) || (len < 0) ||
            ((off + len) > cbuf.length) || ((off + len) < 0)) {
            throw new IndexOutOfBoundsException();
        } else if (len == 0) {
            return;
        }
        buf.append(cbuf, off, len);
    }

    /**
     * Write a string.
     */
    public void write(String str) {
	buf.append(str);
    }

    /**
     * Write a portion of a string.
     *
     * @param  str  String to be written
     * @param  off  Offset from which to start writing characters
     * @param  len  Number of characters to write
     */
    public void write(String str, int off, int len)  {
	buf.append(str.substring(off, off + len));
    }

    /**
     * Return the buffer's current value as a string.
     */
    public String toString() {
	return buf.toString();
    }

    /**
     * Return the string buffer itself.
     *
     * @return StringBuffer holding the current buffer value.
     */
    public StringBuffer getBuffer() {
	return buf;
    }

    /**
     * Flush the stream.
     */
    public void flush() { 
    }

    /**
     * Closing a <tt>StringWriter</tt> has no effect. The methods in this
     * class can be called after the stream has been closed without generating
     * an <tt>IOException</tt>.
     */
    public void close() throws IOException {
    }

}

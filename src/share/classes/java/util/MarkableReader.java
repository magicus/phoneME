/*
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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


package java.util;

import java.io.InputStream;
import java.io.IOException;

/**
 * Intrusive class. Expects Markable interface implementation.
 */

public class MarkableReader {

    // members for mark/reset functionality support
    private byte[] markedBytes;
    // markedBytes offset for reading. -1 means the stream was not resetted
    private int marker = -1;
    // size of the markedBytes buffer. -1 means not marked
    private int markedLimit = -1;
    // markedRead bytes are read to the markedBytes buffer
    // cannot be more than markedLimit
    private int markedRead = 0;
    
    private Markable is;

    public MarkableReader(Markable is) {
        this.is = is;
    }

    /**
     * Clears the marked position.
     */
    private void clearPosition() {
        marker = -1;
        markedLimit = -1;
        markedBytes = null;
        markedRead = 0;
    }

    /**
     * Reads bytes from the stream that implements Markable interface.
     */
    public int read(byte[] b, int off, int len) throws IOException {
        // actually read
        int size;

        if (markedLimit <= 0) {
            // no marked position, just read
            size = is.readNative(b, off, len);
        } else if (marker >= 0 && marker < markedRead) {
            // reading from the buffer, not more than already read
            size = marker + len > markedRead ? markedRead - marker : len;
            System.arraycopy(markedBytes, marker, b, off, size);
        } else {
            // reading to the buffer, marker == -1 or marker == markedRead
            size = markedRead + len > markedLimit ?
                markedLimit - markedRead : len;
            size = is.readNative(b, off, size);
            System.arraycopy(b, off, markedBytes, markedRead, size);
            markedRead += size;
        }

        if (marker>=0) {
            marker += size;
        }

        if (markedLimit == 0 || (markedRead == markedLimit && marker == markedLimit)) {
            clearPosition();
        }

        return size;
    }
    
    /**
     * Marks the current position in this input stream. A subsequent call to
     * the <code>reset</code> method repositions this stream at the last marked
     * position so that subsequent reads re-read the same bytes.
     *
     * <p> The <code>readlimit</code> arguments tells this input stream to
     * allow that many bytes to be read before the mark position gets
     * invalidated.
     *
     * <p> The general contract of <code>mark</code> is that, if the method
     * <code>markSupported</code> returns <code>true</code>, the stream somehow
     * remembers all the bytes read after the call to <code>mark</code> and
     * stands ready to supply those same bytes again if and whenever the method
     * <code>reset</code> is called.  However, the stream is not required to
     * remember any data at all if more than <code>readlimit</code> bytes are
     * read from the stream before <code>reset</code> is called.
     *
     * <p> The <code>mark</code> method of <code>InputStream</code> does
     * nothing.
     *
     * @param   readlimit   the maximum limit of bytes that can be read before
     *                      the mark position becomes invalid.
     * @see     java.io.InputStream#reset()
     */
    public synchronized void mark(int readlimit) {
        if (readlimit < 0) {
            clearPosition();
            return;
        }
        // size to copy:
        int size = 0;

        byte[] oldBytes = markedBytes;
        markedBytes = new byte[readlimit];

        // if stream was resetted and read
        if (marker >= 0) {
            // if requested less than available, enlarge readlimit
            if (marker + readlimit < markedRead) {
                readlimit = markedRead - marker;
            }
            size = markedRead - marker;
            System.arraycopy(oldBytes, marker, markedBytes, 0, size);
            marker = 0;
        }
        markedRead = size;
        markedLimit = readlimit;
    }

    /**
     * Repositions this stream to the position at the time the
     * <code>mark</code> method was last called on this input stream.
     *
     * <p> The general contract of <code>reset</code> is:
     *
     * <p><ul>
     *
     * <li> If the method <code>markSupported</code> returns
     * <code>true</code>, then:
     *
     *     <ul><li> If the method <code>mark</code> has not been called since
     *     the stream was created, or the number of bytes read from the stream
     *     since <code>mark</code> was last called is larger than the argument
     *     to <code>mark</code> at that last call, then an
     *     <code>IOException</code> might be thrown.
     *
     *     <li> If such an <code>IOException</code> is not thrown, then the
     *     stream is reset to a state such that all the bytes read since the
     *     most recent call to <code>mark</code> (or since the start of the
     *     file, if <code>mark</code> has not been called) will be resupplied
     *     to subsequent callers of the <code>read</code> method, followed by
     *     any bytes that otherwise would have been the next input data as of
     *     the time of the call to <code>reset</code>. </ul>
     *
     * <li> If the method <code>markSupported</code> returns
     * <code>false</code>, then:
     *
     *     <ul><li> The call to <code>reset</code> may throw an
     *     <code>IOException</code>.
     *
     *     <li> If an <code>IOException</code> is not thrown, then the stream
     *     is reset to a fixed state that depends on the particular type of the
     *     input stream and how it was created. The bytes that will be supplied
     *     to subsequent callers of the <code>read</code> method depend on the
     *     particular type of the input stream. </ul></ul>
     *
     * <p> The method <code>reset</code> for class <code>InputStream</code>
     * does nothing and always throws an <code>IOException</code>.
     *
     * @exception  IOException  if this stream has not been marked or if the
     *               mark has been invalidated.
     * @see     java.io.InputStream#mark(int)
     * @see     java.io.IOException
     */
    public synchronized void reset() throws IOException {
        if (markedLimit < 0) {
            throw new IOException("Position was not marked");
        }
        marker = 0;
    }

    /**
     * Tests if this input stream supports the <code>mark</code> and
     * <code>reset</code> methods. Whether or not <code>mark</code> and
     * <code>reset</code> are supported is an invariant property of a
     * particular input stream instance. The <code>markSupported</code> method
     * of <code>InputStream</code> returns <code>false</code>.
     *
     * @return  <code>true</code> if this stream instance supports the mark
     *          and reset methods; <code>false</code> otherwise.
     * @see     java.io.InputStream#mark(int)
     * @see     java.io.InputStream#reset()
     */
    public boolean markSupported() {
    	return true;
    }

}
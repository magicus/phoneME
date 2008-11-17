/*
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

import java.io.InputStream;
import java.io.IOException;

/**
 * This class implements mark/reset functionality for InputStreams.
 * Class expects Markable interface implementation from the stream.
 */

public class MarkableReader {

    // Byte buffer for marked data.
    private byte[] markedBytes;
    // Current read position in the markedBytes.
    // -1 means the stream was not resetted.
    private int marker = -1;
    // Size of the marked data. -1 means not marked.
    private int markedLimit = -1;
    // Bytes ready to be read from the buffer. Cannot be more than markedLimit.
    private int markedRead = 0;
    
    // Reference to the owner.
    private Markable is;

    /**
     * @param is reference to Markable interface implementation
     */
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
     * Reads from the stream or from the internal buffer (if the stream was resetted to a marked position) into an array of bytes.
     * Blocks until some input is available.
     * @param b the buffer into which the data is read
     * @param off the start offset of the data
     * @param len the maximum number of bytes read
     * @return the actual number of bytes read, or -1 if the end of the
     *         entry is reached
     * @exception ZipException if a ZIP file error has occurred
     * @exception IOException if an I/O error has occurred
     */
    public int read(byte[] b, int off, int len) throws IOException {
        if ((off | len | (off + len) | (b.length - (off + len))) < 0) {
            throw new IndexOutOfBoundsException();
        } else if (len == 0) {
            return 0;
        }

        // Return value. How many bytes were actually read.
        int size;
        // Clear position if the end of the buffer reached
        if (markedLimit == 0 || (markedRead == markedLimit && marker == markedLimit)) {
            clearPosition();
        }

        if (markedLimit <= 0) {
            // There were no marked position, just read from the stream.
            size = is.readNative(b, off, len);
        } else if (marker >= 0 && marker < markedRead) {
            // Reading from the buffer, but not more than already read.
            // The stream was resetted.
            size = marker + len > markedRead ? markedRead - marker : len;
            System.arraycopy(markedBytes, marker, b, off, size);
        } else {
            // Reading from the stream, copy into the buffer.
            // marker == -1 or marker == markedRead
            // The stream was marked.
            size = markedRead + len > markedLimit ?
                markedLimit - markedRead : len;
            size = is.readNative(b, off, size);
            System.arraycopy(b, off, markedBytes, markedRead, size);
            markedRead += size;
        }

        // Move marker if read from the buffer
        if (marker>=0) {
            marker += size;
        }

        // InputStream.read() must not return 0 when called with len > 0
        return (size == 0) ? -1 : size;
    }
    
    /**
     * Marks the current position in the input stream. A subsequent call to
     * the <code>reset</code> method repositions the stream at the last marked
     * position so that subsequent reads re-read the same bytes.
     *
     * <p> The <code>readlimit</code> arguments tells the input stream to
     * allow that many bytes to be read before the mark position gets
     * invalidated.
     *
     * @param   readlimit   the maximum limit of bytes that can be read before
     *                      the mark position becomes invalid.
     * @see     java.io.InputStream#mark(int)
     * @see     java.io.InputStream#reset()
     */
    public synchronized void mark(int readlimit) {
        if (readlimit <= 0) {
            return;
        }

        // Size to copy from the existing buffer
        int size = 0;

        byte[] oldBytes = markedBytes;
        markedBytes = new byte[readlimit];

        // If the stream was resetted and read
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
     * Repositions the stream to the position at the time the
     * <code>mark</code> method was last called on the input stream.
     *
     * @exception  IOException  if this stream has not been marked or if the
     *               mark has been invalidated.
     * @see     java.io.InputStream#mark(int)
     * @see     java.io.InputStream#reset()
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
     * @see     java.io.InputStream#markSupported()
     * @see     java.io.InputStream#mark(int)
     * @see     java.io.InputStream#reset()
     */
    public boolean markSupported() {
    	return true;
    }

}

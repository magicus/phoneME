/*
 * @(#)InputStreamReader.java	1.35 06/10/10
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

import sun.io.ByteToCharConverter;
import sun.io.ConversionBufferFullException;


/**
 * An InputStreamReader is a bridge from byte streams to character streams: It
 * reads bytes and translates them into characters according to a specified <a
 * href="../lang/package-summary.html#charenc">character encoding</a>.  The
 * encoding that it uses may be specified by name, or the platform's default
 * encoding may be accepted.
 *
 * <p> Each invocation of one of an InputStreamReader's read() methods may
 * cause one or more bytes to be read from the underlying byte-input stream.
 * To enable the efficient conversion of bytes to characters, more bytes may
 * be read ahead from the underlying stream than are necessary to satisfy the
 * current read operation.
 *
 * <p> For top efficiency, consider wrapping an InputStreamReader within a
 * BufferedReader.  For example:
 *
 * <pre>
 * BufferedReader in
 *   = new BufferedReader(new InputStreamReader(System.in));
 * </pre>
 *
 * @see BufferedReader
 * @see InputStream
 * @see <a href="../lang/package-summary.html#charenc">Character encodings</a>
 *
 * @version 	1.25, 00/02/02
 * @author	Mark Reinhold
 * @since	JDK1.1
 */

public class InputStreamReader extends Reader {

    private ByteToCharConverter btc;
    private InputStream in;

    private static final int defaultByteBufferSize = 8192;
    private byte bb[];		/* Input buffer */

    /**
     * Create an InputStreamReader that uses the default character encoding.
     *
     * @param  in   An InputStream
     */
    public InputStreamReader(InputStream in) {
	this(in, ByteToCharConverter.getDefault());
    }

    /**
     * Create an InputStreamReader that uses the named character encoding.
     *
     * @param  in   An InputStream
     * @param  enc  The name of a supported
     *              <a href="../lang/package-summary.html#charenc">character
     *              encoding</a>
     *
     * @exception  UnsupportedEncodingException
     *             If the named encoding is not supported
     */
    public InputStreamReader(InputStream in, String enc)
	throws UnsupportedEncodingException
    {
	this(in, ByteToCharConverter.getConverter(enc));
    }

    /**
     * Create an InputStreamReader that uses the specified byte-to-character
     * converter.  The converter is assumed to have been reset.
     *
     * @param  in   An InputStream
     * @param  btc  A ByteToCharConverter
     */
    private InputStreamReader(InputStream in, ByteToCharConverter btc) {
	super(in);
	if (in == null) 
	    throw new NullPointerException("input stream is null");
	this.in = in;
	this.btc = btc;
	bb = new byte[defaultByteBufferSize];
    }

    /**
     * Returns the canonical name of the character encoding being used by this
     * stream.  
     *
     * <p> If this instance was created with the {@link
     * #InputStreamReader(InputStream, String)} constructor then the returned
     * encoding name, being canonical, may differ from the encoding name passed 
     * to the constructor. This method may return <code>null</code> if the stream
     * has been closed. </p>
     *
     * <p> NOTE : In J2ME CDC, there is no concept of historical name, so only
     * canonical name of character encoding is returned.
     *
     * @return a String representing the encoding name, or possibly
     *         <code>null</code> if the stream has been closed
     *
     * @see <a href="../lang/package-summary.html#charenc">Character
     *      encodings</a>
     */
    public String getEncoding() {
	synchronized (lock) {
	    if (btc != null)
		return btc.getCharacterEncoding();
	    else
		return null;
	}
    }


    /* Buffer handling */

    private int nBytes = 0;	/* -1 implies EOF has been reached */
    private int nextByte = 0;

    private void malfunction() {
	throw new InternalError("Converter malfunction (" +
				btc.getCharacterEncoding() +
				") -- please submit a bug report via " +
				System.getProperty("java.vendor.url.bug"));
    }

    private int convertInto(char cbuf[], int off, int end) throws IOException {
	int nc = 0;
	if (nextByte < nBytes) {
	    try {
		nc = btc.convert(bb, nextByte, nBytes,
				 cbuf, off, end);
		nextByte = nBytes;
		if (btc.nextByteIndex() != nextByte)
		    malfunction();
	    }
	    catch (ConversionBufferFullException x) {
		nextByte = btc.nextByteIndex();
		nc = btc.nextCharIndex() - off;
	    }
	}
	return nc;
    }

    private int flushInto(char cbuf[], int off, int end) throws IOException {
	int nc = 0;
	try {
	    nc = btc.flush(cbuf, off, end);
	}
	catch (ConversionBufferFullException x) {
	    nc = btc.nextCharIndex() - off;
	}
	return nc;
    }

    private int fill(char cbuf[], int off, int end) throws IOException {
	int nc = 0;

	if (nextByte < nBytes)
	    nc = convertInto(cbuf, off, end);

	while (off + nc < end) {

	    if (nBytes != -1) {
		if ((nc > 0) && !inReady())
		    break;	/* Block at most once */
                nBytes = in.read(bb, 0, Math.min(end - off, bb.length));
	    }

	    if (nBytes == -1) {
                nBytes = 0; /* Allow file to grow */
		nc += flushInto(cbuf, off + nc, end);
		if (nc == 0)
		    return -1;
		else
		    break;
	    }
	    else {
		nextByte = 0;
		nc += convertInto(cbuf, off + nc, end);
	    }
	}
	return nc;
    }

    /**
     * Tell whether the underlying byte stream is ready to be read.  Return
     * false for those streams that do not support available(), such as the
     * Win32 console stream.
     */
    private boolean inReady() {
	try {
	    return in.available() > 0;
	} catch (IOException x) {
	    return false;
	}
    }


    /** Check to make sure that the stream has not been closed */
    private void ensureOpen() throws IOException {
	if (in == null)
	    throw new IOException("Stream closed");
    }

    /**
     * Read a single character.
     *
     * @return The character read, or -1 if the end of the stream has been
     *         reached
     *
     * @exception  IOException  If an I/O error occurs
     */
    public int read() throws IOException {
	char cb[] = new char[1];
	if (read(cb, 0, 1) == -1)
	    return -1;
	else
	    return cb[0];
    }

    /**
     * Read characters into a portion of an array.
     *
     * @param      cbuf  Destination buffer
     * @param      off   Offset at which to start storing characters
     * @param      len   Maximum number of characters to read
     *
     * @return     The number of characters read, or -1 if the end of the stream
     *             has been reached
     *
     * @exception  IOException  If an I/O error occurs
     */
    public int read(char cbuf[], int off, int len) throws IOException {
	synchronized (lock) {
	    ensureOpen();
            if ((off < 0) || (off > cbuf.length) || (len < 0) ||
                ((off + len) > cbuf.length) || ((off + len) < 0)) {
                throw new IndexOutOfBoundsException();
            } else if (len == 0) {
                return 0;
            }
	    return fill(cbuf, off, off + len);
	}
    }

    /**
     * Tell whether this stream is ready to be read.  An InputStreamReader is
     * ready if its input buffer is not empty, or if bytes are available to be
     * read from the underlying byte stream.
     *
     * @exception  IOException  If an I/O error occurs
     */
    public boolean ready() throws IOException {
	synchronized (lock) {
	    ensureOpen();
	    return (nextByte < nBytes) || inReady();
	}
    }

    /**
     * Close the stream.
     *
     * @exception  IOException  If an I/O error occurs
     */
    public void close() throws IOException {
	synchronized (lock) {
	    if (in == null)
		return;
	    in.close();
	    in = null;
	    bb = null;
	    btc = null;
	}
    }

}

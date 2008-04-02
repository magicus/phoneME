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
 */

package javax.microedition.media.protocol;

import java.io.IOException;
import javax.microedition.media.Controllable;

/**
 * Abstracts a single stream of media data.  It is used in
 * conjunction with <code>DataSource</code> to provide the
 * input interface to a <code>Player</code>
 * <p>
 * SourceStream may provide type-specific controls.  For that
 * reason, it implements the <code>Controllable</code> interface
 * to provide additional controls.
 * 
 * @see DataSource
 *
 */

public interface SourceStream extends Controllable {

    /**
     * The value returned by <code>getSeekType</code> indicating that this 
     * <code>SourceStream</code> is not seekable.
     * <p>
     * Value 0 is assigned to <code>NOT_SEEKABLE</code>.
     */
    int NOT_SEEKABLE = 0;

    /**
     * The value returned by <code>getSeekType</code> indicating that this 
     * <code>SourceStream</code> can be seeked only to the beginning 
     * of the media stream. 
     * <p>
     * Value 1 is assigned to <code>SEEKABLE_TO_START</code>.
     */
    int SEEKABLE_TO_START = 1;

    /**
     * The value returned by <code>getSeekType</code> indicating that this 
     * <code>SourceStream</code> can be seeked anywhere within the media.
     * <p>
     * Value 2 is assigned to <code>RANDOM_ACCESSIBLE</code>.
     */
    int RANDOM_ACCESSIBLE = 2;
    
    /**
     * Get the content type for this stream.
     *
     * @return The current <CODE>ContentDescriptor</CODE> for this stream.
     */
    ContentDescriptor getContentDescriptor();


    /**
     * Get the size in bytes of the content on this stream.
     *
     * @return The content length in bytes.  -1 is returned if the 
     * length is not known.
     */
    long getContentLength();
    

/* JAVADOC ELIDED */
    int read(byte[] b, int off, int len)
	throws IOException;


    /**
     * Get the size of a "logical" chunk of media data from the source.
     * This method can be used to determine the minimum size of the
     * buffer to use in conjunction with the <code>read</code> method 
     * to read data from the source.
     * 
     * @return The minimum size of the buffer needed to read a "logical"
     * chunk of data from the source.  Returns -1 if the size cannot be
     * determined. 
     * @see #read(byte[], int, int)
     */
    int getTransferSize();


/* JAVADOC ELIDED */
    long seek(long where) throws IOException;


    /**
     * Obtain the current position in the stream.
     * @return The current position in the stream.
     */
    long tell();

   
    /**
     * Find out if the stream is seekable.
     * The return value can be one of these three:
     * <code>NOT_SEEKABLE</code>, <code>SEEKABLE_TO_START</code> and
     * <code>RANDOM_ACCESSIBLE</code>.
     * If the return value is <code>SEEKABLE_TO_START</code>, it means
     * that the stream can only be repositioned to the beginning of
     * the stream.  If the return value is <code>RANDOM_ACCESSIBLE</code>,
     * the stream can be seeked anywhere within the stream.
     *
     * @return Returns an enumerated value to indicate the level of seekability.
     */
    int getSeekType();
}

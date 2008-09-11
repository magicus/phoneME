/*
 * %W% %E%
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
package com.sun.jump.module.download;

import java.io.IOException;
import java.io.InputStream;
import java.net.URL;

/**
 * <code>JUMPDownloadDestination</code> provides the interface to 
 * receive the data that is downloaded by the <code>JUMPDownloadAction</code>.
 * The <Code>start()</code> is called first before the download operation
 * and if not exceptions are thrown by the method, then the 
 * <code>receive()</code> method is called one or more times as the 
 * downloading of data happens. Finally the <code>finish()</code> method
 * is called if the data has been successfully downloaded and delivered to
 * the destination or the <code>abort()</code> method is called if the
 * download operation was aborted by the user or the system.
 */
public interface JUMPDownloadDestination {
    /**
     * Notifies the destination the download is about to start.
     * 
     * @param sourceURL source URL
     * @param mimeType mime type of the object to be sent to
     * this destination.
     * @throws JUMPJUMPDownloadException is at implemntation's discretion
     * to throw an JUMPJUMPDownloadException anything goes wrong. The download
     * will be cancelled and this exception will be bubbled up.
     * @throws IOException signal that there was an IO error.
     */
    public void start(URL sourceURL, String mimeType) 
	throws JUMPDownloadException, IOException;

    /**
     * Requests this destination to receive a part of the object
     * being downloaded.
     * 
     * @param in The InputStream to read from
     * @param desiredLength The number of bytes to receive
     * @return the total number of bytes read, or <code>-1</code> is
     * there is no more data because the end of the stream has been
     * reached.
     * @throws JUMPDJUMPDownloadExceptionis at implemntation's discretion
     * to throw an JUMPDJUMPDownloadExceptionanything goes wrong. The download
     * will be cancelled and this exception will be bubbled up.
     * @throws IOException signal that there was an IO error.
     */
    public int receive( InputStream in, int desiredLength ) 
	throws JUMPDownloadException, IOException;

    /**
     * Notifies the destination that the object download is done. If the
     * destination has successfully stored the data, it returns the URL
     * where the destination has stored the application content.
     * 
     * @return the URL of the application content that was downloaded.
     * @throws JUMPJUMPDownloadException is at implemntation's discretion
     * to throw an JUMPJUMPDownloadException anything goes wrong. The download
     * will be cancelled and this exception will be bubbled up.
     * @throws IOException signal that there was an IO error.
     */
    public URL finish() throws JUMPDownloadException, IOException;

    /**
     * Notifies the destination that the object acquisition has
     * failed or was stopped. There will be no more bits sent, and the
     * {@link #finish()} method will not be called.
     */
    public void abort();

    /**
     * Asks the destination what's the maximum buffer size should
     * be used for downloading.
     * @return maximum buffer size to be used for downloading, or 0,
     * to let the implementation decide what's best.
     * <br>
     */
    public int getMaxChunkSize();
}

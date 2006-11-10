/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

/**
 * @version @(#)Destination.java	1.8 05/09/13
 */

package com.sun.appmanager.ota;

import java.io.IOException;
import java.io.InputStream;

/**
 * Generic interface to be used to specify different ways
 * to store downloaded data from OTA implementation.
 *
 * An instance of destination class should be provided to the
 * {@link OTA#download(Descriptor, Destination, DLIndicator)}
 * method. Since there are few methods that
 * practically change the state of a destination, this class should
 * not be MT safe, and multiple instances should be used for each
 * call to
 * {@link OTA#download(Descriptor, Destination, DLIndicator)}.
 */
public interface Destination {

    /**
     * Asks a destination whether it supports a certain mime type.
     * @param mimeType mimeType to be sent to this destination.
     * @throws OTAException if this mime type is not supported.
     */
    void acceptMimeType( String mimeType ) throws OTAException;

    /**
     * Notifies the destination the download is about to start.
     * @param sourceURL source URL
     * @param mimeType mime type of the object to be sent to
     * this destination.
     * @throws OTAException it is at implemntation's discretion
     * to throw an OTAException if anything goes wrong. The download
     * will be cancelled and this exception will be bubbled up.
     * @throws IOException signal that there was an IO error.
     */
    void start( String sourceURL,
                String mimeType ) throws OTAException, IOException;

    /**
     * Requests this destination to receive a part of the object
     * being downloaded.
     * @param in The InputStream to read from
     * @param desiredLength The number of bytes to receive
     * @return the total number of bytes read, or <code>-1</code> is
     * there is no more data because the end of the stream has been
     * reached.
     * @throws OTAException it is at implemntation's discretion
     * to throw an OTAException if anything goes wrong. The download
     * will be cancelled and this exception will be bubbled up.
     * @throws IOException signal that there was an IO error.
     */
    int receive( InputStream in, int desiredLength ) 
	throws OTAException, IOException;

    /**
     * Notifies the destination that the object download is done.
     * @throws OTAException it is at implemntation's discretion
     * to throw an OTAException if anything goes wrong. The download
     * will be cancelled and this exception will be bubbled up.
     * @throws IOException signal that there was an IO error.
     */
    void finish() throws OTAException, IOException;

    /**
     * Notifies the destination that the object acquisition has
     * failed or was stopped. There will be no more bits sent, and the
     * {@link #finish()} method will not be called.
     */
    void abort();

    /**
     * Asks the destination what's the maximum buffer size should
     * be used for downloading.
     * @return maximum buffer size to be used for downloading, or 0,
     * to let the OTA implementation decide what's best.
     * <br>
     */
    int getMaxChunkSize();
}

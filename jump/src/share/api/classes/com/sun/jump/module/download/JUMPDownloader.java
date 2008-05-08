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

import java.net.URL;

/**
 * <code>JUMPDownloader</code> encapsulates the logic of downloading
 * the application content using some download protocol. The download
 * process is initiated by calling {@link #start(JUMPDownloadDestination)} 
 * method. The download operation can be cancelled by calling
 * {@link #cancel} method. The <code>JUMPDownloadModule</code> is responsible 
 * for creating <code>JUMPDownloader</code> instances.
 */
public interface JUMPDownloader {
    /**
     * Sets the progress listener. The frequency in which the 
     * listener's <code>dataDownloaded()</code> would be called is 
     * left to the implementation of the download action. To control the
     * frequency, use the alternate form of the API.
     *
     * @param listener the listener that is interested in the download 
     *        progress.
     */
    public void setProgressListener(JUMPDownloadProgressListener listener);
    
    /**
     * Sets the progress listener with an update frequency.
     *
     * @param listener the listener that is interested in the download 
     *        progress.
     * @param updatePercent indicates how frequently the listener's
     *        <code>dataDownloaded()</code> method should be
     *        called (as a percentage of the data downloaded).
     *        So a value of 1 causes the listener 
     *        to be invoked everytime one more percent of the data is 
     *        downloaded
     */
    public void setProgressListener(JUMPDownloadProgressListener listener,
        int updatePercent);
    
    /**
     * Start the download action and start delivering the data to the
     * download destination specified. The call blocks till the data 
     * is downloaded and delivered to the destination or some error occurs
     * which results in an exception being thrown.
     * 
     * If a progress listener has been set using 
     * {@link #setProgressListener(JUMPDownloadProgressListener, int)}
     * then it will be notified about the download progress.
     * 
     * @return the URL pointing to the application content that was downloaded.
     * @throws JUMPJUMPDownloadException there was any error during downloading
     *         or the download was cancelled by calling 
     *         {@link #cancel}
     */
    public URL start(JUMPDownloadDestination destination) 
        throws JUMPDownloadException;
    
    /**
     * Cancels the download action. This method does not do anything if
     * the download action has not been started.
     */
    public void cancel();
    
    /**
     * Returns the download descriptor that is associated with the 
     * download action.
     */
    public JUMPDownloadDescriptor getDescriptor();
}

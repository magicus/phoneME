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

/**
 * <code>JUMPDownloadProgressListener</code> allows a component to observe
 * the download progress. The interface is typically implemented by UI
 * elements (like a progress bar/gauge) to show the state of the download
 * progress.
 */
public interface JUMPDownloadProgressListener {
    /**
     * Called before the downloading of data is about to start.
     */
    public void downloadStarted();
    
    /**
     * Called after the download has been cancelled. The cancellation could
     * be driven by the user or by the system. If this method is called then
     * <code>downloadCompleted()</code> will not be invoked.
     */
    public void downloadCancelled();
    
    /**
     * Called to indicate that the download has been complete. This indicates
     * that there was no error in the download process.
     */
    public void downloadCompleted();
    
    /**
     * Called as the data gets downloaded. The frequency of the call depends
     * on the <i>updatePercent</i> argument passed during the registration
     * of the listener. 
     *
     * @param percent the percentage of the data that has been downloaded 
     *        from the start of the download process. It starts from 0 and
     *        and goes upto 100 typically.
     */
    public void dataDownloaded(int percent);
}

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

/*
 * @(#)DLIndicator.java	1.8 05/09/13
 */

package com.sun.appmanager.ota;

/**
 * Provides an interface for a download client to be informed
 * regarding data download progress.
 */

public interface DLIndicator {

    /**
     * Called once before download is about to start.
     * The value returns tells how frequently the indicator
     * should be updated in percents. So, returning 1 will
     * cause download to update indicator every time one more
     * percent is downloaded.
     * @return granularity
     */
    int getGranularity();

    /**
     * Update callback. Called by downloaded each time a certain
     * amount of data has been downloaded.
     * @param value how much data in percentage is downloaded
     */
    void updatePercent( int value );

    /**
     * Check for cancelled download.
     * If at any time this method is called it returns true,
     * download is cancelled.
     * @return true if user/system cancels the download.
     */
    boolean isCancelled();

    /**
     * Return a synchronization object for download.
     * The object returned from this method will be used by
     * download thread to wait on so download is either completed
     * by the separate download thread, or is cancelled by the
     * system. So, to cancel the download, one must call notify
     * on the same object and then make
     * {@link #isCancelled() isCancelled} to return true.
     * If you return null from this method, the cancellation
     * will still be possible, but if IO is waiting, it won't
     * happen until it's done waiting.
     * @return object to syncrhonize download and cancellation
     */
    Object getLockNotifier();

    /**
     * Notify that the download is done. This doesn't mean, however,
     * that the download is successfully completed or 100% has been reached.
     */
    void downloadDone();

}

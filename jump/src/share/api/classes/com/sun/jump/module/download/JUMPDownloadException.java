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
 * <code>JUMPDownloadException</code> is thrown when the download action
 * does not perform successfully.
 */
public class JUMPDownloadException extends Exception {
    public static final int REASON_UNKNOWN = 0;
    public static final int REASON_UNSUPPORTED_MIME = 1;
    public static final int REASON_CANCELLED_BY_USER = 2;
    
    private int reason;
    
    /**
     * Creates a new instance of JUMPDownloadException
     */
    public JUMPDownloadException() {
        this("");
    }
    
    public JUMPDownloadException(String mesg){
        this(REASON_UNKNOWN, mesg);
    }
    
    public JUMPDownloadException(int reason, String mesg){
        super(mesg);
        if ( reason < REASON_UNKNOWN || reason > REASON_CANCELLED_BY_USER )
            reason = REASON_UNKNOWN;
        this.reason = reason;
    }
    
    /**
     * Returns the reason for this exception. If there is no concrete reason
     * for the exception then REASON_UNKNOWN is returned.
     */
    public int getReason() {
        return this.reason;
    }
}

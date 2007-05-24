/*
 *   
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

package com.sun.midp.jadtool;

/** Exception for AppDescriptor errors. */
public class AppDescriptorException extends Exception {

    /** Generic default code. (0) */
    public static int UNSPECIFIED_ERROR = 0;
    /** The keystore for the AppDescriptor was not initialized. */
    public static final int KEYSTORE_NOT_INITIALIZED = 4;

    /** The error code. */
    private int myError = 0;
    
    /**
     * Create an ApplicationDescriptorException.
     *
     * @param errorCode error code of the exception
     */
    public AppDescriptorException(int errorCode) {
        super();
        myError = errorCode;
    }
    
    /**
     * Create an ApplicationDescriptorException.
     *
     * @param msg message of the exception
     */
    public AppDescriptorException(String msg) {
        super(msg);
    }

    /**
     * Create an ApplicationDescriptorException.
     *
     * @param msg message of the exception
     * @param errorCode error code of the exception
     */
    public AppDescriptorException(String msg, int errorCode) {
        super(msg);
        myError = errorCode;
    }

    /**
     * Get the error code of the exception.
     *
     * @return error code
     */
    public int getErrorCode() {
        return myError;
    }
}

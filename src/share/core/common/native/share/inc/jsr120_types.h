/*
 *   
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
 */

#ifndef _JSR120_TYPES_H_
#define _JSR120_TYPES_H_

/**
 * The supported protocol types.
 */
typedef enum {

    /** The SMS protocol type. */
    WMA_SMS_PROTOCOL,

    /** The CBS protocol type. */
    WMA_CBS_PROTOCOL,

    /** The MMS protocol type. */
    WMA_MMS_PROTOCOL

} WMA_PROTOCOLS;

/**
 * The supported encoding formats.
 */
typedef enum {

    /** DCS: GSM Alphabet  */
    GSM_TEXT = 0,

    /** DCS: Binary */
    GSM_BINARY = 1,

    /** DCS: Unicode UCS-2 */
    GSM_UCS2 = 2

} WMA_ENCODING_FORMATS;

/**
 * Various status values that can be returned by a method.
 */
typedef enum {
    /**
     * Status value indicating successful completion.
     */
    WMA_OK,
    /**
     * Status value indicating that a error has occured 
     * successfully.
     */
    WMA_ERR,
    /**
     * Status value indicating that the network operation was completed 
     * successfully.
     */
    WMA_NET_SUCCESS,
    /**
     * Status value indicating that the finish-function will need to
     * be called later in order to complete the network operation.
     */
    WMA_NET_WOULDBLOCK,
    /**
     * Status value indicating that an I/O error of some sort has occurred.
     */
    WMA_NET_IOERROR,
    /**
     * Status value indicating that the operation was terminated by an interrupt.
     * This typically causes <code>InterruptedIOException</code> to be thrown in calling Java
     * thread.
     */
    WMA_NET_INTERRUPTED,
    /**
     * Status value indicating that there was an error and
     * <code>ConnectionNotFoundException</code> needs to be thrwon to the calling Java thread
     */
    WMA_NET_CONNECTION_NOTFOUND,
    /**
     * Status value indicating that a function parameter had an invalid value.
     */
    WMA_NET_INVALID
} WMA_STATUS;

#endif /* #ifdef _JSR120_TYPES_H_ */

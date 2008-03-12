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

package com.sun.midp.pki.ocsp;

import java.lang.String;

/**
 * The <CODE>OCSPException</CODE> encapsulates an error that
 * was indicated in the responce received from OCSP Responder.
 */
public class OCSPException extends Exception {

    /** The reason code for this exception */
    private byte reason;

    /**
     * Indicates that OCSP request doesn't conform to the OCSP syntax.
     */
    public static final byte MALFORMED_REQUEST = 1;

    /**
     * Indicates that OCSP responder reached an inconsistent internal state.
     */
    public static final byte INTERNAL_ERROR    = 2;

    /**
     * Indicates that the service exists, but is temporarily unable to respond.
     */
    public static final byte TRY_LATER         = 3;

    /**
     * Indicates that the server requires the client sign the request.
     */
    public static final byte SIG_REQUIRED      = 4;

    /**
     * Indicates that the client is not authorized to make this query.
     */
    public static final byte UNAUTHORIZED      = 5;

    /**
     * Create a new exception with a specific error reason.
     * The descriptive message for the new exception will be
     * automatically provided, based on the reason.
     *
     * @param status the reason for the exception
     */
    public OCSPException(byte status) {
        reason = status;
    }

    /**
     * Get the reason code.
     * @return the reason code
     */
    public byte getReason() {
        return reason;
    }

    /**
     * Gets the exception message for a reason.
     *
     * @param reason reason code
     *
     * @return exception message
     */
    static String getMessageForReason(int reason) {
        switch (reason) {
            case MALFORMED_REQUEST: {
                return "OCSP request doesn't conform to the OCSP syntax";
            }
        }

        return "Unknown reason (" + reason + ")";
    }
}

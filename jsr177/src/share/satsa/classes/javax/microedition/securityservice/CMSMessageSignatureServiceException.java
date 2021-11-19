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

package javax.microedition.securityservice;

/**
 * 
 * This class is used to identify error conditions
 * detected while signing messages.
 * Thrown by the <code>CMSMessageSignatureService</code> and
 * <code>UserCredentialManager</code>
 * classes.
 */
final public class CMSMessageSignatureServiceException extends Exception
{
    /**
     * Error code returned if a cyptographic error occured.
     */
    public static final byte CRYPTO_FAILURE = 0x01;

    /**
     * Error code returned if an error occurs when formatting a
     * result.
     */
    public static final byte CRYPTO_FORMAT_ERROR = 0x02;

    /**
     * Error code returned if detached signatures
     * are not supported.
     */
    public static final byte CRYPTO_NO_DETACHED_SIG = 0x03;

    /**
     * Error code returned if opaque signatures
     * are not supported.
     */
    public static final byte CRYPTO_NO_OPAQUE_SIG = 0x04;

    /**
     * Error code returned if security element is busy.
     */
    public static final byte SE_BUSY = 0x05;

    /**
     * Error code returned if an operation involving the security
     * element fails.
     */
    public static final byte SE_FAILURE = 0x06;

    /**
     * Error code returned if a cryptographic operation failed in
     * a security element.
     */
    public static final byte SE_CRYPTO_FAILURE = 0x07;

    /**
     * Error code returned if a certificate is not available
     * on the device for the selected public key.
     */
    public static final byte CRYPTO_NO_CERTIFICATE = 0x08;

    /** 
     * The reason code for exception.
     */
    private byte reasonCode;

    /**
     * Construct an exception with specific reason code.
     * @param code  the code for the error condition
     */
    public CMSMessageSignatureServiceException(byte code) {
        super(getMessageForReason(code));
	reasonCode = code;
    }

    /**
     * Gets the reason code.
     * @return the reason code for the error detected
     */
    public byte getReason() {
        return reasonCode; 
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
        case CRYPTO_FAILURE:
            return "Failed to perform cryptographic function";

        case CRYPTO_FORMAT_ERROR:
            return "Formatting error during cryptographic processing";

        case CRYPTO_NO_DETACHED_SIG:
            return "Detached signatures not supported";

        case CRYPTO_NO_OPAQUE_SIG:
            return "Opaque signatures not supported";

        case SE_BUSY:
            return "Security element is busy";

        case SE_FAILURE:
            return "Security element failure";

        case SE_CRYPTO_FAILURE:
            return "Security element failed cryptographic request";

        case CRYPTO_NO_CERTIFICATE:
            return "Certificate was not found for cryptographic operation";
        }

        return "Unknown reason (" + reason + ")";
    }
    
}


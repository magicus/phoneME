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

package javax.microedition.pki;

/**
 * This class is used to identify error conditions in
 * the management of the user certificate store.
 * Thrown by the <code>CMSMessageSignatureService</code> and
 * <code>UserCredentialManager</code> classes.
 */
final public class UserCredentialManagerException extends Exception
{
    /**
     * Code returned if a credential can not be added.
     * For example, it is returned if there is insufficient 
     * memory to add additional credentials
     */
    public static final byte CREDENTIAL_NOT_SAVED = 0x00;

    /**
     * Code returned if a security element does not
     * support key generation.
     */
    public static final byte SE_NO_KEYGEN = 0x01;

    /**
     * Code returned if a security element does not
     * have keys available for certificate requests.
     */
    public static final byte SE_NO_KEYS = 0x02;

    /**
     * Code returned if a security element does not
     * have any keys available that are not already
     * associated with a certificate, and if the 
     * platform does not allow reuse of keys that
     * are associated with an existing certificate.
     */
    public static final byte SE_NO_UNASSOCIATED_KEYS = 0x03;

    /**
     * Code returned if an appropriate security element
     * can not be found.
     */
    public static final byte SE_NOT_FOUND = 0x04;

    /**
     * Code returned if an appropriate certificate
     * can not be found.
     */
    public static final byte CREDENTIAL_NOT_FOUND = 0x05;

    /** 
     * The reason code for exception.
     */
    private byte reasonCode;

    /**
     * Construct an exception with specific reason code.
     * @param code  the code for the error condition
     */
    public UserCredentialManagerException(byte code) {
        super(getMessageForReason(code));
	reasonCode = code;
    }
    /**
     * Gets the reason code.
     * @return the code for the error condition
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
        case CREDENTIAL_NOT_SAVED:
            return "Could not save credential";
        case SE_NO_KEYGEN:
            return "Security element does not support key generation";
        case SE_NO_KEYS:
            return "Security Element has no keys";
        case SE_NO_UNASSOCIATED_KEYS:
            return "Security Element has no unassociated keys";
        case SE_NOT_FOUND:
            return "Security Element was not found";
        case CREDENTIAL_NOT_FOUND:
            return "Credential was not found";
        }

        return "Unknown reason (" + reason + ")";
    }
    

}

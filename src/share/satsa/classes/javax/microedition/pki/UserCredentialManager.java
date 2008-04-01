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

import javax.microedition.securityservice.CMSMessageSignatureServiceException;
import javax.microedition.pki.UserCredentialManagerException;

import com.sun.j2me.main.Configuration;

// JAVADOC COMMENT ELIDED
final public class UserCredentialManager
{
    /**
     * Algorithm identifier for an RSA signature key.
     * This is the <code>String</code> representation of the
     * OID identifying the RSA algorithm.
     */
    public final static String ALGORITHM_RSA = "1.2.840.113549.1.1";

    /**
     * Algorithm identifier for a DSA signature key.
     * This is the <code>String</code> representation of the
     * OID identifying a DSA signature key.
     */
    public final static String ALGORITHM_DSA = "1.2.840.10040.4.1";

    /**
     * Indicates a key used for authentication.
     */
    public final static int KEY_USAGE_AUTHENTICATION = 0;

    /**
     * Indicates a key used for digital signatures.
     */
    public final static int KEY_USAGE_NON_REPUDIATION = 1;

    /**
     * Indicates that key generation is supported
     * on the platform.
     */
    private static boolean keygen = false;

    static  {
	String generation  = Configuration
	    .getProperty("com.sun.satsa.keygen");
    if (generation != null) {
	    keygen = generation.equals("true");
	}
    }
    
    /**
     * Constructor for the <code>UserCredentialManager</code> class.
     */
    private UserCredentialManager() {
    }

    // JAVADOC COMMENT ELIDED
    public static final byte[] generateCSR(String nameInfo, 
                                           String algorithm,
                                           int keyLen, 
                                           int keyUsage, 
                                           String securityElementID,
                                           String securityElementPrompt, 
                                           boolean forceKeyGen)
        throws UserCredentialManagerException,
	CMSMessageSignatureServiceException {

	/* User requested a new key be generated. */
	if (forceKeyGen) {
	    if (! UserCredentialManager.keygen) {
		// Configuration parameter disabled key generation.
		throw new UserCredentialManagerException
		    (UserCredentialManagerException.SE_NO_KEYGEN);
	    }
	}

        return com.sun.satsa.pki.PKIManager.generateCSR(nameInfo,
                    algorithm, keyLen, keyUsage, securityElementID,
                    securityElementPrompt, forceKeyGen);
    }

    // JAVADOC COMMENT ELIDED
    public static final boolean addCredential(String certDisplayName,
                                              byte[] pkiPath, String uri)
            throws UserCredentialManagerException {
        return com.sun.satsa.pki.PKIManager.addCredential(certDisplayName,
                pkiPath, uri);
    }

    // JAVADOC COMMENT ELIDED
    public static final boolean removeCredential(String certDisplayName,
        byte[] issuerAndSerialNumber, String securityElementID,
        String securityElementPrompt)
            throws UserCredentialManagerException {

        return com.sun.satsa.pki.PKIManager.removeCredential(
                certDisplayName, issuerAndSerialNumber, securityElementID,
                securityElementPrompt);
    }
}

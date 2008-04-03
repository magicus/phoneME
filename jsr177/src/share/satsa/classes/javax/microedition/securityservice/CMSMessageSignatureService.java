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

import javax.microedition.securityservice.CMSMessageSignatureServiceException;
import javax.microedition.pki.UserCredentialManagerException;

import com.sun.j2me.main.Configuration;

// JAVADOC COMMENT ELIDED
 
final public class CMSMessageSignatureService
{
    /**
     * Includes the content that was signed in the signature.
     * If this option is specified and the <code>sign</code>
     * and <code>authenticate</code> methods do not support
     * opaque signatures, a
     * <code>CMSMessageSignatureServiceException</code>
     * exception MUST be thrown and the
     * <code>getReason</code> method
     * MUST return the <code>CRYPTO_NO_OPAQUE_SIG</code>
     * error code.
     */
    public static final int SIG_INCLUDE_CONTENT = 1;
    
    /**
     * Includes the user certificate in the signature.
     * If this option is specified and the certificate is not
     * available, a
     * <code>CMSMessageSignatureServiceException</code>
     * exception MUST be thrown and the
     * <code>getReason</code> method
     * MUST return the <code>CRYPTO_NO_CERTIFICATE</code>
     * error code.
     */    
    public static final int SIG_INCLUDE_CERTIFICATE = 2;


    /** Unassigned option bits. */
    static int mask = ~(SIG_INCLUDE_CONTENT|SIG_INCLUDE_CERTIFICATE);


    /**
     * Indicates that certificate inclusion is supported
     * on the platform.
     */
    private static boolean certSig = false;

    static {
        String signprop  = Configuration
            .getProperty("com.sun.satsa.certsig");
        if (signprop != null) {
            certSig = signprop.equals("true");
        }
    }

    /**
     * Constructor for the CMSMessageSignatureService class.
     */
    private CMSMessageSignatureService() {
    }
    
    // JAVADOC COMMENT ELIDED
    public static final byte[] sign(
				    String stringToSign, 
				    int options,
				    String[] caNames,
				    String securityElementPrompt) 
	throws CMSMessageSignatureServiceException,
	       UserCredentialManagerException {
	
	/* Validate the options selected. */
	checkOptions(options);

        return com.sun.satsa.pki.PKIManager.sign(
                com.sun.satsa.pki.PKIManager.SIGN_STRING,
                null, stringToSign, options, caNames,
                securityElementPrompt);
    }

    // JAVADOC COMMENT ELIDED
    public static final byte[] authenticate(
					    byte[] byteArrayToAuthenticate,
					    int options, 
					    String[] caNames,
					    String securityElementPrompt) 
	throws CMSMessageSignatureServiceException,
	       UserCredentialManagerException {

	/* Validate the options selected. */
	checkOptions(options);

        return com.sun.satsa.pki.PKIManager.sign(
                com.sun.satsa.pki.PKIManager.AUTHENTICATE_DATA,
                byteArrayToAuthenticate, null, options, caNames,
                securityElementPrompt);
    }

    // JAVADOC COMMENT ELIDED
    public static final byte[] authenticate(
					    String stringToAuthenticate,
					    int options, 
					    String[] caNames,
					    String securityElementPrompt) 
	throws CMSMessageSignatureServiceException,
	       UserCredentialManagerException {
	/* Validate the options selected. */
	checkOptions(options);

        return com.sun.satsa.pki.PKIManager.sign(
                com.sun.satsa.pki.PKIManager.AUTHENTICATE_STRING,
                null, stringToAuthenticate, options, caNames,
                securityElementPrompt);
    }

    // JAVADOC COMMENT ELIDED
    static void checkOptions(int options) 
	throws CMSMessageSignatureServiceException {
	/* Check for valid arguments. */
	if ((options & mask) != 0) {
	    throw new IllegalArgumentException("Invalid signing options ");
	}

	if ((options & CMSMessageSignatureService. SIG_INCLUDE_CERTIFICATE)
	    == CMSMessageSignatureService.SIG_INCLUDE_CERTIFICATE) {
	    if (!CMSMessageSignatureService.certSig) {
		throw new CMSMessageSignatureServiceException
		    (CMSMessageSignatureServiceException.CRYPTO_NO_CERTIFICATE);
	    }
	}
    }
}



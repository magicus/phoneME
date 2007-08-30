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
  
package java.security;

import com.sun.satsa.crypto.RSAPublicKey;

/**
 * This <code>Signature</code> class is used to provide applications 
 * the functionality
 * of a digital signature algorithm. Digital signatures are used for
 * authentication and integrity assurance of digital data.
 *
 * <p> The signature algorithm can be, among others, the NIST standard
 * DSA, using DSA and SHA-1. The DSA algorithm using the
 * SHA-1 message digest algorithm can be specified as <tt>SHA1withDSA</tt>.
 * In the case of RSA, there are multiple choices for the message digest
 * algorithm, so the signing algorithm could be specified as, for example,
 * <tt>MD2withRSA</tt>, <tt>MD5withRSA</tt>, or <tt>SHA1withRSA</tt>.
 * The algorithm name must be specified, as there is no default.
 *
 * When an algorithm name is specified, the system will
 * determine if there is an implementation of the algorithm requested
 * available in the environment, and if there is more than one, if
 * there is a preferred one.<p>
 *
 * <p>A <code>Signature</code> object can be used to generate and 
 * verify digital signatures.
 *
 * <p>There are three phases to the use of a <code>Signature</code>
 *  object for verifying a signature:<ol>
 *
 * <li>Initialization, with a public key, which initializes the
 * signature for  verification
 * </li>
 *
 * <li>Updating
 *
 * <p>Depending on the type of initialization, this will update the
 * bytes to be verified. </li>
 * <li> Verifying a signature on all updated bytes. </li>
 *
 * </ol>
 *
 *
 * @version 1.91, 01/23/03
 */

public abstract class Signature  {

    /**
     * Signature implementation.
     */
    com.sun.midp.crypto.Signature sign;

    /**
     * Creates a <code>Signature</code> object for the specified algorithm.
     *
     * @param algorithm the standard string name of the algorithm. 
     * See Appendix A in the 
     * Java Cryptography Architecture API Specification &amp; Reference
     * for information about standard algorithm names.
     */
    Signature(String algorithm) {
    }

    /**
     * Generates a <code>Signature</code> object that implements
     * the specified digest
     * algorithm.
     *
     * @param algorithm the standard name of the algorithm requested. 
     * See Appendix A in the 
     * Java Cryptography Architecture API Specification &amp; Reference 
     * for information about standard algorithm names.
     *
     * @return the new <code>Signature</code> object.
     *
     * @exception NoSuchAlgorithmException if the algorithm is
     * not available in the environment.
     */
    public static Signature getInstance(String algorithm) 
                                       throws NoSuchAlgorithmException {
        try {
            return new SignatureImpl(algorithm,
                com.sun.midp.crypto.Signature.getInstance(algorithm));
        } catch (com.sun.midp.crypto.NoSuchAlgorithmException e) {
            throw new NoSuchAlgorithmException(e.getMessage());
        }
    }

    /**
     * Initializes this object for verification. If this method is called
     * again with a different argument, it negates the effect
     * of this call.
     *
     * @param publicKey the public key of the identity whose signature is
     * going to be verified.
     *
     * @exception InvalidKeyException if the key is invalid.
     */
    public final void initVerify(PublicKey publicKey) 
	throws InvalidKeyException {
        if (! (publicKey instanceof RSAPublicKey)) {
            throw new InvalidKeyException();
        }

        try {
            sign.initVerify((com.sun.midp.crypto.PublicKey)((RSAPublicKey)publicKey).getKey().getKey());
        } catch (com.sun.midp.crypto.InvalidKeyException e) {
            throw new InvalidKeyException(e.getMessage());
        }
    }

    /**
     * Verifies the passed-in signature. 
     * 
     * <p>A call to this method resets this signature object to the state 
     * it was in when previously initialized for verification via a
     * call to <code>initVerify(PublicKey)</code>. That is, the object is 
     * reset and available to verify another signature from the identity
     * whose public key was specified in the call to <code>initVerify</code>.
     *      
     * @param signature the signature bytes to be verified.
     *
     * @return true if the signature was verified, false if not. 
     *
     * @exception SignatureException if this signature object is not 
     * initialized properly, or the passed-in signature is improperly 
     * encoded or of the wrong type, etc.
     */
    public final boolean verify(byte[] signature) throws SignatureException {
        try {
            return sign.verify(signature);
        } catch (com.sun.midp.crypto.SignatureException e) {
            throw new SignatureException(e.getMessage());
        }
    }

    /**
     * Updates the data to be verified, using the specified
     * array of bytes, starting at the specified offset.  
     *
     * @param data the array of bytes.  
     * @param off the offset to start from in the array of bytes.  
     * @param len the number of bytes to use, starting at offset.
     *  
     * @exception SignatureException if this signature object is not 
     * initialized properly.          
     */
    public final void update(byte[] data, int off, int len) 
	throws SignatureException {
        try {
            sign.update(data, off, len);
        } catch (com.sun.midp.crypto.SignatureException e) {
            throw new SignatureException(e.getMessage());
        }
    }
}
    

/**
 * The non-abstract signature.
 */
class SignatureImpl extends Signature {

    /**
     * Creates a <code>Signature</code> object for the specified algorithm.
     *
     * @param algorithm the standard string name of the algorithm.
     * See Appendix A in the
     * Java Cryptography Architecture API Specification &amp; Reference
     * for information about standard algorithm names.
     * @param sign signature implementation
     */
    SignatureImpl(String algorithm, com.sun.midp.crypto.Signature sign) {
        super(algorithm);
        this.sign = sign;
    }
}	    



	    
	    
	

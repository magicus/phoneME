/*
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

package com.sun.j2me.crypto;

import java.security.SignatureException;
import java.security.PrivateKey;
import java.security.PublicKey;

import java.security.MessageDigest;
import java.security.GeneralSecurityException;

/**
 * Implements RSA Signatures.
 */ 
public class RSASignature {
    
    /** Current message digest. */
    MessageDigest md = null;

    /** Current cipher. */
    Cipher c = null;

    /** Current key. */
    RSAKey k = null;

    /** Signature prefix. */
    byte[] prefix;    

    /**
     * Constructs an RSA signature object that uses the specified
     * signature algorithm.
     *
     * @param sigPrefix Prefix for the signature
     * @param messageDigest Message digest for the signature
     *
     * @exception NoSuchAlgorithmException if RSA is
     * not available in the caller's environment.  
     */
    RSASignature(byte[] sigPrefix, MessageDigest messageDigest)
            throws NoSuchAlgorithmException {
        prefix = sigPrefix;
        md = messageDigest;

        try {
            c = Cipher.getNewInstance("RSA");
        } catch (NoSuchPaddingException e) {
            // we used the default mode and padding this should not happen
            throw new NoSuchAlgorithmException();
        }
    }

    /**
     * Initializes the <CODE>RSASignature</CODE> object with the appropriate
     * <CODE>Key</CODE> for signature verification.
     * 
     * @param theKey the key object to use for verification
     *
     * @exception InvalidKeyException if the key type is inconsistent 
     * with the mode or signature implementation.
     */
    public void initVerify(PublicKey publicKey) 
        throws InvalidKeyException {

        if (!(publicKey instanceof RSAPublicKey)) {
            throw new InvalidKeyException();
        }

        c.init(Cipher.DECRYPT_MODE, publicKey);
        k = (RSAKey)publicKey;
    }

    /**
     * Accumulates a signature of the input data. When this method is used,
     * temporary storage of intermediate results is required. This method
     * should only be used if all the input data required for the signature
     * is not available in one byte array. The sign() or verify() method is 
     * recommended whenever possible. 
     *
     * @param inBuf the input buffer of data to be signed
     * @param inOff starting offset within the input buffer for data to
     *              be signed
     * @param inLen the byte length of data to be signed
     *
     * @exception SignatureException if this signature object is not 
     * initialized properly.          
     */
    public void update(byte[] b, int off, int len) 
        throws SignatureException {
        if (k == null) {
            throw new SignatureException("Illegal State");
        }

        md.update(b, off, len);
    }

    /**
     * Verifies the signature of all/last input data against the passed
     * in signature. A call to this method also resets this signature 
     * object to the state it was in when previously initialized via a
     * call to init(). That is, the object is reset and available to 
     * verify another message.
     * 
     * @param sigBuf the input buffer containing signature data
     * @param sigOff starting offset within the sigBuf where signature
     *               data begins
     * @param sigLen byte length of signature data
     *
     * @return true if signature verifies, false otherwise
     *
     * @exception SignatureException if this signature object is not 
     * initialized properly, or the passed-in signature is improperly 
     * encoded or of the wrong type, etc.
     */
    public boolean verify(byte[] sigBuf, int sigOff, int sigLen)
            throws SignatureException {
        if (k == null || !(k instanceof RSAPublicKey)) {
            throw new SignatureException("Illegal State");
        }

        byte[] res = null;
        int val;
        byte[] digest = new byte[md.getDigestLength()];

        try {
            md.digest(digest, 0, digest.length);
            res = new byte[k.getModulusLen()];
            val = c.doFinal(sigBuf, sigOff, sigLen, res, 0);
        }
        catch (IllegalArgumentException iae) {
            throw new SignatureException(iae.getMessage());
        }
        catch (GeneralSecurityException e) {
            return false;
        }

        int size = prefix.length + md.getDigestLength();

        if (val != size) {
            return false;
        }

        // Match the prefix corresponding to the signature algorithm
        for (int i = 0; i < prefix.length; i++) {
            if (res[i] != prefix[i]) {
                return false;
            }
        }

        for (int i = prefix.length; i < size; i++) {
            if (res[i] != digest[i - prefix.length]) {
                return false;
            }
        }

        return true;
    }
}

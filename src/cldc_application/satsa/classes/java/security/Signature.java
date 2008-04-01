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

// JAVADOC COMMENT ELIDED

public abstract class Signature  {

    com.sun.midp.crypto.Signature sign;

    // JAVADOC COMMENT ELIDED
    Signature(String algorithm) {
    }

    // JAVADOC COMMENT ELIDED
    public static Signature getInstance(String algorithm) 
                                       throws NoSuchAlgorithmException {
        try {
            return new SignatureImpl(algorithm,
                com.sun.midp.crypto.Signature.getInstance(algorithm));
        } catch (com.sun.midp.crypto.NoSuchAlgorithmException e) {
            throw new NoSuchAlgorithmException(e.getMessage());
        }
    }

    // JAVADOC COMMENT ELIDED
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

    // JAVADOC COMMENT ELIDED
    public final boolean verify(byte[] signature) throws SignatureException {
        try {
            return sign.verify(signature);
        } catch (com.sun.midp.crypto.SignatureException e) {
            throw new SignatureException(e.getMessage());
        }
    }

    // JAVADOC COMMENT ELIDED
    public final void update(byte[] data, int off, int len) 
	throws SignatureException {
        try {
            sign.update(data, off, len);
        } catch (com.sun.midp.crypto.SignatureException e) {
            throw new SignatureException(e.getMessage());
        }
    }
}
    
class SignatureImpl extends Signature {

    // JAVADOC COMMENT ELIDED
    SignatureImpl(String algorithm, com.sun.midp.crypto.Signature sign) {
        super(algorithm);
        this.sign = sign;
    }
}	    



	    
	    
	

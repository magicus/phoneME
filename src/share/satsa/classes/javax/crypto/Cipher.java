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
 *
 * NOTE:
 * Because of various external restrictions (i.e. US export
 * regulations, etc.), the actual source code can not be provided
 * at this time. This file represents the skeleton of the source
 * file, so that javadocs of the API can be created.
 */

package javax.crypto;

import java.security.InvalidAlgorithmParameterException;
import java.security.InvalidKeyException;
import java.security.Key;
import java.security.NoSuchAlgorithmException;
import java.security.spec.*;

import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;

import com.sun.satsa.crypto.RSAPublicKey;

// JAVADOC COMMENT ELIDED
public class Cipher {

    /**
     * Constant used to initialize cipher to encryption mode.
     */
    public static final int ENCRYPT_MODE = 1;

    /**
     * Constant used to initialize cipher to decryption mode.
     */
    public static final int DECRYPT_MODE = 2;

    /** Cipher implementation object. */
    private com.sun.j2me.crypto.Cipher cipher;

    /**
     * Creates a Cipher object.
     * @param cipher cipher implementation
     * of algorithm is being used
     */
    private Cipher(com.sun.j2me.crypto.Cipher cipher) {
        this.cipher = cipher;
    }

    // JAVADOC COMMENT ELIDED
    public static final Cipher getInstance(String transformation)
        throws NoSuchAlgorithmException, NoSuchPaddingException {

        try {
            return new Cipher(
                com.sun.j2me.crypto.Cipher.getNewInstance(transformation));
        } catch (com.sun.j2me.crypto.NoSuchAlgorithmException e) {
            throw new NoSuchAlgorithmException(e.getMessage());
        } catch (com.sun.j2me.crypto.NoSuchPaddingException e) {
            throw new NoSuchPaddingException(e.getMessage());
        }
    }

    // JAVADOC COMMENT ELIDED
    public final void init(int opmode, Key key) throws InvalidKeyException {

        try {
            init(opmode, key, null);
        } catch (InvalidAlgorithmParameterException e) {
            throw new InvalidKeyException();
        }
    }

    // JAVADOC COMMENT ELIDED
    public final void init(int opmode, Key key, AlgorithmParameterSpec params)
        throws InvalidKeyException, InvalidAlgorithmParameterException {

        com.sun.j2me.crypto.Key cipherKey;
        com.sun.j2me.crypto.CryptoParameter cryptoParameter;

        if (opmode == DECRYPT_MODE) {
            opmode = com.sun.j2me.crypto.Cipher.DECRYPT_MODE;
        } else if (opmode == ENCRYPT_MODE) {
            opmode = com.sun.j2me.crypto.Cipher.ENCRYPT_MODE;
        } else {
            throw new IllegalArgumentException("Wrong operation mode");
        }

        if (key instanceof SecretKeySpec) {
            SecretKeySpec temp = (SecretKeySpec)key;
            byte[] secret = key.getEncoded();

            cipherKey = new com.sun.j2me.crypto.SecretKey(
                            secret, 0, secret.length, key.getAlgorithm());
        } else if (key instanceof RSAPublicKey) {
            RSAPublicKey temp = (RSAPublicKey)key;

            cipherKey = temp.getKey();
        } else {
            throw new InvalidKeyException();
        }
    
        if (params == null) {
            cryptoParameter = null;
        } else if (params instanceof IvParameterSpec) {
            byte[] iv = ((IvParameterSpec)params).getIV();

            cryptoParameter = new com.sun.j2me.crypto.IvParameter(
                                  iv, 0, iv.length);
        } else {
            throw new InvalidAlgorithmParameterException();
        }

        try {
             cipher.init(opmode, cipherKey.getKey(), cryptoParameter);
        } catch (com.sun.j2me.crypto.InvalidKeyException e) {
            throw new InvalidKeyException(e.getMessage());
        } catch (com.sun.j2me.crypto.InvalidAlgorithmParameterException e) {
            throw new InvalidAlgorithmParameterException(e.getMessage());
        }
    }

    // JAVADOC COMMENT ELIDED
    public final int update(byte[] input, int inputOffset, int inputLen,
                            byte[] output, int outputOffset)
        throws IllegalStateException, ShortBufferException {

        try {
            return cipher.update(input, inputOffset, inputLen, output,
                                 outputOffset);
        } catch (com.sun.j2me.crypto.ShortBufferException e) {
            throw new ShortBufferException(e.getMessage());
        }
    }

    // JAVADOC COMMENT ELIDED
    public final int doFinal(byte[] input, int inputOffset, int inputLen,
        byte[] output, int outputOffset)
        throws IllegalStateException, ShortBufferException,
        IllegalBlockSizeException, BadPaddingException {

        try {
            return cipher.doFinal(input, inputOffset, inputLen,
                                  output, outputOffset);
        } catch (com.sun.j2me.crypto.ShortBufferException e) {
            throw new ShortBufferException(e.getMessage());
        } catch (com.sun.j2me.crypto.IllegalBlockSizeException e) {
            throw new IllegalBlockSizeException(e.getMessage());
        } catch (com.sun.j2me.crypto.BadPaddingException e) {
            throw new BadPaddingException(e.getMessage());
        }
    }

    /**
     * Returns the initialization vector (IV) in a new buffer.
     * This is useful in the case where a random IV was created.
     * @return the initialization vector in a new buffer,
     * or <code>null</code> if the underlying algorithm does
     * not use an IV, or if the IV has not yet been set.
     */
    public final byte[] getIV() {
        return cipher.getIV();
    }
}

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

package com.sun.midp.crypto;

import com.sun.midp.pki.AlgorithmId;
import com.sun.midp.pki.BigInteger;
import com.sun.midp.pki.DerOutputStream;
import com.sun.midp.pki.BitArray;
import com.sun.midp.pki.DerValue;

import java.io.IOException;

/**
 * Encodes the given RSA key into DER format.
 */
class DEREncoder {
    private RSAKey key;

    private DEREncoder(RSAKey keyToEncode) {
        key = keyToEncode;
    }

    /**
     * Returns the encoding of key.
     *
     * @param keyToEncode key to encode
     * @return DER encoding of the given key
     */
    static byte[] encode(RSAKey keyToEncode) {
        try {
            DEREncoder encoder = new DEREncoder(keyToEncode);
            byte[] tmp = encoder.getEncodedInternal();
            byte[] encodedKey = new byte[tmp.length];
            System.arraycopy(tmp, 0, encodedKey, 0, tmp.length);
            return encodedKey;
        } catch (InvalidKeyException e) {
            // ignore
        }
        return null;
    }

    private byte[] getEncodedInternal() throws InvalidKeyException {
        byte encoded[] = null;
        try {
            DerOutputStream out = new DerOutputStream();
            encode(out);
            encoded = out.toByteArray();
        } catch (IOException e) {
            throw new InvalidKeyException("IOException : " +
                                           e.getMessage());
        }

        return encoded;
    }

    /**
     * Encode SubjectPublicKeyInfo sequence on the DER output stream.
     *
     * @exception IOException on encoding errors.
     */
    public final void encode(DerOutputStream out) throws IOException {
        try {
            encode(out, AlgorithmId.get("RSA"), getKey());
        } catch (NoSuchAlgorithmException nsae) {
            throw new IOException(nsae.getMessage());
        }
    }

    /**
     * Gets the key. The key may or may not be byte aligned.
     * @return a BitArray containing the key.
     */
    protected BitArray getKey() throws IOException {
        byte[] mod = new byte[key.getModulusLen()];
        key.getModulus(mod, (short)0);
        byte[] exp = new byte[3];
        key.getExponent(exp, (short)0);

        DerOutputStream out = new DerOutputStream();
        /*
         * IMPL_NOTE: currently we always write the sign byte
         * for the modulus for proper hash calculation.
         * Actually it must present only if it was present in
         * the original DER-encoded certificate.
         */
        out.putInteger(new BigInteger(1, mod));
        out.putInteger(new BigInteger(exp));
        DerValue val =
            new DerValue(DerValue.tag_Sequence, out.toByteArray());
        byte[] keyBytes = val.toByteArray();

        // there are no unused bits: modulus length % 8 is always 0
        BitArray bitStringKey = new BitArray(
                          keyBytes.length * 8,
                          keyBytes);

        return (BitArray)bitStringKey.clone();
    }

    /*
     * Produce SubjectPublicKey encoding from algorithm id and key material.
     */
    static void encode(DerOutputStream out, AlgorithmId algid, BitArray key)
            throws IOException {
        DerOutputStream tmp = new DerOutputStream();
        algid.encode(tmp);
        tmp.putUnalignedBitString(key);
        out.write(DerValue.tag_Sequence, tmp);
    }
}

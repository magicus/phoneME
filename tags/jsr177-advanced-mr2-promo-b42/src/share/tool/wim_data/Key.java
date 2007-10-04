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

package wim_data;

import java.security.interfaces.RSAPrivateKey;
import java.security.interfaces.RSAPublicKey;
import java.security.NoSuchAlgorithmException;
import java.security.KeyPairGenerator;
import java.security.KeyPair;

/**
 * This class represents RSA key pair.
 */
class Key {

    /** Private key label. */
    String label;

    /** Index of PIN that protects private key. */
    int pinIndex;

    /** ID of PIN that protects private key. */
    int pinId;

    /**
     * If true, this is non-repudiation key, otherwise - authentication
     * key.
     */
    boolean nonRepudiation;

    /** Length of the key. */
    int keyLen;

    /** Private key identifier. */
    int id;

    /** Path for public key. */
    short[] PublicPath;

    /** Path for private key. */
    short[] PrivatePath;

    /** Private key. */
    RSAPrivateKey priv;
    /** Public key. */
    RSAPublicKey pub;

    /** Public key hash used to identify the keys. */
    byte[] hash;

    /**
     * Constructor.
     * @param label private key label
     * @param len length of key
     * @param pinIndex pin index in PINs array
     * @param nonRepudiation is it non-repudiation key?
     * @param id private key identifier
     * @param PublicPath path to file with public key
     * @param PrivatePath path to file with private key
     * @param PINs PIN objectd defined for this WIM
     */
    Key(String label, int len, int pinIndex, boolean nonRepudiation, int id,
        short[] PublicPath, short[] PrivatePath, PIN[] PINs) {

        this.label = label;
        this.keyLen = len;
        this.pinIndex = pinIndex;
        pinId = PINs[pinIndex].id;
        this.nonRepudiation = nonRepudiation;
        this.id = id;
        this.PublicPath = PublicPath;
        this.PrivatePath = PrivatePath;
        hash = new byte[20];
    }

    /**
     * Initializes the object.
     * @throws NoSuchAlgorithmException if RSA algorithm is not supported
     */
    void init() throws NoSuchAlgorithmException {

        KeyPairGenerator kpg = KeyPairGenerator.getInstance("RSA");
        kpg.initialize(keyLen);
        KeyPair kp = kpg.generateKeyPair();

        priv = (RSAPrivateKey) kp.getPrivate();
        pub = (RSAPublicKey) kp.getPublic();

        byte[] modulus = pub.getModulus().toByteArray();
        hash = Utils.getHash(modulus, 0, modulus.length);
    }

    /**
     * Returns TLV structure that represents private key.
     * @param algorithmId RSA algorithm identifier for this WIM
     * @return TLV structure that represents private key
     */
    TLV getPrivate(byte algorithmId) {

        TLV prk = TLV.createSequence();

        TLV t = TLV.createSequence();   // common object attributes
        prk.setChild(t);

        t.setChild(Utils.createLabel(label)).
                setNext(new TLV(TLV.BITSTRING_TYPE,
                        Utils.shortToBytes(0x0780))).
                setNext(TLV.createOctetString(
                        new byte[] {(byte) pinId}));

        t = t.setNext(TLV.createSequence()); // common key attributes

        t.setChild(TLV.createOctetString(hash)).
                setNext(new TLV(TLV.BITSTRING_TYPE,
                        nonRepudiation ? new byte[] {6, 00, 0x40} :
                new byte[] {6, 0x20, 0x00})).
                setNext(TLV.createInteger(id));

        t.setNext(new TLV(0xa1)).       // private RSA key attrs
          setChild(TLV.createSequence()).
          setChild(Utils.createPath(PrivatePath)).
          setNext(TLV.createInteger(keyLen)).
          setNext(TLV.createInteger(algorithmId));

        return prk;
    }

    /**
     * Returns TLV structure that represents public key.
     * @return TLV structure that represents private key
     */
    TLV getPublic() {

        TLV puk = TLV.createSequence();
        TLV t = TLV.createSequence();   // common object attributes
        puk.setChild(t);

        t = t.setNext(TLV.createSequence()); // common key attributes
        t.setChild(TLV.createOctetString(hash)).
                setNext(new TLV(TLV.BITSTRING_TYPE,
                        Utils.shortToBytes(0x0102))).
                setNext(new TLV(TLV.BOOLEAN_TYPE, new byte[] {0}));

        t.setNext(new TLV(0xa1)).   // public RSA key attrs
          setChild(TLV.createSequence()).
          setChild(Utils.createPath(PublicPath)).
          setNext(TLV.createInteger(keyLen));

        return puk;
    }
}

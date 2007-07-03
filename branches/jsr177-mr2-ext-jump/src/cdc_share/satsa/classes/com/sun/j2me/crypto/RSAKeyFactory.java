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

import java.security.KeyFactorySpi;

import java.security.PublicKey;
import java.security.PrivateKey;
import java.security.Key;
import java.security.InvalidKeyException;

import java.security.spec.KeySpec;
import java.security.spec.InvalidKeySpecException;

import com.sun.satsa.crypto.RSAPublicKey;

/**
 * KeyFactory Service Provider Interface class for RSA public key 
 */
public class RSAKeyFactory extends KeyFactorySpi {

    /**
     * Generates a public key object from the provided key specification
     * (key material).
     * 
     * @param keySpec the specification (key material) of the public key.
     * 
     * @return the public key.
     * 
     * @exception InvalidKeySpecException if the given key specification
     * is inappropriate for this key factory to produce a public key.
     */
    public PublicKey engineGeneratePublic(KeySpec keySpec)
        throws InvalidKeySpecException {
        return new RSAPublicKey(keySpec);
    }

    public PrivateKey engineGeneratePrivate(KeySpec keyspec)
        throws InvalidKeySpecException {
        return null;
    }

    public KeySpec engineGetKeySpec(Key key, Class var_class)
	    throws InvalidKeySpecException {
        return null;
    }

    public Key engineTranslateKey(Key key) 
        throws InvalidKeyException {
        return null;
    }
}

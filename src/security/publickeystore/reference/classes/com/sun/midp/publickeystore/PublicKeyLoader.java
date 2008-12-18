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

package com.sun.midp.publickeystore;

import java.io.*;

/** The information that needs to be stored for a public key. */
public class PublicKeyLoader {
	public static void open() {
	};

    public static void close() {
    };


    static PublicKeyInfo getKeyFromStorage(InputStorage storage) 
    throws IOException {
        byte[] tag;
        Object value;
        String owner;
        long notBefore;
        long notAfter;
        byte[] modulus;
        byte[] exponent;
        String domain;
        boolean enabled;

        tag = new byte[1];

        value = storage.readValue(tag);
        if (value == null) {
            // no more keys
            return null;
        }

        if (tag[0] != PublicKeyInfo.OWNER_TAG) {
            throw new IOException("public key storage corrupted");
        }

        owner = (String)value;

        value = storage.readValue(tag);
        if (tag[0] != PublicKeyInfo.NOT_BEFORE_TAG) {
            throw new IOException("public key storage corrupted");
        }

        notBefore = ((Long)value).longValue();

        value = storage.readValue(tag);
        if (tag[0] != PublicKeyInfo.NOT_AFTER_TAG) {
            throw new IOException("public key storage corrupted");
        }

        notAfter = ((Long)value).longValue();

        value = storage.readValue(tag);
        if (tag[0] != PublicKeyInfo.MODULUS_TAG) {
            throw new IOException("public key storage corrupted");
        }

        modulus = (byte[])value;

        value = storage.readValue(tag);
        if (tag[0] != PublicKeyInfo.EXPONENT_TAG) {
            throw new IOException("public key storage corrupted");
        }

        exponent = (byte[])value;

        value = storage.readValue(tag);
        if (tag[0] != PublicKeyInfo.DOMAIN_TAG) {
            throw new IOException("public key storage corrupted");
        }

        domain = (String)value;

        value = storage.readValue(tag);
        if (tag[0] != PublicKeyInfo.ENABLED_TAG) {
            throw new IOException("public key storage corrupted");
        }

        if(value instanceof String) {
            enabled = ((String)value).equals("enabled");
        } else {
            enabled = ((Boolean)value).booleanValue();
        }

        return new PublicKeyInfo(owner, notBefore, notAfter,
                                 modulus, exponent, domain, enabled);

    }


}

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

package com.sun.satsa.pki;

/**
 * This class represents PKCS#15 private key.
 */
class PrivateKey {

    /** User friendly key label. */
    String label;
    /** Authentication object identifier. */
    int authId;
    /** Key identifier. */
    byte[] id;
    /** Can this key be used for authentication purposes? */
    boolean authentication;
    /** Can this key be used for non-repudiation purposes? */
    boolean nonRepudiation;
    /** Key reference attribute. */
    int keyReference;
    /** Modulus length attribute. */
    int modulusLength;
    /** Path attribute. */
    short[] path;

    /**
     * Print information.
     * /
    void print() {
        // IMPL_NOTE: remove after debugging
        System.out.println("Private key");
        System.out.println("label " + label);
        System.out.println("authID " + authId);
        System.out.println("id " + Utils.hexEncode(id, 0, id.length, 9999));
        if (authentication) {
            System.out.println("authentication");
        }
        if (nonRepudiation) {
            System.out.println("nonRepudiation");
        }
        System.out.println("keyReference " + keyReference);
        System.out.println("modulusLength " + modulusLength);
        System.out.print("path ");
        Utils.print(path, path.length);
        System.out.println("");
    }
*/
}

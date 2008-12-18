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
import com.sun.midp.publickeystore.PublicKeyInfo;

/** The information that needs to be stored for a public key. */
public class PublicKeyLoader {
    private final static boolean ENABLE_KEY_CONTENT_PRINT = false;
    static private int keyStoreHandle = 0;

    public static void open() {
        keyStoreHandle = keyStoreInit();
    }

    public static void close() {
        keyStoreFinalize(keyStoreHandle);
    }


    static PublicKeyInfo getKeyFromStorage(InputStorage storage) 
    throws IOException {
        byte[] tag = {0};
        Object value;
        String owner = "owner";
        long notBefore = 1;
        long notAfter = 1;
        byte[] modulus = {0};
        byte[] exponent = {0};
        String domain = "domain";
        int    res;

        System.out.println("****========= GetKeyFromStorage() start ======= ");

        PublicKeyInfo publicKeyInfo = new PublicKeyInfo(owner, notBefore, notAfter,
                                                        modulus,  exponent, domain);

        /* Get the keys from the platform */
        res = getPublicKeyInfo0(publicKeyInfo, keyStoreHandle);
        if (res == 0) {
            System.out.println("PublicKeyInfo: GetKeyFromStorage no more keys available");
            /* no more keys available */
            publicKeyInfo = null; 
        } else if (res == -1) {
            /* an error occured */
            System.out.println("PublicKeyInfo: GetKeyFromStorage an error occured, returning");
            publicKeyInfo = null;
            throw new IOException("PublicKeyInfo:public key storage corrupted\n");
        }

        if(ENABLE_KEY_CONTENT_PRINT) {
            PublicKeyInfo.printPublicKeyInfoContent(publicKeyInfo);
        }

        //System.out.println("========= GetKeyFromStorage() end ======= ");
        return publicKeyInfo;
    }   

    /* end printPublicKeyInfoContent */

    private static native int keyStoreInit();
    private static native int keyStoreFinalize(int handle);
    private static native int getPublicKeyInfo0(PublicKeyInfo publicKeyInfo, int handle);
}

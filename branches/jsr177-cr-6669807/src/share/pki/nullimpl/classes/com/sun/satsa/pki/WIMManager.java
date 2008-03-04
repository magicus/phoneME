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

import com.sun.satsa.util.TLV;
import javax.microedition.pki.UserCredentialManager;
import javax.microedition.pki.UserCredentialManagerException;
import javax.microedition.securityservice.CMSMessageSignatureServiceException;
import java.util.Vector;
import com.sun.j2me.security.Token;

/**
 * This class provides implementation of methods defined by
 * javax.microedition.pki.UserCredentialManager and
 * javax.microedition.securityservice.CMSMessageSignatureService
 * classes.
 */
public class WIMManager {
    
    public static byte[] generateCSR(String securityElementID, String nameInfo, 
            int keyLen, int keyUsage, boolean forceKeyGen, Vector keyIDs,
            Token token) throws UserCredentialManagerException,
                         CMSMessageSignatureServiceException {
        return null;
    }
    
    public static boolean addCredential(String label, TLV top, Vector keyIDs, 
            Token token) {
        return  false;
    }

    public static boolean removeCredential(String securityElementID, String label, 
            TLV isn, Token token) {
        return false;
    }

    public static byte[] generateSignature(boolean nonRepudiation, byte[] data, 
            int options, TLV[] caNames, Token token) {

        return null;
    }
}

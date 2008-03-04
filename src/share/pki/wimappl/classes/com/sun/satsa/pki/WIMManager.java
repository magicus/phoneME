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
import java.util.NoSuchElementException;
import com.sun.cdc.io.j2me.apdu.APDUManager;

//
//import com.sun.j2me.app.AppPackage;
//import com.sun.j2me.dialog.Dialog;
//import com.sun.j2me.dialog.MessageDialog;
//import com.sun.j2me.io.FileAccess;
//import com.sun.j2me.security.SatsaPermission;
//import com.sun.j2me.i18n.Resource;
//import com.sun.j2me.i18n.ResourceConstants;
//import com.sun.j2me.main.Configuration;
//import com.sun.satsa.util.*;
//import javax.microedition.io.Connector;
//import java.io.DataInputStream;
//import java.io.DataOutputStream;
//import java.io.IOException;

/**
 * This class provides implementation of methods defined by
 * javax.microedition.pki.UserCredentialManager and
 * javax.microedition.securityservice.CMSMessageSignatureService
 * classes.
 */
public class WIMManager {

    public static byte[] generateCSR(String securityElementID, String nameInfo, 
            int keyLen, int keyUsage, boolean forceKeyGen, Vector keyIDs,
            Token securityToken) throws UserCredentialManagerException,
                         CMSMessageSignatureServiceException {

        int slotCount = APDUManager.getSlotCount();

        for (int i = 0; i < slotCount; i++) {

            WIMApplication w = WIMApplication.getInstance(
                        i, securityElementID, false);
            if (w == null) {
                continue;
            }
            try {
                Vector CSRs = PKIManager.loadCSRList();
                byte[] CSR = w.generateCSR(nameInfo, keyLen, keyUsage, 
                        forceKeyGen, CSRs, securityToken);
                PKIManager.storeCSRList(CSRs);
                return CSR;
            } finally {
                w.done();
            }
        }
        
        throw new NoSuchElementException("WIM not found");
    }

    public static boolean addCredential(String label, TLV top, Vector keyIDs, 
            Token securityToken) throws UserCredentialManagerException {
        
        int slotCount = APDUManager.getSlotCount();

        for (int i = 0; i < slotCount; i++) {

            WIMApplication w = WIMApplication.getInstance(i, null, false);

            if (w == null) {
                continue;
            }
            try {
                int result = w.addCredential(label, top, keyIDs, securityToken);
                if (result == WIMApplication.SUCCESS) {
                    return true;
                }
                if (result == WIMApplication.CANCEL) {
                    return false;
                }
                if (result == WIMApplication.ERROR) {
                    break;
                }
            } catch (IllegalArgumentException e) {
                throw e;
            } catch (SecurityException e) {
                throw e;
            } finally {
                w.done();
            }
        }
        throw new UserCredentialManagerException(
                UserCredentialManagerException.CREDENTIAL_NOT_SAVED);
    }

    public static boolean removeCredential(String securityElementID,
            String label, TLV isn, Token securityToken) 
            throws UserCredentialManagerException {
       
        int slotCount = APDUManager.getSlotCount();

        for (int i = 0; i < slotCount; i++) {

            WIMApplication w = WIMApplication.getInstance(
                   i, securityElementID, false);
            if (w == null) {
                continue;
            }
            try {
                int result = w.removeCredential(label, isn, securityToken);
                if (result == WIMApplication.SUCCESS) {
                    return true;
                }
                if (result == WIMApplication.CANCEL) {
                    return false;
                }
                throw new UserCredentialManagerException(
                                     UserCredentialManagerException.
                                     CREDENTIAL_NOT_FOUND);
            } finally {
                w.done();
            }
        }
        
        throw new NoSuchElementException("WIM not found");
    }

    public static byte[] generateSignature(boolean nonRepudiation, byte[] data, 
         int options, TLV[] caNames, Token securityToken) {

        int slotCount = APDUManager.getSlotCount();
        for (int i = 0; i < slotCount; i++) {

            WIMApplication w = WIMApplication.getInstance(
                                 i, null, true);
            if (w == null) {
                continue;
            }
            try {
                return w.generateSignature(nonRepudiation, data, options, 
                        caNames, securityToken);
            } finally {
                w.done();
            }
        }
        
        throw new NoSuchElementException("WIM not found");
    }
}

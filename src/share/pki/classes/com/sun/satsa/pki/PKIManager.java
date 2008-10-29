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

import com.sun.j2me.app.AppPackage;
import com.sun.j2me.dialog.Dialog;
import com.sun.j2me.dialog.MessageDialog;
import com.sun.j2me.io.FileAccess;
import com.sun.j2me.security.SatsaPermission;
import com.sun.cdc.io.j2me.apdu.APDUManager;
import com.sun.j2me.i18n.Resource;
import com.sun.j2me.i18n.ResourceConstants;
import com.sun.j2me.main.Configuration;
import com.sun.satsa.util.*;

import javax.microedition.io.Connector;
import javax.microedition.pki.UserCredentialManager;
import javax.microedition.pki.UserCredentialManagerException;
import javax.microedition.securityservice.CMSMessageSignatureServiceException;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.Vector;

import com.sun.j2me.security.TrustedClass;
import com.sun.j2me.security.Token;
import com.sun.satsa.security.SecurityInitializer;

/**
 * This class provides implementation of methods defined by
 * javax.microedition.pki.UserCredentialManager and
 * javax.microedition.securityservice.CMSMessageSignatureService
 * classes.
 */
public class PKIManager {

    /** Signature operation identifier. */
    public static final int AUTHENTICATE_DATA      = 0;
    /** Signature operation identifier. */
    public static final int AUTHENTICATE_STRING    = 1;
    /** Signature operation identifier. */
    public static final int SIGN_STRING            = 2;

    /*
     * Inner class to request security token from SecurityTokenInitializer.
     * SecurityTokenInitializer should be able to check this inner class name.
     */
    static private class SecurityTrusted
        implements TrustedClass { };

    /** This class has a different security domain than the App suite */
    private static Token securityToken =
        SecurityInitializer.requestToken(new SecurityTrusted());

    /**
     * Storage name for identifiers of public keys for which
     * certificates are expected. */
    private static final String CSR_ID_FILE =  "_csr.id";

    /**
     * Contains identifiers of public keys for which certificates are
     * expected if this information is not stored in persistent storage.
     */
    private static Vector tmpCSRList;

    /**
     * Creates a DER encoded PKCS#10 certificate enrollment request.
     * @param nameInfo the distinguished name to be included in the
     * PKCS#10 certificate signing request.
     * @param algorithm the Object Identifier (OID) for the public key
     * algorithm to use.
     * @param keyLen the key length.
     * @param keyUsage the functionality for which the key is marked
     * inside the security element.
     * @param securityElementID identifies the security element on which
     * the key resides.
     * @param securityElementPrompt guides a user to insert the correct
     * security element, if a suitable security element is removable and
     * not detected.
     * @param forceKeyGen if set to true a new key MUST be generated.
     * @return DER encoded PKCS#10 certificate enrollment request
     * @throws UserCredentialManagerException if an error occurs while
     * generating the certificate request
     * @throws CMSMessageSignatureServiceException if an error occurs
     * while signing the certificate request
     * @throws SecurityException if a PIN is blocked due to an excessive
     * number of incorrect PIN entries
     */
    public static synchronized byte[] generateCSR(String nameInfo,
            String algorithm, int keyLen, int keyUsage,
            String securityElementID, String securityElementPrompt,
            boolean forceKeyGen) throws UserCredentialManagerException,
                                    CMSMessageSignatureServiceException {

        if (nameInfo != null && nameInfo.trim().length() == 0) {
            throw new IllegalArgumentException("Invalid name");
        }

        try {
            Utils.StringToOID(algorithm);
        } catch (IllegalArgumentException e) {
            throw new IllegalArgumentException("Invalid algorithm");
        }

        if (keyLen <= 0  || keyLen > 20480) {
            throw new IllegalArgumentException("Invalid key length");
        }

        if (keyUsage != UserCredentialManager.KEY_USAGE_AUTHENTICATION &&
            keyUsage != UserCredentialManager.KEY_USAGE_NON_REPUDIATION) {
                throw new IllegalArgumentException("Invalid key usage");
        }

        if (! algorithm.equals(UserCredentialManager.ALGORITHM_RSA)) {
            throw new UserCredentialManagerException(
                    UserCredentialManagerException.SE_NO_KEYS);
        }

        int slotCount = APDUManager.getSlotCount();

        while (true) {

            for (int i = 0; i < slotCount; i++) {

                WIMApplication w = WIMApplication.getInstance(
                        i, securityElementID, false);
                if (w == null) {
                    continue;
                }
                try {
                    Vector CSRs = loadCSRList();
                    byte[] CSR = w.generateCSR(nameInfo, keyLen,
                                           keyUsage, forceKeyGen, CSRs, securityToken);
                    storeCSRList(CSRs);
                    return CSR;
                } finally {
                    w.done();
                }
            }

            // WIM application is not found

            if (securityElementPrompt != null) {
                try {
                    if (MessageDialog.showMessage(
                            Resource.getString(ResourceConstants
					   .JSR177_WIM_NOT_FOUND),
                        securityElementPrompt,
                        true, securityToken) != -1) {
                        continue;
                    }
                } catch (InterruptedException e) {}
            }
            throw new UserCredentialManagerException(
                          UserCredentialManagerException.SE_NOT_FOUND);
        }
    }

    /**
     * Adds a user certificate to a certificate store.
     * @param certDisplayName the user friendly name associated with the
     * certificate.
     * @param pkiPath the DER encoded PKIPath containing user
     * certificate and certificate authority certificates.
     * @param uri a URI that resolves to a X.509v3 certificate.
     * @return true if successful
     * @throws UserCredentialManagerException if an error occurs while
     * adding a user credential
     * @throws SecurityException if a PIN is blocked due to an excessive
     * number of incorrect PIN entries
     */
    public static synchronized boolean addCredential(String certDisplayName,
            byte[] pkiPath, String uri)
            throws UserCredentialManagerException {

        // check parameters

        if (certDisplayName == null ||
            certDisplayName.trim().length() == 0) {
            throw new IllegalArgumentException("Invalid name");
        }
        certDisplayName = certDisplayName.trim();

        TLV t = null;
        String info = null;

        try {
            t = new TLV(pkiPath, 0).child;
            TLV v = t;
            if (v != null) {
                while (v.next != null) {
                    v = v.next;
                }
                info = Certificate.getInfo(v);
            }
        } catch (TLVException e) {} // ignored

        if (info == null) {
            throw new IllegalArgumentException("Invalid pkiPath");
        }

        // ask user

        try {
            if (MessageDialog.showMessage(
                Resource.getString(ResourceConstants.AMS_CONFIRMATION),
                Resource.getString(ResourceConstants.JSR177_CERTIFICATE_STORED) +
                "\n\n" + 
                // "Label" +
                Resource.getString(ResourceConstants.JSR177_CERTIFICATE_LABEL) +
                ": " + certDisplayName + "\n\n" +
                info + "\n\n",
                true, securityToken) == Dialog.CANCELLED) {
                return false;
            }
        } catch (InterruptedException e) {
            return false;
        }

        // save certificates

        Vector CSRs = loadCSRList();

        int slotCount = APDUManager.getSlotCount();

        for (int i = 0; i < slotCount; i++) {

            WIMApplication w = WIMApplication.getInstance(
                    i, null, false);

            if (w == null) {
                continue;
            }
            try {
                int result = w.addCredential(certDisplayName, t, CSRs, securityToken);
                if (result == WIMApplication.SUCCESS) {
                    storeCSRList(CSRs);
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

    /**
     * Removes a certificate from a certificate store.
     * @param certDisplayName the user friendly name associated with the
     * certificate.
     * @param issuerAndSerialNumber the DER encoded ASN.1 structure that
     * contains the certificate issuer and serial number.
     * @param securityElementID identifies the security element on which
     * the key resides.
     * @param securityElementPrompt guides the user to insert the
     * correct security element if the security element is removable and
     * not detected.
     * @return false if operation cancelled
     * @throws UserCredentialManagerException if
     * an error occurs while removing the credential
     * @throws SecurityException if a PIN is blocked due to an excessive
     * number of incorrect PIN entries
     */
    public static boolean removeCredential(String certDisplayName,
                                        byte[] issuerAndSerialNumber,
                                        String securityElementID,
                                        String securityElementPrompt)
            throws UserCredentialManagerException {

        if (certDisplayName == null ||
            certDisplayName.trim().length() == 0) {
            throw new IllegalArgumentException("Invalid name");
        }
        certDisplayName = certDisplayName.trim();

        TLV isn;
        try {
            isn = new TLV(issuerAndSerialNumber, 0);
            /*
               Compare the name with itself to make sure that it is
               properly formatted
            */
            try {
                RFC2253Name.compare(isn.child, isn.child);
            } catch (IllegalArgumentException iae) {
                throw new IllegalArgumentException(
                          "Invalid issuerAndSerialNumber");
            }
            if (isn.child.next == null || isn.child.next.type != TLV.INTEGER_TYPE) {
                throw new IllegalArgumentException(
                          "Invalid issuerAndSerialNumber");
            }
        } catch (TLVException e) {
            throw new IllegalArgumentException(
                                      "Invalid issuerAndSerialNumber");
        }
        int slotCount = APDUManager.getSlotCount();

        while (true) {

            for (int i = 0; i < slotCount; i++) {

                WIMApplication w = WIMApplication.getInstance(
                       i, securityElementID, false);
                if (w == null) {
                    continue;
                }
                try {
                    int result = w.removeCredential(certDisplayName, isn, securityToken);
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

            // WIM application is not found

            if (securityElementPrompt != null) {
                try {
                    if (MessageDialog.showMessage(
                            Resource.getString(
                                ResourceConstants.JSR177_WIM_NOT_FOUND),
                            securityElementPrompt,
                            true, securityToken) != -1) {
                        continue;
                    }
                } catch (InterruptedException e) {}
            }
            throw new UserCredentialManagerException(
                          UserCredentialManagerException.SE_NOT_FOUND);
        }
    }


    /**
     * Generates a signature.
     * @param action type of signature operation.
     * @param data data to be signed or null
     * @param string string to be signed or null
     * @param options signature format options
     * @param caNames an array of Strings that contain the distinguished
     * names of trusted certification authorities.
     * @param securityElementPrompt guides a user to insert the correct
     * security element if the security element is removable and not
     * detected.
     * @return the DER encoded signature, null if the signature
     * generation was cancelled by the user before completion
     * @throws CMSMessageSignatureServiceException if an error occurs
     * during signature generation
     * @throws UserCredentialManagerException if key not found
     * @throws SecurityException if caller does not have permission
     */
    public static byte[] sign(int action, byte[] data, String string,
                               int options, String[] caNames,
                               String securityElementPrompt)
            throws CMSMessageSignatureServiceException,
		   UserCredentialManagerException {

        // Only CMSMessageSignatureService.sign() is 
        // protected by MIDP permissions
        if (action == AUTHENTICATE_DATA) { 
            try {
                AppPackage.getInstance().
                        checkForPermission(SatsaPermission.SIGN_SERVICE);
            } catch (InterruptedException ie) {
                throw new SecurityException(
                    "Interrupted while trying to ask the user permission");
            }
        }

        if (action == AUTHENTICATE_DATA ?
            (data == null || data.length == 0) :
            (string == null || string.length() == 0)) {
            // IMPL_NOTE: specification ?
            throw new IllegalArgumentException("Invalid data");
        }

        /*
            Parse the CA names, toTLV throws IllegalArgumentException
            if necessary.
        */

        TLV[] names = null;
        if (caNames != null && caNames.length != 0) {
            names = new TLV[caNames.length];
            for (int i = 0; i < caNames.length; i++) {
                names[i] = RFC2253Name.toTLV(caNames[i]);
            }
        }

        // ask user confirmation if necessary
        if (action != AUTHENTICATE_DATA) {
            try {
                if (MessageDialog
		    .showMessage(Resource
				 .getString(ResourceConstants
					    .JSR177_CONFIRM_SIGNATURE),
				 Resource
				 .getString(ResourceConstants
                     .JSR177_STRING_TO_SIGN) +
				 string, true, securityToken) != 1) {
                    return null;
                }
            } catch (InterruptedException e) {
                throw new CMSMessageSignatureServiceException(
                        CMSMessageSignatureServiceException.SE_FAILURE);
            }

            data = Utils.stringToBytes(string);
        }

        int slotCount = APDUManager.getSlotCount();

        while (true) {

            for (int i = 0; i < slotCount; i++) {

                WIMApplication w = WIMApplication.getInstance(
                                     i, null, true);
                if (w == null) {
                    continue;
                }
                try {
                    return w.generateSignature(action == SIGN_STRING,
                                                  data, options, names, securityToken);
                } finally {
                    w.done();
                }
            }
            // WIM application is not found

            if (securityElementPrompt != null) {
                try {
                    if (MessageDialog.showMessage(
                        Resource.getString(ResourceConstants
					   .JSR177_WIM_NOT_FOUND),
                        securityElementPrompt, true, securityToken) != -1) {
                        continue;
                    }
                } catch (InterruptedException e) {}
            }
            throw new UserCredentialManagerException(
                          UserCredentialManagerException.SE_NOT_FOUND);
        }
    }

    /**
     * Loads the list of key identifiers for which certificates are
     * expected.
     * @return the list
     */
    private static Vector loadCSRList() {

        if (! persistentCSRList()) {
            if (tmpCSRList == null) {
                tmpCSRList = new Vector();
            }
            return tmpCSRList;
        }

        Vector CSRs = new Vector();

    	String storeName = FileAccess.getStorageRoot(
	    FileAccess.INTERNAL_STORAGE_ID) + CSR_ID_FILE;
        FileAccess storage = FileAccess.getInstance(storeName, securityToken);
        DataInputStream dis;

        try {
            storage.connect(Connector.READ);
            dis = new DataInputStream(storage.openInputStream());
        } catch (IOException ioe) {

            try {
                storage.connect(Connector.READ_WRITE);
                DataOutputStream dos = storage.openDataOutputStream();
                dos.writeInt(0);
                dos.flush();
                dos.close();
                storage.disconnect();
            } catch (IOException e) {} // ignored
            return CSRs;
        }

        try {
            int count = dis.readInt();
            while (count-- > 0) {
                byte[] id = new byte[20];
                dis.read(id, 0, 20);
                CSRs.addElement(id);
            }
        } catch (IOException e) {} // ignored
        finally {
            try {
                dis.close();
                storage.disconnect();                
            } catch (IOException e) {} // ignored
        }
        return CSRs;
    }

    /**
     * Stores the list of key identifiers for which certificates are
     * expected.
     * @param CSRs the list
     */
    private static void storeCSRList(Vector CSRs) {

        if (! persistentCSRList()) {
            return;
        }

        String storeName = FileAccess.getStorageRoot(
	    FileAccess.INTERNAL_STORAGE_ID) + CSR_ID_FILE;
        FileAccess storage = FileAccess.getInstance(storeName, securityToken);

        DataOutputStream dos;

        try {
            storage.connect(Connector.WRITE);
            dos = storage.openDataOutputStream();
            int len = CSRs.size();
            dos.writeInt(len);
            for (int i = 0; i < len; i++) {
                dos.write((byte[]) CSRs.elementAt(i));
            }
            dos.flush();
            dos.close();
            storage.truncate(4 + len * 20);
        } catch (IOException openwe) {} // ignored
        finally {
            try {
                storage.disconnect();
            } catch (IOException e) {} // ignored
        }
    }

    /**
     * Returns true if the list of generated CSR IDs must be stored
     * in persistent storage.
     * @return true if the list of generated CSR IDs must be stored
     * in persistent storage
     */
    private static boolean persistentCSRList() {
        return "true".equals(
                Configuration.getProperty("com.sun.satsa.store_csr_list"));
    }
}

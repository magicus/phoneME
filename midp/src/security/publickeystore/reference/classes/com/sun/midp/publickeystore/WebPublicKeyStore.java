/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
import java.util.*;
 
import javax.microedition.io.*;

import com.sun.midp.io.j2me.storage.*;

import com.sun.midp.midlet.*;

import com.sun.midp.security.*;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

import com.sun.midp.crypto.*;

import com.sun.midp.pki.*;

/**
 * A public keystore that can used with SSL.
 * To work with SSL this class implements the SSL
 * {@link CertStore} interface.
 */
public class WebPublicKeyStore extends PublicKeyStore 
    implements CertStore, ImplicitlyTrustedClass {

    /** This class has a different security domain than the MIDlet suite */
    private static SecurityToken classSecurityToken;

    /** keystore this package uses for verifying descriptors */
    private static WebPublicKeyStore trustedKeyStore;

    /** keystore this package uses for verifying descriptors */
    private static Vector sharedKeyList;

    /**
     * Initializes the security domain for this class, so it can
     * perform actions that a normal MIDlet Suite cannot.
     *
     * @param token security token for this class.
     */
    public void initSecurityToken(SecurityToken token) {
        if (classSecurityToken == null) {
            classSecurityToken = token;
        }
    }
        
    /**
     * Load the certificate authorities for the MIDP from storage
     * into the SSL keystore.
     */
    public static void loadCertificateAuthorities() {
        RandomAccessStream storage;
        InputStream tks;
        WebPublicKeyStore ks;

        if (trustedKeyStore != null) {
            return;
        }

        try {
            storage = new RandomAccessStream(classSecurityToken);
            storage.connect(File.getStorageRoot() + "_main.ks",
                            Connector.READ);
            tks = storage.openInputStream();
        } catch (Exception e) {
            if (Logging.TRACE_ENABLED) {
                Logging.trace(e, "Could not open the trusted key store, " +
                              "cannot authenticate HTTPS servers");
            }
            return;
        }

        try {
            sharedKeyList = new Vector();
            ks = new WebPublicKeyStore(tks, sharedKeyList);
        } catch (Exception e) {
            if (Logging.TRACE_ENABLED) {
                Logging.trace(e, "Corrupt key store file, cannot" +
                              "authenticate HTTPS servers"); 
            }
            return;
        } finally {
            try {
                storage.disconnect();
            } catch (Exception e) {
                if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                    Logging.report(Logging.WARNING, LogChannels.LC_SECURITY,
                                   "Exception during diconnect");
                }
            }
        }

        WebPublicKeyStore.setTrustedKeyStore(ks);
    }

    /**
     * Disable a certificate authority in the trusted keystore.
     *
     * @param name name of the authority.
     */
    public static void disableCertAuthority(String name) {
        setCertAuthorityEnabledField(name, false);
    }

    /**
     * Enable a certificate authority in the trusted keystore.
     *
     * @param name name of the authority.
     */
    public static void enableCertAuthority(String name) {
        setCertAuthorityEnabledField(name, true);
    }

    /**
     * Disable a certificate authority in the trusted keystore.
     *
     * @param name name of the authority.
     * @param enabled value of enable field
     */
    private static void setCertAuthorityEnabledField(String name,
            boolean enabled) {
        Vector keys;
        PublicKeyInfo keyInfo;
        MIDletSuite midletSuite =
            MIDletStateHandler.getMidletStateHandler().getMIDletSuite();

        if (midletSuite == null) {
            throw new
                IllegalStateException("This method can't be called before " +
                                      "a suite is started.");
        }

        midletSuite.checkIfPermissionAllowed(Permissions.AMS);

        keys = trustedKeyStore.findKeys(name);
        if (keys == null || keys.size() <= 0) {
            return;
        }

        for (int i = 0; i < keys.size(); i++) {
            keyInfo = (PublicKeyInfo)keys.elementAt(i);
            keyInfo.enabled = enabled;
        }

        saveKeyList();
    }

    /** Saves the shared key list to main key store. */
    private static void saveKeyList() {
        PublicKeyStoreBuilderBase keystore;
        RandomAccessStream storage;
        OutputStream outputStream;

        if (trustedKeyStore == null) {
            return;
        }

        keystore = new PublicKeyStoreBuilderBase(sharedKeyList);
        try {
            storage = new RandomAccessStream(classSecurityToken);
            storage.connect(File.getStorageRoot() + "_main.ks",
                            RandomAccessStream.READ_WRITE_TRUNCATE);
            outputStream = storage.openOutputStream();
        } catch (Exception e) {
            if (Logging.TRACE_ENABLED) {
                Logging.trace(e, "Could not open the trusted key store, " +
                              "cannot authenticate HTTPS servers");
            }
            return;
        }

        try {
            keystore.serialize(outputStream);
        } catch (Exception e) {
            if (Logging.TRACE_ENABLED) {
                Logging.trace(e, "Corrupt key store file, cannot" +
                              "authenticate HTTPS servers"); 
            }

            return;
        } finally {
            try {
                storage.disconnect();
            } catch (Exception e) {
                if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                    Logging.report(Logging.WARNING, LogChannels.LC_SECURITY,
                                   "Exception during diconnect");
                }
            }
        }
    }


    /**
     * Establish the given keystore as the system trusted keystore.
     * This is a one-shot method, it will only set the trusted keystore
     * it there is no keystore set. For security purposes only
     * read-only PublicKeyStores should be set.
     * @param keyStore keystore to be the system trusted keystore
     * @see #getTrustedKeyStore
     */
    private static void setTrustedKeyStore(WebPublicKeyStore keyStore) {
        if (trustedKeyStore != null) {
            return;
        }

        trustedKeyStore = keyStore;
    }

    /**
     * Provides the keystore of resident public keys for
     * security domain owners and other CA's. Loads the public key store if
     * it has not already been loaded.
     *
     * @return keystore of domain owner and CA keys
     * @see #setTrustedKeyStore
     */
    public static WebPublicKeyStore getTrustedKeyStore() {
        if (trustedKeyStore == null) {
            loadCertificateAuthorities();
        }

        return trustedKeyStore;
    }

    /**
     * Constructs an keystore to initialize the class security token.
     */
    public WebPublicKeyStore() {
    }

    /**
     * Constructs an extendable keystore from a serialized keystore created
     * by {@link PublicKeyStoreBuilder}.
     * @param in stream to read a keystore serialized by
     *        {@link PublicKeyStoreBuilder#serialize(OutputStream)} from
     * @exception IOException if the key storage was corrupted
     */
    public WebPublicKeyStore(InputStream in) throws IOException {
        super(in);
    }

    /**
     * Constructs an extendable keystore from a serialized keystore created
     * by {@link PublicKeyStoreBuilder}.
     * @param in stream to read a keystore serialized by
     *        {@link PublicKeyStoreBuilder#serialize(OutputStream)} from
     * @param sharedKeyList shared key list
     * @exception IOException if the key storage was corrupted
     */
    public WebPublicKeyStore(InputStream in, Vector sharedKeyList)
        throws IOException {
        super(in, sharedKeyList);
    }

    /**
     * Returns the certificate(s) corresponding to a 
     * subject name string.
     * 
     * @param subjectName subject name of the certificate in printable form.
     *
     * @return corresponding certificates or null (if not found)
     */ 
    public X509Certificate[] getCertificates(String subjectName) {
        Vector keys;
        X509Certificate[] certs;

        keys = findKeys(subjectName);
        if (keys == null) {
            return null;
        }

        certs = new X509Certificate[keys.size()];
        for (int i = 0; i < keys.size(); i++) {
            certs[i] = createCertificate((PublicKeyInfo)keys.elementAt(i));
        }

        return certs;
    }

    /**
     * Creates an {@link X509Certificate} using the given public key
     * information.
     * @param keyInfo key information
     * @return X509 certificate
     */
    public static X509Certificate createCertificate(PublicKeyInfo keyInfo) {
        if (keyInfo == null) {
            return null;
        }

        try {
            X509Certificate cert;

            cert = new X509Certificate((byte)1, // fixed at version 1
                                new byte[0],
                                keyInfo.getOwner(),
                                keyInfo.getOwner(), // issuer same as subject
                                keyInfo.getNotBefore(),
                                keyInfo.getNotAfter(),
                                keyInfo.getModulus(),
                                keyInfo.getExponent(),
                                null, // we don't use finger prints
                                0);
            return cert;
        } catch (Exception e) {
            return null;
        }
    }
}

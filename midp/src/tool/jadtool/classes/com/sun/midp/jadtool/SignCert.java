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

package com.sun.midp.jadtool;

import java.io.*;
import java.util.Date;

import java.security.KeyStore;
import java.security.NoSuchAlgorithmException;
import java.security.KeyStoreException;
import java.security.Key;
import java.security.PrivateKey;

import java.security.SignatureException;
import java.security.InvalidKeyException;
import java.security.NoSuchProviderException;
import java.security.UnrecoverableKeyException;

import java.security.cert.Certificate;
import java.security.cert.X509Certificate;
import java.security.cert.CertificateException;

import com.sun.midp.jadtool.AppDescriptorException;

// FIX: Use of these classes is not portable.  (See below.)
//      Perhaps they should be re-implemented at some point.
import sun.security.x509.AlgorithmId;
import sun.security.x509.X509CertImpl;
import sun.security.x509.X509CertInfo;
import sun.security.x509.X500Name;
import sun.security.x509.CertificateSubjectName;
import sun.security.x509.CertificateIssuerName;
import sun.security.x509.CertificateValidity;
import sun.security.x509.CertificateSerialNumber;
import sun.security.x509.CertificateAlgorithmId;


/**
 * SignCert is a utility class used by the AppDescriptor Class to
 * modify a self-signed certificate in a KeyStore.  These methods
 * do not modify an AppDescriptor at all, but only modify the contents
 * of a Java KeyStore.  
 * <p>
 *
 * Given the alias of a self-signed certificate and the alias of 
 * a "signing" certificate...the alias of another certificate that 
 * is paired with a private key...the static method 
 * <code>SignACert</code> will replace the self-signed certificate
 * with the same certificate signed by the owner of the "signing"
 * certificate.
 * <p>
 *
 * PORTABILITY WARNING!
 * --------------------
 * This class uses Sun implementation specific classes in the
 * <code>sun.security.x509.*</code> namespace imported here.  This
 * is NOT portable to Java runtime environments provided by other
 * vendors.  It may not even be supported in future releases
 * of the Sun JDK.  This code works under Sun's JDK1.3.    
 * <p>
 *
 * We were forced to use Sun's X509 certificate implementation classes 
 * directly because the public X509 certificate methods in
 * <code>java.security.cert.*</code> do not provide ways to 
 * programmatically modify fields within a X509 certificate object.
 * <p>
 *
 * For more information about the sun.* classes, see:<br>
 * <a href=http://java.sun.com/products/jdk/faq/faq-sun-packages.html>
 * http://java.sun.com/products/jdk/faq/faq-sun-packages.html</a>
 */

public class SignCert {

    /**
     * Signs a certificate <code>signee_alias</code> using the signing 
     * (private) key associated with <code>signing_alias</code>.  
     * <code>keyPass</code> unlocks the signing key.
     * <p>
     * Creates a signed certificate and stores it as a single-element 
     * certificate chain associated with <code>signee_alias</code>.
     */
    
    public static void signACert(String signee_alias, String signing_alias,
                                  char[] keyPass, KeyStore keyStore,
                                  char[] storePass)
        throws AppDescriptorException, CertificateException, IOException,
               KeyStoreException, NoSuchAlgorithmException, 
               InvalidKeyException, UnrecoverableKeyException,
               NoSuchProviderException, SignatureException, Exception {

        String sigAlgName;

        if (signee_alias == null || signing_alias == null || 
            keyPass == null || keyStore == null) {
            throw new AppDescriptorException("signACert got a null argument",
                                             4);
        }
        
        Object[] objs = recoverPrivateKey(signing_alias, storePass, 
                                          keyPass, keyStore);
        PrivateKey privKey = (PrivateKey)objs[0];
        if (keyPass == null)
            keyPass = (char[])objs[1];
        
        // Determine the signature algorithm
        // If no signature algorithm was specified at the command line,
        // we choose one that is compatible with the selected private key
        String keyAlgName = privKey.getAlgorithm();
        if (keyAlgName.equalsIgnoreCase("DSA")
            || keyAlgName.equalsIgnoreCase("DSS")) {
            sigAlgName = "SHA1WithDSA";
        } else if (keyAlgName.equalsIgnoreCase("RSA")) {
            sigAlgName = "SHA1WithRSA";
        } else {
            throw new
                AppDescriptorException("Cannot derive signature algorithm", 5);
        }

        // Get the old certificate
        Certificate oldCert = keyStore.getCertificate(signee_alias);
        if (oldCert == null) {
            throw new
                AppDescriptorException(signee_alias + " has no public key", 4);
        }

        if (!(oldCert instanceof X509Certificate)) {
            throw new AppDescriptorException(signee_alias +
                " has no X.509 certificate", 6);
        }
        
        // Get the "signing" certificate
        Certificate signingCert = keyStore.getCertificate(signing_alias);
        if (signingCert == null) {
            throw new
                AppDescriptorException(signee_alias + " has no public key", 7);
        }

        if (!(signingCert instanceof X509Certificate)) {
            throw new AppDescriptorException(signee_alias +
                " has no X.509 certificate", 8);
        }       

        // convert to X509CertImpl, so that we can modify selected fields
        // (no public APIs available yet)
        byte[] encoded = oldCert.getEncoded();
        X509CertImpl certImpl = new X509CertImpl(encoded);
        X509CertInfo certInfo = (X509CertInfo)certImpl.get(X509CertImpl.NAME +
                                "." + X509CertImpl.INFO);       
        
        // get an X509Certificate from the signing_alias
        encoded = signingCert.getEncoded();
        X509CertImpl signingCertImpl = new X509CertImpl(encoded);
        X509CertInfo signingCertInfo = (X509CertInfo) 
            signingCertImpl.get(X509CertImpl.NAME
                                + "." + X509CertImpl.INFO);     
        
        // Extend its validity
        int validity = 180;  // 180 days default
        Date firstDate = new Date();
        Date lastDate = new Date();
        lastDate.setTime(firstDate.getTime() + validity*1000*24*60*60L);
        CertificateValidity interval = new CertificateValidity(firstDate,
                                                               lastDate);
        certInfo.set(X509CertInfo.VALIDITY, interval);
        
        // Make new serial number
        certInfo.set(X509CertInfo.SERIAL_NUMBER, new CertificateSerialNumber
            ((int)(firstDate.getTime()/1000)));

        // Set owner and issuer fields
        X500Name owner;
        // Get the owner name from the certificate
        owner = (X500Name)certInfo.get(X509CertInfo.SUBJECT + "." +
                                       CertificateSubjectName.DN_NAME);

        // Get the issuer name - the owner of the signing certificate
        X500Name issuer;
        issuer = (X500Name)signingCertInfo.get(X509CertInfo.SUBJECT + "." +
                                           CertificateSubjectName.DN_NAME);
        
        certInfo.set(X509CertInfo.ISSUER + "." +
                     CertificateIssuerName.DN_NAME, issuer);
        
        // The inner and outer signature algorithms have to match.
        // The way we achieve that is really ugly, but there seems to be no
        // other solution: We first sign the cert, then retrieve the
        // outer sigalg and use it to set the inner sigalg

        X509CertImpl newCert = new X509CertImpl(certInfo);
        newCert.sign(privKey, sigAlgName);
        AlgorithmId sigAlgid = (AlgorithmId)newCert.get(X509CertImpl.SIG_ALG);
        certInfo.set(CertificateAlgorithmId.NAME + "." +
                     CertificateAlgorithmId.ALGORITHM, sigAlgid);

        // Sign the new certificate
        newCert = new X509CertImpl(certInfo);
        newCert.sign(privKey, sigAlgName);

        // Store the new certificate as a single-element certificate chain
        keyStore.setKeyEntry(signee_alias, privKey,
                             (keyPass != null) ? keyPass : storePass,
                             new Certificate[] { newCert });


        System.err.println("New certificate signed & inserted into KeyStore!");
        System.err.print(newCert.toString());
        System.err.println();
    }

    /**
     * Recovers (private) key associated with given alias.
     *
     * @return an array of objects, where the 1st element in the array is the
     * recovered private key, and the 2nd element is the password used to
     * recover it.
     */
    private static Object[] recoverPrivateKey(String alias, char[] storePass,
                                              char[] keyPass, KeyStore keyStore)
        throws KeyStoreException, NoSuchAlgorithmException, 
               UnrecoverableKeyException, Exception
    {
        Key key = null;
        
        if (keyStore.containsAlias(alias) == false) {
            throw new Exception("Alias <" + alias + "> does not exist");
        }
        if (keyStore.isKeyEntry(alias) == false) {
            throw new Exception("Alias <" + alias + "> has no (private) key");
        }

        if (keyPass == null) {
            // Try to recover the key using the keystore password
            try {
                key = keyStore.getKey(alias, storePass);
                keyPass = storePass;
            } catch (UnrecoverableKeyException e) {
                throw new Exception("Invalid Key password entered");
                // Did not work out, so prompt user for key password
                // keyPass = getKeyPasswd(alias, null, null);
                // key = keyStore.getKey(alias, keyPass);
            }
        } else {
            key = keyStore.getKey(alias, keyPass);
        }
        if (!(key instanceof PrivateKey)) {
            throw new Exception("Recovered key is not a private key");
        }
        return new Object[] {(PrivateKey)key, keyPass};
    }
}

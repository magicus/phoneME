/*
 * @(#)Sun.java	1.4 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.  
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
 *
 */

/*
 * Note that there are three versions of the Sun security provider
 * in this tree:  this subsetted one for the CDC/FP, one for the
 * CDC/FP optional security package, and a severely subsetted
 * one for CDC. Make sure you're editing the right one!
 */

package sun.security.provider;

import java.io.*;
import java.util.*;
import java.security.*;

/**
 * The SUN Security Provider.
 *
 * @author Benjamin Renaud 
 *
 * @version 1.30, 02/02/00
 */

/**
 * Defines the SUN provider.
 *
 * Algorithms supported, and their names:
 *
 * - SHA is the message digest scheme described in FIPS 180-1. 
 *   Aliases for SHA are SHA-1 and SHA1.
 *
 * - SHA1withDSA is the signature scheme described in FIPS 186.
 *   (SHA used in DSA is SHA-1: FIPS 186 with Change No 1.)
 *   Aliases for SHA1withDSA are DSA, DSS, SHA/DSA, SHA-1/DSA, SHA1/DSA,
 *   SHAwithDSA, DSAWithSHA1, and the object
 *   identifier strings "OID.1.3.14.3.2.13", "OID.1.3.14.3.2.27" and
 *   "OID.1.2.840.10040.4.3".
 *
 * - DSA is the key generation scheme as described in FIPS 186.
 *   Aliases for DSA include the OID strings "OID.1.3.14.3.2.12"
 *   and "OID.1.2.840.10040.4.1".
 *
 * - MD5 is the message digest scheme described in RFC 1321.
 *   There are no aliases for MD5.
 *
 * - X.509 is the certificate factory type for X.509 certificates
 *   and CRLs. Aliases for X.509 are X509.
 *
 * - PKIX is the certification path validation algorithm described
 *   in RFC 3280. The ValidationAlgorithm attribute notes the 
 *   specification that this provider implements.
 *
 * - LDAP is the CertStore type for LDAP repositories. The 
 *   LDAPSchema attribute notes the specification defining the
 *   schema that this provider uses to find certificates and CRLs.
 */

public final class Sun extends Provider {

    private static final String INFO = "SUN " + 
    "(DSA key/parameter generation; DSA signing; SHA-1, MD5 digests; " +
    "SecureRandom; X.509 certificates; JKS keystore; PKIX CertPathValidator; " +
    "PKIX CertPathBuilder; LDAP, Collection CertStores)";

    public Sun() {
	/* We are the SUN provider */
	super("SUN", 1.42, INFO);
	
	final Map map = new HashMap();

	/*
	 * Signature engines 
	 */
	map.put("Signature.SHA1withDSA", "sun.security.provider.DSA");
    
	map.put("Alg.Alias.Signature.DSA", "SHA1withDSA");
	map.put("Alg.Alias.Signature.DSS", "SHA1withDSA");
	map.put("Alg.Alias.Signature.SHA/DSA", "SHA1withDSA");
	map.put("Alg.Alias.Signature.SHA-1/DSA", "SHA1withDSA");
	map.put("Alg.Alias.Signature.SHA1/DSA", "SHA1withDSA");
	map.put("Alg.Alias.Signature.SHAwithDSA", "SHA1withDSA");
	map.put("Alg.Alias.Signature.DSAWithSHA1", "SHA1withDSA");
	map.put("Alg.Alias.Signature.OID.1.2.840.10040.4.3",
	    "SHA1withDSA");
	map.put("Alg.Alias.Signature.1.2.840.10040.4.3", "SHA1withDSA");
	map.put("Alg.Alias.Signature.1.3.14.3.2.13", "SHA1withDSA");
	map.put("Alg.Alias.Signature.1.3.14.3.2.27", "SHA1withDSA");
    
	/*
	 *  Key Pair Generator engines 
	 */
	map.put("KeyPairGenerator.DSA", 
	    "sun.security.provider.DSAKeyPairGenerator");
	map.put("Alg.Alias.KeyPairGenerator.OID.1.2.840.10040.4.1", "DSA");
	map.put("Alg.Alias.KeyPairGenerator.1.2.840.10040.4.1", "DSA");
	map.put("Alg.Alias.KeyPairGenerator.1.3.14.3.2.12", "DSA");

	/* 
	 * Digest engines 
	 */
	map.put("MessageDigest.MD5", "sun.security.provider.MD5");
	map.put("MessageDigest.SHA", "sun.security.provider.SHA");

	map.put("Alg.Alias.MessageDigest.SHA-1", "SHA");
	map.put("Alg.Alias.MessageDigest.SHA1", "SHA");

	map.put("MessageDigest.SHA-256", "sun.security.provider.SHA2");
	map.put("MessageDigest.SHA-384", "sun.security.provider.SHA3");
	map.put("MessageDigest.SHA-512", "sun.security.provider.SHA5");

	/*
	 * Algorithm Parameter Generator engines
	 */
	map.put("AlgorithmParameterGenerator.DSA",
	    "sun.security.provider.DSAParameterGenerator");

	/*
	 * Algorithm Parameter engines
	 */
	map.put("AlgorithmParameters.DSA",
	    "sun.security.provider.DSAParameters");
	map.put("Alg.Alias.AlgorithmParameters.1.3.14.3.2.12", "DSA");
	map.put("Alg.Alias.AlgorithmParameters.1.2.840.10040.4.1", "DSA");

	/*
	 * Key factories
	 */
	map.put("KeyFactory.DSA", "sun.security.provider.DSAKeyFactory");
	map.put("Alg.Alias.KeyFactory.1.3.14.3.2.12", "DSA");
	map.put("Alg.Alias.KeyFactory.1.2.840.10040.4.1", "DSA");

	/*
	 * SecureRandom
	 */
	map.put("SecureRandom.SHA1PRNG",
	     "sun.security.provider.SecureRandom");

	/*
	 * Certificates
	 */
	map.put("CertificateFactory.X.509",
	    "sun.security.provider.X509Factory");
	map.put("Alg.Alias.CertificateFactory.X509", "X.509");

	/*
	 * KeyStore
	 */
	map.put("KeyStore.JKS", "sun.security.provider.JavaKeyStore");

	/*
	 * CertPathBuilder
         * Sun provider functionality subsetted for CDC/FP.
	map.put("CertPathBuilder.PKIX",
	    "sun.security.provider.certpath.SunCertPathBuilder");
	map.put("CertPathBuilder.PKIX ValidationAlgorithm", 
	    "RFC3280");
	 */

	/*
	 * CertPathValidator
         * Sun provider functionality subsetted for CDC/FP.
	map.put("CertPathValidator.PKIX",
	    "sun.security.provider.certpath.PKIXCertPathValidator");
	map.put("CertPathValidator.PKIX ValidationAlgorithm", 
	    "RFC3280");
	 */

	/*
	 * CertStores
         * Sun provider functionality subsetted for CDC/FP.
	map.put("CertStore.LDAP",
	    "sun.security.provider.certpath.LDAPCertStore");
	map.put("CertStore.LDAP LDAPSchema", "RFC2587");
	map.put("CertStore.Collection",
	    "sun.security.provider.certpath.CollectionCertStore");
	map.put("CertStore.com.sun.security.IndexedCollection",
	    "sun.security.provider.certpath.IndexedCollectionCertStore");
         */

	/*
	 * KeySize
	 */
	map.put("Signature.SHA1withDSA KeySize", "1024");
	map.put("KeyPairGenerator.DSA KeySize", "1024");
	map.put("AlgorithmParameterGenerator.DSA KeySize", "1024");

	/*
	 * Implementation type: software or hardware
	 */
	map.put("Signature.SHA1withDSA ImplementedIn", "Software");
	map.put("KeyPairGenerator.DSA ImplementedIn", "Software");
	map.put("MessageDigest.MD5 ImplementedIn", "Software");
	map.put("MessageDigest.SHA ImplementedIn", "Software");
	map.put("AlgorithmParameterGenerator.DSA ImplementedIn", 
	    "Software");
	map.put("AlgorithmParameters.DSA ImplementedIn", "Software");
	map.put("KeyFactory.DSA ImplementedIn", "Software");
	map.put("SecureRandom.SHA1PRNG ImplementedIn", "Software");
	map.put("CertificateFactory.X.509 ImplementedIn", "Software");
	map.put("KeyStore.JKS ImplementedIn", "Software");
	map.put("CertPathValidator.PKIX ImplementedIn", "Software");
	map.put("CertPathBuilder.PKIX ImplementedIn", "Software");
	map.put("CertStore.LDAP ImplementedIn", "Software");
        /*
         * Sun provider functionality subsetted for CDC/FP.
	map.put("CertStore.Collection ImplementedIn", "Software");
	map.put("CertStore.com.sun.security.IndexedCollection ImplementedIn",
	    "Software");
         */

	AccessController.doPrivileged(new java.security.PrivilegedAction() {
	    public Object run() {
		putAll(map);
		return null;
	    }
	});
    }
}

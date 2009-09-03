/*
 * @(#)OIDMap.java	1.4 06/10/10
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
 * Note that there are two versions of OIDMap, this subsetted
 * one for CDC/FP and another for the security optional package.
 * Be sure you're editing the right one!
 */

package sun.security.x509;

import java.util.*;
import java.io.IOException;

import java.security.cert.CertificateException;
import java.security.cert.CertificateParsingException;

import sun.security.util.*;

/**
 * This class defines the mapping from OID & name to classes and vice
 * versa.  Used by CertificateExtensions & PKCS10 to get the java
 * classes associated with a particular OID/name.
 *
 * @author Amit Kapoor
 * @author Hemma Prafullchandra
 * @author Andreas Sterbenz
 *
 * @version 1.4, 10/10/06
 */
public class OIDMap {
    
    private OIDMap() {
	// empty
    }

    // "user-friendly" names
    private static final String ROOT = X509CertImpl.NAME + "." +
                                 X509CertInfo.NAME + "." +
                                 X509CertInfo.EXTENSIONS;
    private static final String AUTH_KEY_IDENTIFIER = ROOT + "." +
                                          AuthorityKeyIdentifierExtension.NAME;
    private static final String SUB_KEY_IDENTIFIER  = ROOT + "." +
                                          SubjectKeyIdentifierExtension.NAME;
    private static final String KEY_USAGE           = ROOT + "." +
                                          KeyUsageExtension.NAME;
    private static final String PRIVATE_KEY_USAGE   = ROOT + "." +
                                          PrivateKeyUsageExtension.NAME;
    private static final String POLICY_MAPPINGS     = ROOT + "." +
                                          PolicyMappingsExtension.NAME;
    private static final String SUB_ALT_NAME        = ROOT + "." +
                                          SubjectAlternativeNameExtension.NAME;
    private static final String ISSUER_ALT_NAME     = ROOT + "." +
                                          IssuerAlternativeNameExtension.NAME;
    private static final String BASIC_CONSTRAINTS   = ROOT + "." +
                                          BasicConstraintsExtension.NAME;
    private static final String NAME_CONSTRAINTS    = ROOT + "." +
                                          NameConstraintsExtension.NAME;
    private static final String POLICY_CONSTRAINTS  = ROOT + "." +
                                          PolicyConstraintsExtension.NAME;
    private static final String CRL_NUMBER  = ROOT + "." +
                                              CRLNumberExtension.NAME;
    private static final String CRL_REASON  = ROOT + "." +
                                              CRLReasonCodeExtension.NAME;
    private static final String NETSCAPE_CERT  = ROOT + "." +
                                              NetscapeCertTypeExtension.NAME;
    /* CDC/FP subsets away CertificatePoliciesExtension
     *private static final String CERT_POLICIES = ROOT + "." +
     *                                     CertificatePoliciesExtension.NAME;
     */
    private static final String EXT_KEY_USAGE       = ROOT + "." +
                                          ExtendedKeyUsageExtension.NAME;
    /*
     * CDC/FP subsets InhibitAnyPolicyExtension and
     * CRLDistributionPointsExtension to
     * the security optional package
    private static final String INHIBIT_ANY_POLICY  = ROOT + "." +
                                          InhibitAnyPolicyExtension.NAME;
    private static final String CRL_DIST_POINTS = ROOT + "." +
    					CRLDistributionPointsExtension.NAME;
     */

    /** Map ObjectIdentifier(oid) -> OIDInfo(info) */
    private final static Map oidMap;

    /** Map String(friendly name) -> OIDInfo(info) */
    private final static Map nameMap;

    static {
	oidMap = new HashMap();
	nameMap = new HashMap();
	try {
	    addInternal(SUB_KEY_IDENTIFIER,  "2.5.29.14",
			"sun.security.x509.SubjectKeyIdentifierExtension");
	    addInternal(KEY_USAGE, 	       "2.5.29.15", 
			"sun.security.x509.KeyUsageExtension");
	    addInternal(PRIVATE_KEY_USAGE,   "2.5.29.16", 
			"sun.security.x509.PrivateKeyUsageExtension");
	    addInternal(SUB_ALT_NAME,        "2.5.29.17", 
			"sun.security.x509.SubjectAlternativeNameExtension");
	    addInternal(ISSUER_ALT_NAME,     "2.5.29.18", 
			"sun.security.x509.IssuerAlternativeNameExtension");
	    addInternal(BASIC_CONSTRAINTS,   "2.5.29.19", 
			"sun.security.x509.BasicConstraintsExtension");
	    addInternal(CRL_NUMBER,          "2.5.29.20", 
			"sun.security.x509.CRLNumberExtension");
	    addInternal(CRL_REASON,          "2.5.29.21", 
			"sun.security.x509.CRLReasonCodeExtension");
	    addInternal(NAME_CONSTRAINTS,    "2.5.29.30", 
			"sun.security.x509.NameConstraintsExtension");
	    addInternal(POLICY_MAPPINGS,     "2.5.29.33", 
			"sun.security.x509.PolicyMappingsExtension");
	    addInternal(AUTH_KEY_IDENTIFIER, "2.5.29.35", 
			"sun.security.x509.AuthorityKeyIdentifierExtension");
	    addInternal(POLICY_CONSTRAINTS,  "2.5.29.36", 
			"sun.security.x509.PolicyConstraintsExtension");
	    addInternal(NETSCAPE_CERT,       "2.16.840.1.113730.1.1", 
			"sun.security.x509.NetscapeCertTypeExtension");
            /* CDC/FP subsets away CertificatePoliciesExtension
             * addInternal(CERT_POLICIES,       "2.5.29.32", 
             *	"sun.security.x509.CertificatePoliciesExtension");
             */
	    addInternal(EXT_KEY_USAGE,       "2.5.29.37", 
			"sun.security.x509.ExtendedKeyUsageExtension");
            /*
             * CDC/FP subsets InhibitAnyPolicyExtension and
             * CRLDistributionPointsExtension to the
             * security optional package.
	    addInternal(INHIBIT_ANY_POLICY,  "2.5.29.54", 
			"sun.security.x509.InhibitAnyPolicyExtension");
	    addInternal(CRL_DIST_POINTS,     "2.5.29.31",
	    		"sun.security.x509.CRLDistributionPointsExtension");
             */
	} catch (IOException e) {
	    throw new RuntimeException("Internal error: " + e, e);
	}
    }

    /**
     * Add attributes to the table. For internal use in the static
     * initializer.
     */
    private static void addInternal(String name, String oidString, 
	    String className) throws IOException {
	ObjectIdentifier oid = new ObjectIdentifier(oidString);
	OIDInfo info = new OIDInfo(name, oid, className);
	oidMap.put(oid, info);
	nameMap.put(name, info);
    }
    
    /**
     * Inner class encapsulating the mapping info and Class loading.
     */
    private static class OIDInfo {
	
	final ObjectIdentifier oid;
	final String name;
	final String className;
	private volatile Class clazz;
	
	OIDInfo(String name, ObjectIdentifier oid, String className) {
	    this.name = name;
	    this.oid = oid;
	    this.className = className;
	}
	
	OIDInfo(String name, ObjectIdentifier oid, Class clazz) {
	    this.name = name;
	    this.oid = oid;
	    this.className = clazz.getName();
	    this.clazz = clazz;
	}
	
	/**
	 * Return the Class object associated with this attribute.
	 */
	Class getClazz() throws CertificateException {
	    try {
		Class c = clazz;
		if (c == null) {
		    c = Class.forName(className);
		    clazz = c;
		}
		return c;
	    } catch (ClassNotFoundException e) {
		throw (CertificateException)new CertificateException
				("Could not load class: " + e).initCause(e);
	    }
	}
    }

    /**
     * Add a name to lookup table.
     *
     * @param name the name of the attr
     * @param oid the string representation of the object identifier for
     *         the class.
     * @param clazz the Class object associated with this attribute
     * @exception CertificateException on errors.
     */
    public static void addAttribute(String name, String oid, Class clazz)
	    throws CertificateException {
	ObjectIdentifier objId;
	try {
            objId = new ObjectIdentifier(oid);
	} catch (IOException ioe) {
	    throw new CertificateException
	    			("Invalid Object identifier: " + oid);
	}
	OIDInfo info = new OIDInfo(name, objId, clazz);
	if (oidMap.put(objId, info) != null) {
	    throw new CertificateException
	    			("Object identifier already exists: " + oid);
	}
	if (nameMap.put(name, info) != null) {
	    throw new CertificateException("Name already exists: " + name);
	}
    }
    
    /**
     * Return user friendly name associated with the OID.
     *
     * @param oid the name of the object identifier to be returned.
     * @return the user friendly name or null if no name
     * is registered for this oid.
     */
    public static String getName(ObjectIdentifier oid) {
	OIDInfo info = (OIDInfo)oidMap.get(oid);
	return (info == null) ? null : info.name;
    }

    /**
     * Return Object identifier for user friendly name.
     *
     * @param name the user friendly name.
     * @return the Object Identifier or null if no oid
     * is registered for this name.
     */
    public static ObjectIdentifier getOID(String name) {
	OIDInfo info = (OIDInfo)nameMap.get(name);
	return (info == null) ? null : info.oid;
    }

    /**
     * Return the java class object associated with the user friendly name.
     *
     * @param name the user friendly name.
     * @exception CertificateException if class cannot be instantiated.
     */
    public static Class getClass(String name) throws CertificateException {
	OIDInfo info = (OIDInfo)nameMap.get(name);
	return (info == null) ? null : info.getClazz();
    }

    /**
     * Return the java class object associated with the object identifier.
     *
     * @param oid the name of the object identifier to be returned.
     * @exception CertificateException if class cannot be instatiated.
     */
    public static Class getClass(ObjectIdentifier oid) 
	    throws CertificateException {
	OIDInfo info = (OIDInfo)oidMap.get(oid);
	return (info == null) ? null : info.getClazz();
    }

}

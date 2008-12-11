/*
 * @(#)ManifestEntryVerifier.java	1.21 06/10/10
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

package sun.security.util;

import java.security.*;
import java.io.*;
import java.util.*;
import java.util.jar.*;

import sun.misc.BASE64Decoder;

/**
 * This class is used to verify each entry in a jar file with its
 * manifest value.
 */

public class ManifestEntryVerifier {

    private static final Debug debug = Debug.getInstance("jar");

    private static final Provider sunProvider = new sun.security.provider.Sun();

    /** the created digest objects */
    HashMap createdDigests;

    /** the digests in use for a given entry*/
    ArrayList digests;

    /** the manifest hashes for the digests in use */
    ArrayList manifestHashes;

    /** the hash values in the Manifest */
    private byte manifestHash[][] = { null, null};
    private BASE64Decoder decoder = null;
    private String name = null;
    private Manifest man;

    private boolean skip = true;

    private JarEntry entry;

    private java.security.cert.Certificate[] certs = null;

    /**
     * Create a new ManifestEntryVerifier object.
     */
    public ManifestEntryVerifier(Manifest man)
    {
	createdDigests = new HashMap(11);
	digests = new ArrayList();
	manifestHashes = new ArrayList();
	decoder = new BASE64Decoder();
	this.man = man;
    }

    /**
     * Find the hashes in the
     * manifest for this entry, save them, and set the MessageDigest
     * objects to calculate the hashes on the fly. If name is
     * null it signifies that update/verify should ignore this entry.
     */
    public void setEntry(String name, JarEntry entry)
	throws IOException
    {
	digests.clear();
	manifestHashes.clear();
	this.name = name;
	this.entry = entry;

	skip = true;
	certs = null;

	if (man == null || name == null) {
	    return;
	}

	/* get the headers from the manifest for this entry */
	/* if there aren't any, we can't verify any digests for this entry */

	Attributes attr = man.getAttributes(name);
	if (attr == null) {
	    // We should be able to remove this at some point.
	    // there are broken jars floating around with ./name and /name
	    // in the manifest, and "name" in the zip/jar file.
	    attr = man.getAttributes("./"+name);
	    if (attr == null) {
		attr = man.getAttributes("/"+name);
		if (attr == null)
		    return;
	    }
	}

	Iterator it = attr.entrySet().iterator();

	while(it.hasNext()) {
	    Map.Entry se = (Map.Entry)it.next();
	    String key = se.getKey().toString();

	    if (key.toUpperCase(Locale.ENGLISH).endsWith("-DIGEST")) {
		// 7 is length of "-Digest"
		String algorithm = key.substring(0, key.length()-7);

		MessageDigest digest =
		    (MessageDigest) createdDigests.get(algorithm);

		if (digest == null) {
		    try {

			digest = MessageDigest.getInstance
					(algorithm, sunProvider);
			createdDigests.put(algorithm, digest);
		    } catch (NoSuchAlgorithmException nsae) {
			// ignore
		    }
		}

		if (digest != null) {
		    skip = false;
		    digest.reset();
		    digests.add(digest);
		    manifestHashes.add(
				decoder.decodeBuffer((String)se.getValue()));
		}
	    }
	}
    }

    /**
     * update the digests for the digests we are interested in
     */
    public void update(byte buffer) {
	if (skip) return;

	for (int i=0; i < digests.size(); i++) {
	    ((MessageDigest)digests.get(i)).update(buffer);
	}
    }

    /**
     * update the digests for the digests we are interested in
     */
    public void update(byte buffer[], int off, int len) {
	if (skip) return;

	for (int i=0; i < digests.size(); i++) {
	    ((MessageDigest)digests.get(i)).update(buffer, off, len);
	}
    }

    /**
     * get the JarEntry for this object
     */
    public JarEntry getEntry()
    {
	return entry;
    }

    /**
     * go through all the digests, calculating the final digest
     * and comparing it to the one in the manifest. If this is
     * the first time we have verified this object, remove its
     * Certs from sigFileCerts and place in verifiedCerts.
     *
     *
     */
    public java.security.cert.Certificate[] verify(Hashtable verifiedCerts,
		Hashtable sigFileCerts)
	throws JarException
    {
 	if (skip) return null;

	if (certs != null)
	    return certs;

	for (int i=0; i < digests.size(); i++) {

	    MessageDigest digest  = (MessageDigest)digests.get(i);
	    byte [] manHash = (byte[]) manifestHashes.get(i);
	    byte [] theHash = digest.digest();

	    if (debug != null) {
		debug.println("Manifest Entry: " +
				   name + " digest=" + digest.getAlgorithm());
		debug.println("  manifest " + toHex(manHash));
		debug.println("  computed " + toHex(theHash));
		debug.println();
	    }

	    if (!MessageDigest.isEqual(theHash, manHash))
		throw new SecurityException(digest.getAlgorithm()+
					    " digest error for "+name);
	}

	// take it out of sigFileCerts and put it in verifiedIds...
	certs = (java.security.cert.Certificate[]) sigFileCerts.remove(name);
	if (certs != null) {
	    verifiedCerts.put(name, certs);
	}
	return certs;
    }

    // for the toHex function
    private static final char[] hexc =
            {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
    /**
     * convert a byte array to a hex string for debugging purposes
     * @param data the binary data to be converted to a hex string
     * @return an ASCII hex string
     */

    static String toHex(byte[] data) {

	StringBuffer sb = new StringBuffer(data.length*2);

	for (int i=0; i<data.length; i++) {
	    sb.append(hexc[(data[i] >>4) & 0x0f]);
	    sb.append(hexc[data[i] & 0x0f]);
	}
	return sb.toString();
    }

}

/*
 * @(#)SignatureFileVerifier.java	1.29 06/10/10
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

import java.security.cert.Certificate;
import java.security.cert.X509Certificate;
import java.security.cert.CertificateException;
import java.security.*;
import java.io.*;
import java.util.*;
import java.util.jar.*;

import sun.security.pkcs.*;
import sun.misc.BASE64Decoder;

public class SignatureFileVerifier {

    /* Are we debugging ? */
    private static final Debug debug = Debug.getInstance("jar");

    /* cache of Certificate[] objects */
    private ArrayList certCache;

    /** the PKCS7 block for this .DSA/.RSA file */
    private PKCS7 block;

    /** the raw bytes of the .SF file */
    private byte sfBytes[];

    /** the name of the signature block file, uppercased and without
     *  the extension (.DSA/.RSA) 
     */
    private String name;

    /** the ManifestDigester */
    private ManifestDigester md;

    /** cache of created MessageDigest objects */
    private HashMap createdDigests;

    /* workaround for parsing Netscape jars  */
    private boolean workaround = false;

    /**
     * Create the named SignatureFileVerifier.
     *
     * @param name the name of the signature block file (.DSA/.RSA)
     *
     * @param rawBytes the raw bytes of the signature block file
     */
    public SignatureFileVerifier(ArrayList certCache,
				 ManifestDigester md,
				 String name,
				 byte rawBytes[])
	throws IOException
    {
	block = new PKCS7(rawBytes);
	sfBytes = block.getContentInfo().getData();
	this.name = name.substring(0, name.lastIndexOf("."))
                                                   .toUpperCase(Locale.ENGLISH);
	this.md = md;
	this.certCache = certCache;
    }

    /**
     * returns true if we need the .SF file
     */
    public boolean needSignatureFileBytes()
    {

	return sfBytes == null;
    }


    /**
     * returns true if we need this .SF file.
     *
     * @param name the name of the .SF file without the extension
     *
     */
    public boolean needSignatureFile(String name)
    {
	return this.name.equalsIgnoreCase(name);
    }

    /**
     * used to set the raw bytes of the .SF file when it 
     * is external to the signature block file.
     */
    public void setSignatureFile(byte sfBytes[])
    {
	this.sfBytes = sfBytes;
    }

    /** get digest from cache */

    private MessageDigest getDigest(String algorithm)
    {
	if (createdDigests == null)
	    createdDigests = new HashMap();

	MessageDigest digest = 
	    (MessageDigest) createdDigests.get(algorithm);

	if (digest == null) {
	    try {
		digest = MessageDigest.getInstance(algorithm);
		createdDigests.put(algorithm, digest);
	    } catch (NoSuchAlgorithmException nsae) {
		// ignore
	    }
	}
	return digest;
    }

    /**
     * process the signature block file. Goes through the .SF file
     * and adds certificates for each section where the .SF section
     * hash was verified against the Manifest section.
     *
     *
     */
    public void process(Hashtable certificates)
	throws IOException, SignatureException, NoSuchAlgorithmException,
	    JarException
    {
	Manifest sf = new Manifest();
	sf.read(new ByteArrayInputStream(sfBytes));

	String version = 
	    sf.getMainAttributes().getValue(Attributes.Name.SIGNATURE_VERSION);

	if ((version == null) || !(version.equalsIgnoreCase("1.0"))) {
	    // TODO: should this be an exception?
	    // for now we just ignore this signature file
	    return;
	}

	SignerInfo[] infos = block.verify(sfBytes);

	if (infos == null) {
	    throw new SecurityException("cannot verify signature block file " +
					name);
	}

	BASE64Decoder decoder = new BASE64Decoder();

	Certificate[] newCerts = getCertificates(infos, block);

	// make sure we have something to do all this work for...
	if (newCerts == null)
	    return;

	Iterator entries = sf.getEntries().entrySet().iterator();

	// see if we can verify the whole manifest first
	boolean manifestSigned = verifyManifestHash(sf, md, decoder);

	// go through each section in the signature file
	while(entries.hasNext()) {

	    Map.Entry e = (Map.Entry)entries.next();
	    String name = (String) e.getKey();

	    if (manifestSigned ||
	       (verifySection((Attributes) e.getValue(), name, md, decoder))) {

		if (name.startsWith("./"))
		    name = name.substring(2);

		if (name.startsWith("/"))
		    name = name.substring(1);

		updateCerts(newCerts, certificates, name);

		if (debug != null) {
		    debug.println("processSignature signed name = "+name);
		}

	    } else if (debug != null) {
		debug.println("processSignature unsigned name = "+name);
	    }

	}
    }

    /**
     * See if the whole manifest was signed.
     */
    private boolean verifyManifestHash(Manifest sf,
				       ManifestDigester md,
				       BASE64Decoder decoder)
	 throws IOException
    {
	Attributes mattr = sf.getMainAttributes();
	boolean manifestSigned = false;
	Iterator mit = mattr.entrySet().iterator();

	// go through all the attributes and process *-Digest-Manifest entries
	while(mit.hasNext()) {
	    Map.Entry se = (Map.Entry)mit.next();
	    String key = se.getKey().toString();

	    if (key.toUpperCase(Locale.ENGLISH).endsWith("-DIGEST-MANIFEST")) {
		// 16 is length of "-Digest-Manifest"
		String algorithm = key.substring(0, key.length()-16);

		MessageDigest digest = getDigest(algorithm);
		if (digest != null) {
		    byte[] computedHash = md.manifestDigest(digest);
		    byte[] expectedHash = 
			decoder.decodeBuffer((String)se.getValue());

		    if (debug != null) {
		     debug.println("Signature File: Manifest digest " +
					  digest.getAlgorithm());
		     debug.println( "  sigfile  " + toHex(expectedHash));
		     debug.println( "  computed " + toHex(computedHash));
		     debug.println();
		    }

		    if (MessageDigest.isEqual(computedHash,
					      expectedHash)) {
			manifestSigned = true;
		    } else {
			//we will continue and verify each section
		    }
		}
	    }
	}
	return manifestSigned;
    }

    /**
     * given the .SF digest header, and the data from the
     * section in the manifest, see if the hashes match.
     * if not, throw a SecurityException.
     *
     * @return true if all the -Digest headers verified
     * @exception SecurityException if the hash was not equal
     */

    private boolean verifySection(Attributes sfAttr,
				  String name,
				  ManifestDigester md,
				  BASE64Decoder decoder)
	 throws IOException
    {
	boolean oneDigestVerified = false;
	ManifestDigester.Entry mde = md.get(name,block.isOldStyle());

	if (mde == null) {
	    throw new SecurityException(
		  "no manifiest section for signature file entry "+name);
	}

	if (sfAttr != null) {

	    //sun.misc.HexDumpEncoder hex = new sun.misc.HexDumpEncoder();
	    //hex.encodeBuffer(data, System.out);

	    Iterator it = sfAttr.entrySet().iterator();

	    // go through all the attributes and process *-Digest entries
	    while(it.hasNext()) {
		Map.Entry se = (Map.Entry)it.next();
		String key = se.getKey().toString();
		
		if (key.toUpperCase(Locale.ENGLISH).endsWith("-DIGEST")) {
		    // 7 is length of "-Digest"
		    String algorithm = key.substring(0, key.length()-7);

		    MessageDigest digest = getDigest(algorithm);

		    if (digest != null) {
			boolean ok = false;

			byte[] expected = 
			    decoder.decodeBuffer((String)se.getValue());
			byte[] computed;
			if (workaround) {
			    computed = mde.digestWorkaround(digest);
			} else {
			    computed = mde.digest(digest);
			}

			if (debug != null) {
			  debug.println("Signature Block File: " +
				   name + " digest=" + digest.getAlgorithm());
			  debug.println("  expected " + toHex(expected));
			  debug.println("  computed " + toHex(computed));
			  debug.println();
			}

			if (MessageDigest.isEqual(computed, expected)) {
			    oneDigestVerified = true;
			    ok = true;
			} else {
			    // attempt to fallback to the workaround
			    if (!workaround) {
			       computed = mde.digestWorkaround(digest);
			       if (MessageDigest.isEqual(computed, expected)) {
				   if (debug != null) {
				       debug.println("  re-computed " + toHex(computed));
				       debug.println();
				   }
				   workaround = true;
				   oneDigestVerified = true;
				   ok = true;
			       }
			    }
			}
			if (!ok){
			    throw new SecurityException("invalid " + 
				       digest.getAlgorithm() + 
				       " signature file digest for " + name);
			}
		    }
		}
	    }
	}
	return oneDigestVerified;
    }

    /**
     * Given the PKCS7 blocks and SignerInfo[], create a Vector
     * of certificate objects. We do this only *once* for a given
     * signature block file.
     */
    private Certificate[] getCertificates(SignerInfo infos[], PKCS7 block) {

	ArrayList certs = null;

	for (int i = 0; i < infos.length; i++) {
	    SignerInfo info = infos[i];
	    try {
		ArrayList certChain = info.getCertificateChain(block);
		if (certs == null)
		    certs = new ArrayList();
		certs.addAll(certChain);
		if (debug != null) {
		    debug.println("Signature Block Certificate: " +
				  (X509Certificate)certChain.get(0));
		}
	    } catch (IOException e) {
	    }
	}

	if (certs != null) {
	    Certificate[] certificates = new Certificate[certs.size()];
	    System.arraycopy(certs.toArray(), 0,
			     certificates, 0, 
			     certs.size());
	    return certificates;
	} else {
	    return null;
	}
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

    // returns true if set contains cert
    static boolean contains(Certificate[] set, Certificate cert)
    {
	for (int i=0; i < set.length; i++) {
	    if (set[i].equals(cert))
		return true;
	}
	return false;
    }

    // returns true if subset is a subset of set
    static boolean isSubSet(Certificate[] subset, Certificate[] set)
    {
	// check for the same object
	if (set == subset) 
	    return true;

	for (int i = 0; i < subset.length; i++) {
	    if (!contains(set, subset[i]))
		return false;
	}
	return true;
    }

    /**
     * returns true if certs contains exactly the same certs as
     * oldCerts and newCerts, false otherwise. oldCerts
     * is allowed to be null.
     */
    static boolean matches(Certificate[] certs,
			   Certificate[] oldCerts, Certificate[] newCerts)
    {
	// special case 
	if ((oldCerts == null) && (certs == newCerts))
	    return true;

	// make sure all oldCerts are in certs
	if ((oldCerts != null) && !isSubSet(oldCerts, certs))
	    return false;
	
	// make sure all newCerts are in certs
	if (!isSubSet(newCerts, certs)) {
	    return false;
	}

	// now make sure all the certificates in certs are
	// also in oldCerts or newCerts

	for (int i = 0; i < certs.length; i++) {
	    boolean found = 
		((oldCerts != null) && contains(oldCerts, certs[i])) ||
		contains(newCerts, certs[i]);
	    if (!found)
		return false;
	}
	return true;
    }

    void updateCerts(Certificate[] newCerts, Hashtable certHash, String name)
    {
	Certificate[] oldCerts =
	    (Certificate[])certHash.get(name);

	// search through the cache for a match, go in reverse order
	// as we are more likely to find a match with the last one
	// added to the cache

	Certificate[] certs;
	for (int i = certCache.size()-1; i != -1; i--) {
	    certs = (Certificate[]) certCache.get(i);
	    if (matches(certs, oldCerts, newCerts)) {
		certHash.put(name, certs);
		return;
	    }
	}

	if (oldCerts == null) {
	    certs = newCerts;
	} else {
	    certs = 
		new Certificate[oldCerts.length + newCerts.length];
	    System.arraycopy(oldCerts, 0, certs, 0, oldCerts.length);
	    System.arraycopy(newCerts, 0, certs, oldCerts.length, 
			     newCerts.length);
	}
	certCache.add(certs);
	certHash.put(name, certs);
    }
}


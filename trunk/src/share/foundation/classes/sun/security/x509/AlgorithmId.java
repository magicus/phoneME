/*
 * @(#)AlgorithmId.java	1.93 06/10/11
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

package sun.security.x509;

import java.io.*;
import java.security.*;
import java.util.Hashtable;
import java.util.Enumeration;

import sun.security.util.*;


/**
 * This class identifies algorithms, such as cryptographic transforms, each
 * of which may be associated with parameters.  Instances of this base class
 * are used when this runtime environment has no special knowledge of the
 * algorithm type, and may also be used in other cases.  Equivalence is
 * defined according to OID and (where relevant) parameters.
 *
 * <P>Subclasses may be used, for example when when the algorithm ID has
 * associated parameters which some code (e.g. code using public keys) needs
 * to have parsed.  Two examples of such algorithms are Diffie-Hellman key
 * exchange, and the Digital Signature Standard Algorithm (DSS/DSA).
 *
 * <P>The OID constants defined in this class correspond to some widely
 * used algorithms, for which conventional string names have been defined.
 * This class is not a general repository for OIDs, or for such string names.
 * Note that the mappings between algorithm IDs and algorithm names is
 * not one-to-one.
 *
 * @version 1.84
 *
 * @author David Brownell
 * @author Amit Kapoor
 * @author Hemma Prafullchandra
 */
public class AlgorithmId implements Serializable, DerEncoder {

    /** use serialVersionUID from JDK 1.1. for interoperability */
    private static final long serialVersionUID = 7205873507486557157L;

    private static boolean initOidTable = false;
    private static Hashtable oidTable;

    /**
     * The object identitifer being used for this algorithm.
     */
    private ObjectIdentifier	algid;

    // The (parsed) parameters
    private AlgorithmParameters algParams;
    private boolean constructedFromDer = true;

    /**
     * Parameters for this algorithm.  These are stored in unparsed
     * DER-encoded form; subclasses can be made to automaticaly parse
     * them so there is fast access to these parameters.
     */
    protected DerValue		params;


    /**
     * Returns one of the algorithm IDs most commonly associated
     * with this algorithm name.
     *
     * @param algname the name being used
     * @deprecated use the short get form of this method.
     * @exception NoSuchAlgorithmException on error.
     */
    public static AlgorithmId getAlgorithmId(String algname)
    throws NoSuchAlgorithmException
    {
	return get(algname);
    }

    public AlgorithmParameters getParameters() {
	return this.algParams;
    }

    /**
     * Returns one of the algorithm IDs most commonly associated
     * with this algorithm name.
     *
     * @param algname the name being used
     * @exception NoSuchAlgorithmException on error.
     */
    public static AlgorithmId get(String algname)
    throws NoSuchAlgorithmException
    {
	ObjectIdentifier oid = null;
	try {
	    oid = algOID(algname);
	} catch (IOException ioe) {
	    throw new NoSuchAlgorithmException
		("Invalid ObjectIdentifier " + algname);
	}

	if (oid == null)
	    throw new NoSuchAlgorithmException
		("unrecognized algorithm name: " + algname);

	return new AlgorithmId(oid);
    }

    /**
     * Returns one of the algorithm IDs most commonly associated
     * with this algorithm parameters.
     *
     * @param algparams the associated algorithm parameters.
     * @exception NoSuchAlgorithmException on error.
     */
    public static AlgorithmId get(AlgorithmParameters algparams)
    throws NoSuchAlgorithmException
    {
        ObjectIdentifier oid = null;
	String algname = algparams.getAlgorithm();
        try {
            oid = algOID(algname);
       } catch (IOException ioe) {
            throw new NoSuchAlgorithmException
                ("Invalid ObjectIdentifier " + algname);
        }
	if (oid == null) {
	    throw new NoSuchAlgorithmException
                ("unrecognized algorithm name: " + algname);
	}
        return new AlgorithmId(oid, algparams); 
    }
    /**
     * Parse (unmarshal) an ID from a DER sequence input value.  This form
     * parsing might be used when expanding a value which has already been
     * partially unmarshaled as a set or sequence member.
     *
     * @exception IOException on error.
     * @param val the input value, which contains the algid and, if
     *		there are any parameters, those parameters.
     * @return an ID for the algorithm.  If the system is configured
     * 		appropriately, this may be an instance of a class
     *		with some kind of special support for this algorithm.
     *		In that case, you may "narrow" the type of the ID.
     */
    public static AlgorithmId parse(DerValue val)
    throws IOException
    {
	if (val.tag != DerValue.tag_Sequence)
	    throw new IOException("algid parse error, not a sequence");

	/*
	 * Get the algorithm ID and any parameters.
	 */
	ObjectIdentifier	algid;
	DerValue		params;
	DerInputStream		in = val.toDerInputStream();

	algid = in.getOID();
	if (in.available() == 0)
	    params = null;
	else {
	    params = in.getDerValue();
	    if (params.tag == DerValue.tag_Null)
		params = null;
	}

	/*
	 * Figure out what class (if any) knows about this oid's
	 * parameters.  Make one, and give it the data to decode.
	 */
	AlgorithmId alg = new AlgorithmId(algid, params);
        if (params != null)
	    alg.decodeParams();
	return alg;
    }

    /**
     * Constructs a parameterless algorithm ID.
     *
     * @param oid the identifier for the algorithm
     */
    public AlgorithmId(ObjectIdentifier oid) {
	algid = oid;
    }


    /**
     * Constructs an algorithm ID with algorithm parameters.
     *
     * @param oid the identifier for the algorithm.
     * @param algparams the associated algorithm parameters.
     */
    public AlgorithmId(ObjectIdentifier oid,
	AlgorithmParameters algparams) {
        algid = oid;
	algParams = algparams;
	constructedFromDer = false;
    }             

    private AlgorithmId(ObjectIdentifier oid, DerValue params)
    throws IOException {
	this.algid = oid;
	this.params = params;
        if (this.params != null)
	    decodeParams();
    }


    /**
     * Constructs an algorithm ID which will be initialized
     * separately, for example by deserialization.
     * @deprecated use one of the other constructors.
     */
    public AlgorithmId() { }


    protected void decodeParams() throws IOException {
	try {
	    this.algParams = AlgorithmParameters.getInstance
		(this.algid.toString());
	} catch (NoSuchAlgorithmException e) {
	    /*
	     * This algorithm parameter type is not supported, so we cannot
	     * parse the parameters.
	     */
	    this.algParams = null;
	    return;
	}
	// Decode (parse) the parameters
	this.algParams.init(this.params.toByteArray());
    }

    /**
     * Marshal a DER-encoded "AlgorithmID" sequence on the DER stream.
     */
    public final void encode(DerOutputStream out)
    throws IOException
    {
	derEncode(out);
    }

    /**
     * DER encode this object onto an output stream.
     * Implements the <code>DerEncoder</code> interface.
     *
     * @param out
     * the output stream on which to write the DER encoding.
     *
     * @exception IOException on encoding error.
     */
    public void derEncode (OutputStream out) throws IOException
    {
	DerOutputStream	bytes = new DerOutputStream();
	DerOutputStream	tmp = new DerOutputStream();

	bytes.putOID(algid);
	// Setup params from algParams since no DER encoding is given
	if (constructedFromDer == false) {
	    if (algParams != null) {
		params = new DerValue(algParams.getEncoded());
	    } else {
		params = null;
	    }
	}
	if (params == null)
	    bytes.putNull();
	else
	    bytes.putDerValue(params);
	tmp.write(DerValue.tag_Sequence, bytes);
	out.write(tmp.toByteArray());
    }


// TODO: cleaning required
    /**
     * Returns the DER-encoded X.509 AlgorithmId as a byte array.
     */
    public final byte[] encode() throws IOException
    {
	DerOutputStream out = new DerOutputStream ();
	DerOutputStream bytes = new DerOutputStream ();

	bytes.putOID(algid);
	if (params == null)
	    bytes.putNull();
	else
	    bytes.putDerValue(params);
	out.write(DerValue.tag_Sequence, bytes);
	return out.toByteArray();
    }

    /*
     * Translates from some common algorithm names to the
     * OID with which they're usually associated ... this mapping
     * is the reverse of the one below, except in those cases
     * where synonyms are supported or where a given algorithm
     * is commonly associated with multiple OIDs.
     *
     * TODO: This method needs to be enhanced so that we can also pass the
     * scope of the algorithm name to it, e.g., the algorithm name "DSA"
     * may have a different OID when used as a "Signature" algorithm than when
     * used as a "KeyPairGenerator" algorithm.
     */
    private static ObjectIdentifier algOID (String name) 
	throws IOException
    {
	// See if algname is in printable OID ("dot-dot") notation
	if (name.indexOf('.') != -1) {
	    if (name.startsWith("OID."))
		return new ObjectIdentifier(name.substring("OID.".length()));
	    else
		return new ObjectIdentifier(name);
	}

	// Digesting algorithms
	if (name.equalsIgnoreCase("MD5"))
	    return AlgorithmId.MD5_oid;
	if (name.equalsIgnoreCase("MD2"))
	    return AlgorithmId.MD2_oid;
	if (name.equalsIgnoreCase("SHA") || name.equalsIgnoreCase("SHA1")
	    || name.equalsIgnoreCase("SHA-1"))
	    return AlgorithmId.SHA_oid;

	// Various public key algorithms
	if (name.equalsIgnoreCase("RSA"))
	    return AlgorithmId.RSAEncryption_oid;
	if (name.equalsIgnoreCase("Diffie-Hellman")
	    || name.equalsIgnoreCase("DH"))
	     return AlgorithmId.DH_oid;
	if (name.equalsIgnoreCase("DSA"))
	    return AlgorithmId.DSA_oid;

	// Common signature types
	if (name.equalsIgnoreCase("MD5withRSA")
	    || name.equalsIgnoreCase("MD5/RSA"))
	    return AlgorithmId.md5WithRSAEncryption_oid;
	if (name.equalsIgnoreCase("MD2withRSA")
	    || name.equalsIgnoreCase("MD2/RSA"))
	    return AlgorithmId.md2WithRSAEncryption_oid;
	if (name.equalsIgnoreCase("SHAwithDSA")
	    || name.equalsIgnoreCase("SHA1withDSA")
            || name.equalsIgnoreCase("SHA/DSA")
	    || name.equalsIgnoreCase("SHA1/DSA")
	    || name.equalsIgnoreCase("DSAWithSHA1"))
	    return AlgorithmId.sha1WithDSA_oid;
	if (name.equalsIgnoreCase("SHA1WithRSA")
	    || name.equalsIgnoreCase("SHA1/RSA"))
	    return AlgorithmId.sha1WithRSAEncryption_oid;

	// See if any of the installed providers supply a mapping from
	// the given algorithm name to an OID string
	String oidString;
	if (!initOidTable) {
	    Provider[] provs = Security.getProviders();
	    for (int i=0; i<provs.length; i++) {
		for (Enumeration enum_ = provs[i].keys();
		     enum_.hasMoreElements(); ) {
		    String alias = (String)enum_.nextElement();
		    int index;
		    if (alias.toUpperCase().startsWith("ALG.ALIAS") &&
			(index=alias.toUpperCase().indexOf("OID.", 0)) != -1) {
			index += "OID.".length();
			if (index == alias.length()) {
			    // invalid alias entry
			    break;
			}
			if (oidTable == null)
			    oidTable = new Hashtable(11);
			oidString = alias.substring(index);
			String stdAlgName
			    = provs[i].getProperty(alias).toUpperCase();
			if (oidTable.get(stdAlgName) == null)
			    oidTable.put(stdAlgName,
					 new ObjectIdentifier(oidString));
		    }
		}
	    }
	    initOidTable = true;
	}

	return ((ObjectIdentifier)oidTable.get(name.toUpperCase()));
    }

    /*
     * For the inevitable cases where key or signature types are not
     * configured in an environment which encounters such keys or
     * signatures, we still attempt to provide user-friendly names
     * for some of the most common algorithms.  Subclasses can of
     * course override getName().
     *
     * Wherever possible, the names are those defined by the IETF.
     * Such names are noted below.
     */
    private String algName()
    {
	// Common message digest algorithms

	if (algid.equals(AlgorithmId.MD5_oid))
	    return "MD5";	// RFC 1423
	if (algid.equals(AlgorithmId.MD2_oid))
	    return "MD2";	// RFC 1423
	if (algid.equals(AlgorithmId.SHA_oid))
	    return "SHA";

	// Common key types

	if (algid.equals(AlgorithmId.RSAEncryption_oid)
		|| algid.equals(AlgorithmId.RSA_oid))
	    return "RSA";
	if (algid.equals(AlgorithmId.DH_oid)
                || algid.equals(AlgorithmId.DH_PKIX_oid))
	    return "Diffie-Hellman";
	if (algid.equals(AlgorithmId.DSA_oid)
                || algid.equals(AlgorithmId.DSA_OIW_oid))
	    return "DSA";

	// Common signature types

	if (algid.equals (AlgorithmId.md5WithRSAEncryption_oid))
	    return "MD5withRSA";
	if (algid.equals (AlgorithmId.md2WithRSAEncryption_oid))
	    return "MD2withRSA";
	if (algid.equals(AlgorithmId.sha1WithDSA_oid)
                || algid.equals(AlgorithmId.sha1WithDSA_OIW_oid)
                || algid.equals(AlgorithmId.shaWithDSA_OIW_oid))
	    return "SHA1withDSA";
	if (algid.equals (AlgorithmId.sha1WithRSAEncryption_oid)
	    || algid.equals(AlgorithmId.sha1WithRSAEncryption_OIW_oid))
	    return "SHA1withRSA";

	// default returns a dot-notation ID
	return algid.toString ();
    }

    /**
     * Returns the ISO OID for this algorithm.  This is usually converted
     * to a string and used as part of an algorithm name, for example
     * "OID.1.3.14.3.2.13" style notation.  Use the <code>getName</code>
     * call when you do not need to ensure cross-system portability
     * of algorithm names, or need a user friendly name.
     */
    final public ObjectIdentifier	getOID ()
	{ return algid; }


    /**
     * Returns a name for the algorithm which may be more intelligible
     * to humans than the algorithm's OID, but which won't necessarily
     * be comprehensible on other systems.  For example, this might
     * return a name such as "MD5withRSA" for a signature algorithm on
     * some systems.  It also returns names like "OID.1.2.3.4", when
     * no particular name for the algorithm is known.
     */
    public String getName()
	{ return algName (); }

    /**
     * Returns a string describing the algorithm and its parameters.
     */
    public String toString()
    {
	return (algName() + paramsToString());
    }

    /**
     * Returns the DER encoded parameter, which can then be
     * used to initialize java.security.AlgorithmParamters.
     *
     * @return DER encoded parameters, or null not present.
     */
    public byte[] getEncodedParams() throws IOException {
        if (params == null)
            return null;
        else
            return params.toByteArray();
    }

    /**
     * Provides a human-readable description of the algorithm parameters.
     * This may be redefined by subclasses which parse those parameters.
     */
    protected String paramsToString()
    {
	if (params == null) {
	    return "";
	} else if (algParams != null) {
	    return algParams.toString();
	} else {
	    return ", params unparsed";
	}
    }


    /**
     * Returns true iff the argument indicates the same algorithm
     * with the same parameters.
     */
    public boolean equals(AlgorithmId other)
    {
        if (!algid.equals (other.algid))
            return false;
	else if (params == null && other.params == null)
	    return true;
	else if (params == null)
	    return false;
	else
	    return params.equals(other.params);
    }


    /**
     * Compares this AlgorithmID to another.  If algorithm parameters are
     * available, they are compared.  Otherwise, just the object IDs
     * for the algorithm are compared.
     *
     * @param other preferably an AlgorithmId, else an ObjectIdentifier
     */
    public boolean equals(Object other)
    {
	if (other instanceof AlgorithmId)
	    return equals((AlgorithmId) other);
	else if (other instanceof ObjectIdentifier)
	    return equals((ObjectIdentifier) other);
	else
	    return false;
    }


    /**
     * Compares two algorithm IDs for equality.  Returns true iff
     * they are the same algorithm, ignoring algorithm parameters.
     */
    public final boolean equals(ObjectIdentifier id)
    {
	return algid.equals(id);
    }


    /**
     * Returns a hashcode for this AlgorithmId.
     *
     * @return a hashcode for this AlgorithmId.
     */
    public int hashCode() {
	StringBuffer sbuf = new StringBuffer();
	sbuf.append(algid.toString());
	sbuf.append(paramsToString());
	return sbuf.toString().hashCode();
    }

    /*****************************************************************/

    /*
     * HASHING ALGORITHMS
     */
    private static final int MD2_data[] = { 1, 2, 840, 113549, 2, 2 };
    private static final int MD5_data[] = { 1, 2, 840, 113549, 2, 5 };
    // sha = { 1, 3, 14, 3, 2, 18 };
    private static final int SHA1_OIW_data[] = { 1, 3, 14, 3, 2, 26 };

    public static ObjectIdentifier MD2_oid;
    public static ObjectIdentifier MD5_oid;
    public static ObjectIdentifier SHA5_oid;
    public static ObjectIdentifier SHA_oid;
    public static ObjectIdentifier SHA1_OIW_oid;
    public static ObjectIdentifier DH_oid;

    /*
     * COMMON PUBLIC KEY TYPES
     */
    private static final int DH_data[] = { 1, 2, 840, 113549, 1, 3, 1 };
    private static final int DH_PKIX_data[] = { 1, 2, 840, 10046, 2, 1 };
    private static final int DSA_OIW_data[] = { 1, 3, 14, 3, 2, 12 };
    private static final int DSA_PKIX_data[] = { 1, 2, 840, 10040, 4, 1 };
    private static final int RSA_data[] = { 1, 2, 5, 8, 1, 1 };
    private static final int RSAEncryption_data[] = 
                                 { 1, 2, 840, 113549, 1, 1, 1 };

    public static ObjectIdentifier DH_PKIX_oid;
    public static ObjectIdentifier DSA_oid;;
    public static ObjectIdentifier DSA_OIW_oid;;
    public static ObjectIdentifier RSA_oid;;
    public static ObjectIdentifier RSAEncryption_oid;

    /*
     * COMMON SIGNATURE ALGORITHMS
     */
    private static final int md2WithRSAEncryption_data[] =
                                       { 1, 2, 840, 113549, 1, 1, 2 };
    private static final int md5WithRSAEncryption_data[] =
	                               { 1, 2, 840, 113549, 1, 1, 4 };
    private static final int sha1WithRSAEncryption_data[] =
	                               { 1, 2, 840, 113549, 1, 1, 5 };
    private static final int sha1WithRSAEncryption_OIW_data[] =
	                               { 1, 3, 14, 3, 2, 29 };
    private static final int shaWithDSA_OIW_data[] =
	                               { 1, 3, 14, 3, 2, 13 };
    private static final int sha1WithDSA_OIW_data[] =
	                               { 1, 3, 14, 3, 2, 27 };
    private static final int dsaWithSHA1_PKIX_data[] =
	                               { 1, 2, 840, 10040, 4, 3 };

    public static ObjectIdentifier md2WithRSAEncryption_oid;
    public static ObjectIdentifier md5WithRSAEncryption_oid;
    public static ObjectIdentifier sha1WithRSAEncryption_oid;
    public static ObjectIdentifier sha1WithRSAEncryption_OIW_oid;
    public static ObjectIdentifier shaWithDSA_OIW_oid;
    public static ObjectIdentifier sha1WithDSA_OIW_oid;
    public static ObjectIdentifier sha1WithDSA_oid;
 

    static { 
        try {
    /**
     * Algorithm ID for the MD2 Message Digest Algorthm, from RFC 1319.
     * OID = 1.2.840.113549.2.2
     */
	    MD2_oid = new ObjectIdentifier(MD2_data);

    /**
     * Algorithm ID for the MD5 Message Digest Algorthm, from RFC 1321.
     * OID = 1.2.840.113549.2.5
     */
	    MD5_oid = new ObjectIdentifier(MD5_data);

    /**
     * Algorithm ID for the SHA1 Message Digest Algorithm, from FIPS 180-1.
     * This is sometimes called "SHA", though that is often confusing since
     * many people refer to FIPS 180 (which has an error) as defining SHA.
     * OID = 1.3.14.3.2.26
     */
	    SHA_oid = new ObjectIdentifier(SHA1_OIW_data);


    /*
     * Note the preferred OIDs are named simply with no "OIW" or
     * "PKIX" in them, even though they may point to data from these
     * specs; e.g. SHA_oid, DH_oid, DSA_oid, SHA1WithDSA_oid...
     */
    /**
     * Algorithm ID for Diffie Hellman Key agreement, from PKCS #3.
     * Parameters include public values P and G, and may optionally specify
     * the length of the private key X.  Alternatively, algorithm parameters
     * may be derived from another source such as a Certificate Authority's
     * certificate.
     * OID = 1.2.840.113549.1.3.1
     */
	    DH_oid = new ObjectIdentifier(DH_data);

    /**
     * Algorithm ID for the Diffie Hellman Key Agreement (DH), from 
     * RFC 2459.
     * Parameters may include public values P and G.
     * OID = 1.2.840.10046.2.1
     */
	    DH_PKIX_oid = new ObjectIdentifier(DH_PKIX_data);

    /**
     * Algorithm ID for the Digital Signing Algorithm (DSA), from the
     * NIST OIW Stable Agreements part 12.
     * Parameters may include public values P, Q, and G; or these may be
     * derived from
     * another source such as a Certificate Authority's certificate.
     * OID = 1.3.14.3.2.12
     */
	    DSA_OIW_oid = new ObjectIdentifier(DSA_OIW_data);

    /**
     * Algorithm ID for the Digital Signing Algorithm (DSA), from
     * RFC 2459.
     * Parameters may include public values P, Q, and G; or these may be
     * derived from
     * another source such as a Certificate Authority's certificate.
     * OID = 1.2.840.10040.4.1
     */
	    DSA_oid = new ObjectIdentifier(DSA_PKIX_data);

    /**
     * Algorithm ID for RSA keys used for any purpose, as defined in X.509.
     * The algorithm parameter is a single value, the number of bits in the
     * public modulus.
     * OID = 1.2.5.8.1.1
     */
	    RSA_oid = new ObjectIdentifier(RSA_data);

    /**
     * Algorithm ID for RSA keys used with RSA encryption, as defined
     * in PKCS #1.  There are no parameters associated with this algorithm.
     * OID = 1.2.840.113549.1.1.1
     */
	    RSAEncryption_oid = new ObjectIdentifier(RSAEncryption_data);

    /**
     * Identifies a signing algorithm where an MD2 digest is encrypted
     * using an RSA private key; defined in PKCS #1.  Use of this
     * signing algorithm is discouraged due to MD2 vulnerabilities.
     * OID = 1.2.840.113549.1.1.2
     */
	    md2WithRSAEncryption_oid = new
		ObjectIdentifier(md2WithRSAEncryption_data);

    /**
     * Identifies a signing algorithm where an MD5 digest is
     * encrypted using an RSA private key; defined in PKCS #1.
     * OID = 1.2.840.113549.1.1.4
     */
	    md5WithRSAEncryption_oid = new
		ObjectIdentifier(md5WithRSAEncryption_data);

    /**
     * Identifies a signing algorithm where a SHA1 digest is
     * encrypted using an RSA private key; defined by RSA DSI.
     * OID = 1.2.840.113549.1.1.5
     */
	    sha1WithRSAEncryption_oid = new
		ObjectIdentifier(sha1WithRSAEncryption_data);

    /**
     * Identifies a signing algorithm where a SHA1 digest is
     * encrypted using an RSA private key; defined in NIST OIW.
     * OID = 1.3.14.3.2.29
     */
	    sha1WithRSAEncryption_OIW_oid = new
		ObjectIdentifier(sha1WithRSAEncryption_OIW_data);
	    
    /**
     * Identifies the FIPS 186 "Digital Signature Standard" (DSS), where a
     * SHA digest is signed using the Digital Signing Algorithm (DSA).
     * This should not be used.
     * OID = 1.3.14.3.2.13
     */
	    shaWithDSA_OIW_oid = new ObjectIdentifier(shaWithDSA_OIW_data);

    /**
     * Identifies the FIPS 186 "Digital Signature Standard" (DSS), where a
     * SHA1 digest is signed using the Digital Signing Algorithm (DSA).
     * OID = 1.3.14.3.2.27
     */
	    sha1WithDSA_OIW_oid = new ObjectIdentifier(sha1WithDSA_OIW_data);

    /**
     * Identifies the FIPS 186 "Digital Signature Standard" (DSS), where a
     * SHA1 digest is signed using the Digital Signing Algorithm (DSA).
     * OID = 1.2.840.10040.4.3
     */
	    sha1WithDSA_oid = new ObjectIdentifier(dsaWithSHA1_PKIX_data);
	} catch (IOException ioe) {
	    // should not happen
	}
    }
}

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


/*
 * Web proxy  - The URL retrieval mechanism in AppDescriptor does not
 *              work through a proxy server at this time.
 */

import java.io.*;
import java.util.*;
import java.net.URL;
import java.net.MalformedURLException;

import java.security.KeyStore;
import java.security.MessageDigest;
import java.security.DigestInputStream;
import java.security.NoSuchAlgorithmException;
import java.security.KeyStoreException;
import java.security.Key;
import java.security.PrivateKey;
import java.security.Signature;

import java.security.SignatureException;
import java.security.InvalidKeyException;
import java.security.NoSuchProviderException;
import java.security.UnrecoverableKeyException;

import java.security.cert.Certificate;
import java.security.cert.X509Certificate;
import java.security.cert.CertificateFactory;

import java.security.cert.CertificateException;
import java.security.cert.CertificateEncodingException;
import java.security.cert.CertificateExpiredException;
import java.security.cert.CertificateNotYetValidException;

import com.sun.midp.jadtool.AppDescriptorException;

import com.sun.midp.installer.*;

/**
 * Java API for signing MIDletSuites.
 * <p>
 * AppDescriptor is an extension of the Properties class which provides
 * additional methods for adding message digest and certificate 
 * properties as well as signing the app descriptor file and verifying
 * a signed app descriptor file.
 */
public class AppDescriptor extends JadProperties {
    
    /** Index of the key in arrays returned by getAllCerts. */
    public static int KEY = 0;

    /** Index of the cert in arrays returned by getAllCerts. */
    public static int CERT = 1;

    /** App Descriptor key for . */
    public static final String JAR_SIGNATURE = "MIDlet-Jar-RSA-SHA1";
    /** App Descriptor key for . */
    public static final String CP_ATTR = "MIDlet-Certificate-";
    /** App Descriptor key for . */
    public static final String JAR_URL = "MIDlet-Jar-URL";
    /** App Descriptor key for . */
    public static final String JAR_SIZE = "MIDlet-Jar-Size";
    /** App Descriptor key for . */
    public static final String SEP_ATTR = ": ";    

    /** SHA1 with RSA constant. */
    public static final String SIGN_ALG = "SHA1withRSA";

    /** KeyStore to get certificates and keys from. */
    private KeyStore keystore = null;

    /**
     * Default constructor
     */
    public AppDescriptor() {
        super();
    }

    /**
     * Used to input a stored app descriptor into an AppDescriptor
     * instance from a stream.  The input stream will be converted 
     * to Unicode using <code>encoding</code> if it is specified.
     * If <code>encoding</code> is not specified, a default 
     * encoding of type "UTF8" escapes will be used.
     *
     * Overrides <code>Properties.load</code>
     *
     * @param inputJad App descriptor input stream.
     * @param encoding Encoding of the inputJad stream.
     *
     * @exception IOException If an error occurs while loading the 
     *            inputJad stream.
     * @exception UnsupportedEncodingException If the given encoding
     *            type is not supported.
     * @exception InvalidJadException If the JAD has a format error
     */
    public synchronized void load(InputStream inputJad, String encoding)
        throws UnsupportedEncodingException, IOException, InvalidJadException {
            super.load(inputJad, encoding);
    }

    /**
     * Used to store an app descriptor instance into a jad file 
     * through an output stream.  The internal Unicode stream 
     * will be converted to an output format using
     * <code>encoding</code> if it is specified.
     * If <code>encoding</code> is not specified, a default 
     * encoding of type Ascii with Unicode escapes will be used.
     *
     * Overrides <code>Properties.store</code>
     *
     * @param outputJad App descriptor output stream.
     * @param encoding Encoding of the outputJad stream.
     *
     * @exception IOException If an error occurs while writing the 
     *            inputJad stream.
     * @exception UnsupportedEncodingException If the given encoding
     *            type is not supported.
     */
    public synchronized void store(OutputStream outputJad, String encoding)
            throws UnsupportedEncodingException, IOException {
   
        if (encoding != null) {
            JadWriter.write(this, outputJad, encoding);
        } else {
            JadWriter.write(this, outputJad);
        }
    }

    /**
     * Provides a KeyStore instance for use by this AppDescriptor
     * object.  (The default KeyStore type from the Java security
     * properties file is used.  This should be type "JKS", and is
     * the only supported keystore format.)
     * <p>
     * This KeyStore is required by the <code>addcert</code>,
     * <code>sign</code>, <code>signcert</code>, and <code>verify</code> 
     * methods.  If any of these methods is called before 
     * <code>loadKeyStore</code> an exception will be thrown. 
     *
     * @param ksfile The input stream stream to load a KeyStore from.
     * @param storepass The password to unlock the KeyStore, can be null.
     * @exception KeyStoreException The default keystore provider type
     *            is not available in any of the provider packages 
     *            searched.  
     * @exception IOException Thrown if there is a problem parsing 
     *            the input stream or loading its data.
     * @exception CertificateException Thrown if there is trouble loading
     *            certificates into the KeyStore
     * @exception NoSuchAlgorithmException Thrown if the algorithm
     *            needed to verify the KeyStore cannot be found.
     */
    public synchronized void loadKeyStore(InputStream ksfile,
                                          char[] storepass)
        throws KeyStoreException, IOException, 
               NoSuchAlgorithmException, CertificateException, Exception {
        try {
            keystore = KeyStore.getInstance(KeyStore.getDefaultType());
            keystore.load(ksfile, storepass);
        } catch (Exception e) {
            throw new Exception("loadKeyStore failed");
        }
    }

    /**
     * Store a the keystore in the descriptor in a file.
     *
     * @param ksfname file to use
     * @param storepass password of keystore
     */
    public synchronized void storeKeyStore(String ksfname,
                                           char[] storepass)
        throws IOException, KeyStoreException, 
        NoSuchAlgorithmException, CertificateException {

        FileOutputStream fout = new FileOutputStream(ksfname);
        keystore.store(fout, storepass);
        fout.close();
    }

    /**
     * Adds a Base64 encoded signature of the jar file at the URL 
     * specified by the MIDlet-Jar-URL key in the app descriptor.
     * <code>load</code> and <code>loadKeyStore</code> must be
     * call before this method.
     * <p>
     * The line in the app descriptor corresponding to this
     * insertion will look like this:
     * <p>
     *   MIDlet-Jar-RSA-SHA1:j3zKCv6eud2Ubkw80XjpNb7tk5s...
     *
     * If a MIDlet-Jar-RSA-SHA1 property already exists it will
     * be replaced.
     *
     * @param alias  Alias of the signing key in the keystore.
     * @param keypass Password to access the signing (private) key.
     *
     * @exception AppDescriptorException JAR URL or content provider
     *            certificate was
     *            not found in the app descriptor.
     * @exception MalformedURLException The URL corresponding to 
     *            the MIDlet-Jar-URL key could not be parsed.
     * @exception IOException error reading the JAR
     * @exception NoSuchAlgorithmException If SHA1 or RSA need by
     *            getEncodedSig could not be found in an
     *            installed JCA provider.    
     * @exception KeyStoreException
     * @exception InvalidKeyException
     * @exception SignatureException
     * @exception UnrecoverableKeyException
     */
    public void addJarSignature(String alias, char[] keypass)
        throws AppDescriptorException, MalformedURLException, IOException, 
               KeyStoreException, InvalidKeyException, 
               SignatureException, NoSuchAlgorithmException, 
               UnrecoverableKeyException {

        String urlStr = getProperty(JAR_URL);

        if (urlStr == null) {
            throw new AppDescriptorException(JAR_URL + " not in descriptor");
        }

        URL url = new URL(urlStr);
        InputStream jarStream = url.openStream();
        addJarSignature(alias, keypass, jarStream);
    }
    
    /**
     * Adds a Base64 encoded signature of the jar file provided in
     * an input stream to the app descriptor.
     * <p>
     * The line in the app descriptor corresponding to this
     * insertion will look like this:
     *
     *   MIDlet-Jar-RSA-SHA1:j3zKCv6eud2Ubkw80XjpNb7tk5s...
     *
     * If a MIDlet-Jar-RSA-SHA1 property already exists it will
     * be replaced.
     *
     * @param alias  Alias of the signing key in the keystore.
     * @param keypass Password to access the signing (private) key.
     * @param jarStream stream to read the jar file from
     *
     * @exception IOException If there is a problem reading the 
     *            input stream.
     * @exception NoSuchAlgorithmException If SHA1 or RSA need by
     *            getEncodedSig could not be found in an
     *            installed JCA provider.    
     * @exception KeyStoreException
     * @exception InvalidKeyException
     * @exception SignatureException
     * @exception UnrecoverableKeyException
     */
    public void addJarSignature(String alias, char[] keypass,
            InputStream jarStream) throws IOException,
        NoSuchAlgorithmException, 
        KeyStoreException, InvalidKeyException, SignatureException,
        NoSuchAlgorithmException, UnrecoverableKeyException {

        setProperty(JAR_SIGNATURE, getEncodedSig(alias, keypass, jarStream));
    }

    /**
     * Retrieves a certificate out of a KeyStore and adds it 
     * to the app descriptor as:
     * <p>
     *   content_provider.certificate-1-1:<Base64 encoded newcert>
     * <p>
     * Instance variable <code>keystore</code> must not be null, 
     * and should have been set by <code>loadKeyStore</code>
     * before this method is called.
     *
     * @param alias Alias of the chosen certificate in the keystore
     * @param chainNum number of the chain to add certificate to
     * @param certNum number of the certificate in the chain to replace it,
     *        or 0 to add the certificate at the end of the chain
     *
     * @exception KeyStoreException If there is an error with the keystore.
     * @exception CertificateException If there is a problem with the
     *            encoding of the certificate.
     * @exception AppDescriptorException If the KeyStore has not been 
     *            initialized (keystore is null)
     */    
    public void addCert(String alias, int chainNum, int certNum)
    throws CertificateException, KeyStoreException, AppDescriptorException {
	Certificate[] chain = getCertificatesFromKeyStore(alias);
	if (certNum == 0) {
	    // find next number after the highest existing certificate number
	    for (certNum = 1;
		 getProperty(CP_ATTR + chainNum + "-" + certNum) != null;
		 certNum++);

	    for (int i = 0; i < chain.length; i++) {
		addEncodedCertificate(chain[i], chainNum, certNum + i);
	    }
	} else {
	    addEncodedCertificate(chain[certNum-1], chainNum, certNum);
	}
    }

    /**
     * Returns an X509Certificate object from the app descriptor property
     * chosen by <code>certnum</code>, or 
     * null if that certificate does not exist in the descriptor.
     * <p>
     * After finding the chosen property in the app descriptor, 
     * decodes it from Base64 into a byte-encoded certificate and then
     * creates the X509 format certificate from that opaque data.
     *
     * @param chainNum number of the certificate chain
     * @param certNum number of the certificate in the chain
     *
     * @return an X509Certificate object or null if the certificate is
     * not in the JAD
     *
     * @exception CertificateException If there is a format problem with
     *            the certificate
     */
    public X509Certificate getCert(int chainNum, int certNum)
        throws CertificateException {

        X509Certificate c = null;
        
        String base64 = getProperty(CP_ATTR + chainNum + "-" + certNum);
        if (base64 != null) {
            c = base64CertToX509Cert(base64);
        } 

        return c;
    }
  
    /**
     * Returns an X509Certificate object from the app descriptor property
     * chosen by <code>certnum</code>, or 
     * null if that certificate does not exist in the descriptor.
     * <p>
     * After finding the chosen property in the app descriptor, 
     * decodes it from Base64 into a byte-encoded certificate and then
     * creates the X509 format certificate from that opaque data.
     *
     * @param chainNum number of the certificate chain
     * @param certNum number of the certificate in the chain
     *
     * @return an X509Certificate object or null if the certificate is
     * not in the JAD
     *
     * @exception CertificateException If there is a format problem with
     *            the certificate
     */
    public X509Certificate getCertAttribute(int chainNum, int certNum)
        throws CertificateException {

        X509Certificate c = null;
        
        String base64 = getProperty(CP_ATTR + chainNum + "-" + certNum);
        if (base64 != null) {
            c = base64CertToX509Cert(base64);
        } 

        return c;
    }
  
    /**
     * Returns all X509Certificate objects from the app descriptor.
     * <p>
     * After finding a certificate property in the app descriptor, 
     * decodes it from Base64 into a byte-encoded certificate and then
     * creates the X509 format certificate from that opaque data.
     *
     * @return Vector of object arrays, each containing key, and a
     * X509Certificate object
     */
    public Vector getAllCerts() throws CertificateException {
        Vector certs = new Vector();

	for (int idx = 0; idx < size(); idx++) {
            String key = getKeyAt(idx);
	    String base64 = getValueAt(idx);

            if (key.startsWith(CP_ATTR)) {
                X509Certificate c = base64CertToX509Cert(base64);
                Object[] temp = new Object[2];

                temp[KEY] = key;
                temp[CERT] = c;
                certs.addElement(temp);
            }
        }

        return certs;
    }
  
    /**
     * Returns a message digest of a certificate in "human readable" 
     * from from the app descriptor property
     * chosen by <code>certnum</code>, or 
     * null if that certificate does not exist in the descriptor.
     * <p>
     * After finding the chosen property in the app descriptor, 
     * decodes it from Base64 into a byte-encoded certificate and then
     * creates a readable digest String based on that data.
     *
     * @param chainNum number of the certificate chain
     * @param certNum number of the certificate in the chain
     * @param alg A Digest algorithm to use, e.g. "SHA1" or "MD5".
     *
     * @return A message digest of a certificate in hex as a String or
     *    null if the certificate is not in the JAD.
     *
     * @exception NoSuchAlgorithmException Thrown if the digest
     *            <code>algorithm</code> could not be found.
     */
    public String getCertDigest(int chainNum, int certNum, String alg)
        throws NoSuchAlgorithmException {

        String digest = null;
        
        String base64 = getProperty(CP_ATTR + chainNum + "-" + certNum);
        if (base64 != null) {
            byte[] certificateData = Base64.decode(base64);
            digest = createFingerprint(certificateData, alg);
        }

        return digest;      
    }

    /* ************ PRIVATE METHODS ************* */

    /**
     * Retrieves a certificate chain out of a KeyStore.
     * Instance variable <code>keystore</code> must not be null,
     * and should have been set by <code>loadKeyStore</code>
     * before this method is called.
     *
     * @param alias Alias of the chosen certificate in the keystore
     * @return all certificates in the chain
     * @exception KeyStoreException If there is an error with the keystore.
     * @exception CertificateException If there is a problem with the
     *            encoding of the certificate.
     * @exception AppDescriptorException If the KeyStore has not been 
     *            initialized (keystore is null);
     */    
    private Certificate[] getCertificatesFromKeyStore(String alias)
    throws AppDescriptorException, CertificateException, KeyStoreException {

	if (keystore == null) {
	    throw new AppDescriptorException(
	    AppDescriptorException.KEYSTORE_NOT_INITIALIZED);
	}

	// Return a certifcate chain if we can get it.
	Certificate[] chain = keystore.getCertificateChain(alias);
	if (chain == null) {
	    // Otherwise try for a certificate.
	    Certificate cert = keystore.getCertificate(alias);
	    if (cert == null) {
		throw new CertificateException("Certificate not found");
	    }
	    chain = new Certificate[] { cert };
	}

	return chain;
    }

    /**
     * Add a certificate to the app descriptor as:
     * <p>
     *   content_provider.certificate-1-1:<Base64 encoded newcert>
     * <p>
     * Instance variable <code>keystore</code> must not be null, 
     * and should have been set by <code>loadKeyStore</code>
     * before this method is called.
     *
     * @param cert the certificate to add
     * @param chainNum number of the chain to add certificate to
     * @param certNum number of the certificate in the chain to replace it,
     *        or 0 to add the certificate at the end of the chain
     *
     * @exception CertificateEncodingException If there is a problem with the
     *            encoding of the certificate.
     */    
    private void addEncodedCertificate(Certificate cert,
				       int chainNum, int certNum)
    throws CertificateEncodingException {

	// encode the (x.509?) cert in base64
	byte[] certbytes = cert.getEncoded();
	String certtoadd = Base64.encode(certbytes);
	// replace any existing certificate
	setProperty(CP_ATTR + chainNum + "-" + certNum, certtoadd);
    }

    /**
     * <code>getEncodedCertificate</code> - A helper function used
     * by <code>addCert</code>.
     *
     * Retrieves a certificate out of a KeyStore and returns it 
     * as a Base64 encoded String.  Instance variable <code>keystore</code>
     * must not be null, and should have been set by <code>loadKeyStore</code>
     * before this method is called.
     *
     * @param alias Alias of the chosen certificate in the keystore
     * @return Base64 encoded certificate as a String
     * @exception KeyStoreException If there is an error with the keystore.
     * @exception CertificateException If there is a problem with the
     *            encoding of the certificate.
     * @exception AppDescriptorException If the KeyStore has not been 
     *            initialized (keystore is null);
     */    
    private String getEncodedCertificate(String alias) 
        throws KeyStoreException, CertificateException, 
               AppDescriptorException {

        Certificate cert;

        if (keystore == null) {
            throw new AppDescriptorException(
                AppDescriptorException.KEYSTORE_NOT_INITIALIZED);
        }

        // Load a keystore data structure to get keys from      
        cert = keystore.getCertificate(alias);
        if (cert == null) {
            throw new CertificateException("Certificate not found");
        }

        byte[] certbytes = cert.getEncoded();

        // return the (x.509?) encoded cert in base64
        return Base64.encode(certbytes);
    }

    /**
     * <code>getNextCertIndex</code> - A helper function used by
     * <code>addcert</code>.
     *
     * Iterates through the current properties data returns the 
     * index of the first unused key index for either a content-provider
     * (cp) or https certificate. 
     *
     * @param key What cert key should be used...CP_ATTR or HTTPS_ATTR
     * @return The first unused index for a particular certificate type.
     */
    private int getNextCertIndex(String key) {
        int idx = 1;

        while (idx > 0) {
            String value = getProperty(key + idx);
            if (value == null) {
                break;
            }

            idx++;
        }

        return idx;
    }

    /** 
     * <code>createFingerprint</code> - A helper function used by
     * <code>getdigest</code>.
     *
     * <code>createFingerprint</code>, given a certificated encoded 
     * as a byte array will compute a "fingerprint", or Message 
     * Digest of the certificate using the selected <code>algorithm</code>
     * type.
     *
     * A fingerprint is meant to be human readable, and is thus
     * returned as a hex string separated at byte boundaries by
     * a delimiter ":".
     *
     * @param certificateBytes - a certificate encoded as a byte array
     * @param algorithm - The name of a digest algorithm to use, 
     *                    e.g. "SHA1" or "MD5"
     * @return the fingerprint in String form.
     * @exception NoSuchAlgorithmException Thrown if the digest
     *            <code>algorithm</code> could not be found.
     */
    static String createFingerprint(byte[] certificateBytes,
                                     String algorithm) 
            throws NoSuchAlgorithmException {
        MessageDigest md = MessageDigest.getInstance(algorithm);
        md.update(certificateBytes);
        byte[] digest = md.digest();
        StringBuffer sb = new StringBuffer();
        
        for (int i = 0; i < digest.length; i++) {
            int b = digest[i] & 0xff;
            String hex = Integer.toHexString(b);

            if (i != 0) {
                sb.append(":");
            }

            if (hex.length() == 1) {
                sb.append("0");
            }

            sb.append(hex);
        }

        return sb.toString();
    }

    /**
     * A helper function used by <code>sign</code>.
     * Produces a base64 encoded signature for the given buffer.
     *
     * @param alias  Alias of the signing key in the keystore.
     * @param keypass Password to access the signing (private) key.
     * @param stream stream to read the bytes from
     *
     * @return Base64 encoded signature of bits in the buffer
     *
     * @exception IOException If there is a problem reading the 
     *            input stream.
     * @exception KeyStoreException
     * @exception InvalidKeyException
     * @exception SignatureException
     * @exception NoSuchAlgorithmException
     * @exception UnrecoverableKeyException
     */
    private String getEncodedSig(String alias, char[] keypass,
            InputStream stream) throws KeyStoreException, InvalidKeyException, 
            SignatureException, NoSuchAlgorithmException, 
            UnrecoverableKeyException, IOException {
        int bytesRead;
        byte[] buffer = new byte[10240];

        // get a signature object
        Signature signature = Signature.getInstance(SIGN_ALG);

        // init the signature with a private key for signing
        Key pk = keystore.getKey(alias, keypass);
        signature.initSign((PrivateKey)pk);

        for (; ; ) {
            bytesRead = stream.read(buffer);
            if (bytesRead == -1) {
                break;
            }

            signature.update(buffer, 0, bytesRead);
        }
        
        // return the signature
        byte[] raw = signature.sign();
        return Base64.encode(raw);
    }

    /**
     * <code>getVerifyCert</code> - A helper function used by
     * <code>verify</code>.
     *
     * Outputs an app descriptor file to the caller provided 
     * ByteArrayOutputStream <code>baos</code> and returns a
     * base64 encoded signature for those bits.  The signature
     * is valid only for exactly the returned bits.  If 
     * <code>encoding</code> is not specified, a default encoding
     * type of Ascii with Unicode escapes is used.
     *
     * @param alias  Alias of the verify key in the keystore. 
     * @return Verified X509Certificate containing the public key
     *         with which this app descriptor file's signature should
     *         be verified.
     * @exception AppDescriptorException
     * @exception NoSuchAlgorithmException
     * @exception KeyStoreException
     * @exception CertificateException
     * @exception UnrecoverableKeyException
     * @exception InvalidKeyException
     * @exception NoSuchProviderException
     * @exception SignatureException
     */

    private X509Certificate getVerifyCert(String alias) 
        throws AppDescriptorException, NoSuchAlgorithmException, 
               KeyStoreException, CertificateException, 
               UnrecoverableKeyException, InvalidKeyException, 
               NoSuchProviderException, SignatureException {
        X509Certificate returncert = null;    
        X509Certificate current = null;
        X509Certificate operatorXcert = null;
        Certificate operatorcert = null;
        byte[] operatordata;
        String operatordn = null;
        String currentdn = null;
        String rv = null;
        
        // get the operator verification cert from the keystore
        operatorcert = keystore.getCertificate(alias);

        // convert opaque operator cert into X509 encoding so we
        // can use it.
        operatordata = operatorcert.getEncoded();
        CertificateFactory cf = CertificateFactory.getInstance("X.509");
        ByteArrayInputStream bais = new ByteArrayInputStream(operatordata);
        while (bais.available() > 0) {
            operatorXcert = (X509Certificate)cf.generateCertificate(bais);
        }

        try {
            bais.close();
        } catch (IOException ioe) {
        }

        // Get the operator's distinguished name
        operatordn = (operatorXcert.getSubjectDN()).getName();

        /*
         * Now we search for a CP_ATTR type certificate 
         * who's issuer DN matches "operatordn."
         * It is the certificate that should verify the
         * signature on this app descriptor.
         *
         * Before it can do that, it must be trusted by being
         * verified by the "operator" certificate.
         * The calls to verify() and checkValidity() do this.
         */
        int count = 1;
        while ((current = getCert(1, count)) != null) {
            currentdn = (current.getIssuerDN()).getName();
            if (operatordn.equals(currentdn)) {
                // verify cert sig with operator public key
                current.verify(operatorcert.getPublicKey());
                // check cert validity dates
                current.checkValidity();
                returncert = current;   
                break;
            } 

            count++;
        }

        return returncert;
    }

    /**
     * <code>base64CertToX509Cert</code> - A helper function used by
     * <code>verify</code>
     *
     * @param b64str A certificate encoded as Base64 String
     * @return An X509Certificate object.                              
     * @exception CertificateException Thrown if there is an error
     *            converting the <code>b64str</code> certificate
     *            into X509 format.
     */
    private X509Certificate base64CertToX509Cert(String b64str) 
        throws CertificateException 
    {
        X509Certificate c = null;
        
        byte[] certificateData = Base64.decode(b64str);
        
        CertificateFactory cf = CertificateFactory.getInstance("X.509");
        ByteArrayInputStream bais = 
            new ByteArrayInputStream(certificateData);
        
        while (bais.available() > 0) {
            c = (X509Certificate)cf.generateCertificate(bais);
        }

        try {
            bais.close();
        } catch (IOException ioe) {
        }

        return c;           
    }
}




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

package dummyCA;

import java.security.spec.X509EncodedKeySpec;
import java.security.spec.InvalidKeySpecException;
import java.security.*;
import java.security.cert.CertificateEncodingException;
import java.io.*;
import java.math.BigInteger;
import java.util.Calendar;
import java.util.TimeZone;

/**
 * This class represents certificate authority.
 */
public class Authority {

    /** Keystore name for CA credentials. */
    private static String keystoreFilename = "j2se_test_keystore.bin";
    /** Keystore password. */
    private static String keystorePassword = "keystorepwd";
    /** Alias for CA keys. */
    private static String keyAlias = "dummyca";
    /** CA key password. */
    private static String keyPassword = "keypwd";
    /** Algorithm name for CA signature for new certificates. */
    private static String CASign = "SHA1withRSA";

    /**
     * Signature algorithm attributes - OID, crypto algorithm ID, signature
     * algorithm ID.
     */
    private static String[][] algorithms = {
        {"1.2.840.10040.4.3", "DSA", "SHA1withDSA"},
        {"1.2.840.113549.1.1.4", "RSA", "MD5withRSA"},
        {"1.2.840.113549.1.1.5", "RSA", "SHA1withRSA"},
        {"1.2.840.10045.4.1", "ECDSA", "ECDSA"}
    };

    /** Is this authority initialized? */
    private static boolean init;
    /** CA certificate. */
    private static java.security.cert.X509Certificate CACert;
    /** CA private key. */
    private static PrivateKey CAPrivKey;
    /** Current serial number for new certificate. */
    private static long SerialNumber;
    /** TLV structure to be used in new certificates. */
    private static TLV CASignatureAlgorithm;
    /** TLV structure to be used for generation of new certificates. */
    private static TLV CACertPointer;

    /**
     * Initializes the CA.
     * @param o servlet instance that should be used to obtain keystore or
     *          null if keystore should be opened as file.
     * @return true if initialization was successful.
     */
    synchronized public static boolean init(Object o) {

        if (init) {
            return true;
        }

        try {
            InputStream keystoreStream;

            if (o == null) {
                keystoreStream =
                        new FileInputStream(new File(keystoreFilename));
            } else {
                keystoreStream =
                        o.getClass().getResourceAsStream(keystoreFilename);
            }
            KeyStore jcaKeystore =
                    KeyStore.getInstance(KeyStore.getDefaultType());

            try {

                if (keystorePassword == null) {
                    jcaKeystore.load(keystoreStream, null);
                } else {
                    jcaKeystore.load(keystoreStream,
                            keystorePassword.toCharArray());
                }
            } finally {
                keystoreStream.close();
            }

            // retrieve CA certificate and private key

            CACert = (java.security.cert.X509Certificate)
                    jcaKeystore.getCertificate(keyAlias);
            CAPrivKey = (PrivateKey) jcaKeystore.getKey(keyAlias,
                    keyPassword.toCharArray());

            String CASignOID = null;
            for (int i = 0 ; i < algorithms.length; i++) {
                if (CASign.equals(algorithms[i][2])) {
                    CASignOID = algorithms[i][0];
                    break;
                }
            }

            if (CASignOID == null) {
                return false;
            }

            // CA signature algorithm identifier
            CASignatureAlgorithm = new TLV(TLV.SEQUENCE_TYPE);
            CASignatureAlgorithm.child =
                    new TLV(TLV.OID_TYPE, TLV.StringToOID(CASignOID));
            CASignatureAlgorithm.child.next =
                    new TLV(TLV.NULL_TYPE, new byte[0]);

            // Parse CA certificate
            CACertPointer = new TLV(CACert.getEncoded(), 0);

            CACertPointer = CACertPointer.child.child;
            if (CACertPointer.type == TLV.VERSION_TYPE) {
                CACertPointer = CACertPointer.next;
            }
            // CACertPointer is at SerialNumber field

        } catch (Exception e){
            return false;
        }

        // serial number initial value

        Calendar c = Calendar.getInstance();
        long t = c.get(Calendar.YEAR) - 2000;
        t = t * 365 + c.get(Calendar.DAY_OF_YEAR) - 1;
        t = t * 24 + c.get(Calendar.HOUR_OF_DAY);
        t = t * 60 + c.get(Calendar.MINUTE);
        t = t * 60 + c.get(Calendar.SECOND);
        t = t * 1000 + c.get(Calendar.MILLISECOND);
        SerialNumber = t;

        init = true;

        return true;
    }

    /**
     * Returns the serial number value to be used for new certificate.
     * @return the serial number value.
     */
    synchronized private static long getSerialNumber() {
        return SerialNumber++;
    }

    /** Parsed certificate enrollment request. */
    private TLV CSR;
    /** Generated certificate. */
    private TLV Certificate;
    /** Generated IssuerAndSerialNumber data structure. */
    private TLV IssuerAndSerialNumber;
    /** Current status of CA. */
    private String Status = "Ready.";

    /**
     * Creates a new certificate.
     * @param data certificate enrollment request
     * @return true if a new certificate was generated
     */
    public boolean createCertificate(byte[] data) {

        if (! init) {
            Status = "Can't load CA credentials.";
            return false;
        }

        try {
            CSR = new TLV(data, 0);
        } catch (Exception e) {
            Status = "Error parsing the CSR.";
            return false;
        }

        try {
            if (! checkSign()) {
                Status = "Signature mismatch.";
                return false;
            }
        } catch (Exception e) {
            Status = "Can't check signature.";
            return false;
        }

        try {
            create();
        } catch (Exception e) {
            Status = "Can't create certificate.";
            return false;
        }

        ByteArrayOutputStream os = new ByteArrayOutputStream();
        PrintStream ps = new PrintStream(os);
        ps.println("Last CSR:");
        CSR.print(ps);
        ps.println();
        ps.println("Certificate:");
        Certificate.print(ps);
        ps.println();
        ps.println("IssuerAndSerialNumber:");
        IssuerAndSerialNumber.print(ps);
        ps.close();
        Status = os.toString();

        return true;
    }

    /**
     * Returns current status.
     * @return current status
     */
    public String getStatus() {
        return Status;
    }

    /**
     * Verifies signature in certificate enrollment request.
     * @return true if signature is verified
     * @throws IOException if IOException occurs
     * @throws NoSuchAlgorithmException if NoSuchAlgorithmException occurs
     * @throws InvalidKeySpecException if InvalidKeySpecException occurs
     * @throws InvalidKeyException if InvalidKeyException occurs
     * @throws SignatureException if SignatureException occurs
     */
    private boolean checkSign() throws IOException, NoSuchAlgorithmException,
            InvalidKeySpecException, InvalidKeyException, SignatureException {

        String algorithmOID = CSR.child.next.child.getOID();
        String cryptoAlg = "";
        String signAlg = "";

        for (int i = 0 ; i < algorithms.length; i++) {
            if (algorithmOID.equals(algorithms[i][0])) {
                cryptoAlg = algorithms[i][1];
                signAlg = algorithms[i][2];
                break;
            }
        }

        byte[] subjectPKInfo = CSR.child.child.next.next.getDERData();
        X509EncodedKeySpec keySpec = new X509EncodedKeySpec(subjectPKInfo);
        KeyFactory factory = KeyFactory.getInstance(cryptoAlg);
        PublicKey key = factory.generatePublic(keySpec);

        Signature sig = Signature.getInstance(signAlg);
        sig.initVerify(key);
        sig.update(CSR.child.getDERData());

        byte[] sign = CSR.child.next.next.getValue();
        byte[] signature = new byte[sign.length - 1];
        System.arraycopy(sign, 1, signature, 0, signature.length);
        return sig.verify(signature);
    }

    /**
     * Creates a new certificate.
     * @throws NoSuchAlgorithmException if an exception occurs during signature
     *         operation
     * @throws InvalidKeyException if an exception occurs during signature
     *         operation
     * @throws SignatureException if an exception occurs during signature
     *         operation
     */
    private void create() throws NoSuchAlgorithmException,
            InvalidKeyException, SignatureException {

        // Prepare TBSCertificate data structure
        TLV TBSCert = new TLV(TLV.SEQUENCE_TYPE);

        // serial number
        BigInteger serialNumber = BigInteger.valueOf(getSerialNumber());

        TLV current = new TLV(TLV.INTEGER_TYPE, serialNumber.toByteArray());
        TBSCert.child = current;

        // signature algorithm identifier (CA)
        current.next = CASignatureAlgorithm.copy();
        current = current.next;

        // issuer
        current.next = CACertPointer.next.next.next.next.copy();
        current = current.next;

        // validity
        current.next = new TLV(TLV.SEQUENCE_TYPE);
        current = current.next;

        Calendar calendar = Calendar.getInstance();
        calendar.setTimeZone(TimeZone.getTimeZone("GMT"));
        current.child = TLV.createUTCTime(calendar);
        calendar.add(Calendar.DAY_OF_MONTH, 30);
        current.child.next = TLV.createUTCTime(calendar);

        // subject
        current.next = CSR.child.child.next.copy();
        current = current.next;

        // subject public key info
        current.next = CSR.child.child.next.next.copy();

        // TBSCertificate is complete, now sign it

        Signature s = Signature.getInstance(CASign);
        s.initSign(CAPrivKey);
        s.update(TBSCert.getDERData());

        byte[] sign1 = s.sign();
        byte[] sign2 = new byte[sign1.length + 1];
        System.arraycopy(sign1, 0, sign2, 1, sign1.length);

        // create Certificate data structure

        Certificate = new TLV(TLV.SEQUENCE_TYPE);

        // TBSCertificate
        Certificate.child = TBSCert;

        // signatureAlgorithm
        current = CASignatureAlgorithm.copy();
        TBSCert.next = current;

        // signatureValue
        current.next = new TLV(TLV.BITSTRING_TYPE, sign2);

        // create IssuerAndSerialNumber data structure

        IssuerAndSerialNumber = new TLV(TLV.SEQUENCE_TYPE);
        IssuerAndSerialNumber.child = CACertPointer.next.next.next.next.copy();
        IssuerAndSerialNumber.child.next = new TLV(TLV.INTEGER_TYPE,
                                               serialNumber.toByteArray());
    }

    /**
     * Returns PkiPath data structure for generated certificate.
     * @return PkiPath data structure for generated certificate
     */
    public byte[] getPkiPath() {
        TLV PkiPath = new TLV(TLV.SEQUENCE_TYPE);
        try {
            PkiPath.child = new TLV(CACert.getEncoded(), 0);
        } catch (CertificateEncodingException e) {
            // it was already requested during initialization without
            // exception
        }
        PkiPath.child.next = Certificate;
        return PkiPath.getDERData();
    }

    /**
     * Returns IssuerAndSerialNumber data structure for generated certificate.
     * @return IssuerAndSerialNumber data structure for generated certificate
     */
    public byte[] getIssuerAndSerialNumber() {
        return IssuerAndSerialNumber.getDERData();
    }

    /**
     * Returns DER encoded new certificate.
     * @return DER encoded new certificate
     */
    public byte[] getCertificate() {
        return Certificate.getDERData();
    }
}

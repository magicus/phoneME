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

package com.sun.midp.pki.ocsp;

import java.io.IOException;
import java.util.Hashtable;

import com.sun.midp.pki.DerInputStream;
import com.sun.midp.pki.DerOutputStream;
import com.sun.midp.pki.DerValue;
import com.sun.midp.pki.X509Certificate;
import com.sun.midp.pki.Utils;
import com.sun.midp.pki.AlgorithmId;
import com.sun.midp.pki.SerialNumber;
import com.sun.midp.pki.BigInteger;

import com.sun.midp.crypto.MessageDigest;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

/**
 * This class corresponds to the CertId field in OCSP Request
 * and the OCSP Response. The ASN.1 definition for CertID is defined
 * in RFC 2560 as:
 * <pre>
 *
 * CertID          ::=     SEQUENCE {
 *      hashAlgorithm       AlgorithmIdentifier,
 *      issuerNameHash      OCTET STRING, -- Hash of Issuer's DN
 *      issuerKeyHash       OCTET STRING, -- Hash of Issuers public key
 *      serialNumber        CertificateSerialNumber
 *      }
 *
 * </pre>
 */
public class CertId {
    private AlgorithmId hashAlgId;
    private byte[] issuerNameHash;
    private byte[] issuerKeyHash;
    private SerialNumber certSerialNumber;
    private int myhash = -1; // hashcode for this CertId

    /**
     * Creates a CertId. The hash algorithm used is SHA-1.
     */
    public CertId(X509Certificate issuerCert, SerialNumber serialNumber)
            throws Exception {
        // compute issuerNameHash
        MessageDigest md = MessageDigest.getInstance("SHA-1");
        hashAlgId = AlgorithmId.get("SHA-1");

        byte[] data = getNameEncoded(issuerCert.getSubject());

        md.update(data, 0, data.length);

        issuerNameHash = new byte[md.getDigestLength()];
        md.digest(issuerNameHash, 0, issuerNameHash.length);

        // compute issuerKeyHash (remove the tag and length)
        byte[] pubKey = issuerCert.getPublicKey().getEncoded();
        DerValue val = new DerValue(pubKey);
        DerValue[] seq = new DerValue[2];
        seq[0] = val.data.getDerValue(); // AlgorithmID
        seq[1] = val.data.getDerValue(); // Key

        byte[] keyBytes = seq[1].getBitString();
        md.update(keyBytes, 0, keyBytes.length);

        issuerKeyHash = new byte[md.getDigestLength()];
        md.digest(issuerKeyHash, 0, issuerKeyHash.length);
    
        certSerialNumber = serialNumber;

        if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
            Logging.report(Logging.INFORMATION, LogChannels.LC_SECURITY,
                       "Issuer Certificate is " + issuerCert);
            Logging.report(Logging.INFORMATION, LogChannels.LC_SECURITY,
                       "issuerNameHash is " + Utils.hexEncode(issuerNameHash));
            Logging.report(Logging.INFORMATION, LogChannels.LC_SECURITY,
                       "issuerKeyHash is " + Utils.hexEncode(issuerKeyHash));
        }
    }

    /**
     * Creates a CertId from its ASN.1 DER encoding.
     */
    public CertId(DerInputStream derIn) throws IOException {
        hashAlgId = AlgorithmId.parse(derIn.getDerValue());
        issuerNameHash = derIn.getOctetString();
        issuerKeyHash = derIn.getOctetString();
        certSerialNumber = new SerialNumber(derIn);
    }

    /**
     * Return the hash algorithm identifier.
     */
    public AlgorithmId getHashAlgorithm() {
        return hashAlgId;
    }

    /**
     * Return the hash value for the issuer name.
     */
    public byte[] getIssuerNameHash() {
        return issuerNameHash;
    }

    /**
     * Return the hash value for the issuer key.
     */
    public byte[] getIssuerKeyHash() {
        return issuerKeyHash;
    }

    /**
     * Return the serial number.
     */
    public BigInteger getSerialNumber() {
        return certSerialNumber.getNumber();
    }

    /**
     * Encode the CertId using ASN.1 DER.
     * The hash algorithm used is SHA-1.
     */
    public void encode(DerOutputStream out) throws IOException {
        DerOutputStream tmp = new DerOutputStream();
        hashAlgId.encode(tmp);
        tmp.putOctetString(issuerNameHash);
        tmp.putOctetString(issuerKeyHash);
        certSerialNumber.encode(tmp);
        out.write(DerValue.tag_Sequence, tmp);

        if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
            Logging.report(Logging.INFORMATION, LogChannels.LC_SECURITY,
                   "Encoded certId is " + Utils.hexEncode(out.toByteArray()));
        }
    }

   /**
     * Returns a hashcode value for this CertId.
     *
     * @return the hashcode value.
     */
    public int hashCode() {
        if (myhash == -1) {
            myhash = hashAlgId.hashCode();
            for (int i = 0; i < issuerNameHash.length; i++) {
                myhash += issuerNameHash[i] * i;
            }
            for (int i = 0; i < issuerKeyHash.length; i++) {
                myhash += issuerKeyHash[i] * i;
            }
            myhash += certSerialNumber.getNumber().hashCode();
        }
        return myhash;
    }

    /**
     * Compares this CertId for equality with the specified
     * object. Two CertId objects are considered equal if their hash algorithms,
     * their issuer name and issuer key hash values and their serial numbers
     * are equal.
     *
     * @param other the object to test for equality with this object.
     * @return true if the objects are considered equal, false otherwise.
     */
    public boolean equals(Object other) {
        if (this == other) {
            return true;
        }
        if (other == null || (!(other instanceof CertId))) {
            return false;
        }

        CertId that = (CertId) other;

        if (hashAlgId.equals(that.getHashAlgorithm()) &&
            arraysEqual(issuerNameHash, that.getIssuerNameHash()) &&
            arraysEqual(issuerKeyHash, that.getIssuerKeyHash()) &&
            certSerialNumber.getNumber().equals(that.getSerialNumber())) {
            return true;
        } else {
            return false;
        }
    }

    /**
     * Returns DER-encoded form of the given name.
     *
     * @param name name to encode
     * @return array of bytes containing DER-encoded name
     * @throws IOException if error occured when encoding the subject name
     */
    private byte[] getNameEncoded(String name)
            throws IOException {
        Hashtable nameTagToCode = new Hashtable(20);
        nameTagToCode.put("CN", new Integer(3));  // Common name: id-at 3
        nameTagToCode.put("SN", new Integer(4));  // Surname: id-at 4
        nameTagToCode.put("C", new Integer(6));   // Country: id-at 6
        nameTagToCode.put("L", new Integer(7));   // Locality: id-at 7
        nameTagToCode.put("ST", new Integer(8));  // State or province: id-at 8
        nameTagToCode.put("STREET", new Integer(9)); // Street address: id-at 9
        nameTagToCode.put("O", new Integer(10));  // Organization: id-at 10
        nameTagToCode.put("OU", new Integer(11)); // Organization unit: id-at 11
        // "EmailAddress"

        //String issuer = cert.getIssuer();
        StringBuffer currTag = new StringBuffer();
        int i = 0;

        DerOutputStream tmpStream = new DerOutputStream();

        while (i < name.length()) {
            DerOutputStream out = new DerOutputStream();
            char c = name.charAt(i);
            i++;

            if (c == '=') {
                Integer code = (Integer)nameTagToCode.get(currTag.toString());

                if (code != null) {
                    DerValue v = new DerValue(DerValue.tag_ObjectId,
                            new byte[] {0x55, 0x04, code.byteValue()});
                    out.putDerValue(v);
                } else {
                    // IMPL_NOTE: handle e-mail and unknown names
                    throw new IOException("Can't encode: " + currTag);
                }

                String restOfName = name.substring(i);
                int idx = restOfName.indexOf(";");
                if (idx < 0) {
                    idx = restOfName.length();
                }

                String attrValue = name.substring(i, i + idx);

                out.putPrintableString(attrValue);

                DerOutputStream tmpStream2 = new DerOutputStream();
                tmpStream2.write(DerValue.tag_Sequence, out);
                tmpStream.write(DerValue.tag_Set, tmpStream2);

                i += idx + 1;
                currTag = new StringBuffer();
                continue;
            }

            currTag.append(c);
        }

        DerOutputStream finalOut = new DerOutputStream();
        finalOut.write(DerValue.tag_Sequence, tmpStream);

        return finalOut.toByteArray();
    }

    /**
     * Returns <tt>true</tt> if the two specified arrays of bytes are
     * <i>equal</i> to one another.  Two arrays are considered equal if both
     * arrays contain the same number of elements, and all corresponding pairs
     * of elements in the two arrays are equal.  In other words, two arrays
     * are equal if they contain the same elements in the same order.  Also,
     * two array references are considered equal if both are <tt>null</tt>.<p>
     *
     * @param a one array to be tested for equality.
     * @param a2 the other array to be tested for equality.
     * @return <tt>true</tt> if the two arrays are equal.
     */
    private static boolean arraysEqual(byte[] a, byte[] a2) {
        if (a==a2) {
            return true;
        }
        if (a==null || a2==null) {
            return false;
        }

        int length = a.length;
        if (a2.length != length) {
            return false;
        }

        for (int i=0; i<length; i++) {
            if (a[i] != a2[i]) {
                return false;
            }
        }

        return true;
    }

    /**
     * Create a string representation of the CertId.
     */
    public String toString() {
        StringBuffer sb = new StringBuffer();
        sb.append("-- CertId ---\n");
        sb.append("Algorithm: ");
        sb.append(hashAlgId.toString());
        sb.append("\n");

        sb.append("issuerNameHash: \n");
        sb.append(Utils.hexEncode(issuerNameHash));

        sb.append("\nissuerKeyHash: \n");
        sb.append(Utils.hexEncode(issuerKeyHash));

        sb.append("\n");
        sb.append(certSerialNumber.toString());
        
        return sb.toString();
    }
}

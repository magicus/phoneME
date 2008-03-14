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

import com.sun.midp.pki.DerInputStream;
import com.sun.midp.pki.DerOutputStream;
import com.sun.midp.pki.DerValue;
import com.sun.midp.pki.X509Certificate;
import com.sun.midp.pki.Utils;
import com.sun.midp.pki.AlgorithmId;
import com.sun.midp.pki.SerialNumber;

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
 *
 * @author      Ram Marti
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
        MessageDigest md = MessageDigest.getInstance("SHA1");
        hashAlgId = AlgorithmId.get("SHA1");

        //md.update(issuerCert.getSubjectX500Principal().getEncoded());
        byte[] data = issuerCert.getSubject().getBytes();
        md.update(data, 0, issuerNameHash.length);

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
    public int getSerialNumber() {
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
            myhash += new Integer(certSerialNumber.getNumber()).hashCode();
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
            certSerialNumber.getNumber() == that.getSerialNumber()) {
            return true;
        } else {
            return false;
        }
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
        if (a==a2)
            return true;
        if (a==null || a2==null)
            return false;

        int length = a.length;
        if (a2.length != length)
            return false;

        for (int i=0; i<length; i++)
            if (a[i] != a2[i])
                return false;

        return true;
    }

    /**
     * Create a string representation of the CertId.
     */
    public String toString() {
        StringBuffer sb = new StringBuffer();
        sb.append("CertId \n");
        sb.append("Algorithm: ");
        sb.append(hashAlgId.toString());
        sb.append("\n");
        sb.append("issuerNameHash \n");

        sb.append(Utils.hexEncode(issuerNameHash));
        sb.append("\nissuerKeyHash: \n");
        sb.append(Utils.hexEncode(issuerKeyHash));
        sb.append("\n");
        sb.append(certSerialNumber.toString());
        
        return sb.toString();
    }
}

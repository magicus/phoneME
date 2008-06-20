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

import com.sun.midp.pki.SerialNumber;
import com.sun.midp.pki.X509Certificate;
import com.sun.midp.pki.BigInteger;
import com.sun.midp.pki.DerOutputStream;
import com.sun.midp.pki.DerValue;
import com.sun.midp.pki.Extension;
import com.sun.midp.pki.Utils;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

import java.io.IOException;

/**
 * This class can be used to generate an OCSP request and send it over
 * an outputstream. Currently we do not support signing requests
 * The OCSP Request is specified in RFC 2560 and
 * the ASN.1 definition is as follows:
 * <pre>
 *
 * OCSPRequest     ::=     SEQUENCE {
 *      tbsRequest                  TBSRequest,
 *      optionalSignature   [0]     EXPLICIT Signature OPTIONAL }
 *
 *   TBSRequest      ::=     SEQUENCE {
 *      version             [0]     EXPLICIT Version DEFAULT v1,
 *      requestorName       [1]     EXPLICIT GeneralName OPTIONAL,
 *      requestList                 SEQUENCE OF Request,
 *      requestExtensions   [2]     EXPLICIT Extensions OPTIONAL }
 *
 *  Signature       ::=     SEQUENCE {
 *      signatureAlgorithm      AlgorithmIdentifier,
 *      signature               BIT STRING,
 *      certs               [0] EXPLICIT SEQUENCE OF Certificate OPTIONAL
 *   }
 *
 *  Version         ::=             INTEGER  {  v1(0) }
 *
 *  Request         ::=     SEQUENCE {
 *      reqCert                     CertID,
 *      singleRequestExtensions     [0] EXPLICIT Extensions OPTIONAL }
 *
 *  CertID          ::= SEQUENCE {
 *       hashAlgorithm  AlgorithmIdentifier,
 *       issuerNameHash OCTET STRING, -- Hash of Issuer's DN
 *       issuerKeyHash  OCTET STRING, -- Hash of Issuers public key
 *       serialNumber   CertificateSerialNumber
 * }
 *
 * </pre>
 */

public class OCSPRequest {
    /** Serial number of the certificates to be checked for revocation */
    private SerialNumber serialNumber;

    /** Issuer's certificate (for computing certId hash values) */
    private X509Certificate issuerCert;

    /** CertId of the certificate to be checked */
    private CertId certId = null;

    /** Extensions of this request (currently only nonce is supported) */
    private final Extension extensions[];

    /**
     * Constructs an OCSPRequest. This constructor is used
     * to construct an unsigned OCSP Request for a single user cert.
     *
     * @param userCert certificate to check
     * @param issuerCert certificate of the userCert's issuer
     * @param requestExtensions array of request extensions (currently only
     *                          nonce is supported); can be NULL
     */
    OCSPRequest(X509Certificate userCert, X509Certificate issuerCert,
                final Extension[] requestExtensions) {
        if (issuerCert == null) {
            throw new IllegalArgumentException("Null IssuerCertificate");
        }
        
        this.issuerCert = issuerCert;
        extensions = requestExtensions;

        byte[] sn = userCert.getRawSerialNumber();
        if (sn != null) {
            serialNumber = new SerialNumber(new BigInteger(sn));
        } else {
            // actually, this field is not used by some servers
            throw new IllegalArgumentException(
                    "Certificate's Serial Number is not known.");
        }

        if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
            try {
                byte[] data = getRequestAsByteArray();
                Logging.report(Logging.INFORMATION, LogChannels.LC_SECURITY,
                    "OCSPRequest first bytes are... " +
                        Utils.hexEncode(data, 0,
                                (data.length > 256) ? 256 : data.length));
            } catch (Exception e) {
                // ignore
            }
        }
    }

    /**
     * Returns this request encoded as an array of bytes.
     *
     * @return this request encoded as an array of bytes
     * @throws IOException if an error occured during encoding
     */
    public byte[] getRequestAsByteArray() throws IOException {
        // encode tbsRequest
        DerOutputStream tmp = new DerOutputStream();
        DerOutputStream derSingleReqList  = new DerOutputStream();
        SingleRequest singleRequest = null;

        try {
            singleRequest = new SingleRequest(issuerCert, serialNumber);
        } catch (Exception e) {
            throw new IOException("Error encoding OCSP request");
        }

        certId = singleRequest.getCertId();
        singleRequest.encode(derSingleReqList);
        tmp.write(DerValue.tag_Sequence, derSingleReqList);

        DerOutputStream tbsRequest = new DerOutputStream();

        // Encode request extensions (if any)
        if (extensions != null) {
            DerOutputStream tmp1 = new DerOutputStream();
            for (int i = 0; i < extensions.length; i++) {
                extensions[i].encode(tmp1);
            }
            // write context tag [2], EXPLICIT, constructed
            tmp.write(new byte[] {
                    DerValue.createTag(DerValue.TAG_CONTEXT, true, (byte)2),
                    (byte)(tmp1.toByteArray().length + 2)
                }
            );
            tmp.write(DerValue.tag_Sequence, tmp1);
        }

        tbsRequest.write(DerValue.tag_Sequence, tmp);

        // OCSPRequest without the signature
        DerOutputStream ocspRequest = new DerOutputStream();
        ocspRequest.write(DerValue.tag_Sequence, tbsRequest);

        return ocspRequest.toByteArray();
    }

    // used by OCSPValidatorImpl
    CertId getCertId() {
        return certId;
    }

    /**
     * Returns string respresentation of this request. Useful for debug.
     *
     * @return string respresentation of this request
     */
    public String toString() {
        try {
            byte[] data = getRequestAsByteArray();
            return Utils.hexEncode(data, 0,
                (data.length > 256) ? 256 : data.length);
        } catch (Exception e) {
            // ignore
        }
        return "";
    }

    private static class SingleRequest {
        private CertId certId;

        // No extensions are set

        private SingleRequest(X509Certificate cert,
                              SerialNumber serialNo) throws Exception {
            certId = new CertId(cert, serialNo);
        }

        private void encode(DerOutputStream out) throws IOException {
            DerOutputStream tmp = new DerOutputStream();
            certId.encode(tmp);
            out.write(DerValue.tag_Sequence, tmp);
        }

        private CertId getCertId() {
            return certId;
        }

        /**
         * Returns string respresentation of this request. Useful for debug.
         *
         * @return string respresentation of this request
         */
        public String toString() {
            String str = "SingleRequest (" +
                    "\n " + certId;
            str += "\n)";
            return str;
        }
    }
    
}

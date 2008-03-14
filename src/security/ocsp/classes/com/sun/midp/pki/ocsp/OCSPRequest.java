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
import com.sun.midp.pki.DerOutputStream;
import com.sun.midp.pki.X509Certificate;
import com.sun.midp.pki.DerValue;

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
 *
 * @author      Ram Marti
 */

public class OCSPRequest {
    // Serial number of the certificates to be checked for revocation
    private SerialNumber serialNumber;

    // Issuer's certificate (for computing certId hash values)
    private X509Certificate issuerCert;

    // CertId of the certificate to be checked
    private CertId certId = null;

    /**
     * Version of the OCSP request. Default is 0 (means v1).
     */
    //private int version;

    /**
     * Name of the requestor. Optional.
     */
    //private String requestorName;

    /**
     * List of requests to server.
     */
    //private Vector requestList = new Vector();

    /**
     * List of extensions.
     */
    //private Vector requestExtensions = new Vector();

    /**
     * Signature (optional).
     */
    //private byte[] signature;

    /** */
    //private class Request {
        /** Hash algorithm identifier */
        //String hashAlgorithm;
        /** Hash of Issuer's DN */
        //String issuerNameHash;
        /** Hash of Issuer's public key */
        //String issuerKeyHash;
        /** Certificate serial number */
        //String certSerialNumber;
        /** This request extensions, optional */
        //private Vector extensions = new Vector();

        /**
         * Returns string respresentation of this request. Useful for debug.
         *
         * @return string respresentation of this request
         */
        /*
        public String toString() {
            String str = "(" +
                    "\n  hashAlgorithm = " + hashAlgorithm +
                    "\n  issuerNameHash = " + issuerNameHash +
                    "\n  certSerialNumber = " + certSerialNumber;

            for (int i = 0; i < extensions.size(); i++) {
                str += "\n Extension " + i + ": \n" +
                        extensions.elementAt(i).toString();
            }

            str += "\n)";

            return str;
        }
    }*/

    //private class Extension {
        /** ID of this extension */
        //String id;
        /** true is this extension is critical, false otherwise */
        //boolean isCritical;
        /** Value of the extension (octet string) */
        //String value;

        /**
         * Returns string respresentation of this extension. Useful for debug.
         *
         * @return string respresentation of this extension
         */
        //public String toString() {
        //    return "[" +
        //            "\n  id = " + id +
        //            "\n  isCritical = " + isCritical +
        //            "\n  value = " + value +
        //            "\n]";
        //}
    //}

    /**
     * Constructs an OCSPRequest. This constructor is used
     * to construct an unsigned OCSP Request for a single user cert.
     */
    OCSPRequest(X509Certificate userCert, X509Certificate issuerCert) {

        if (issuerCert == null) {
            throw new IllegalArgumentException("Null IssuerCertificate");
        }
        
        this.issuerCert = issuerCert;

        serialNumber = new SerialNumber(
                new BigInteger(userCert.getRawSerialNumber()));
        //serialNumber = userCert.getSerialNumberObject();
    }

    /**
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

        // No extensions supported
        DerOutputStream tbsRequest = new DerOutputStream();
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
        /*
        String str = "version = " + version +
                     "\nrequestorName = " + requestorName + "\n";

        for (int i = 0; i < requestList.size(); i++) {
            str += "\n Request " + i + ": \n" +
                    requestList.elementAt(i).toString();
        }

        for (int i = 0; i < requestExtensions.size(); i++) {
            str += "\n Extension " + i + ": \n" +
                    requestExtensions.elementAt(i).toString();
        }

        return str;
        */
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
    }
    
}

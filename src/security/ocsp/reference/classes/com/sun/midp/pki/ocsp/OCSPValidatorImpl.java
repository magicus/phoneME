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

import com.sun.midp.io.Base64;
import com.sun.midp.main.Configuration;
import com.sun.midp.pki.ObjectIdentifier;
import com.sun.midp.pki.X509Certificate;
import com.sun.midp.publickeystore.WebPublicKeyStore;
import com.sun.midp.publickeystore.PublicKeyInfo;

import javax.microedition.pki.Certificate;
import javax.microedition.io.HttpConnection;
import javax.microedition.io.Connector;
import javax.microedition.io.ConnectionNotFoundException;

import java.io.InputStream;
import java.io.OutputStream;
import java.io.IOException;
import java.util.Vector;

/**
 * Validates the certificates.
 */
public class OCSPValidatorImpl implements OCSPValidator {
    private static final String OCSP_REQUEST_MIME_TYPE =
            "application/ocsp-request";
    private static final String OCSP_RESPONSE_MIME_TYPE =
            "application/ocsp-response";

    /** Connection to OCSP responder. */
    private HttpConnection httpConnection;

    /** Output stream to send a request to OCSP server. */
    private OutputStream httpOutputStream;

    /** Input stream to read a response from OCSP server. */
    private InputStream httpInputStream;

    // Supported extensions
    private static final int OCSP_NONCE_DATA[] =
        { 1, 3, 6, 1, 5, 5, 7, 48, 1, 2 };
    private static final ObjectIdentifier OCSP_NONCE_OID;

    private static final int CHUNK_SIZE = 10 * 1024;

    static {
        OCSP_NONCE_OID = ObjectIdentifier.newInternal(OCSP_NONCE_DATA);
    }
    
    /**
     * Retrieves the status of the given certificate.
     *
     * @param cert X.509 certificate status of which must be checked
     * @param issuerCert certificate of the trusted authority issued
     *                   the certificate given by cert
     * @return status of the certificate
     * @throws OCSPException if the OCSP Responder returned an error message
     */
    public int validate(Certificate cert, Certificate issuerCert)
            throws OCSPException {
        OCSPResponse response;

        try {
            openConnection();

            //X509Certificate issuerCert = (X509Certificate)certPath.elementAt(0);
            OCSPRequest request =
                    new OCSPRequest((X509Certificate)cert,
                            (X509Certificate)issuerCert);
            CertId certId = request.getCertId();
            sendRequest(request);

            //WebPublicKeyStore keyStore = WebPublicKeyStore.getTrustedKeyStore();
            //X509Certificate[] caCerts = keyStore.getKey(0);
            WebPublicKeyStore keyStore = WebPublicKeyStore.getTrustedKeyStore();
            Vector keys = keyStore.getKeys();

            Vector caCerts = new Vector();
            caCerts.addElement(issuerCert);
            caCerts.addElement(cert);
            for (int i = 0; i < keys.size(); i++) {
                PublicKeyInfo ki = (PublicKeyInfo)keys.elementAt(i);
                caCerts.addElement(WebPublicKeyStore.createCertificate(ki));
            }
            
            response = receiveResponse(caCerts);

            // Check that response applies to the cert that was supplied
            if (! certId.equals(response.getCertId())) {
                throw new OCSPException(OCSPException.CANNOT_VERIFY_SIGNATURE,
                    "Certificate in the OCSP response does not match the " +
                    "certificate supplied in the OCSP request.");
            }

            int certOCSPStatus = response.getCertStatus();

            // -----
            System.out.println("Status of certificate is: " +
                OCSPResponse.certStatusToText(certOCSPStatus));
            // -----

            if (certOCSPStatus != CertStatus.GOOD) {
                return certOCSPStatus;
            }
        } catch (OCSPException e) {
            System.out.println("ERROR: " + e.getErrorMessage());            
            e.printStackTrace();
            throw e;
        } catch (Exception e) {
            e.printStackTrace();
            throw new OCSPException(OCSPException.UNKNOWN_ERROR, e.getMessage());
        } finally {
            cleanup();
        }

        return response.getCertStatus();
    }

    /**
     * Opens a connection to the OCSP server.
     *
     * @throws OCSPException if the connection can't be established
     */
    private void openConnection() throws OCSPException {
        String proxyUsername = null, proxyPassword = null;
        //String responderUrl = Configuration.getProperty("ocsp.responderURL");
        String responderUrl = "http://cingular-ocsp.geotrust.com/responder";
        //String responderUrl = "http://ocsp.digsigtrust.com";

        try {
            httpConnection = (HttpConnection)
                Connector.open(responderUrl, Connector.READ_WRITE);

            httpConnection.setRequestMethod(HttpConnection.POST);

            if (proxyUsername != null && proxyPassword != null) {
                httpConnection.setRequestProperty("Proxy-Authorization",
                    formatAuthCredentials(proxyUsername, proxyPassword));
            }
        } catch (ConnectionNotFoundException e) {
            throw new OCSPException(OCSPException.SERVER_NOT_FOUND);
        } catch (IOException ioe) {
            throw new OCSPException(OCSPException.CANNOT_OPEN_CONNECTION,
                                    ioe.getMessage());
        }
    }

    /**
     * Sends a request to the OCSP server.
     *
     * @param request OCSP request to send
     * @throws OCSPException if an error occured while sending the request 
     */
    private void sendRequest(OCSPRequest request) throws OCSPException {
        System.out.println(">>> sendRequest():\n" + request.toString());

        try {
            byte[] requestBytes = request.getRequestAsByteArray();

            httpConnection.setRequestProperty("Accept",
                                              OCSP_RESPONSE_MIME_TYPE);
            httpConnection.setRequestProperty("ContentType",
                                              OCSP_REQUEST_MIME_TYPE);
            httpConnection.setRequestProperty("Content-length",
                String.valueOf(requestBytes.length));

            httpOutputStream = httpConnection.openOutputStream();
            httpOutputStream.write(requestBytes);

            int responseCode = httpConnection.getResponseCode();
            if (responseCode != HttpConnection.HTTP_OK) {
                throw new OCSPException(OCSPException.CANNOT_SEND_REQUEST,
                    httpConnection.getResponseMessage() +
                            " (" + responseCode + ")");
            }
        } catch (IOException ioe) {
            throw new OCSPException(OCSPException.CANNOT_SEND_REQUEST,
                                    ioe.getMessage());
        }

        System.out.println(">>> sendRequest(): exiting");
    }

    /**
     * Receives a response from the OCSP server.
     *
     * @param caCerts X.509 certificates of known CAs
     * @return OCSP response received from the server
     * @throws OCSPException if an error occured while receiving response
     */
    private OCSPResponse receiveResponse(Vector caCerts)
            throws OCSPException {
        System.out.println(">>> receiveResponse(): started");

        try {
            httpInputStream = httpConnection.openInputStream();

            int bufSize = CHUNK_SIZE;
            byte[] tmpBuf = new byte[bufSize];
            int total = 0;
            int count = 0;

            while (count != -1) {
                count = httpInputStream.read(tmpBuf, total,
                                             tmpBuf.length - total);

                if (count > 0) {
                    total += count;
                }

                if (total == tmpBuf.length) {
                    // allocate more memory to hold the response
                    int newBufSize = bufSize + CHUNK_SIZE;
                    byte[] newResponseBuf = new byte[newBufSize];
                    System.arraycopy(tmpBuf, 0, newResponseBuf, 0, total);
                    tmpBuf = newResponseBuf;
                }
            }

            byte[] responseBuf = new byte[total];
            System.arraycopy(tmpBuf, 0, responseBuf, 0, total);

            OCSPResponse ocspResponse = new OCSPResponse(responseBuf, caCerts);

            System.out.println(">>> receiveResponse(): exiting");

            return ocspResponse;
        } catch (IOException ioe) {
            throw new OCSPException(OCSPException.SERVER_NOT_RESPONDING,
                                    ioe.getMessage());
        }
    }

    /**
     * Closes the http connection to the OCSP server and the corresponding
     * input and output streams if opened.
     */
    private void cleanup() {
        if (httpInputStream != null) {
            try {
                httpInputStream.close();
            } catch (Exception ex) {
                // ignore
            }
        }

        if (httpOutputStream != null) {
            try {
                httpOutputStream.close();
            } catch (Exception ex) {
                // ignore
            }
        }

        if (httpConnection != null) {
            try {
                httpConnection.close();
            } catch (Exception ex) {
                // ignore
            }
        }
    }

    /**
     * IMPL_NOTE: copied from OtaNotifier. Will be moved to a common part.
     *
     * Formats the username and password for HTTP basic authentication
     * according RFC 2617.
     *
     * @param username for HTTP authentication
     * @param password for HTTP authentication
     *
     * @return properly formated basic authentication credential
     */
    private static String formatAuthCredentials(String username,
                                                String password) {
        byte[] data = new byte[username.length() + password.length() + 1];
        int j = 0;

        for (int i = 0; i < username.length(); i++, j++) {
            data[j] = (byte)username.charAt(i);
        }

        data[j] = (byte)':';
        j++;

        for (int i = 0; i < password.length(); i++, j++) {
            data[j] = (byte)password.charAt(i);
        }

        return "Basic " + Base64.encode(data, 0, data.length);
    }

}

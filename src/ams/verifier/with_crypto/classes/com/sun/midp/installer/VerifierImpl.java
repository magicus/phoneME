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

package com.sun.midp.installer;

import java.io.InputStream;
import java.io.IOException;
import java.util.Vector;

import javax.microedition.io.Connector;
import javax.microedition.pki.CertificateException;

import com.sun.midp.pki.X509Certificate;
import com.sun.midp.pki.AuthorityInfoAccessEntry;

import com.sun.midp.pki.ocsp.OCSPValidator;
import com.sun.midp.pki.ocsp.CertStatus;
import com.sun.midp.pki.ocsp.OCSPValidatorImpl;
import com.sun.midp.pki.ocsp.OCSPException;

import com.sun.midp.publickeystore.WebPublicKeyStore;
import com.sun.midp.publickeystore.PublicKeyInfo;

import com.sun.midp.crypto.PublicKey;
import com.sun.midp.crypto.Signature;
import com.sun.midp.crypto.GeneralSecurityException;

import com.sun.midp.security.Permissions;

import com.sun.midp.io.j2me.storage.RandomAccessStream;
import com.sun.midp.io.Base64;

/**
 * Verifier that is able to verify midlet suite's signature.
 * It is used when the crypto code is present in the build.
 */
public class VerifierImpl implements Verifier {
    /**
     * Current installation state.
     */
    InstallState state;

    /**
     * Authorization Path: A list of authority names from the verification,
     * begining with the most trusted.
     */
    private String[] authPath;

    /** Authenticated content provider certificate. */
    private X509Certificate cpCert;

    /** Online Certificate Status Protocol validator. */
    private OCSPValidator certValidator;

    /**
     * True if the certificates used to sign the midlet suite being
     * installed must be checked using Online Certificate Status
     * Protocol (if enabled at the build-time), false otherwise.
     */
    private boolean isOCSPEnabled;

    /**
     * Constructor.
     *
     * @param installState current state of the installation
     */
    public VerifierImpl(InstallState installState) {
        state = installState;
    }

    /**
     * Checks to see if the JAD has a signature, but does not verify the
     * signature.
     *
     * @return true if the JAD has a signature
     */
    public boolean isJadSigned() {
        return state.getAppProperty(SIG_PROP) != null;
    }

    /**
     * Looks up the domain of a MIDlet suite.
     *
     * @param ca CA of an installed suite
     *
     * @return security domain of the MIDlet suite
     */
    public String getSecurityDomainName(String ca) {
        Vector keys;
        String domain;

        /*
         * look up the domain owner, then get the domain from the
         * trusted key store and set the security domain
         */
        try {
            keys = WebPublicKeyStore.getTrustedKeyStore().
                         findKeys(ca);

            domain = ((PublicKeyInfo)keys.elementAt(0)).getDomain();
        } catch (Exception e) {
            domain = Permissions.UNIDENTIFIED_DOMAIN_BINDING;
        }

        return domain;
    }

    /**
     * Verifies a Jar. On success set the name of the domain owner in the
     * install state. Post any error back to the server.
     *
     * @param jarStorage System store for applications
     * @param jarFilename name of the jar to read.
     *
     * @return authorization path: a list of authority names begining with
     *         the most trusted, or null if jar is not signed
     *
     * @exception IOException if any error prevents the reading
     *   of the JAR
     * @exception InvalidJadException if the JAR is not valid or the
     *   provider certificate is missing
     */
    public String[] verifyJar(RandomAccessStream jarStorage,
            String jarFilename) throws IOException, InvalidJadException {
        InputStream jarStream;
        String jarSig;

        jarSig = state.getAppProperty(SIG_PROP);
        if (jarSig == null) {
            // no signature to verify
            return null;
        }

        authPath = null;

        // This will fill in the cpCert and authPath fields
        findProviderCert();

        jarStorage.connect(jarFilename, Connector.READ);

        try {
            jarStream = jarStorage.openInputStream();

            try {
                verifyStream(jarStream, jarSig);
            } finally {
                jarStream.close();
            }
        } finally {
            jarStorage.disconnect();
        }

        return authPath;
    }

    /**
     * Enables or disables certificate revocation checking using OCSP.
     *
     * @param enable true to enable OCSP checking, false - to disable it
     */
    public void enableOCSPCheck(boolean enable) {
        isOCSPEnabled = enable;
    }

    /**
     * Returns true if OCSP certificate revocation checking is enabled,
     * false if it is disabled.
     *
     * @return true if OCSP checking is enabled, false otherwise
     */
    public boolean isOCSPCheckEnabled() {
        return isOCSPEnabled;
    }

    /**
     * Find the first provider certificate that is signed by a known CA.
     * Set the lastCA field to name of the CA. Set the cpCert field to the
     * provider certificate.
     *
     * IMPL_NOTE: in the case of erroneous certificate chains the first
     *            chain error will be thrown.
     *
     * @exception InvalidJadException if the JAR is not valid or the
     *   provider certificate is missing or a general certificate error
     */
    private void findProviderCert() throws InvalidJadException {
        int chain;
        int result;
        InvalidJadException pendingException = null;

        for (chain = 1; ; chain++) {
            // sets the authPath and cpCert
            try {
                result = checkCertChain(chain);
            } catch (InvalidJadException ije) {
                // According to the spec, if some chain is invalid and
                // the next chain exists, it should also be verified;
                // the first valid chain should be used for the jar
                // verification.
                if (pendingException == null) {
                    pendingException = ije;
                }
                continue;
            }

            if (result == 1) {
                // we found the good chain
                return;
            }

            if (result == -1) {
                // chain not found, done
                break;
            }
        }

        if (pendingException != null) {
            throw pendingException;
        }

        if (chain == 1) {
            throw new
                InvalidJadException(InvalidJadException.MISSING_PROVIDER_CERT);
        }

        // None of the certificates were issued by a known CA
        throw new
            InvalidJadException(InvalidJadException.UNKNOWN_CA,
                                authPath[0]);
    }

    /**
     * Check to see if a provider certificate chain is issued by a known
     * CA. Set the authPath field to names of the auth chain in any case.
     * Authenticate the chain and set the cpCert field to the provider's
     * certificate if the CA is known.
     *
     * @param chainNum the number of the chain
     *
     * @return 1 if the CA of the chain is known, 0 if not, -1 if the
     *    chain is not found
     *
     * @exception InvalidJadException if something other wrong with a
     *   other than an unknown CA
     */
    private int checkCertChain(int chainNum)
            throws InvalidJadException {
        int certNum;
        Vector derCerts = new Vector();
        String base64Cert;
        byte[] derCert;
        WebPublicKeyStore keyStore;
        Vector keys;
        PublicKeyInfo keyInfo;

        for (certNum = 1; ; certNum++) {
            base64Cert = state.getAppProperty(CERT_PROP +
                                              chainNum + "-" + certNum);
            if (base64Cert == null) {
                break;
            }

            try {
                derCert = Base64.decode(base64Cert);
                derCerts.addElement(X509Certificate.generateCertificate(
                    derCert, 0, derCert.length));
            } catch (Exception e) {
                throw new InvalidJadException(
                    InvalidJadException.CORRUPT_PROVIDER_CERT);
            }
        }

        if (certNum == 1) {
            // Chain not found
            return -1;
        }

        Vector issuer = new Vector(1);

        try {
            keyStore = WebPublicKeyStore.getTrustedKeyStore();
            authPath = X509Certificate.verifyChain(derCerts,
                            X509Certificate.DIGITAL_SIG_KEY_USAGE,
                            X509Certificate.CODE_SIGN_EXT_KEY_USAGE,
                            keyStore, issuer);
        } catch (CertificateException ce) {
            switch (ce.getReason()) {
            case CertificateException.UNRECOGNIZED_ISSUER:
                authPath = new String[1];
                authPath[0] = ce.getCertificate().getIssuer();

                // Issuer not found
                return 0;

            case CertificateException.EXPIRED:
            case CertificateException.NOT_YET_VALID:
                throw new InvalidJadException(
                    InvalidJadException.EXPIRED_PROVIDER_CERT,
                    ce.getCertificate().getSubject());

            case CertificateException.ROOT_CA_EXPIRED:
                throw new InvalidJadException(
                    InvalidJadException.EXPIRED_CA_KEY,
                    ce.getCertificate().getIssuer());
            }

            throw new InvalidJadException(
                InvalidJadException.INVALID_PROVIDER_CERT,
                ce.getCertificate().getSubject());
        }

        // The root CA may have been disabled for software authorization.
        keys = keyStore.findKeys(authPath[0]);
        keyInfo = (PublicKeyInfo)keys.elementAt(0);

        if (!keyInfo.isEnabled()) {
            throw new InvalidJadException(
                InvalidJadException.CA_DISABLED,
                authPath[0]);
        }

        if (isOCSPEnabled) {
            /*
             * If USE_OSCP=true, the following code checks the certificate's
             * validity using Online Certificate Status Protocol.
             * Otherwise, certValidator.checkCertStatus() always returns
             * CertStatus.GOOD.
             */

            if (certValidator == null) {
                certValidator = new OCSPValidatorImpl();
            }

            /*
             * Go through the authorization path and send OCSP requests
             * begin with the most trusted certificate.
             */
            for (int i = 0; i < derCerts.size(); i++) {
                int status;
                X509Certificate cert = (X509Certificate)
                    derCerts.elementAt(derCerts.size() - i - 1);

                if (!cert.hasAuthorityInfoAccess()) {
                    /*
                     * Don't use OCSP for the certificates that have not
                     * AuthorityInfoAccess extension. 
                     */
                    continue;
                } else {
                    /*
                     * Searching for an entry in AuthorityInfoAccess having
                     * access method with OID == id-ad-ocsp.
                     */
                    Vector authInfoAccess = cert.getAuthorityInfoAccess(
                            AuthorityInfoAccessEntry.ACCESS_METHOD_OCSP);

                    if (authInfoAccess == null || authInfoAccess.size() == 0) {
                        // unsupported access method
                        throw new InvalidJadException(
                            InvalidJadException.UNKNOWN_CERT_STATUS,
                                cert.getSubject());
                    }
                }

                try {
                    status = certValidator.checkCertStatus(cert,
                        (i == 0 ? (X509Certificate)issuer.elementAt(0) :
                              (X509Certificate)
                                  derCerts.elementAt(derCerts.size() - i)));
                } catch (OCSPException ocspEx) {
                    /*
                     * IMPL_NOTE: exception with some status other then
                     * UNKNOWN_CERT_STATUS should be thrown here to allow
                     * the caller to display a proper message to the user.
                     */
                    status = CertStatus.UNKNOWN;
                }

                if (status == CertStatus.REVOKED) {
                    throw new InvalidJadException(
                        InvalidJadException.REVOKED_CERT, cert.getSubject());
                } else if (status != CertStatus.GOOD) {
                    throw new InvalidJadException(
                        InvalidJadException.UNKNOWN_CERT_STATUS,
                            cert.getSubject());
                }
            }
        }

        cpCert = (X509Certificate)derCerts.elementAt(0);

        // Authenticated
        return 1;
    }

    /**
     * Common routine that verifies a stream of bytes.
     * The cpCert field must be set before calling.
     *
     * @param stream stream to verify
     * @param base64Signature The base64 encoding of the PKCS v1.5 SHA with
     *        RSA signature of this stream.
     *
     * @exception NullPointerException if the public keystore has not been
     *            established.
     * @exception InvalidJadException the JAR signature is not valid
     * @exception IOException if any error prevents the reading
     *   of the JAR
     */
    private void verifyStream(InputStream stream, String base64Signature)
            throws InvalidJadException, IOException {
        PublicKey cpKey;
        byte[] sig;
        Signature sigVerifier;
        byte[] temp;
        int bytesRead;
        byte[] hash;

        try {
            cpKey = cpCert.getPublicKey();
        } catch (CertificateException e) {
            throw new
                InvalidJadException(InvalidJadException.INVALID_PROVIDER_CERT);
        }

        try {
            sig = Base64.decode(base64Signature);
        } catch (IOException e) {
            throw new
                InvalidJadException(InvalidJadException.CORRUPT_SIGNATURE);
        }

        try {
            // verify the jad signature
            sigVerifier = Signature.getInstance("SHA1withRSA");
            sigVerifier.initVerify(cpKey);

            temp = new byte[1024];
            for (; ; ) {
                bytesRead = stream.read(temp);
                if (bytesRead == -1) {
                    break;
                }

                sigVerifier.update(temp, 0, bytesRead);
            }

            if (!sigVerifier.verify(sig)) {
                throw new
                    InvalidJadException(InvalidJadException.INVALID_SIGNATURE);
            }
        } catch (GeneralSecurityException e) {
            throw new
                InvalidJadException(InvalidJadException.INVALID_SIGNATURE);
        }
    }
}

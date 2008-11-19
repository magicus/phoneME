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

package javax.microedition.pki;

import javax.microedition.securityservice.CMSMessageSignatureServiceException;
import javax.microedition.pki.UserCredentialManagerException;

import com.sun.j2me.main.Configuration;

/**
 * This class provides functionality for user credential management
 * which includes 
 * creating certificate signing requests, adding user credentials,
 * and removing credentials that may be used to generate digital
 * signatures as specified in the 
 * <code>CMSMessageSignatureService</code> class.
 * The initial version of credential management supports
 * X.509 version 3 Certificates and URIs that resolve to X.509
 * Certificates.
 * <p>
 * In a public key cryptographic system, a user has two distinct keys.
 * One key is kept private while the other is made public. There are
 * a number of public key cryptographic systems, some of which may
 * be used for the creation of digital signatures (for example, DSA), while
 * others can be used for encryption (for example, Rabin). Some systems may
 * be used for both encryption and digital signatures (for example, RSA).
 * Generally the private key, which is only known to the user,
 * is used to generate a signature or to decrypt a message. The
 * public key, as can be deduced from the name, is public knowledge
 * and is used to verify the user's signature or to encrypt
 * information intended for the user.
 * </p>
 *
 * <p>
 * When selecting a public key to encrypt a message or verify a
 * digital signature, it is important to be able to link the public
 * key either to the user for which the encrypted message is intended or to
 * the user that generated the signature. Public key infrastructure
 * (PKI) provides a mechanism for binding an identity to a public
 * key. The binding is expressed in a data structure known as a
 * certificate. The X.509 certificate format is one of the most
 * widely adopted certificate formats. X.509 certificates generally
 * contain at least the following information:
 * <ul>
 * <li>
 * An X.500 name that can potentially be linked to the identity
 * of the user.
 * </li>
 * <li>
 * The public key associated with the X.500 name.
 * </li>
 * <li>
 * A validity period for the certificate (Not Before and Not After).
 * </li>
 * <li>
 * Information on the certificate issuer (an X.500 name for the
 * issuer and a serial number). This uniquely identifies a
 * certificate in a PKI.
 * </li>
 * </ul>
 *
 * The certificate may contain additional information. The certificate
 * itself is signed by the certificate issuer. The certificate issuer
 * is usually referred to as a Certificate Authority (CA).
 * </p>
 *
 * <p>
 * The process that a CA follows before issuing a certificate is
 * governed by the certification practice statement (CPS) of the
 * CA. This usually involves both technical and non-technical
 * steps that need to be completed before a certificate is issued.
 * Technical steps include obtaining the public key that must be
 * certified, verifying that the user is in possession of the
 * corresponding private key, and returning the certificate or a
 * reference to the certificate once it is issued. Non-technical
 * steps include the processes followed to establish the
 * certificate requesters identity.
 * Upon completion of the technical and non-technical steps of the
 * registration process, the user is said to be enrolled into the PKI.
 * </p>
 *
 * <p>
 * The purpose of this class is to provide the technical building
 * blocks required to enroll a user in a PKI. This will allow a
 * user to obtain a certificate that can be used in conjunction
 * with the <code>sign</code> and <code>authenticate</code> methods
 * in the <code>
 * javax.microedition.securityservice.CMSMessageSignatureService
 * </code> class.
 * This can also be used for
 * renewing and deleting certificates once they have expired. 
 * With this package it is possible to:
 * <ul>
 * <li>
 * Obtain a certificate signing request that can be sent to a PKI.
 * </li>
 * <li>
 * Add a certificate or certificate URI to a certificate store.
 * </li>
 * <li>
 * Remove a certificate or certificate URI from a certificate store.
 * </li>
 * </ul>
 * </p>
 *
 * <h2>Example</h2>
 *
 * <pre>
 *    // Parameters for certificate request message.
 *    String nameInfo = new String("CN=User Name");
 *    byte[] enrollmentRequest = null;
 *    int keyLength = 1024;
 *
 *    // User friendly names and prompts.
 *    String securityElementID = new String("Bank XYZ");
 *    String securityElementPrompt = new String
 *        ("Please insert bank XYZ security element before proceeding");
 *    String friendlyName = new String("My Credential");
 *
 *    // Certificate chain and URI from registration response.
 *    byte[] pkiPath; 
 *    String uri;     
 * 
 * 
 *    // Obtain a certificate enrollment request message.
 *    try {
 *        enrollmentRequest = UserCredentialManager.generateCSR
 *            (nameInfo, UserCredentialManager.ALGORITHM_RSA, keyLength,
 *             UserCredentialManager.KEY_USAGE_NON_REPUDIATION, 
 *             securityElementID, securityElementPrompt, false);
 * 
 *        // Send it to a registration server.
 *         ...
 *        // Assign values for pkipath and certificate uri
 *        // from the registration response.
 *         ...
 * 
 *        // Store the certificate on the security element.
 *        UserCredentialManager.addCredential(friendlyName,
 *                                            pkiPath, uri);
 *    } catch (IllegalArgumentException iae) {
 *        iae.printStackTrace();
 *    } catch (NullPointerException npe) {
 *        npe.printStackTrace();
 *    } catch (CMSMessageSignatureServiceException cmse) {
 *        cmse.printStackTrace();
 *    } catch (UserCredentialManagerException pkie) {
 *        pkie.printStackTrace();
 *    }
 *
 * </pre>
 * <p>
 * <h2>Note regarding UI implementations</h2>
 * </p>
 * <p>
 * User prompts and notifications should be implemented in such a way that:
 * <ul>
 * <li>
 * the UI is distinguishable from a UI generated by external sources 
 * (for example J2ME applications).
 * </li>
 * <li>
 * external sources are not able to modify the data presented to the user.
 * </li>
 * <li>
 * external sources are not able to retrieve the PIN data.
 * </li>
 * </ul>
 * </p>
 */
final public class UserCredentialManager
{
    /**
     * Algorithm identifier for an RSA signature key.
     * This is the <code>String</code> representation of the
     * OID identifying the RSA algorithm.
     */
    public final static String ALGORITHM_RSA = "1.2.840.113549.1.1";

    /**
     * Algorithm identifier for a DSA signature key.
     * This is the <code>String</code> representation of the
     * OID identifying a DSA signature key.
     */
    public final static String ALGORITHM_DSA = "1.2.840.10040.4.1";

    /**
     * Indicates a key used for authentication.
     */
    public final static int KEY_USAGE_AUTHENTICATION = 0;

    /**
     * Indicates a key used for digital signatures.
     */
    public final static int KEY_USAGE_NON_REPUDIATION = 1;

    /**
     * Indicates that key generation is supported
     * on the platform.
     */
    private static boolean keygen = false;

    static  {
	String generation  = Configuration
	    .getProperty("com.sun.satsa.keygen");
    if (generation != null) {
	    keygen = generation.equals("true");
	}
    }
    
    /**
     * Constructor for the <code>UserCredentialManager</code> class.
     */
    private UserCredentialManager() {
    }

    /**
     * Creates a DER encoded PKCS#10 certificate enrollment request.
     * <p>
     * The implementation uses the <code>securityElementID</code>
     * and the <code>securityElementPrompt</code> to
     * choose an appropriate security element. 
     * If an appropriate
     * security element cannot be found, a
     * <code>UserCredentialManagerException</code> is thrown and
     * the <code>getReason</code> method MUST return
     * <code>SE_NOT_FOUND</code>.
     * The implementation MUST use the <code>algorithm</code>,
     * <code>keyLen</code>, and <code>keyUsage</code> parameters
     * to select a specific key to use in signing the 
     * certificate request.
     * If the algorithm is not supported or the specific key
     * parameters can not be fulfilled,  then a
     * <code>UserCredentialManagerException</code> MUST be thrown
     * and the <code>getReason</code> method MUST return
     * <code>SE_NO_KEYS</code>. 
     * </p>
     *
     * <p>
     * If the platform can select a security element and the security
     * element contains multiple keys, it is up to the platform to
     * select an appropriate key
     * (when key generation is not forced). 
     *
     * If a key is found that is not yet associated with a user
     * certificate or a user certificate request, the platform MUST
     * select such a key in preference to keys that are already
     * associated with a user certificate or certificate request. 
     *
     * If all keys are associated with a certificate or a certificate
     * request, the implementation MUST select the key associated with
     * a certificate request in preference to keys that are 
     * associated with a certificate.
     *
     * If all keys are already associated with a user certificate and
     * key generation was not forced, the platform MAY select one
     * of the existing keys for inclusion in the certificate signing
     * request, depending on the security element policy.
     * </p>
     *
     * <p>
     * The application requests key generation by setting the
     * <code>forceKeyGen</code> flag. 
     * If a key is requested of a security element that is not 
     * capable of key generation, a
     * <code>UserCredentialManagerException</code> MUST be thrown
     * and the <code>getReason</code> method MUST return
     * <code>SE_NO_KEYGEN</code>. If the key can not be 
     * generated with the requested key parameters, a
     * <code>UserCredentialManagerException</code> MUST be thrown
     * and the <code>getReason</code> method MUST return
     * <code>SE_NO_KEYS</code>.
     * If the security element requires the user to specify a new PIN
     * that is used to protect the keys to be generated, the implementation
     * of this method is responsible for collecting the new PIN from the user.
     * </p>
     *
     * <p>
     * If a security element is found, but it contains no 
     * keys that can be used, then a
     * <code>UserCredentialManagerException</code> MUST be thrown
     * and the <code>getReason</code> method MUST return
     * <code>SE_NO_KEYS</code>.
     * If a security element is found, but all available 
     * keys have been associated with certificates and
     * if the platform does not allow selection of keys already
     * associated with certificates, then a
     * <code>UserCredentialManagerException</code> MUST be thrown
     * and the <code>getReason</code> method MUST return
     * <code>SE_NO_UNASSOCIATED_KEYS</code>.
     * </p>
     * <p>
     * If a security element can be selected and an appropriate
     * key is available (either generated or already existing)
     * the certification request is generated and formatted. The
     * certification request will be formatted as a PKCS#10
     * certificate request. The request may contain
     * additional attributes.
     * </p>
     * <p>
     * See "X.690 - Information technology - ASN.1 encoding rules:
     * Specification of Basic Encoding Rules (BER),
     * Canonical Encoding Rules (CER) and
     * Distinguished Encoding Rules (DER)" at
     * <a href=
     * "http://www.itu.int/ITU-T/studygroups/com17/languages/">
     * http://www.itu.int/ITU-T/studygroups/com17/languages/
     * </a> for details about ASN.1 encoding rules.
     * 
     * </p>
     * <p>
     * Generation of the certificate enrollment request and the key pair 
     * must be confirmed by the user. The user should have the option
     * to view the detailed information of the key used in signing the
     * certificate request, such as
     * the key usage, key length, public key algorithm.
     * This method returns <code>null</code> if the user cancels the
     * certificate enrollment request.
     * </p>
     * <p>
     * Authorization to generate certificate enrollment request is
     * also subject to the policy
     * of the underlying security element. If user authorization is required 
     * through the entry of PIN, the implementation of this method 
     * is responsible for collecting the PIN from the user. Incorrect
     * PIN entry is handled by
     * the implementation. The number of retries following incorrect
     * PIN entry is
     * governed by the security element policy. If the PIN is blocked due to an 
     * excessive number of incorrect PIN entries, the implementation
     * must throw a
     * <code>SecurityException</code> exception.
     * </p>
     * @param nameInfo the distinguished name
     * to be included in the PKCS#10 certificate signing request.
     * The distinguished name MUST follow the encoding rules of
     * <a href="http://www.ietf.org/rfc/rfc2253.txt">
     * RFC2253</a>. If <code>null</code> is passed as the
     * parameter value, it is up to the implementation to
     * choose an appropriate distinguished name (for example, the WIM
     * serial number).
     * If <code>nameInfo</code> is empty or not formatted according
     * RFC2253 an <code>IllegalArgumentException</code> is thrown.
     *
     * @param algorithm the Object Identifier (OID) for the public key
     * algorithm to use. (see     
     * <a href="http://www.ietf.org/rfc/rfc1778.txt">RFC 1778</a>)
     * The static variables
     * <code>UserCredentialManager.ALGORITHM_RSA</code> and
     * <code>UserCredentialManager.ALGORITHM_DSA</code> may
     * be used to indicate either RSA or DSA signature keys.
     * If <code>algorithm</code> is empty or not formatted according
     * RFC1778 an <code>IllegalArgumentException</code> is thrown.
     * If the requested <code>algorithm</code> is not supported
     * on the platform then a
     * <code>UserCredentialManagerException</code> MUST be thrown
     * and the <code>getReason</code> method MUST return
     * <code>SE_NO_KEYS</code>.  
     *
     * @param keyLen  the key length (typically 1024 for RSA)
     * If <code>keyLen</code> is incorrect an 
     * <code>IllegalArgumentException</code> is thrown.
     *
     * @param keyUsage the functionality for which the key is marked
     * inside the security element. This may be one of
     * <code>UserCredentialManager.KEY_USAGE_AUTHENTICATION</code>
     * or
     * <code>UserCredentialManager.KEY_USAGE_NON_REPUDIATION</code>.
     * If <code>keyUsage</code> is incorrect an 
     * <code>IllegalArgumentException</code> is thrown.
     *
     * @param securityElementID  identifies the security element on
     * which the key resides or will be generated. 
     * If this parameter is
     * <code>null</code> the implementation MUST choose the first
     * available security element that meets the specified requirements.
     * If no appropriate security element is found, the
     * <code>securityElementPrompt</code> parameter MUST be used
     * to guide the user on selecting the correct security element.
     * If no security element can be selected, a
     * <code>UserCredentialManagerException</code> is thrown and
     * the <code>getReason</code> method MUST return
     * <code>SE_NOT_FOUND</code>.
     *
     * @param securityElementPrompt guides a user to insert the
     * correct security element, if the suitable security element is
     * removable and not detected. 
     * If this parameter is set to <code>null</code>, a user prompt is not
     * used to guide the user to select an appropriate security element.
     *
     * @param forceKeyGen if set to <code>true</code> a new key MUST be
     * generated. If the security element does not support key generation
     * it MUST throw an <code>UserCredentialManagerException</code> and
     * the <code>getReason</code> method MUST return a
     * <code>SE_NO_KEYGEN</code> error code.
     * If set to <code>false</code> no key
     * generation is required and an existing key may be used.
     *
     * @return DER encoded PKCS#10 certificate enrollment request, or
     * <code>null</code> if the certificate
     * enrollment request was cancelled by the user before completion.
     * @throws IllegalArgumentException if the parameters are not valid
     * @throws UserCredentialManagerException if an error occurs while
     * generating the certificate request
     * @throws SecurityException if the caller is not
     * authorized to access the user certificate store
     * @throws  CMSMessageSignatureServiceException if an
     * error occurs while signing the certificate request
    */
    public static byte[] generateCSR(String nameInfo, 
                                     String algorithm,
                                     int keyLen, 
                                     int keyUsage, 
                                     String securityElementID,
                                     String securityElementPrompt, 
                                     boolean forceKeyGen)
        throws UserCredentialManagerException,
	CMSMessageSignatureServiceException
    {

	/* User requested a new key be generated. */
	if (forceKeyGen) {
	    if (! UserCredentialManager.keygen) {
		// Configuration parameter disabled key generation.
		throw new UserCredentialManagerException
		    (UserCredentialManagerException.SE_NO_KEYGEN);
	    }
	}

        return com.sun.satsa.pki.PKIManager.generateCSR(nameInfo,
                    algorithm, keyLen, keyUsage, securityElementID,
                    securityElementPrompt, forceKeyGen);
    }


    /**
     * Adds a user certificate or certificate
     * URI to a certificate store.
     * <p>
     * A credential is registered using an ordered sequence of
     * certificates, called a PKI Path.
     * The PKI Path is defined in <A HREF=
     * "ftp://ftp.bull.com/pub/OSIdirectory/DefectResolution/TechnicalCorrigenda/ApprovedTechnicalCorrigendaToX.509/8%7CX.509-TC3%284th%29.pdf">
     * ITU-T RECOMMENDATION X.509 (2000) | ISO/IEC 9594-8:2001, 
     * Technical Corrigendum 1 (DTC 2) </A> and is used
     * by the J2SE <A HREF=
     * "http://java.sun.com/j2se/1.4.1/docs/guide/security/certpath/CertPathProgGuide.html#AppA">
     * CertPath Encodings</A>.</p>
     * <p>
     * <strong>PkiPath:</strong> an ASN.1 DER encoded sequence of
     * certificates, defined as follows:<pre>
     *      <code>PkiPath ::= SEQUENCE OF Certificate</code>
     * </pre>
     * Within the sequence, the order of certificates is such that the
     * subject of the first certificate is the issuer of the second
     * certificate, etc. Each certificate in PkiPath shall be
     * unique. No certificate may appear more than once in a value of
     * Certificate in PkiPath. The last certificate is the end entity
     * user certificate.
     * </p>
     * <p>
     * The use of the certificate URI is platform dependent.
     * Some platforms may not store the user certificate, but may
     * instead keep a copy of the URI. 
     * If only the URI is retained instead of the certificates
     * included in the pkiPath, the implementation MUST parse the user
     * certificate in the pkiPath to obtain relevant information such
     * as the issuing CA name, the user certificate serial number, the
     * public key, and user distinguished name. Some of these fields
     * may be required by the underlying security element.
     * The certificate URI parameter can be
     * <code>null</code> in deployments where off device access to
     * certificate storage is not supported.
     * </p>
     * <p>
     * Some platforms MAY store the credential information with
     * a specific security element, while other platforms MAY
     * have a central repository for credentials. It is platform
     * dependent where the information is maintained.
     * </p>
     * <p> 
     * Storing the requested credential must be confirmed by the
     * user. The implementation must display the user friendly
     * name of the certificate or the certificate URL. The user should
     * have the option
     * to view the detailed information of the credential, such as the
     * certificate issuer, certificate subject, and certificate
     * validity period.
     * This method returns <code>false</code> if the user cancels the
     * request to add the credential.
     * </p>
     * <p>
     * Authorization to store the requested credential is also subject
     * to the policy
     * of the underlying security element or the platform. If user
     * authorization is required
     * through the entry of PIN, the implementation of this method 
     * is responsible for collecting the PIN from the user. Incorrect
     * PIN entry is handled by
     * the implementation. The number of retries following incorrect
     * PIN entry is
     * governed by the policy of the security element or the
     * platform. If the PIN is blocked due to an
     * excessive number of incorrect PIN entries, the implementation
     * must throw a
     * <code>SecurityException</code> exception.
     * </p>
     * If the requested certificate can not be stored, a
     * <code>UserCredentialManagerException</code> is thrown and
     * the <code>getReason</code> method MUST return
     * <code>CREDENTIAL_NOT_SAVED</code>.
     * @param certDisplayName the user friendly
     * name associated with the certificate.
     * If <code>certDisplayName</code> is <code>null</code> or
     * an empty string an <code>IllegalArgumentException</code>
     * is thrown. Applications MUST use unique user
     * friendly names to make selection easier.
     * If the <code>certDisplayName</code> is already
     * registered, an <code>IllegalArgumentException</code>
     * is thrown.
     *
     * @param pkiPath the DER encoded PKIPath containing user
     * certificate and certificate authority certificates.
     * If <code>pkiPath</code> is <code>null</code> or
     * incorrectly formatted an <code>IllegalArgumentException</code>
     * is thrown. 
     * If <code>pkiPath</code> is already registered, 
     * an <code>IllegalArgumentException</code> is thrown. 
     *
     * @param uri a URI that resolves to a X.509v3 certificate.
     * The uri can be <code>null</code>.
     *
     * @throws IllegalArgumentException if parameters are not
     *  valid
     * @throws UserCredentialManagerException if an error occurs
     * while adding a user credential
     * @throws SecurityException if the caller is not
     * authorized to add to the user certificate store
     *
     * @return <code>false</code> if the operation to add the
     * credential was cancelled by the user
     * before completion. 
     */
    public static boolean addCredential(String certDisplayName,
                                        byte[] pkiPath, String uri)
            throws UserCredentialManagerException {
        return com.sun.satsa.pki.PKIManager.addCredential(certDisplayName,
                pkiPath, uri);
    }

    /**
     * Removes a certificate or certificate
     * URI from a certificate store.
     * <p>
     * Removal of the credential from the certificate store
     * must be confirmed by the user. The implementation must display
     * the user friendly
     * name of the certificate or the certificate URL. The user should
     * have the option
     * to view the detailed information of the credential, such as the
     * certificate issuer, certificate subject, and certificate
     * validity period.
     * This method returns <code>false</code> if the user cancels the
     * request to remove the credential.
     * </p>
     * <p>
     * Authorization to remove the requested credential is also
     * subject to the policy
     * of the underlying security element or the platform. If user
     * authorization is required
     * through the entry of PIN, the implementation of this method 
     * is responsible for collecting the PIN from the user. Incorrect
     * PIN entry is handled
     * by the implementation. The number of retries following
     * incorrect PIN entry is
     * governed by the policy of the security element or the
     * platform. If the PIN is blocked due to an
     * excessive number of incorrect PIN entries, the implementation
     * must throw a
     * <code>SecurityException</code> exception.
     * </p>
     * @param certDisplayName the user friendly
     * name associated with the certificate.
     * If <code>certDisplayName</code> is <code>null</code> or
     * an empty string an <code>IllegalArgumentException</code>
     * is thrown.
     *
     * @param issuerAndSerialNumber the DER encoded ASN.1 structure
     * that contains the certificate issuer and serial number as
     * defined in <a href="http://www.ietf.org/rfc/rfc3369.txt">
     * RFC 3369</a>.
     * If <code>issuerAndSerialNumber</code> is <code>null</code> or
     * not properly formatted according to RFC3369
     * an <code>IllegalArgumentException</code> is thrown.
     * If the requested certificate is not found, a
     * <code>UserCredentialManagerException</code> is thrown and
     * the <code>getReason</code> method MUST return
     * <code>CREDENTIAL_NOT_FOUND</code>.
     *
     * @param securityElementID identifies the security element on
     * which the key resides.  If this parameter is
     * <code>null</code> the implementation MUST choose the first
     * available security element that meets the specified requirements.
     * If no appropriate security element is found, the
     * <code>securityElementPrompt</code> parameter MUST be used
     * to guide the user on selecting the correct security element.
     * If no security element can be selected, a
     * <code>UserCredentialManagerException</code> is thrown and
     * the <code>getReason</code> method MUST return
     * <code>SE_NOT_FOUND</code>.
     *
     * @param securityElementPrompt  guides the user to insert 
     * the correct security element if the security element is
     * removable and not detected. 
     * If this parameter is set to <code>null</code>, no information
     * regarding which security element to use is displayed to
     * the user.
     *
     * @throws IllegalArgumentException if the parameters are not valid
     * @throws UserCredentialManagerException if an error occurs
     * while removing the credential
     * @throws SecurityException if the caller is not
     * authorized to remove from the user certificate store
     * @return <code>false</code> if the operation to remove the 
     * credential was cancelled by the user
     * before completion. 
     */
    public static boolean removeCredential(String certDisplayName,
        byte[] issuerAndSerialNumber, String securityElementID,
        String securityElementPrompt)
            throws UserCredentialManagerException {

        return com.sun.satsa.pki.PKIManager.removeCredential(
                certDisplayName, issuerAndSerialNumber, securityElementID,
                securityElementPrompt);
    }
}

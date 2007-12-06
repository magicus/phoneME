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

package javax.microedition.securityservice;

import javax.microedition.securityservice.CMSMessageSignatureServiceException;
import javax.microedition.pki.UserCredentialManagerException;

import com.sun.j2me.main.Configuration;

/**
 * Provides signature services for cryptographic messages.
 * <p>
 * Cryptographic operations are frequently required to realize
 * authentication, authorization, integrity, and privacy services.
 * These services may be provided at various layers (for example, 
 * communication layer, application layer, and so on). These services
 * form critical security components of larger systems
 * and services such as e-commerce, e-government, or corporate
 * applications. The layer at which these services are used are
 * governed by the security requirements of the system and the policies
 * used to formulate those requirements.
 * </p>
 *
 * <p>
 * The services of authentication and authorization are often delivered
 * through the use of digital signatures. For a
 * digital signature to be useful, it must be possible to determine
 * what was signed, which algorithms were used, and who generated the
 * signature. Additional information may be included. The combination of
 * this information into a single construct is referred to as a formatted
 * digital signature.
 * </p>
 *
 * <p>
 * A formatted digital signature is well suited to deliver the services
 * of authentication, authorization, and integrity of information at the
 * application layer within numerous systems. The generation of a formatted
 * digital signature involves the use of multiple cryptographic
 * operations including random number generation, hash generation, and the
 * application of a suitable signature algorithm. The cryptographic
 * operations are often performed on a security element that is trusted for
 * secure storage of cryptographic keys and secure computation of cryptographic
 * operations. The result of these cryptographic operations is combined with
 * information such as the user's identity and the data on which the
 * cryptographic operations are performed. This is then combined and presented
 * in a specific format that is expressed in ASN.1 or XML. The result is
 * referred to as a formatted digital signature. Examples of formatted digital
 * signatures include PKCS#7, CMS, and XML Digital Signature. This 
 * class supports
 * signature messages that conform to the Cryptographic Message Syntax
 * (CMS) format as specified in
 * <a href="http://www.ietf.org/rfc/rfc2630.txt">RFC 2630</a>
 * with enhanced security services for 
 * <a href="http://www.ietf.org/rfc/rfc2634.txt">RFC 2634</a>.
 * </p>
 *
 * <p>
 * The complexity of generating a formatted digital signature is reduced
 * through the use of a high-level interface. The implementation 
 * is responsible for identifying and managing the appropriate security
 * elements, requesting the required cryptographic operations from the system
 * and security element, as well as performing the appropriate formatting of the
 * results. The advantages of this interface is twofold. First, the
 * complexity of
 * generating a formatted digital signature is removed from the application
 * developer. Second, the size of the implementation can be
 * drastically reduced, removing
 * the need for separate packages to access and manage security elements,
 * perform specific cryptographic operations, and formatting the results
 * appropriately.
 * </p>
 *
 * <p>
 * This class provides a compact and high-level cryptographic
 * interface that utilizes security elements available on a J2ME device.
 * </p>
 *
 * <p>
 * In this version of the interface, signature generation is defined for
 * authentication and authorization purposes. The <code>sign</code>
 * and <code>authenticate</code> methods return CMS-formatted
 * signatures. The signed message can be constructed with content
 * included (opaque signature) or without the content (detached 
 * signature).
 * </p>
 *
 * <p>
 * For the purpose of this interface, a security element is a device
 * or part of a
 * device that is trusted to provide secure storage of user credentials
 * and certificates as well as cryptographic keys. In addition, such a
 * device is trusted to perform secure computation involving
 * the cryptographic keys that are securely stored on the device.
 * If a requested security element can not be found, a
 * <code>UserCredentialManagerException</code> is thrown and
 * the <code>getReason</code> method MUST return
 * <code>UserCredentialManagerException.SE_NOT_FOUND</code>.
 * </p>
 *
 * <p>
 * A device may have multiple security elements. The security elements may be
 * removable. Public information such as the user credential and a reference to
 * the corresponding security element may be cached on the J2ME device.
 * </p>
 *
 * <p>
 * Authorization of the use of a key in a security element will be governed by
 * the policy of the security element (for example, no authorization
 * required, PIN
 * entry required, biometric required, and so on).
 * </p>
 *
 * <p>
 * Cryptographic keys may be marked for different purposes, such as
 * non-repudiation
 * or authentication. The implementation will honor the key
 * usage defined
 * by a security element.
 * </p>
 *
 * <p>
 * This class honors the key usage policy defined by the security
 * element by splitting
 * the generation of formatted digital signatures between two methods,
 * namely the
 * <code>sign</code> and <code>authenticate</code> methods. The sign method is
 * associated with keys marked for digital signature and non-repudiation. The
 * authenticate method is associated with keys marked for digital
 * signature only,
 * or digital signature and a key usage apart from non-repudiation (such as
 * authentication).
 * </p>
 *
 * <p>
 * These two methods, apart from being associated with a specific key
 * usage, also have
 * distinct behavior regarding user interaction. The
 * <code>sign</code> method will always display the text that is
 * about to be signed. The <code>authenticate</code> method
 * is overloaded and does not display the data that is about to be
 * signed, if that data is passed as a <code>byte</code> array. If the
 * data that is about to be signed is passed as a <code>String</code>,
 * it is always displayed.
 * </p>
 *
 * <p>
 * The <code>sign</code> method should be used for higher-value transactions and
 * authorizations while the <code>authenticate</code> method should be used
 * whenever a digital signature is required to authenticate a user. 
 * </p>
 *
 * <h2>Example Code</h2>
 * <p>
 * This is an example of how this interface may be used to generate a
 * formatted digital
 * signature used for authentication or non-repudiation purposes.
 * </p>
 * <pre>
 *
 * String caName = new String("cn=ca_name,ou=ou_name,o=org_name,c=ie");
 * String[] caNames = new String[1];
 * String stringToSign = new String("JSR 177 Approved");
 * String userPrompt = new String("Please insert the security element "
 *                                + "issued by bank ABC" 
 *                                + "for the application XYZ.");
 * byte[] byteArrayToSign = new byte[8];
 * byte[] authSignature;
 * byte[] signSignature;
 *
 * caNames[0] = caName;
 *
 * try {
 *     // Generate a formatted authentication signature that includes the
 *     // content that was signed in addition to the certificate.
 *     // Selection of the key is implicit in selection of the certificate, 
 *     // which is selected through the caNames parameter.
 *     // If the appropriate key is not found in any of the security 
 *     // elements present in the device, the implementation may guide 
 *     // the user to insert an alternative security element using 
 *     // the securityElementPrompt parameter.
 *     authSignature = CMSMessageSignatureService.authenticate(
 *                  byteArrayToSign,
 *                  CMSMessageSignatureService.SIG_INCLUDE_CERTIFICATE
 *                  |CMSMessageSignatureService.SIG_INCLUDE_CONTENT,
 *                  caNames, userPrompt);
 *
 *     // Generate a formatted signature that includes the
 *     // content that was signed in addition to the certificate.
 *     // Selection of the key is implicit in selection of the certificate, 
 *     // which is selected through the caNames parameter.
 *     // If the appropriate key is not found in any of the 
 *     // security elements present in the device, the implementation 
 *     // may guide the user to insert an alternative
 *     // security element using the securityElementPrompt parameter.
 *     signSignature = CMSMessageSignatureService.sign(
 *                  stringToSign,
 *                  CMSMessageSignatureService.SIG_INCLUDE_CERTIFICATE
 *                  |CMSMessageSignatureService.SIG_INCLUDE_CONTENT,
 *                  caNames, userPrompt);
 *     } catch (IllegalArgumentException iae) {
 *         // Perform error handling
 *         iae.printStackTrace();
 *     } catch (CMSMessageSignatureServiceException ce) {
 *         if (ce.getReason() == ce.CRYPTO_FORMAT_ERROR) {
 *             System.out.println("Error formatting signature.");
 *         } else {
 *             System.out.println(ce.getMessage());
 *         }
 *     }
 *     ...
 * </pre>
 * <p>
 * <h2>Note regarding UI implementations</h2>
 * </p>
 * <p>
 * The user prompts and notifications should be implemented in such a way that:
 * <ul>
 * <li>
 * the UI is distinguishable from a UI generated by external sources 
 * (for example J2ME applications)
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
 
final public class CMSMessageSignatureService
{
    /**
     * Includes the content that was signed in the signature.
     * If this option is specified and the <code>sign</code>
     * and <code>authenticate</code> methods do not support
     * opaque signatures, a
     * <code>CMSMessageSignatureServiceException</code>
     * exception MUST be thrown and the
     * <code>getReason</code> method
     * MUST return the <code>CRYPTO_NO_OPAQUE_SIG</code>
     * error code.
     */
    public static final int SIG_INCLUDE_CONTENT = 1;
    
    /**
     * Includes the user certificate in the signature.
     * If this option is specified and the certificate is not
     * available, a
     * <code>CMSMessageSignatureServiceException</code>
     * exception MUST be thrown and the
     * <code>getReason</code> method
     * MUST return the <code>CRYPTO_NO_CERTIFICATE</code>
     * error code.
     */    
    public static final int SIG_INCLUDE_CERTIFICATE = 2;


    /** Unassigned option bits. */
    static int mask = ~(SIG_INCLUDE_CONTENT|SIG_INCLUDE_CERTIFICATE);


    /**
     * Indicates that certificate inclusion is supported
     * on the platform.
     */
    private static boolean certSig = false;

    static {
        String signprop  = Configuration
            .getProperty("com.sun.satsa.certsig");
        if (signprop != null) {
            certSig = signprop.equals("true");
        }
    }

    /**
     * Constructor for the CMSMessageSignatureService class.
     */
    private CMSMessageSignatureService() {
    }
    
    /**
     * Generates a CMS signed message. 
     * <p> 
     * The signature may be generated using key pairs that are marked
     * for non-repudiation. It is up to
     * the implementation to search the available security
     * elements for relevant keys. Selection of the appropriate
     * key is facilitated by the <code>caNames</code>
     * parameter. Only keys certified by the specified certificate 
     * authority will be eligible for key selection.
     * If the appropriate key is not found in any of the security elements
     * present in the device, the implementation may guide the user to 
     * insert an alternative security element using the 
     * <code>securityElementPrompt</code> parameter.
     * </p>
     *
     * <p>
     * The implementation MUST display the user friendly name of the
     * certificate or the certificate URI to the user. If more than 
     * one certificate is found, the user MUST be presented with a 
     * list of certificate friendly names. It is up to the user to
     * select the appropriate certificate based on the certificate
     * friendly name.
     * </p>
     *
     * <p>
     * The signature format is controlled through the
     * <code>options</code> parameter. If the
     * <code>options</code> parameter is non-zero and is
     * not a valid combination of
     * <code>CMSMessageSignatureService.SIG_INCLUDE_CERTIFICATE</code>
     * and
     * <code>CMSMessageSignatureService.SIG_INCLUDE_CONTENT</code>,
     * an <code>IllegalArgumentException</code> exception
     * MUST be thrown.
     * </p>
     *
     * <p>
     * Before requesting confirmation of the signature through a
     * PIN or some other means, the implementation of this method
     * MUST display the <code>stringToSign</code> to the user.
     * </p>
     *
     * <p>
     * Authorization of the use of the private key to generate a
     * signature is subject to the policy of the underlying
     * security element.
     * If signature authorization is required through the entry
     * of a PIN, the implementation of this method is responsible
     * for collecting the PIN from the user and presenting the
     * PIN to the security element. Incorrect PIN entry is handled
     * by the implementation. The number of retries following
     * incorrect PIN entry is governed by the security element
     * policy. If the PIN is blocked due to
     * an excessive number of incorrect PIN entries, the
     * implementation MUST throw a
     * <code>SecurityException</code> exception.
     * </p>
     *
     * <p>
     * The signature MUST be generated on the UTF-8
     * encoding of the <code>stringToSign</code>.
     * </p>
     *
     * <p>
     * The signature format returned by this method MUST follow
     * the CMS <code><a href="http://www.ietf.org/rfc/rfc2630.txt">
     * </a></code> signature format as specified in RFC2630.
     * </p>
     *
     * @param stringToSign the string that is to be signed
     * @param options the bitwise OR of the following options:
     * <code>CMSMessageSignatureService.SIG_INCLUDE_CONTENT</code> and 
     * <code>CMSMessageSignatureService.SIG_INCLUDE_CERTIFICATE</code>.
     * If the implementation
     * does not support detached signatures
     * (signatures without the original content), 
     * the absence of the
     * <code>CMSMessageSignatureService.SIG_INCLUDE_CONTENT</code>
     * option will result in a
     * <code>CMSMessageSignatureServiceException</code> with a reason
     * code of
     * <code>CRYPTO_NO_DETACHED_SIG</code>. If the implementation
     * does not support opaque signatures and the
     * <code>CMSMessageSignatureService.SIG_INCLUDE_CONTENT</code>
     * option is specified, a
     * <code>CMSMessageSignatureServiceException</code>
     * exception with a reason code of
     * <code>CRYPTO_NO_OPAQUE_SIG</code> MUST be thrown. If
     * certificate URIs are used instead of certificates and the
     * <code>CMSMessageSignatureService.SIG_INCLUDE_CERTIFICATE</code>
     * option is specified, a
     * <code>CMSMessageSignatureServiceException</code> exception
     * with a reason code of <code>CRYPTO_NO_CERTIFICATE</code>
     * MUST be thrown.
     *
     * @param caNames an array of <code>String</code>s that
     * contain the distinguished names of certification
     * authorities that are trusted to issue certificates that
     * may be used for authenticating a user.
     * The distinguished name MUST be formatted according
     * to <a href="http://www.ietf.org/rfc/rfc2253.txt">
     * RFC 2253</a>. If this parameter is set to <code>null</code>
     * or the array is empty,
     * it is up to the implementation to interact with the user to
     * select an appropriate certificate that may be used for
     * digital signatures.
     * If an entry in the <code>caNames</code> array is
     * <code>null</code>, contains an empty String, or
     * is not properly formatted according to RFC2253
     * an <code>IllegalArgumentException</code>
     * is thrown.
     * If, for a given collection of <code>caNames</code> more than
     * one certificate
     * is available, a list of available certificates that are
     * currently valid (i.e. not expired) MUST be displayed to the
     * user for selection. The system clock MUST be used to determine
     * the validity of certificates.
     *
     * @param securityElementPrompt guides a user to insert the
     * correct security element if the security element is removable
     * and not detected. If
     * this parameter is set to <code>null</code> no information
     * regarding which security element to use is displayed to
     * the user.
     * If there are no certificates that can be selected to
     * complete the operation a 
     * <code>CMSMessageSignatureServiceException</code> is thrown
     * and the <code>getReason</code> method
     * MUST return the <code>CRYPTO_NO_CERTIFICATE</code>
     * error code.
     *
     * @return the DER encoded signature,
     * <code>null</code> if the signature generation was cancelled by
     * the user before completion
     * @throws  CMSMessageSignatureServiceException if an
     * error occurs during signature generation
     * @throws IllegalArgumentException if the parameters are not
     * valid
     * @throws SecurityException if the caller is not
     * authorized to sign messages
     * @throws UserCredentialManagerException if a security element
     * is not found
     */
    public static final byte[] sign(
				    String stringToSign, 
				    int options,
				    String[] caNames,
				    String securityElementPrompt) 
	throws CMSMessageSignatureServiceException,
	       UserCredentialManagerException {
	
	/* Validate the options selected. */
	checkOptions(options);

        return com.sun.satsa.pki.PKIManager.sign(
                com.sun.satsa.pki.PKIManager.SIGN_STRING,
                null, stringToSign, options, caNames,
                securityElementPrompt);
    }

    /**
     * Generates a signature that may be used for
     * authentication purposes. If the authentication signature is
     * generated using public key technology, this method may be
     * tied to key pairs marked for digital signature and
     * authentication operations. It is
     * up to the implementation to search the available security
     * elements for relevant keys. Selection of the appropriate key is
     * facilitated by the <code>caNames</code> parameter. 
     * If the appropriate key is not found in any of the security elements
     * present in the device, the implementation may guide the user to
     * insert an alternative security element using the
     * <code>securityElementPrompt</code> parameter.
     *
     * <p>
     * The implementation SHOULD display the user friendly name of the
     * certificate or the certificate URI to the user. If more than
     * one certificate is found, the user SHOULD be presented with a
     * list of certificate friendly names. It is up to the user
     * to select the appropriate certificate based on the certificate
     * friendly name.
     * </p>
     * 
     * 
     * <p>
     * The signature format is controlled through the
     * <code>options</code> parameter. If the
     * <code>options</code> parameter is non-zero and is
     * not a valid combination of
     * <code>CMSMessageSignatureService.SIG_INCLUDE_CERTIFICATE</code>
     * and
     * <code>CMSMessageSignatureService.SIG_INCLUDE_CONTENT</code>, 
     * an <code>IllegalArgumentException</code> exception
     * MUST be thrown.
     * </p>
     *
     * <p>
     * Authorization of the use of the private key to generate a
     * signature is subject to the policy of the underlying
     * security element.
     * If signature authorization is required through the entry of
     * PIN, the implementation of this method is responsible for
     * collecting the PIN from the user and presenting the PIN to
     * the security element. Incorrect PIN entry is handled
     * by the implementation. The number of retries following
     * incorrect PIN entry is governed by the security element
     * policy. If the PIN is blocked due to
     * an excessive number of incorrect PIN entries, the
     * implementation MUST throw a
     * <code>SecurityException</code> exception.
     * </p>
     *
     * <p>
     * This method does not display the data that is about to be
     * signed. The fact that a user is blindly signing information
     * introduces the risk of a man-in-the-middle attack. For this
     * reason it is recommended that access to this method is
     * controlled using the standard access control mechanisms as
     * defined by MIDP.
     * </p>
     *
     * <p>
     * The signature format returned by this method MUST follow
     * the CMS <code><a href=http://www.ietf.org/rfc/rfc2630.txt>
     * </a></code> signature format as specified in RFC 2630.
     * </p>
     *
     * @param byteArrayToAuthenticate the byte array that is to be signed
     * @param options the bitwise OR of the following options:
     * <code>CMSMessageSignatureService.SIG_INCLUDE_CONTENT</code> and 
     * <code>CMSMessageSignatureService.SIG_INCLUDE_CERTIFICATE</code>.
     * If the implementation
     * does not support detached signatures
     * (signatures without the original content), 
     * the absence of the
     * <code>CMSMessageSignatureService.SIG_INCLUDE_CONTENT</code>
     * option will result in a
     * <code>CMSMessageSignatureServiceException</code> with a reason
     * code of
     * <code>CRYPTO_NO_DETACHED_SIG</code>. If the implementation
     * does not support opaque signatures and the
     * <code>CMSMessageSignatureService.SIG_INCLUDE_CONTENT</code>
     * option is specified, a
     * <code>CMSMessageSignatureServiceException</code>
     * exception with a reason code of
     * <code>CRYPTO_NO_OPAQUE_SIG</code> MUST be thrown. If
     * certificate URIs are used instead of certificates and the
     * <code>CMSMessageSignatureService.SIG_INCLUDE_CERTIFICATE</code>
     * option is specified, a
     * <code>CMSMessageSignatureServiceException</code> exception
     * with a reason code of <code>CRYPTO_NO_CERTIFICATE</code>
     * MUST be thrown.
     *
     * @param caNames an array of <code>String</code>s that
     * contain the distinguished names of certification
     * authorities that are trusted to issue certificates that
     * may be used for authenticating a user.
     * The distinguished name MUST be formatted according
     * to <a href="http://www.ietf.org/rfc/rfc2253.txt">
     * RFC 2253</a>. If this parameter is set to <code>null</code>
     * or the array is empty,
     * it is up to the implementation to interact with the user to
     * select an appropriate certificate that may be used for
     * authentication.
     * If an entry in the <code>caNames</code> array is
     * <code>null</code>, contains an empty String, or
     * is not properly formatted according to RFC2253
     * an <code>IllegalArgumentException</code>
     * is thrown.
     * If, for a given collection of <code>caNames</code> more than
     * one certificate
     * is available, a list of available certificates that are
     * currently valid (i.e. not expired) MUST be displayed to the
     * user for selection. The system clock MUST be used to determine
     * the validity of certificates.
     *
     * @param securityElementPrompt guides a user to insert the
     * correct security element if the security element is removable
     * and not detected. If
     * this parameter is set to <code>null</code> no information
     * regarding which security element to use is displayed to
     * the user.
     * If there are no certificates that can be selected to
     * complete the operation a 
     * <code>CMSMessageSignatureServiceException</code> is thrown
     * and the <code>getReason</code> method
     * MUST return the <code>CRYPTO_NO_CERTIFICATE</code>
     * error code.
     *
     * @return the DER encoded signature,
     * <code>null</code> if the signature generation was cancelled by
     * the user before completion
     * @throws  CMSMessageSignatureServiceException if an
     * error occurs during signature generation
     * @throws IllegalArgumentException if the parameters are not
     * valid
     * @throws SecurityException if the caller is not
     * authorized to generate authentication messages
     * @throws UserCredentialManagerException if a security element
     * is not found
     */
    public static final byte[] authenticate(
					    byte[] byteArrayToAuthenticate,
					    int options, 
					    String[] caNames,
					    String securityElementPrompt) 
	throws CMSMessageSignatureServiceException,
	       UserCredentialManagerException {

	/* Validate the options selected. */
	checkOptions(options);

        return com.sun.satsa.pki.PKIManager.sign(
                com.sun.satsa.pki.PKIManager.AUTHENTICATE_DATA,
                byteArrayToAuthenticate, null, options, caNames,
                securityElementPrompt);
    }

    /**
     * Generates a signature that may be used for
     * authentication purposes. If the authentication signature is
     * generated using public key technology, 
     * this method may be tied to key pairs
     * marked for digital signature and authentication operations. It is
     * up to the implementation to search the available security
     * elements for relevant keys. Selection of the appropriate key is
     * facilitated by the <code>caNames</code> parameter. 
     * If the appropriate key is not found in any of the security elements
     * present in the device, the implementation may guide the user to
     * insert an alternative security element using the
     * <code>securityElementPrompt</code> parameter.
     *
     * <p>
     * The implementation MUST display the user friendly name of the
     * certificate or the certificate URI to the user. If more than
     * one certificate is found, the user MUST be presented with a
     * list of certificate friendly names. It is up to the user to
     * select the appropriate certificate based on the certificate
     * friendly name.
     * </p>
     *
     * <p>
     * The signature format is controlled through the
     * <code>options</code> parameter. If the
     * <code>options</code> parameter is non-zero and is
     * not a valid combination of
     * <code>CMSMessageSignatureService.SIG_INCLUDE_CERTIFICATE</code>
     * and
     * <code>CMSMessageSignatureService.SIG_INCLUDE_CONTENT</code>,
     * an <code>IllegalArgumentException</code> exception
     * MUST be thrown.
     * </p>
     *
     * <p>
     * Before requesting confirmation of the signature through a
     * PIN or some other means, the implementation of this method
     * MUST display the <code>stringToAuthenticate</code> to the user.
     * </p>
     *
     * <p>
     * Authorization to use a private key to generate a
     * signature is subject to the policy of the underlying
     * security element.
     * If signature authorization is required through the entry of a
     * PIN, the implementation of this method is responsible for
     * collecting the PIN from the user and presenting the PIN to
     * the security element. Incorrect PIN entry is handled
     * by the implementation. The number of retries following an 
     * incorrect PIN entry is governed by the security element
     * policy. If the PIN is blocked due to
     * an excessive number of incorrect PIN entries, the
     * implementation MUST throw a
     * <code>SecurityException</code> exception.
     * </p>
     *
     * <p>
     * The signature MUST be generated on the UTF-8
     * encoding of the <code>stringToAuthenticate</code>.
     * </p>
     *
     * <p>
     * The signature format returned by this method MUST follow
     * the CMS <code><a href="http://www.ietf.org/rfc/rfc2630.txt">
     * </a></code> signature format as specified in RFC 2630.
     * </p>
     *
     * @param stringToAuthenticate the string that is to be signed
     * @param options the bitwise OR of the following options:
     * <code>CMSMessageSignatureService.SIG_INCLUDE_CONTENT</code> and 
     * <code>CMSMessageSignatureService.SIG_INCLUDE_CERTIFICATE</code>.
     * If the implementation
     * does not support detached signatures
     * (signatures without the original content), 
     * the absence of the
     * <code>CMSMessageSignatureService.SIG_INCLUDE_CONTENT</code>
     * option will result in a
     * <code>CMSMessageSignatureServiceException</code> with a reason
     * code of
     * <code>CRYPTO_NO_DETACHED_SIG</code>. If the implementation
     * does not support opaque signatures and the
     * <code>CMSMessageSignatureService.SIG_INCLUDE_CONTENT</code>
     * option is specified, a
     * <code>CMSMessageSignatureServiceException</code>
     * exception with a reason code of
     * <code>CRYPTO_NO_OPAQUE_SIG</code> MUST be thrown. If
     * certificate URIs are used instead of certificates and the
     * <code>CMSMessageSignatureService.SIG_INCLUDE_CERTIFICATE</code>
     * option is specified, a
     * <code>CMSMessageSignatureServiceException</code> exception
     * with a reason code of <code>CRYPTO_NO_CERTIFICATE</code>
     * MUST be thrown.
     *
     * @param caNames an array of <code>String</code>s that
     * contain the distinguished names of certification
     * authorities that are trusted to issue certificates that
     * may be used for authenticating a user.
     * The distinguished name MUST be formatted according
     * to <a href="http://www.ietf.org/rfc/rfc2253.txt">
     * RFC 2253</a>. If this parameter is set to <code>null</code>
     * or the array is empty,
     * it is up to the implementation to interact with the user to
     * select an appropriate certificate that may be used for
     * authentication.
     * If an entry in the <code>caNames</code> array is
     * <code>null</code>, contains an empty String, or
     * is not properly formatted according to RFC2253
     * an <code>IllegalArgumentException</code>
     * is thrown.
     * If, for a given collection of <code>caNames</code> more than
     * one certificate
     * is available, a list of available certificates that are
     * currently valid (i.e. not expired) MUST be displayed to the
     * user for selection. The system clock MUST be used to determine
     * the validity of certificates.
     *
     * @param securityElementPrompt guides a user to insert the
     * correct security element
     * if the security element is removable and not detected. If
     * this parameter is set to <code>null</code> no information
     * regarding which security element to use is displayed to
     * the user.
     * If there are no certificates that can be selected to
     * complete the operation a 
     * <code>CMSMessageSignatureServiceException</code> is thrown
     * and the <code>getReason</code> method
     * MUST return the <code>CRYPTO_NO_CERTIFICATE</code>
     * error code.
     *
     * @return the DER encoded signature, <code>null</code>
     * if the signature generation was cancelled by the
     * user before completion
     * @throws  CMSMessageSignatureServiceException if an
     * error occurs during signature generation
     * @throws IllegalArgumentException if the parameters are not
     * valid
     * @throws SecurityException if the caller is not
     * authorized to generate authentication messages
     * @throws UserCredentialManagerException if a security element
     * is not found
     */
    public static final byte[] authenticate(
					    String stringToAuthenticate,
					    int options, 
					    String[] caNames,
					    String securityElementPrompt) 
	throws CMSMessageSignatureServiceException,
	       UserCredentialManagerException {
	/* Validate the options selected. */
	checkOptions(options);

        return com.sun.satsa.pki.PKIManager.sign(
                com.sun.satsa.pki.PKIManager.AUTHENTICATE_STRING,
                null, stringToAuthenticate, options, caNames,
                securityElementPrompt);
    }

    /**
     * Checks signing option word. 
     * @param options the bitwise OR of the following options:
     * <code>CMSMessageSignatureService.SIG_INCLUDE_CONTENT</code> and 
     * <code>CMSMessageSignatureService.SIG_INCLUDE_CERTIFICATE</code>.
     * If the implementation
     * does not support detached signatures
     * (signatures without the original content), 
     * the absence of the
     * <code>CMSMessageSignatureService.SIG_INCLUDE_CONTENT</code>
     * option will result in a
     * <code>CMSMessageSignatureServiceException</code> with a reason
     * code of
     * <code>CRYPTO_NO_DETACHED_SIG</code>. If the implementation
     * does not support opaque signatures and the
     * <code>CMSMessageSignatureService.SIG_INCLUDE_CONTENT</code>
     * option is specified, a
     * <code>CMSMessageSignatureServiceException</code>
     * exception with a reason code of
     * <code>CRYPTO_NO_OPAQUE_SIG</code> MUST be thrown. If
     * certificate URIs are used instead of certificates and the
     * <code>CMSMessageSignatureService.SIG_INCLUDE_CERTIFICATE</code>
     * option is specified, a
     * <code>CMSMessageSignatureServiceException</code> exception
     * with a reason code of <code>CRYPTO_NO_CERTIFICATE</code>
     * MUST be thrown.
     * @throws IllegalArgumentException if the <code>options</code>
     * parameters are not valid
     * @throws  CMSMessageSignatureServiceException if an
     * error occurs during signature generation
     */
    static void checkOptions(int options) 
	throws CMSMessageSignatureServiceException {
	/* Check for valid arguments. */
	if ((options & mask) != 0) {
	    throw new IllegalArgumentException("Invalid signing options ");
	}

	if ((options & CMSMessageSignatureService. SIG_INCLUDE_CERTIFICATE)
	    == CMSMessageSignatureService.SIG_INCLUDE_CERTIFICATE) {
	    if (!CMSMessageSignatureService.certSig) {
		throw new CMSMessageSignatureServiceException
		    (CMSMessageSignatureServiceException.CRYPTO_NO_CERTIFICATE);
	    }
	}
    }
}



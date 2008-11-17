/*
 * @(#)CRLReasonCodeExtension.java	1.14 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.  
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
 *
 */

package sun.security.x509;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Array;
import java.util.Enumeration;

import sun.security.util.*;

/**
 * The reasonCode is a non-critical CRL entry extension that identifies
 * the reason for the certificate revocation. CAs are strongly
 * encouraged to include reason codes in CRL entries; however, the
 * reason code CRL entry extension should be absent instead of using the
 * unspecified (0) reasonCode value.
 * <p>The ASN.1 syntax for this is:
 * <pre>
 *  id-ce-cRLReason OBJECT IDENTIFIER ::= { id-ce 21 }
 *
 *  -- reasonCode ::= { CRLReason }
 *
 * CRLReason ::= ENUMERATED {
 *    unspecified             (0),
 *    keyCompromise           (1),
 *    cACompromise            (2),
 *    affiliationChanged      (3),
 *    superseded              (4),
 *    cessationOfOperation    (5),
 *    certificateHold         (6),
 *    removeFromCRL           (8),
 *    privilegeWithdrawn      (9),
 *    aACompromise           (10) }
 * </pre>
 * @author Hemma Prafullchandra
 * @version 1.7
 * @see Extension
 * @see CertAttrSet
 */
public class CRLReasonCodeExtension extends Extension implements CertAttrSet {

    /**
     * Attribute name and Reason codes
     */
    public static final String NAME = "CRLReasonCode";
    public static final String REASON = "reason";

    public static final int UNSPECIFIED = 0;
    public static final int KEY_COMPROMISE = 1;
    public static final int CA_COMPROMISE = 2;
    public static final int AFFLIATION_CHANGED = 3;
    public static final int SUPERSEDED = 4;
    public static final int CESSATION_OF_OPERATION = 5;
    public static final int CERTIFICATE_HOLD = 6;
    // note 7 missing in syntax
    public static final int REMOVE_FROM_CRL = 8;
    public static final int PRIVILEGE_WITHDRAWN = 9;
    public static final int AA_COMPROMISE = 10;

    private int reasonCode = 0;

    private void encodeThis() throws IOException {
        if (reasonCode == 0) {
            this.extensionValue = null;
            return;
        }
        DerOutputStream dos = new DerOutputStream();
        dos.putEnumerated(reasonCode);
        this.extensionValue = dos.toByteArray();
    }

    /**
     * Create a CRLReasonCodeExtension with the passed in reason.
     * Criticality automatically set to false.
     *
     * @param reason the enumerated value for the reason code.
     */
    public CRLReasonCodeExtension(int reason) throws IOException {
        reasonCode = reason;
        this.extensionId = PKIXExtensions.ReasonCode_Id;
        this.critical = false;
        encodeThis();
    }

    /**
     * Create a CRLReasonCodeExtension with the passed in reason.
     *
     * @param critical true if the extension is to be treated as critical.
     * @param reason the enumerated value for the reason code.
     */
    public CRLReasonCodeExtension(boolean critical, int reason)
    throws IOException {
        this.extensionId = PKIXExtensions.ReasonCode_Id;
        this.critical = critical;
        this.reasonCode = reason;
        encodeThis();
    }

    /**
     * Create the extension from the passed DER encoded value of the same.
     *
     * @param critical true if the extension is to be treated as critical.
     * @param value Array of DER encoded bytes of the actual value.
     * @exception IOException on error.
     */
    public CRLReasonCodeExtension(Boolean critical, Object value)
    throws IOException {
	    this.extensionId = PKIXExtensions.ReasonCode_Id;
	    this.critical = critical.booleanValue();

            if (!(value instanceof byte[]))
                throw new IOException("Illegal argument type");

	    int len = Array.getLength(value);
	    byte[] extValue = new byte[len];
            System.arraycopy(value, 0, extValue, 0, len);

	    this.extensionValue = extValue;
	    DerValue val = new DerValue(extValue);
	    this.reasonCode = val.getEnumerated();
    }

    /**
     * Set the attribute value.
     */
    public void set(String name, Object obj) throws IOException {
        if (!(obj instanceof Integer)) {
	    throw new IOException("Attribute must be of type Integer.");
	}
	if (name.equalsIgnoreCase(REASON)) {
            reasonCode = ((Integer)obj).intValue();
	} else {
	    throw new IOException
		("Name not supported by CRLReasonCodeExtension");
	}
        encodeThis();
    }

    /**
     * Get the attribute value.
     */
    public Object get(String name) throws IOException {
	if (name.equalsIgnoreCase(REASON)) {
	    return new Integer(reasonCode);
	} else {
	    throw new IOException
		("Name not supported by CRLReasonCodeExtension");
	}
    }

    /**
     * Delete the attribute value.
     */
    public void delete(String name) throws IOException {
	if (name.equalsIgnoreCase(REASON)) {
	    reasonCode = 0;
	} else {
	    throw new IOException
		("Name not supported by CRLReasonCodeExtension");
	}
        encodeThis();
    }

    /**
     * Returns a printable representation of the Reason code.
     */
    public String toString() {
        String s = super.toString() + "    Reason Code: ";

        switch (reasonCode) {
        case UNSPECIFIED: s += "Unspecified"; break;
        case KEY_COMPROMISE: s += "Key Compromise"; break;
        case CA_COMPROMISE: s += "CA Compromise"; break;
        case AFFLIATION_CHANGED: s += "Affiliation Changed"; break;
        case SUPERSEDED: s += "Superseded"; break;
        case CESSATION_OF_OPERATION: s += "Cessation Of Operation"; break;
        case CERTIFICATE_HOLD: s += "Certificate Hold"; break;
        case REMOVE_FROM_CRL: s += "Remove from CRL"; break;
        case PRIVILEGE_WITHDRAWN: s += "Privilege Withdrawn"; break;
        case AA_COMPROMISE: s += "AA Compromise"; break;
        default: s += "Unrecognized reason code (" + reasonCode + ")";
        }
        return (s);
    }

    /**
     * Decode the extension from the InputStream - not just the
     * value but the OID, criticality and the extension value.
     *
     * @param in the InputStream to unmarshal the contents from.
     * @exception IOException on parsing errors.
     */
    public void decode(InputStream in) throws IOException {

        DerValue val = new DerValue(in);
        DerInputStream derStrm = val.toDerInputStream();

        // Object identifier
        this.extensionId = derStrm.getOID();

        // If the criticality flag was false, it will not have been encoded.
        DerValue derVal = derStrm.getDerValue();
        if (derVal.tag == DerValue.tag_Boolean) {
            this.critical = derVal.getBoolean();
            // Extension value (DER encoded)
            derVal = derStrm.getDerValue();
            this.extensionValue = derVal.getOctetString();
        } else {
            this.critical = false;
            this.extensionValue = derVal.getOctetString();
        }
        val = new DerValue(this.extensionValue);
        this.reasonCode = val.getEnumerated();
        if (val.data.available() != 0)
            throw new IOException
		("Illegal encoding of CRLReasonCodeExtension");
    }

    /**
     * Write the extension to the DerOutputStream.
     *
     * @param out the DerOutputStream to write the extension to.
     * @exception IOException on encoding errors.
     */
    public void encode(OutputStream out) throws IOException {
	DerOutputStream  tmp = new DerOutputStream();

	if (this.extensionValue == null) {
	    this.extensionId = PKIXExtensions.ReasonCode_Id;
	    this.critical = false;
	    encodeThis();
	}
	super.encode(tmp);
	out.write(tmp.toByteArray());
    }

    /**
     * Return an enumeration of names of attributes existing within this
     * attribute.
     */
    public Enumeration getElements() {
        AttributeNameEnumeration elements = new AttributeNameEnumeration();
        elements.addElement(REASON);

	return (elements.elements());
    }

    /**
     * Return the name of this attribute.
     */
    public String getName() {
        return (NAME);
    }
}

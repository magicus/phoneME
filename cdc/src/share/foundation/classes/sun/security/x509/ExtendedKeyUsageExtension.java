/*
 * @(#)ExtendedKeyUsageExtension.java	1.8 06/10/10
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
 */

package sun.security.x509;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Array;
import java.util.Enumeration;
import java.util.Vector;
import java.util.List;
import java.util.ArrayList;

import sun.security.util.DerValue;
import sun.security.util.DerOutputStream;
import sun.security.util.ObjectIdentifier;

/**
 * This class defines the Extended Key Usage Extension, which
 * indicates one or more purposes for which the certified public key
 * may be used, in addition to or in place of the basic purposes
 * indicated in the key usage extension field.  This field is defined
 * as follows:<p>
 *
 * id-ce-extKeyUsage OBJECT IDENTIFIER ::= {id-ce 37}<p>
 *
 * ExtKeyUsageSyntax ::= SEQUENCE SIZE (1..MAX) OF KeyPurposeId<p>
 *
 * KeyPurposeId ::= OBJECT IDENTIFIER<p>
 *
 * Key purposes may be defined by any organization with a need. Object
 * identifiers used to identify key purposes shall be assigned in
 * accordance with IANA or ITU-T Rec. X.660 | ISO/IEC/ITU 9834-1.<p>
 *
 * This extension may, at the option of the certificate issuer, be
 * either critical or non-critical.<p>
 *
 * If the extension is flagged critical, then the certificate MUST be
 * used only for one of the purposes indicated.<p>
 *
 * If the extension is flagged non-critical, then it indicates the
 * intended purpose or purposes of the key, and may be used in finding
 * the correct key/certificate of an entity that has multiple
 * keys/certificates. It is an advisory field and does not imply that
 * usage of the key is restricted by the certification authority to
 * the purpose indicated. Certificate using applications may
 * nevertheless require that a particular purpose be indicated in
 * order for the certificate to be acceptable to that application.<p>

 * If a certificate contains both a critical key usage field and a
 * critical extended key usage field, then both fields MUST be
 * processed independently and the certificate MUST only be used for a
 * purpose consistent with both fields.  If there is no purpose
 * consistent with both fields, then the certificate MUST NOT be used
 * for any purpose.<p>
 *
 * @version 1.8, 10/10/06
 * @since	1.4
 */
public class ExtendedKeyUsageExtension extends Extension
implements CertAttrSet {

    /**
     * Identifier for this attribute, to be used with the
     * get, set, delete methods of Certificate, x509 type.
     */
    public static final String IDENT = "x509.info.extensions.ExtendedKeyUsage";

    /**
     * Attribute names.
     */
    public static final String NAME = "ExtendedKeyUsage";
    public static final String USAGES = "usages";

    /**
     * Vector of KeyUsages for this object.
     */
    private Vector keyUsages;

    // Encode this extension value.
    private void encodeThis() throws IOException {
        if (keyUsages == null || keyUsages.isEmpty()) {
            this.extensionValue = null;
	    return;
        }
	DerOutputStream os = new DerOutputStream();
	DerOutputStream tmp = new DerOutputStream();

	for (int i = 0; i < keyUsages.size(); i++) {
	    tmp.putOID((ObjectIdentifier)keyUsages.elementAt(i));
	}

	os.write(DerValue.tag_Sequence, tmp);
	this.extensionValue = os.toByteArray();
    }

    /**
     * Create a ExtendedKeyUsageExtension object from
     * a Vector of Key Usages; the criticality is set to false.
     *
     * @param keyUsages the Vector of KeyUsages (ObjectIdentifiers)
     */
    public ExtendedKeyUsageExtension(Vector keyUsages)
    throws IOException {
	this(Boolean.FALSE, keyUsages);
    }

    /**
     * Create a ExtendedKeyUsageExtension object from
     * a Vector of KeyUsages with specified criticality.
     *
     * @param critical true if the extension is to be treated as critical.
     * @param keyUsages the Vector of KeyUsages (ObjectIdentifiers)
     */
    public ExtendedKeyUsageExtension(Boolean critical, Vector keyUsages)
    throws IOException {
	this.keyUsages = keyUsages;
	this.extensionId = PKIXExtensions.ExtendedKeyUsage_Id;
	this.critical = critical.booleanValue();
        encodeThis();
    }

    /**
     * Create the extension from its DER encoded value and criticality.
     *
     * @param critical true if the extension is to be treated as critical.
     * @param value Array of DER encoded bytes of the actual value.
     * @exception IOException on error.
     */
    public ExtendedKeyUsageExtension(Boolean critical, Object value)
    throws IOException {
	this.extensionId = PKIXExtensions.ExtendedKeyUsage_Id;
	this.critical = critical.booleanValue();
	//Array.getLength will throw an IllegalArgumentException if
	//value is not an array, and Array.getByte will do so if the
	//values are not bytes, so a type check is not needed.
	int len = 0;
	byte[] extValue = null;
	try {
	    len = Array.getLength(value);
	    extValue = new byte[len];
	    for (int i=0; i < len; i++)
		extValue[i] = Array.getByte(value, i);
	} catch (IllegalArgumentException iae) {
	    throw new IOException(iae.toString());
	}
	this.extensionValue = extValue;
	DerValue val = new DerValue(extValue);
	if (val.tag != DerValue.tag_Sequence) {
	    throw new IOException("Invalid encoding for " +
				   "ExtendedKeyUsageExtension.");
	}
	keyUsages = new Vector(1, 1);
	while (val.data.available() != 0) {
	    DerValue seq = val.data.getDerValue();
	    ObjectIdentifier usage = seq.getOID();
	    keyUsages.addElement(usage);
	}
    }

    /**
     * Return the extension as user readable string.
     */
    public String toString() {
	if (keyUsages == null) return "";
        return super.toString() + "ExtendedKeyUsages [\n"
	       + keyUsages.toString() + "]\n";
    }

    /**
     * Decode the extension from the InputStream.
     *
     * @param in the InputStream to unmarshal the contents from.
     * @exception IOException on decoding or validity errors.
     */
    public void decode(InputStream in) throws IOException {
        throw new IOException("Method not to be called directly.");
    }

    /**
     * Write the extension to the DerOutputStream.
     *
     * @param out the DerOutputStream to write the extension to.
     * @exception IOException on encoding errors.
     */
    public void encode(OutputStream out) throws IOException {
        DerOutputStream tmp = new DerOutputStream();
        if (extensionValue == null) {
	  extensionId = PKIXExtensions.ExtendedKeyUsage_Id;
	  critical = false;
	  encodeThis();
	}
	super.encode(tmp);
	out.write(tmp.toByteArray());
    }

    /**
     * Set the attribute value.
     */
    public void set(String name, Object obj) throws IOException {
	if (name.equalsIgnoreCase(USAGES)) {
	    if (!(obj instanceof Vector)) {
	        throw new IOException("Attribute value should be of type Vector.");
	    }
	    this.keyUsages = (Vector)obj;
	} else {
	  throw new IOException("Attribute name [" + name + 
                                "] not recognized by " +
				"CertAttrSet:ExtendedKeyUsageExtension.");
	}
        encodeThis();
    }

    /**
     * Get the attribute value.
     */
    public Object get(String name) throws IOException {
	if (name.equalsIgnoreCase(USAGES)) {
	    //NOTE: May want to consider cloning this
	    return keyUsages;
	} else {
	  throw new IOException("Attribute name [" + name + 
				"] not recognized by " +
				"CertAttrSet:ExtendedKeyUsageExtension.");
	}
    }

    /**
     * Delete the attribute value.
     */
    public void delete(String name) throws IOException {
	if (name.equalsIgnoreCase(USAGES)) {
	    keyUsages = null;
	} else {
	  throw new IOException("Attribute name [" + name + 
			        "] not recognized by " +
				"CertAttrSet:ExtendedKeyUsageExtension.");
	}
        encodeThis();
    }

    /**
     * Return an enumeration of names of attributes existing within this
     * attribute.
     */
    public Enumeration getElements() {
        AttributeNameEnumeration elements = new AttributeNameEnumeration();
        elements.addElement(USAGES);

	return (elements.elements());
    }

    /**
     * Return the name of this attribute.
     */
    public String getName() {
        return (NAME);
    }

    public List getExtendedKeyUsage() {
	ArrayList al = new ArrayList(keyUsages.size());
	for (int i = 0; i < keyUsages.size(); i++) {
	    al.add(keyUsages.elementAt(i).toString());
	}
	return al;
    }

}

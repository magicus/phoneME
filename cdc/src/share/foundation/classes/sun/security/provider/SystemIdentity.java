/*
 * @(#)SystemIdentity.java	1.31 06/10/10
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

package sun.security.provider;

import java.io.Serializable;
import java.util.Enumeration;
import java.security.*;

/**
 * An identity with a very simple trust mechanism.
 *
 * @version 1.31, 10/10/06
 * @author 	Benjamin Renaud
 */

public class SystemIdentity extends Identity implements Serializable {

    /** use serialVersionUID from JDK 1.1. for interoperability */
    private static final long serialVersionUID = 9060648952088498478L;

    /* This should be changed to ACL */
    boolean trusted = false;

    /* Free form additional information about this identity. */
    private String info;

    public SystemIdentity(String name, IdentityScope scope)
    throws InvalidParameterException, KeyManagementException {
	super(name, scope);
    }

    /**
     * Is this identity trusted by sun.* facilities?
     */
    public boolean isTrusted() {
	return trusted;
    }

    /**
     * Set the trust status of this identity.
     */
    protected void setTrusted(boolean trusted) {
	this.trusted = trusted;
    }

    void setIdentityInfo(String info) {
	super.setInfo(info);
    }

    String getIndentityInfo() {
	return super.getInfo();
    }

    /**
     * Call back method into a protected method for package friends.
     */
    void setIdentityPublicKey(PublicKey key) throws KeyManagementException {
	setPublicKey(key);
    }

    /**
     * Call back method into a protected method for package friends.
     */
    void addIdentityCertificate(Certificate cert)
    throws KeyManagementException {
	addCertificate(cert);
    }

    void clearCertificates() throws KeyManagementException {
	Certificate[] certs = certificates();
	for (int i = 0; i < certs.length; i++) {
	    removeCertificate(certs[i]);
	}
    }

    public String toString() {
	String trustedString = "not trusted";
	if (trusted) {
	    trustedString = "trusted";
	}
	return super.toString() + "[" + trustedString + "]";
    }


}

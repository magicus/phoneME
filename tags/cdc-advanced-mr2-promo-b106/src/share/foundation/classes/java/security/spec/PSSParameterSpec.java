/*
 * @(#)PSSParameterSpec.java	1.6 06/10/10
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

package java.security.spec;

import java.math.BigInteger;

/**
 * This class specifies a parameter spec for RSA PSS encoding scheme, 
 * as defined in the PKCS#1 v2.1.
 *
 * @author Valerie Peng
 *
 * @version 1.6 06/10/10
 *
 * @see AlgorithmParameterSpec
 * @see java.security.Signature
 *
 * @since 1.4
 */

public class PSSParameterSpec implements AlgorithmParameterSpec {

    private int saltLen = 0;

   /**
    * Creates a new <code>PSSParameterSpec</code>
    * given the salt length as defined in PKCS#1.
    *
    * @param saltLen the length of salt in bits to be used in PKCS#1 
    * PSS encoding.
    * @exception IllegalArgumentException if <code>saltLen</code> is
    * less than 0.
    */
    public PSSParameterSpec(int saltLen) {
	if (saltLen < 0) {
	    throw new IllegalArgumentException("invalid saltLen value");
	}
	this.saltLen = saltLen;
    }

    /**
     * Returns the salt length in bits.
     *
     * @return the salt length.
     */
    public int getSaltLength() {
	return saltLen;
    }
}

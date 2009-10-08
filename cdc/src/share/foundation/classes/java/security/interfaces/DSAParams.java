/*
 * @(#)DSAParams.java	1.23 06/10/10
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
 
package java.security.interfaces;

import java.math.BigInteger;

/**
 * Interface to a DSA-specific set of key parameters, which defines a 
 * DSA <em>key family</em>. DSA (Digital Signature Algorithm) is defined 
 * in NIST's FIPS-186.
 *
 * @see DSAKey
 * @see java.security.Key
 * @see java.security.Signature
 * 
 * @version 1.17 00/02/02
 * @author Benjamin Renaud 
 * @author Josh Bloch 
 */
public interface DSAParams {

    /**
     * Returns the prime, <code>p</code>.
     *
     * @return the prime, <code>p</code>. 
     */
    public BigInteger getP();

    /**
     * Returns the subprime, <code>q</code>.
     * 
     * @return the subprime, <code>q</code>. 
     */
    public BigInteger getQ();

    /**
     * Returns the base, <code>g</code>.
     * 
     * @return the base, <code>g</code>. 
     */
    public BigInteger getG();
}

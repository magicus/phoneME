/*
 * @(#)SecureRandomSpi.java	1.13 06/10/10
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
 
package java.security;

/**
 * This class defines the <i>Service Provider Interface</i> (<b>SPI</b>)
 * for the <code>SecureRandom</code> class.
 * All the abstract methods in this class must be implemented by each
 * service provider who wishes to supply the implementation
 * of a cryptographically strong pseudo-random number generator.
 *
 * @version 1.7, 02/02/00
 *
 * @see SecureRandom
 * @since 1.2
 */

public abstract class SecureRandomSpi implements java.io.Serializable {

    /**
     * Reseeds this random object. The given seed supplements, rather than
     * replaces, the existing seed. Thus, repeated calls are guaranteed
     * never to reduce randomness.
     *
     * @param seed the seed.
     */
    protected abstract void engineSetSeed(byte[] seed);

    /**
     * Generates a user-specified number of random bytes.
     * 
     * @param bytes the array to be filled in with random bytes.
     */
    protected abstract void engineNextBytes(byte[] bytes);

    /**
     * Returns the given number of seed bytes.  This call may be used to
     * seed other random number generators.
     *
     * @param numBytes the number of seed bytes to generate.
     * 
     * @return the seed bytes.
     */
     protected abstract byte[] engineGenerateSeed(int numBytes);
}

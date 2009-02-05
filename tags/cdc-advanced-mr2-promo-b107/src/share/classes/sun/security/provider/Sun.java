/*
 * @(#)Sun.java	1.7 06/10/10
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

import java.io.*;
import java.util.*;
import java.security.*;

/**
 * The SUN Security Provider.
 *
 * @author Benjamin Renaud 
 *
 * @version 1.30, 02/02/00
 */

/**
 * Defines the SUN provider.
 *
 * Algorithms supported, and their names:
 *
 * - SHA is the message digest scheme described in FIPS 180-1. 
 *   Aliases for SHA are SHA-1 and SHA1.
 *
 */

public final class Sun extends Provider {

    private static final String INFO = "SUN " + 
    "(DSA key/parameter generation; DSA signing; " + "SHA-1 digests)";

    public Sun() {
	/* We are the SUN provider */
	super("SUN", 1.2, INFO);

	AccessController.doPrivileged(new java.security.PrivilegedAction() {
	    public Object run() {

	        /* 
	         * Digest engines 
	         */
		put("MessageDigest.SHA", "sun.security.provider.SHA");

		put("Alg.Alias.MessageDigest.SHA-1", "SHA");
		put("Alg.Alias.MessageDigest.SHA1", "SHA");

		/*
		 * Implementation type: software or hardware
		 */
		put("MessageDigest.SHA ImplementedIn", "Software");

		return null;
	    }
	});
    }
}

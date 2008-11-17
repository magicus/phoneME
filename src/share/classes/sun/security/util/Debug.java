/*
 * @(#)Debug.java	1.16 06/10/10
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

package sun.security.util;

import java.math.BigInteger;

/**
 * A utility class for debuging.
 *
 * @version 1.9
 * @author Roland Schemers
 */
public class Debug {

    private String prefix;

    private static String args;

    static {
	args = (String)java.security.AccessController.doPrivileged
		(new sun.security.action.GetPropertyAction
		("java.security.debug"));

        String args2 = (String)java.security.AccessController.doPrivileged
		(new sun.security.action.GetPropertyAction
		("java.security.auth.debug"));

        if (args == null) {
            args = args2;
        } else {
            if (args2 != null)
               args = args + "," + args2;
        }

	if (args != null) {
	    args = args.toLowerCase();
	    if (args.equals("help")) {
		Help();
	    }
	}
    }

    public static void Help() 
    {      
	System.err.println();
	System.err.println("all           turn on all debugging");
	System.err.println("access        print all checkPermission results");
	System.err.println("combiner      SubjectDomainCombiner debugging");
	System.err.println("jar           jar verification");
	System.err.println("logincontext  login context results");
	System.err.println("policy        loading and granting");
	System.err.println("provider      security provider debugging");
	System.err.println("scl           permissions SecureClassLoader assigns");
	System.err.println();
	System.err.println("The following can be used with access:");
	System.err.println();
	System.err.println("stack     include stack trace");
	System.err.println("domain    dumps all domains in context");
	System.err.println("failure   before throwing exception, dump stack");
	System.err.println("          and domain that didn't have permission");
	System.err.println();
	System.err.println("Note: Separate multiple options with a comma");
	System.exit(0);
    }

    /**
     * Get a Debug object corresponding to whether or not the given
     * option is set. Set the prefix to be the same as option.
     */

    public static Debug getInstance(String option)
    {
	return getInstance(option, option);
    }

    /**
     * Get a Debug object corresponding to whether or not the given
     * option is set. Set the prefix to be prefix.
     */
    public static Debug getInstance(String option, String prefix)
    {
	if (isOn(option)) {
	    Debug d = new Debug();
	    d.prefix = prefix;
	    return d;
	} else {
	    return null;
	}
    }

    /**
     * True if the system property "security.debug" contains the 
     * string "option".
     */
    public static boolean isOn(String option)
    {
	if (args == null)
	    return false;
	else {
	    if (args.indexOf("all") != -1)
		return true;
	    else 
		return (args.indexOf(option) != -1);
	}
    }

    /**
     * print a message to stderr that is prefixed with the prefix
     * created from the call to getInstance.
     */

    public void println(String message)
    {
	System.err.println(prefix + ": "+message);
    }

    /**
     * print a blank line to stderr that is prefixed with the prefix.
     */

    public void println()
    {
	System.err.println(prefix + ":");
    }

    /**
     * print a message to stderr that is prefixed with the prefix.
     */

    public static void println(String prefix, String message)
    {
	System.err.println(prefix + ": "+message);
    }

    /**
     * return a hexadecimal printed representation of the specified 
     * BigInteger object. the value is formatted to fit on lines of
     * at least 75 characters, with embedded newlines. Words are 
     * separated for readability, with eight words (32 bytes) per line.
     */
    public static String toHexString(BigInteger b) {
	String hexValue = b.toString(16);
	StringBuffer buf = new StringBuffer(hexValue.length()*2);
	
	if (hexValue.startsWith("-")) {
	    buf.append("   -");
	    hexValue = hexValue.substring(1);
	} else {
	    buf.append("    ");     // four spaces
	}
	if ((hexValue.length()%2) != 0) {
	    // add back the leading 0
	    hexValue = "0" + hexValue;
	}
	int i=0;
        while (i < hexValue.length()) {
	    // one byte at a time
	    buf.append(hexValue.substring(i, i+2));
	    i+=2;
	    if (i!= hexValue.length()) {
		if ((i%64) == 0) {
		    buf.append("\n    ");     // line after eight words
		} else if (i%8 == 0) {
		    buf.append(" ");     // space between words
		}
	    }
	}
	return buf.toString();
    }	
} 


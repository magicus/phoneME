/*
 * @(#)sha1.java	1.2 06/10/10
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

/*
 * This tool takes a file as the input and computes the hash
 * value using SHA-1 cryptographic algorithm for the input
 * file. The output is the SHA-1 hash value.
 */
import java.io.*;
import java.security.*;

public class sha1 {
    
    private static final int BUFSIZE = 16*1024; // 16k buffer

    public static void main(String[] args) {
	
	try {
	    MessageDigest sha = MessageDigest.getInstance("SHA-1");
	    FileInputStream fis = new FileInputStream(args[0]);
	    byte[] data = new byte[BUFSIZE];

	    do {
		int numread = fis.read(data);
		if (numread == -1) {
		    break;
		} else {
		    sha.update(data, 0, numread);
		}
	    } while (true);
	    
	    fis.close();
	    
	    byte[] hash = sha.digest();
	    System.out.println(stringForm(hash));
        } catch (Exception e) {
	    e.printStackTrace();
        }
    }

    private static String stringForm(byte[] h) {
	String s = "";
	for (int i = 0; i < h.length; i++) {
	    String prefix;
	    int ub = h[i] & 0xff;
	    if (ub <= 0xf) {
		prefix = "0";
	    } else {
		prefix = "";
	    }
	    s += prefix + Integer.toHexString(ub);
	}
	return s;
    }
}

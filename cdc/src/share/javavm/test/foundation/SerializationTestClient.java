/*
 * @(#)SerializationTestClient.java	1.9 06/10/10
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
package foundation;

import java.net.Socket;
import java.io.OutputStream;
import java.io.ObjectOutputStream;
import foundation.SerializationTestObject;

/** Connects to a SerializationTestServer and sends two objects (a
    String array and a custom object) to it to be printed. */

public class SerializationTestClient {
    public static void usage() {
	System.out.println("java SerializationTestClient [server name]");
    }

    public static void main(String[] args) {
	try {
	    if (args.length != 1) {
		usage();
		return;
	    }
	    Socket sock = new Socket(args[0], 13913);
	    OutputStream outStr = sock.getOutputStream();
	    ObjectOutputStream objOutStr = new ObjectOutputStream(outStr);
	    String[] strs = new String[3];
	    strs[0] = "Hello,";
	    strs[1] = "serialized";
	    strs[2] = "world!";
	    System.out.println("Writing string array...");
	    objOutStr.writeObject(strs);
	    SerializationTestObject obj = new SerializationTestObject();
	    obj.booleanField = true;
	    obj.byteField = 4;
	    obj.charField = 'L';
	    obj.shortField = 17;
	    obj.intField = 39;
	    obj.floatField = 5.0f;
	    obj.longField = 8000000000001L; // 2^33 + 1
	    obj.doubleField = 7.0f;
	    System.out.println("Writing SerializationTestObject...");
	    objOutStr.writeObject(obj);
	    System.out.println("Flushing ObjectOutputStream...");
	    objOutStr.flush();
	    System.out.println("Closing socket...");
	    sock.close();
	    System.out.println(
	        "SerializationTestClient exiting successfully."
	    );
	}
	catch (Exception e) {
	    e.printStackTrace();
	    System.out.println(
	        "ERROR: SerializationTestClient exiting unsuccessfully."
	    );
	}
    }
}

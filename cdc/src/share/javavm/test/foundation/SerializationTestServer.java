/*
 * @(#)SerializationTestServer.java	1.9 06/10/10
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
import java.net.ServerSocket;
import java.io.InputStream;
import java.io.ObjectInputStream;
import foundation.SerializationTestObject;

/** Receives connection from a SerializationTestClient and receives
    and prints two objects (a String array and a custom object). */

public class SerializationTestServer {
    public static void main(String[] args) {
	try {
	    System.out.println("Waiting for connection from " +
			       "SerializationTestClient...");
	    ServerSocket serv = new ServerSocket(13913);
	    Socket sock = serv.accept();
	    InputStream inStr = sock.getInputStream();
	    ObjectInputStream objInStr = new ObjectInputStream(inStr);
	    String[] strs = (String[]) objInStr.readObject();
	    System.out.println("Received " + strs.length + " strings:");
	    for (int i = 0; i < strs.length; i++) {
		System.out.println("String " + i + ": " + strs[i]);
	    }
	    SerializationTestObject obj =
		(SerializationTestObject) objInStr.readObject();
	    System.out.println("Received SerializationTestObject:\n" + obj);
	    System.out.println("Closing sockets...");
	    sock.close();
	    serv.close();
	    System.out.println(
	        "SerializationTestServer exiting successfully."
	    );
	}
	catch (Exception e) {
	    e.printStackTrace();
	    System.out.println(
	        "ERROR: SerializationTestServer exiting unsuccessfully."
	    );
	}
    }
}

/*
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
import java.io.*;
import java.net.*;

public class cvmclient
{
    String hostname = "localhost";
    int port = 4321;

    Socket socket = null;
    PrintWriter out = null;
    BufferedReader in = null;


    static int getInt(String token) throws NumberFormatException {
        int result;
	int radix = 10;
        if (token.startsWith("0x")) {
	    int index;
	    index = token.indexOf('x');
	    token = token.substring(index + 1);
	    radix = 16;
        }
        result = Integer.parseInt(token, radix);
	return result;
    }

    public void processArgs(String[] args) {
	for (int i = 0; i < args.length; i++) {
	    String arg = args[i];
	    if (arg.equals("-host")) {
		if (++i < args.length) {
		    hostname = args[i];
		} else {
		    System.err.println("ERROR: missing host name or IP " +
				       "address after -host");
		}
	    } else if (arg.equals("-port")) {
		if (++i < args.length) {
		    String portStr = args[i];
		    try {
			port = getInt(portStr);
		    } catch (NumberFormatException e) {
			System.err.println("ERROR: Invalid port number: " +
					   portStr);
		    }
		} else {
		    System.err.println("ERROR: missing port number " +
				       "after -port");
		}
	    }
	}
    }

    public void connectSocket() {
	//Create socket connection
	System.out.println("Connecting to " + hostname + ":" + port);
	try{
	    socket = new Socket(hostname, port);
	    out = new PrintWriter(socket.getOutputStream(), true);
	    in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
	} catch (UnknownHostException e) {
	    System.out.println("Unknown host: " + hostname);
	    System.exit(1);
	} catch  (IOException e) {
	    System.out.println("Unable to open connection to host " + hostname +
			       " on port " + port + ".");
	    System.out.println("Check host, port number, and also if the " +
			       "cvmsh server has been started yet.");
	    System.exit(1);
	}
	System.out.println("Connected successfully");
    }

    public void runShell() {
        InputStreamReader in = new InputStreamReader(System.in);
        int numberOfChars;
	char[] buf = new char[1000];
	try {
	    boolean done = false;
	    final char CR = (char)0xD;
	    final char LF = (char)0xA;

	    while (!done) {
		String input;
		System.out.print("cvmsh> ");
		numberOfChars = in.read(buf, 0, buf.length);

		// First strip off the LF character if present: */
		if ((numberOfChars > 0) && (buf[numberOfChars - 1] == LF)) {
		    numberOfChars--;
		}
		// Next, strip off the CR character if present: */
		if ((numberOfChars > 0) && (buf[numberOfChars - 1] == CR)) {
		    numberOfChars--;
		}

		input = new String(buf, 0, numberOfChars);
		//System.out.println("INPUT: length " + input.length() +
		//		   " value \"" + input + "\"");

		if (input.equals("quit!")) {
		    // Make sure we are exiting the VM.
		    input = "quit!";
		} else if (input.startsWith("quit")) {
		    // Make sure we are exiting the VM.
		    input = "quit";
		} else if (input.startsWith("detach")) {
		    // Make sure we are exiting the VM.
		    input = "detach";
		} 

		// Send to the server.
		out.println(input);

		// If any of the following options were sent to the server,
		// then we need to shutdown because the server is no longer
		// available.  The only exception is "detach" where we're
		// voluntarily detaching from the server and shutting down.
		if (input.equals("quit") ||
		    input.equals("quit!") ||
		    input.equals("detach") ||
		    input.equals("stopServer")) {
		    done = true;
		}
	    }
	} catch (Throwable e) {
	    System.err.println("ERROR: " + e);
	}
    }

    public static void main(String[] args) {
	cvmclient sh = new cvmclient();
	sh.processArgs(args);
	sh.connectSocket();
	sh.runShell();
    }
}

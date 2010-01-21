/*
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
 * @(#)ResourceMidlet.java	1.7	06/10/10
 * Skeleton midlet.
 * Accesses a resource from our own JAR file and prints it
 * Also checks that reading files from the file system is not allowed.
 * Also checks that reading a class file from the JAR is not allowed.
 * These limitations are enforced by the midlet's class loader.
 */
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.IOException;

public class ResourceMidlet extends javax.microedition.midlet.MIDlet{
// get named resource from the JAR file and print it.
public void doit(String resourceName, boolean doPrint){
    InputStream i;
    i = this.getClass().getResourceAsStream(resourceName);
    if (i == null){
	System.out.println("Could not open "+resourceName);
    } else if (doPrint){
	byte buf[] = new byte[100];
	int readsize;
	do {
	    try {
		readsize = i.read(buf, 0, buf.length);
		if (readsize != -1){
		    System.out.write(buf, 0, readsize);
		}
	    }catch( IOException e){
		System.out.println("Got IOException");
		e.printStackTrace();
		break;
	    }
	}while (readsize != -1);
    }else{
	System.out.println("Successfully opened "+resourceName);
    }
}
public void startApp(){
    InputStream i;
    System.out.println("ResourceMidlet:");
    doit("embeddedResource", true);
    System.out.println("NOW FOR SOMETHING NOT POSSIBLE");
    try{
	doit("//etc/motd", true);
    }catch( Throwable t){
	System.out.print("CAUGHT ");
	t.printStackTrace();
    }
    System.out.println("AND NOW FOR SOMETHING NOT ALLOWED");
    try{
	doit("ResourceMidlet.class", false);
    }catch( Throwable t){
	System.out.print("CAUGHT ");
	t.printStackTrace();
    }
    System.out.println("ResourceMidlet exiting");
    notifyDestroyed();
}

public static void
main( String ignore[] ){
    new ResourceMidlet().startApp();
}
public void destroyApp(boolean ignore){}
}

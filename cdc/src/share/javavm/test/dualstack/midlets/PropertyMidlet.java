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
 * @(#)PropertyMidlet.java	1.5	06/10/10
 * Skeleton midlet.
 *
 * Investigate the system properties available to us.
 * Make sure we can see the MIDP/CLDC properties, but not the
 * CDC/PP or any other properties.
 * These are not the MIDlet properties, which are managed
 * by MIDlet.
 */
public class PropertyMidlet extends javax.microedition.midlet.MIDlet{
    String permittedProperty[] = {
	// from MIDP 2.0
	"microedition.locale",
	// from CLDC 1.1
	"microedition.platform",
	"microedition.encoding",
	"microedition.configuration",
	"microedition.profiles"
    };

    // a sampling of those listed with java.lang.System.getProperties
    String prohibitedProperty[] = {
	"java.version",
	"java.vendor",
	"java.home",
	"java.class.path",
	"os.name",
	"os.arch",
	"file.separator",
	"path.separator",
	"line.separator",
	"user.name",
	"user.home",
	"user.dir"
    };

    int successes = 0;
    int trials    = 0;

    private void
    trial(String key, boolean shouldSucceed){
	String val = System.getProperty(key);
	if (val == null){
	    if (shouldSucceed){
		System.out.print("Unexpected getProperty failure for: ");
		System.out.println(key);
		return;
	    } else {
		successes += 1;
	    }
	} else {
	    if (!shouldSucceed){
		System.out.println("Unexpected getProperty success: ");
	    } else {
		successes += 1;
	    }
	    System.out.print("    ");
	    System.out.print(key);
	    System.out.print(" = ");
	    System.out.println(val);
	}
    }

    private void
    tryMany(String keys[], boolean shouldSucceed){
	int nKeys = keys.length;
	for (int i=0; i<nKeys; i++){
	    trial(keys[i], shouldSucceed);
	    trials += 1;
	}
    }

public void startApp(){
    tryMany(permittedProperty, true);
    tryMany(prohibitedProperty, false);
    // these two are trickier to detect.
    Exception x = null;
    String v = null;
    try {
	v = System.getProperty(null);
    }catch(Exception e){
	x = e;
    }
    if (x instanceof NullPointerException){
	successes += 1;
    }else{
	System.out.print("System.getProperty(null) did not return NullPointerException: ");
	System.out.println(x);
    }
    trials += 1;

    x = null;
    try {
	v = System.getProperty("");
    }catch(Exception e){
	x = e;
    }
    if (x instanceof IllegalArgumentException){
	successes += 1;
    }else{
	System.out.print("System.getProperty(\"\") did not return IllegalArgumentException: ");
	System.out.println(x);
    }
    trials += 1;

    System.out.println();
    System.out.print(trials);
    System.out.print(" trials and ");
    System.out.print(successes);
    System.out.println(" successes.");

    System.out.println("PropertyMidlet exiting");
    notifyDestroyed();
}

public static void
main( String ignore[] ){
    new PropertyMidlet().startApp();
}

public void destroyApp(boolean ignore){}
}

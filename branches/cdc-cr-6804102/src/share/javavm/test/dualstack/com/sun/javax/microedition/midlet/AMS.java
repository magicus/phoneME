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
 * @(#)AMS.java	1.5	06/10/10
 */
package com.sun.javax.microedition.midlet;
import javax.microedition.midlet.*;
import java.util.jar.*;
import java.util.*;
import java.io.*;
import java.net.URL;
import sun.misc.MIDletClassLoader;
import sun.misc.MIDPImplementationClassLoader;
import sun.misc.MemberFilter;
import sun.misc.MIDPConfig;
import java.security.PermissionCollection;
import java.security.Permissions;

/*
 * A pair of strings used for maintaining our MIDlet suite table of contents.
 */
class MIDletPair {
    String name;
    String className;

    public MIDletPair(String printname, String classname){
	name = printname;
	className = classname;
    }
}

public
class AMS implements MidletAMS {

/*
 * Shared structures:
 * These are used by all midlet suites
 */
    static MIDPImplementationClassLoader midpImpl;
    static boolean			 setup = false;

/*
 * Per-midlet-suite structures:
 * This includes the JAR that we're loading out of,
 * the table of contents, which we read from the manifest,
 * and the class loader we set up to load the midlets.
 * If we were managing muliple midlets at once, then we'd
 * also have a collection of MIDlets here, and perhaps of
 * Threads as well.
 */
    String	    midpURL;	// url of a JAR
    String	    midpPath;	// file path of JAR
    JarFile	    midpSuite;  // the JAR once its open
    MIDletPair      toc[];      // JAR manifest information
    int             tocSize;
    MIDletClassLoader midpSuiteLoader;

/*
 * Construct one AMS instance per midlet suite.
 */
    public
    AMS(){ }
/*
 * Some places we want a file name,
 * some places we want a URL.
 */
    static String
    filenameToURL(String filename){
	java.io.File f = new File(filename);
	String longname;
	try {
	    longname = f.getCanonicalPath();
	} catch (IOException e ){
	    throw new Error("IOException");
	}
        return longname;
    }

/*
 * Table-of-contents management.
 * Read from the MIDlet suite manifest.
 * Print.
 */

    void
    formToc(Manifest m){
	Vector tempToc = new Vector();
	int nameend;
	int imageend;
	int ordinal = 1;
	Attributes attr = m.getMainAttributes();
	while(true){
	    // this assumes well-formed entries.
	    String attributeName = "MIDlet-".concat(Integer.toString(ordinal));
	    String attributeValue = attr.getValue(attributeName);
	    if (attributeValue == null)
		break;
	    nameend = attributeValue.indexOf(',');
	    imageend = attributeValue.lastIndexOf(',');
	    tempToc.add(
		new MIDletPair(
		    attributeValue.substring(0,nameend),
		    attributeValue.substring(imageend+1)
		)
	    );
	    ordinal += 1;
	}
	toc = new MIDletPair[ordinal-1];
	tempToc.copyInto(toc);
	tocSize = ordinal-1;
    }

    void
    printToc(){
	for (int i=0; i<tocSize; i++ ){
	    System.out.print("    ");
	    System.out.print(i+1);
	    System.out.print("\t");
	    System.out.println(toc[i].name);
	}
    }

    byte inBuff[] = new byte[10]; // private to promptForMidletNumber

    int
    promptForMidletNumber(){
	int nchars = 0;
	int v;
	while(true){
	    printToc();
	    System.out.print("midletNumber (eof to end)? ");
	    try {
		nchars = System.in.read(inBuff);
	    }catch(IOException e){
		// don't try to deal with error.
		return 0;
	    }
	    if (nchars <= 0) return 0; // EOF.
	    if (inBuff[nchars-1] == '\n')
		nchars -= 1; // exclude newline from int conversion.
	    String numString = new String(inBuff, 0, nchars);
	    try {
		v = Integer.parseInt(numString);
	    }catch(NumberFormatException e){
		System.out.print(numString);
		System.out.println(" not recognized as decimal integer");
		continue;
	    }
	    if (v < 0 || v > tocSize ){
		System.out.print(numString);
		System.out.println(" out of range");
		continue;
	    }
	    return v;
	}
    }



/*
 * Once only: all AMS instances share the same
 * MIDPImplementationClassLoader and MemberFilter
 * Security needs to be turned on only once.
 * Would more logically be static and invoked using
 * reflection, but that would be more work and would not be
 * very enlightening.
 */
    public boolean
    setupSharedState(MIDPImplementationClassLoader m, MemberFilter f)
	throws SecurityException
    {
	if (setup == true)
	    return false; // once only.
	/*
	 * Initialize the shared resources we'll use:
	 * the MemberFilter and the MIDPImplementationClassLoader
	 */
	midpImpl = m;
	if (midpImpl == null){
	    System.out.println("MIDPImplementationClassLoader is null");
	    return false;
	}

	/*
	 * Make sure that a security manager is installed.
	 */
	if (System.getSecurityManager() == null)
	    System.setSecurityManager( new java.lang.SecurityManager() );
	
	setup = true;
	return true;
    }


/*
 * Per-instance: each gets a different path to the jar containing
 * the midlet suite.
 *
 * Read the manifest so we know what the midlets in the suite are.
 * Make a MIDletClassLoader to load classes from it.
 */
    public boolean
    initializeSuite(String path){
	Manifest m;
	midpPath = path;
	midpURL = filenameToURL(path);
	try{
	    midpSuite = new JarFile(midpPath);
	} catch (IOException e){
	    System.err.println("Caught error opening "+midpPath);
	    e.printStackTrace();
	    return false;
	}
	try{
	    m = midpSuite.getManifest();
	} catch (IOException e){
	    System.err.println("Caught error getting manifest from "+midpPath);
	    e.printStackTrace();
	    return false;
	}
	if (m == null){
	    System.err.println("JAR file has no manifest: "+midpPath);
	    return false;
	}
	formToc(m);
        midpSuiteLoader = MIDPConfig.newMIDletClassLoader(new String[]{midpURL});
	if (midpSuiteLoader == null){
	    System.err.println("Could not instantiate MIDletClassLoader");
	    return false;
	}
	return true;
    }

    /*
     * promptForMidletNumber will display the menu of midlets
     * and wait for user input (very crude, but we have no
     * other user interface here). Run the midlet indicated.
     * When it is done, repeat.
     */
    public void runSuite(){
	while (true){
	    int midletNumber = promptForMidletNumber();
	    if (midletNumber <= 0){
		System.out.println();
		return;
	    }
	    /*DEBUG*/System.out.println("Running MIDlet named "+
		toc[midletNumber-1].name+" from class "+
		toc[midletNumber-1].className +
		" in path "+midpPath);
	    runMidlet( toc[midletNumber-1].className );
	}
    }

/*
 * Try to load the designated class using the midpSuiteLoader.
 * Cast it to a midlet.
 * Start it.
 *
 */
    public void runMidlet(String classname)
    {
	Class targetClass;
	MIDlet m;

	try {
	    targetClass = midpSuiteLoader.loadClass(classname, true);
	} catch (Exception e){
	    System.err.println("MIDlet class lookup:");
	    e.printStackTrace();
	    return;
	}
	try{
	    m = (MIDlet)targetClass.newInstance();
	    m.startApp();
	}catch(Throwable e){
	    System.err.println("MIDlet create/start:");
	    e.printStackTrace();
	    return;
	}
    }

}

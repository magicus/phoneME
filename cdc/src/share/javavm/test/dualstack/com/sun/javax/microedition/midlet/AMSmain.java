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
 * @(#)AMSmain.java	1.5	06/10/10
 */
package com.sun.javax.microedition.midlet;
import sun.misc.MIDPImplementationClassLoader;
import sun.misc.MIDPConfig;
import sun.misc.MemberFilter;
import javax.microedition.midlet.*;
import java.io.*;

public class AMSmain{

/* Important parameters */
    final static String AMSclassName = "com.sun.javax.microedition.midlet.AMS";
    final static String MIDPimplProperty =  "com.sun.javax.microedition.implementation";
/*
 * Some places we want a file name,
 * some places we want a URL.
 * Duplicated from javax.microedition.midlet.AMS.
 *
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
    }*/

/*
 * MAIN: 
 * Instantiate class loader & com.sun.javax.microedition.midlet.AMS class.
 * initialze shared structures,
 * instantiate just one AMS instance to go do the work.
 * First and only parameter is URL to MIDlet suite JAR.
 */

    public static void
    main( String args[] ){
	String suitePath = args[0];
        File midImplPath = new File(System.getProperty(MIDPimplProperty));

	MIDPImplementationClassLoader midpImpl = 
		MIDPConfig.newMIDPImplementationClassLoader(
			new File[]{midImplPath});
	MemberFilter mf =
		MIDPConfig.newMemberFilter();
	MidletAMS suiteRunner;

	try {
	    Class myAMSClass = midpImpl.loadClass(AMSclassName, true);
	    suiteRunner = (MidletAMS)(myAMSClass.newInstance());
	}catch( Exception e ){
	    System.err.println("Instantiating AMS");
	    e.printStackTrace();
	    return;
	}
	/*
	 * setupSharedState would most logically be a
	 * static method. But calling a static method on a
	 * dynamically loaded class is more work than this
	 * and not very enlightening.
	 */
	try {
	     if (!suiteRunner.setupSharedState(midpImpl, mf)){
		return; // error message already printed.
	    }
	}catch(SecurityException e){
	    System.err.println("Suite Runner AMS SecurityException");
	    e.printStackTrace();
	    return;
	}
	/*
	 * Setup each suiteRunner with the path of the JAR
	 * containing the suite of midlets for it to manage.
	 * In this case there is only one, so we only see
	 * one such call.
	 */
	if (!suiteRunner.initializeSuite(suitePath)){
	    return; // error message already printed.
	}
	/*
	 * Start the suiteRunner. It will return when there
	 * is nothing more for it to do.
	 */
	suiteRunner.runSuite();
    }
}

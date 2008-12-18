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
 * @(#)CLDCmain.java	1.7	06/10/10
 */
package com.sun.javax.microedition;
import java.lang.reflect.Method;
import sun.misc.MIDletClassLoader;
import sun.misc.MIDPImplementationClassLoader;
import sun.misc.MIDPConfig;
import sun.misc.MemberFilter;
import javax.microedition.midlet.*;
import java.io.*;

public class CLDCmain{

/* Important parameters */
    final static String MIDPimplProperty =  "com.sun.javax.microedition.implementation";
/*
 * Some places we want a file name,
 * some places we want a URL.
 */
    static String[]
    filenamesToURL(File[] files){
	String URLs[] = new String[files.length];
	for(int i=0; i<files.length; i++){
	    java.io.File f = files[i];
	    String longname;
	    try {
		longname = f.getCanonicalPath();
	    } catch (IOException e ){
		throw new Error("IOException");
	    }
	    URLs[i] =  longname;
	}
	return URLs;
    }

/* Debug methods */
    static void
    printStrings( String strings[] ){
	for (int i=0; i<strings.length; i++){
	    System.out.print(strings[i]);
	    System.out.print(' ');
	}
	System.out.println();
    }

    static File[]
    split(String path){
	int nComponents = 1;
	char separator = System.getProperty("path.separator", ":").charAt(0);
	File components[];
	int length = path.length();
	int start;
	int componentIndex;
	for (int i=0; i<length; i++){
	    if (path.charAt(i) == separator)
		nComponents += 1;
	}
	components = new File[nComponents];
	start = 0;
	componentIndex = 0;
	/* could optimize here for the common case of nComponents == 1 */
	for (int i=0; i<length; i++){
	    if (path.charAt(i) == separator){
		components[componentIndex] = new File(path.substring(start, i));
		componentIndex += 1;
		start = i+1;
	    }
	}
	/* and the last components is delimited by end of String */
	components[componentIndex] = new File(path.substring(start, length));

	return components;

    }

    static boolean hadError = false;

    static void
    reportError(String msg){
	System.err.println("Error: "+msg);
	hadError = true;
    }
/*
 * MAIN: 
 * Instantiate class loaders.
 * Use class loaders to load named class.
 * Use reflection to call its main, passing parameters.
 * Args:
 * -impl <path> -- list of JAR files to pass to the implementation class loader
 *	   default is value of property "com.sun.javax.microedition.implementation"
 * -classpath <path> -- list of JAR files to pass to the MIDlet class loader
 * <name>	-- name of the main class to run. Needs not be a MIDlet.
 *		all args following this are passed to the program.
 */

    public static void
    main( String args[] ){
	String midImplString = null;
	String suiteString = null;
	File   midImplPath[];
	String suitePath[] = null;
	String testClassName = null;
	int    argCount = 0;

	/* iterate over arguments */
	for(int i=0; i<args.length; i++){
	    String thisArg = args[i];
	    if (thisArg.equals("-impl")){
		if (midImplString != null)
		    reportError("Implementation path set twice");
		midImplString = args[++i];
	    } else if (thisArg.equals("-classpath")){
		if (suiteString != null)
		    reportError("Application/test path set twice");
		suiteString = args[++i];
	    } else {
		testClassName = args[i];
		argCount = i+1;
		break;
	    }
	}
	if (midImplString == null){
	    midImplString = System.getProperty(MIDPimplProperty);
	}
	if (midImplString == null)
	    reportError("Implementation path not set");
	if (suiteString == null)
	    reportError("Application/test path not set");
	if (testClassName == null)
	    reportError("Application/test class name not set");
	if (hadError){
	    System.exit(1);
	}
	midImplPath = split(midImplString);
	suitePath = filenamesToURL(split(suiteString));
	
	MIDPImplementationClassLoader midpImpl = 
		MIDPConfig.newMIDPImplementationClassLoader(midImplPath);
	MIDletClassLoader midpSuiteLoader;
	Class testClass;

        midpSuiteLoader = MIDPConfig.newMIDletClassLoader(suitePath);
	if (midpSuiteLoader == null){
	    System.err.println("Could not instantiate MIDletClassLoader");
	    return;
	}

	try {
	    testClass = midpSuiteLoader.loadClass(testClassName, true);
	}catch( Exception e ){
	    System.err.println("Instantiating test class "+testClassName);
	    e.printStackTrace();
	    return;
	}
	Method mainMethod;
	Class  mainSignature[] = { args.getClass() };
	try {
	    mainMethod = testClass.getMethod( "main", mainSignature );
	}catch( Exception e ){
	    System.err.println("Finding method main of test class "+testClassName);
	    e.printStackTrace();
	    return;
	}
	// make new arg array of all the remaining args
	String outArgs[] = new String[ args.length-argCount ];
	for( int i = argCount; i < args.length; i++){
	    outArgs[ i-argCount ] = args[i];
	}
	// DEBUG
	// System.out.println("CLDCmainDEBUG");
	// System.out.println("Impl is at "); printStrings(midImplPath);
	// System.out.println("Suite is at "); printStrings(suitePath);
	// System.out.println("Test class name is "+ testClassName);
	// System.out.println("Test class args are "); printStrings(outArgs);
	// System.out.println("END CLDCmainDEBUG");

	try {
	    mainMethod.invoke(null, new Object[]{outArgs} ); 
	}catch( Exception e ){
	    System.err.println("Invoking method main of test class "+testClassName);
	    e.printStackTrace();
	    return;
	}

    }
}

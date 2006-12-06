/*
 * @(#)Warmup.java	1.24 06/10/10
 * 
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
package sun.mtask;

import java.io.*;
import java.net.*;
import java.util.*;
import java.lang.reflect.*;

import sun.misc.JIT;
import sun.misc.CVM;

//
// A generic warmup module that works with lists of classes to
// pre-load, and methods to pre-compile.
//
// Each list is in its own file
//
public class Warmup
{
    private static String classNamesFile = null;
    private static String memberNamesFile = null;
    private static ClassLoader warmupClassLoader =
	ClassLoader.getSystemClassLoader(); // The system class loader

    private static boolean verbose = (System.getProperty("cdcams.verbose") != null) &&
        (System.getProperty("cdcams.verbose").toLowerCase().equals("true"));
	
    //
    // The entry point of the warm-up program
    // arg[0] is a filename containing a list of classes to load and initialize
    // arg[1] is a filename containing a list of methods to pre-compile
    //
    public static void main(String[] args)
    {
	processOptions(args);
	runit(classNamesFile, memberNamesFile);
    }
    
    private static void processOptions(String[] args)
    {
	classNamesFile = null;
	memberNamesFile = null;
	for (int i = 0; i < args.length; i++) {
	    if (args[i].equals("-initClasses")) {
		classNamesFile = args[++i];
	    } else if (args[i].equals("-precompileMethods")) {
		memberNamesFile = args[++i];
	    } else {
                if (verbose) {
		    System.err.println("UNRECOGNIZED OPTION \""+args[i]+"\"");
                } 
	    }
	}
    }
    
    //
    // Read a list of elements and return it as a String[]
    //
    private static String[] readElements(BufferedReader inReader)
	throws IOException
    {
	java.util.Vector v = new java.util.Vector();
	java.io.StreamTokenizer in;
	in = new StreamTokenizer(inReader);
	in.resetSyntax();
	in.eolIsSignificant( false );
	in.whitespaceChars( 0, 0x20 );
	in.wordChars( '!', '~' );
	in.commentChar('#');

	while (in.nextToken() != java.io.StreamTokenizer.TT_EOF){
	    v.addElement(in.sval);
	}

	int n = v.size();
	String olist[] = new String[n];
	v.copyInto(olist);
	return olist;
    }
    
    static Class getClassFromName(String className, boolean init){
	Class c = null;
	try {
	    // Load and maybe initialize class
            c = Class.forName(className, init, warmupClassLoader);
	} catch (ClassNotFoundException e){
	    return null;
	}
	return c;
    }

    //
    // If 'init' is not mentioned, assume it's false.
    // That's the desired behaviour when looking up argument lists.
    //
    static Class getClassFromName(String className) 
    {
	return getClassFromName(className, false);
    }

    private static boolean processClasses(BufferedReader in)
    {
	String[] classes = null;
	
	try {
	    classes = readElements(in);
	} catch (IOException e) {
            if (verbose) {
	        System.err.println("IO Exception: "+e);
            }
	    e.printStackTrace();
	    return false;
	}
	
        if (verbose) {
	    System.err.println("CLASSES TO INITIALIZE");
        }

	for (int i = 0; i < classes.length; i++) {
	    // Load and initialize class
            /* System.err.print((i+1)+":\t "); */
	    Class cl = getClassFromName(classes[i], true);
	    if (cl == null) {
                if (verbose) {
		    System.err.println("\nCould not find class "+classes[i]);
                }
		return false;
	    } else {
                /* System.err.println("cl="+cl); */
	    }
	}
	return true;
    }
    
    //
    // Parsing methods is somewhat more complex.
    //
    // If we see a class name in the method list, we compile
    // all the methods in it.
    //
    // If we see a method name, we parse it, and then pass it on to
    // JIT.compileMethod().
    //
    private static boolean processPrecompilation(BufferedReader in)
    {
	String[] methods = null;
	
	if (!CVM.isCompilerSupported()) {
            if (verbose) {
	        System.err.println("Compiler not supported, cannot precompile");
            }
	    return false;
	}
	
	try {
	    methods = readElements(in);
	} catch (IOException e) {
            if (verbose) {
	        System.err.println("IO Exception: "+e);
            }
	    e.printStackTrace();
	    return false;
	}
	mLineNo = 1;

        // initialized AOT code
        CVM.initializeAOTCode();
	
	for (int i = 0; i < methods.length; i++) {
	    // Parse and precompile
	    // Upon error, fail and return.
	    if (!parseAndPrecompileMethods(methods[i])) {
	        CVM.markCodeBuffer(); // mark shared codecache
		return false;
	    }
        }
        CVM.markCodeBuffer(); // mark shared codecache

        // initialize JIT policy
        CVM.initializeJITPolicy();
	return true;
    }
    
    static private int mLineNo;
    private static boolean parseAndPrecompileMethods(String s)
    {
	// First off replace all '/' by '.'. This saves us the trouble
	// of converting these for each signature string found.
	s = s.replace('/', '.');
	if (s.indexOf('(') == -1) {
	    // Looks like a class. Check
	    Class c = getClassFromName(s);
	    if (c == null) {
                if (verbose) {
		    System.err.println("Class "+s+" not found");
                }
		return false;
	    }
	    // It is a class. Find all of its declared methods...
	    Method[] cmethods = c.getDeclaredMethods();
	    if (cmethods == null) {
                if (verbose) {
		    System.err.println("Could not get methods in class "+s);
                }
		return false;
	    }
	    for (int i = 0; i < cmethods.length; i++) {
                /* System.err.println(mLineNo+":\t"+cmethods[i]); */
		mLineNo++;
		if (!JIT.compileMethod(cmethods[i], false)) {
                    if (verbose) {
		        System.err.println("Failed to compile "+cmethods[i]);
                    }
		    return false;
		}
		//methods[index].addElement(cmethods[i]);
	    }
	    // And find all of its declared constructors...
	    Constructor[] cconstructors = c.getDeclaredConstructors();
	    if (cconstructors == null) {
                if (verbose) {
		    System.err.println("Could not get constructors in class "+s);
                }
		return false;
	    }
	    for (int i = 0; i < cconstructors.length; i++) {
                /* System.err.println(mLineNo+":\t"+cconstructors[i]); */
		mLineNo++;
		if (!JIT.compileMethod(cconstructors[i], false)) {
                    if (verbose) {
		        System.err.println("Failed to compile "+cconstructors[i]);
                    }
		    return false;
		}
		// methods[index].addElement(cconstructors[i]);
	    }
	} else {
	    Member m = parseMethod(s);
	    if (m == null) {
                if (verbose) {
		    System.err.println("Could not find method "+s);
                }
		return false;
	    } else {
                /* System.err.println(mLineNo+":\t"+m); */
		mLineNo++;
		if (!JIT.compileMethod(m, false)) {
                    if (verbose) {
		        System.err.println("Failed to compile "+m);
                    }
		    return false;
		}
	    }
	    // methods[index].addElement(m);
	}
	return true;
    }
    
    //
    // Parse a string describing a method or constructor, and return a
    // Member object
    //
    private static Member parseMethod(String s) 
    {
	//
	// We are looking at a method. Parse that
	//
	int beginArgs = s.indexOf('(');
	int endArgs = s.indexOf(')', beginArgs + 1);
	if (endArgs == -1){
            if (verbose) {
	        System.err.println("Missing ')' in "+s);
            }
	    return null;
	}
	if (false) {
	    // There is no real need to print out this warning
	    if (endArgs < s.length() - 1) {
                if (verbose) {
		    System.err.println("Ignoring return type " +
				   s.substring(endArgs + 1));
                } 
	    }
	}
	
	int methodDot = s.lastIndexOf('.', beginArgs);
	if (methodDot == -1) {
            if (verbose) {
	        System.err.println("Couldn't find method name in "+s);
            }
	    return null;
	}
	String  signature   = s.substring(beginArgs, endArgs + 1);
	String  className   = s.substring(0, methodDot);
	String  methodName  = s.substring(methodDot+1, beginArgs);
	Class   parentClass = getClassFromName(className);
	
	if (parentClass == null) {
            if (verbose) {
	        System.err.println("Class "+parentClass+" not found");
            }  
	    return null;
	}
	Class[] args = parseArglist(signature);
	if (args == null) {
            if (verbose) {
	        System.err.println("Could not parse arguments in signature: "+
		        	       signature);
            }
	    return null;
	}
	Member thisMethod = null;
	try {
	    // There is no way to find <clinit> using reflection...
	    if (methodName.equals("<init>")) {
		thisMethod = parentClass.getDeclaredConstructor(args);
	    } else {
		thisMethod = parentClass.getDeclaredMethod(methodName, args);
	    }
	} catch (Throwable t0){
	    return null;
	}
	return thisMethod;
    }
    
    //
    // Given a signature string, return an array of classes for
    // each element of the signature, to be used by reflection code.
    //
    private static Class[] parseArglist(String signature) {
	Vector v = new Vector();
	int pos = 1;
	int endpos = signature.length()-1;
	while(pos < endpos){
	    int arrayDepth = 0;
	    Class targetClass = null;
	    while (signature.charAt(pos) == '['){
		arrayDepth ++;
		pos++;
	    }
	    switch (signature.charAt(pos)){
	    case 'I':
		targetClass = Integer.TYPE;
		pos++;
		break;
	    case 'S':
		targetClass = Short.TYPE;
		pos++;
		break;
	    case 'C':
		targetClass = Character.TYPE;
		pos++;
		break;
	    case 'Z':
		targetClass = Boolean.TYPE;
		pos++;
		break;
	    case 'B':
		targetClass = Byte.TYPE;
		pos++;
		break;
	    case 'F':
		targetClass = Float.TYPE;
		pos++;
		break;
	    case 'D':
		targetClass = Double.TYPE;
		pos++;
		break;
	    case 'J':
		targetClass = Long.TYPE;
		pos++;
		break;
	    case 'L':
		int semi = signature.indexOf(';', pos);
		if (semi == -1){
                    if (verbose) {
		        System.err.println("L not followed by ; in "+signature);
                    }
		    return null;
		}
		String targetName;
		targetName = signature.substring(pos+1, semi);
		
		targetClass = getClassFromName(targetName);
		if (targetClass == null){
                    if (verbose) {
		        System.err.println("COULD NOT FIND TARGET="+
			        	       targetName);
                    }
		    return null;
		}
		pos = semi+1;
		break;
	    default:
                if (verbose) {
		    System.err.println("Unexpected '" +
		    signature.charAt(pos) + "' in signature");
                }
		return null;
	    }
	    if (arrayDepth > 0){
		// this is grotesque
		// make a 1 x 1 x 1 x 1 x 1 x ... array of foo.
		// then get its class.
		int lArray[] = new int[ arrayDepth ];
		for (int i = 0; i < arrayDepth; i++){
		    lArray[i] = 1;
		}
		try {
		    Object arrayOfThing = 
			java.lang.reflect.Array.newInstance(targetClass,
							    lArray);
		    targetClass = arrayOfThing.getClass();
		} catch (Throwable t){
                    if (verbose) {
		        System.err.println(
			    "Could not instantiate (or get class of) "+
			     arrayDepth+ "-dimensional array of "+
			     targetClass.toString());
                    }
		    return null;
		}
	    }
	    v.addElement(targetClass);
	}
	Class[] args = new Class[ v.size() ];
	v.copyInto(args);
	return args;
    }

    public static void runit(String classNamesFile, String memberNamesFile)
    {
	BufferedReader classesIn = null;
	BufferedReader membersIn = null;
	
	try {
	    FileReader clinreader, meminreader;

	    // List of class names
	    if (classNamesFile != null) {
		clinreader = new FileReader(classNamesFile);
		classesIn = new BufferedReader(clinreader);
                if (verbose) {
                    System.err.println("classesIn=" + classesIn);
                    System.err.println("classesFile=" + classNamesFile);
                    System.err.println("processing classes...");
                }
	        if (processClasses(classesIn)) {
                    if (verbose) {
		        System.err.println("processing classes done...");
                    }
	        } else {
                    if (verbose) {
		        System.err.println("processing classes failed...");
                    }
	        }
                classesIn.close();
	    }
	    
	    // List of method names
	    if (memberNamesFile != null) {
		meminreader = new FileReader(memberNamesFile);
		membersIn = new BufferedReader(meminreader);
                if (verbose) {
                    System.err.println("membersIn=" + membersIn);
                    System.err.println("membersFile=" + memberNamesFile);
                    System.err.println("processing methods...");
                }
	        if (processPrecompilation(membersIn)) {
                    if (verbose) {
		        System.err.println("processing methods done...");
                    }
	        } else {
                    if (verbose) {
		        System.err.println("processing methods failed...");
                    }
	        }
                membersIn.close();
	    }
	} catch (IOException e) {
            if (verbose) {
	        System.err.println("IO Exception: "+e);
            }
	    e.printStackTrace();
	    return;
	}
    }
}

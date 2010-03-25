/*
 * @(#)CompilerTest.java	1.16 06/10/10
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

import java.lang.reflect.*;
import java.util.*;
import sun.misc.JIT;

/*
 * A simple class to parse name+signatures on the command line,
 * look them up using reflection, and pass to the method compiler.
 *
 * Examples of use:
 *     cvm CompilerTest -v 'java.lang.String.length()'
 *     cvm CompilerTest -n 'java.lang.String.length()' 'java.util.Vector'
 *     cvm CompilerTest 'java.lang.String.substring(II)'
 *     cvm CompilerTest 'java.lang.String.regionMatches(ILjava.lang.String;II)'
 *     cvm CompilerTest 'java.lang.String.regionMatches(ILjava.lang.String;II)'
 *
 * Passing in a class name finds all methods in the class
 * Passing in a method name just finds that method
 * Multiple classes and methods can be given on the command line.
 * -n just prints out method names, but will not run compiler
 *
 * Output depends on the compiler.
 * The 'verbose' boolean and the CVMMethodBlock* of the indicated method
 * are available to it.
 */

public class CompilerTest {

    static final int GOOD = 1;
    static final int BAD = 0;

    static Class getClassFromName(String className){
	Class c = null;
	try {
            c = Class.forName(className, false,
                              CompilerTest.class.getClassLoader());
	} catch (ClassNotFoundException e){
	    return null;
	}
	return c;
    }

    //
    // Parse a string describing a method or constructor, and return a
    // Member object
    //
    private Member 
    parseMethod(String s) 
    {
	//
	// We are looking at a method. Parse that
	//
	int beginArgs = s.indexOf('(');
	int endArgs = s.indexOf(')', beginArgs + 1);
	if (endArgs == -1){
	    System.err.println("Missing ')' in "+s);
	    return null;
	}
	if (endArgs < s.length() - 1) {
	    System.err.println("Ignoring return type " +
		s.substring(endArgs + 1));
	}
	int methodDot = s.lastIndexOf('.', beginArgs);
	if (methodDot == -1) {
	    System.err.println("Couldn't find method name in "+s);
	    return null;
	}
	String  signature   = s.substring(beginArgs, endArgs + 1);
	String  className   = s.substring(0, methodDot);
	String  methodName  = s.substring(methodDot+1, beginArgs);
	Class   parentClass = getClassFromName(className);
	
	if (parentClass == null) {
	    System.err.println("Class "+parentClass+" not found");
	    return null;
	}
	Class[] args = parseArglist(signature);
	if (args == null) {
	    System.err.println("Could not parse arguments in signature: "+
			       signature);
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
    // Parse an input string and add contained methods to a vector of methods.
    //
    // If the input string is a class (no parens), add all methods in the class
    // If the input string is a method, just add it
    //
    private boolean
    parseInputString(String s, Vector[] methods) {
	boolean good = true;
	if (s.charAt(0) == '~') {
	    good = false;
	    s = s.substring(1);
	}
	s = s.replace('/', '.');
	int index = good ? GOOD : BAD;
	if (s.indexOf('(') == -1) {
	    // Looks like a class. Check
	    Class c = getClassFromName(s);
	    if (c == null) {
		System.err.println("Class "+s+" not found");
		return false;
	    }
	    // It is a class. Find all of its declared methods...
	    Method[] cmethods = c.getDeclaredMethods();
	    if (cmethods == null) {
		System.err.println("Could not get methods in class "+s);
		return false;
	    }
	    for (int i = 0; i < cmethods.length; i++) {
		methods[index].addElement(cmethods[i]);
	    }
	    // And find all of its declared constructors...
	    Constructor[] cconstructors = c.getDeclaredConstructors();
	    if (cconstructors == null) {
		System.err.println("Could not get constructors in class "+s);
		return false;
	    }
	    for (int i = 0; i < cconstructors.length; i++) {
		methods[index].addElement(cconstructors[i]);
	    }
	} else {
	    Member m = parseMethod(s);
	    if (m == null) {
		System.err.println("Could not find method "+s);
		return false;
	    }
	    methods[index].addElement(m);
	}
	return true;
    }

    private Class[]
    parseArglist(String signature) {
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
		    System.err.println("L not followed by ; in "+signature);
		    return null;
		}
		targetClass = getClassFromName(signature.substring(pos+1,
		    semi));
		if (targetClass == null){
		    return null;
		}
		pos = semi+1;
		break;
	    default:
		System.err.println("Unexpected '" +
		    signature.charAt(pos) + "' in signature");
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
		    System.err.println(
			"Could not instantiate (or get class of) "+
			arrayDepth+ "-dimensional array of "+
			targetClass.toString()
		    );
		    return null;
		}
	    }
	    v.addElement(targetClass);
	}
	Class[] args = new Class[ v.size() ];
	v.copyInto(args);
	return args;
    }

    /*
     * Read the named file.
     * Tokenize the input.
     * Returns an array of Strings which are the tokens.
     * Propagates FileNotFound if the named file cannot be opened.
     * In the input stream, the character '#' is understood
     * to start a comment which continues to end-of-line.
     */
    private static String[]
    parseOptionFile(String fname) throws java.io.IOException {
	java.util.Vector v = new java.util.Vector();
	java.io.StreamTokenizer in;
	in = new java.io.StreamTokenizer(
	         new java.io.BufferedReader(
                     new java.io.FileReader(fname)));
	in.resetSyntax();
	in.eolIsSignificant( false );
	in.whitespaceChars( 0, 0x20 );
	in.wordChars( '!', '~' );
	in.commentChar('#');

	while (in.nextToken() != java.io.StreamTokenizer.TT_EOF){
	    v.addElement(in.sval);
	}

	int n = v.size();
	String olist[] = new String[ n ];
	v.copyInto( olist );
	return olist;
    }

    public static void
    main(String args[]){
	boolean verbose = false;
	boolean printOnly = false;
	Vector[] methods = new Vector[] {new Vector(), new Vector()};
	CompilerTest cd = new CompilerTest();
	
	for (int i = 0; i < args.length; i++){
	    if (args[i].equals("-v")){
		verbose = true;
		continue;
	    } else if (args[i].equals("-n")){
		printOnly = true;
		continue;
	    } else if (args[i].equals("-f")){
		String filename = args[++i];
		try {
		    String newargs[] = parseOptionFile(filename);
		    for (int j = 0; j < newargs.length; j++) {
			if (!cd.parseInputString(newargs[j], methods)) {
			    System.err.println("Failed to parse "+newargs[j]);
			}
		    }
		} catch (java.io.IOException ioe) {
		    System.err.println("Failed to process file "+filename);
		    ioe.printStackTrace();
		}
		continue;
	    } else {
		// Treat this one as a class or method
		if (!cd.parseInputString(args[i], methods)) {
		    System.err.println("Failed to parse "+args[i]);
		}
		continue;
	    }
	    
	}
	Member[] badMemberArray = new Member[methods[BAD].size()];
	methods[BAD].copyInto(badMemberArray);
	for (int i = 0; i < badMemberArray.length; i++) {
	    Member m = badMemberArray[i];
	    if (!printOnly) {
		sun.misc.JIT.neverCompileMethod(m);
	    } else {
		System.err.println("Found method ~"+m);
	    }
	}
	Member[] goodMemberArray = new Member[methods[GOOD].size()];
	methods[GOOD].copyInto(goodMemberArray);
	for (int i = 0; i < goodMemberArray.length; i++) {
	    Member m = goodMemberArray[i];
	    if (!printOnly) {
		sun.misc.JIT.compileMethod(m, verbose);
	    } else {
		System.err.println("Found method "+m);
	    }
	}
    }
}

/*
 * @(#)Warmup.java	1.24 06/10/10
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
package sun.misc;

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
    private static String classNamesFileString = null;
    private static String memberNamesFileString = null;
    private static String classNamesFiles[] = null;
    private static String memberNamesFiles[] = null;
    private static ClassLoader currClassLoader = null;
    private static ClassLoader midpClassLoader = null;
    private static String midpPathString =
        System.getProperty("sun.midp.home.path", "midp/midp_fb") + "/classes.zip";
    private static File midpPath[] = null;

    private static boolean verbose = 
        (System.getProperty("cdcams.verbose") != null) &&
         (System.getProperty("cdcams.verbose").toLowerCase().equals("true"));
	
    //
    // The entry point of the warm-up program
    // arg[0] is a filename containing a list of classes to load and initialize
    // arg[1] is a filename containing a list of methods to pre-compile
    //
    public static void main(String[] args)
    {
	processOptions(args);
	runit(classNamesFileString, memberNamesFileString);
    }
    
    private static void processOptions(String[] args)
    {
	classNamesFileString = null;
	memberNamesFileString = null;
	for (int i = 0; i < args.length; i++) {
	    if (args[i].equals("-initClasses")) {
		classNamesFileString = args[++i];
                classNamesFiles = split(classNamesFileString);
	    } else if (args[i].equals("-precompileMethods")) {
		memberNamesFileString = args[++i];
                memberNamesFiles = split(memberNamesFileString);
	    } else {
                if (verbose) {
		    System.err.println("UNRECOGNIZED OPTION \""+args[i]+"\"");
                } 
	    }
	}
    }

    //
    // Set current classloader, which is used to load warmup classes.
    //
    private static void setCurrClassLoader(String loaderString)
        throws IOException
    {
        String clname = loaderString.substring(12);
        if (clname == null || clname.equals("")) {
            /* The current classloader is the null ClassLoader */
            currClassLoader = null;
        } else if (clname.equals("sun.misc.Launcher$AppClassLoader")) {
            /* The current classloader is the AppClassLoader */
            currClassLoader = ClassLoader.getSystemClassLoader();
        } else if (clname.equals("sun.misc.MIDPImplementationClassLoader")) {
            if (midpClassLoader == null) {
                if (midpPath == null) {
                    StringTokenizer mp = new StringTokenizer(midpPathString,
                        System.getProperty("path.separator", ":"));
                    int num = mp.countTokens();
                    midpPath = new File[num];
                    for (int i = 0; i < num; i++) {
                        midpPath[i] = new File(mp.nextToken());
                    }
                }
                /*
                 * Create the MIDPImplementationClassLoader.
                 * Use reflection to avoid hard-coded references
                 * to dual-stack APIs, which are conditionally 
                 * included.
                 */
                try {
                    Class midpconfig = Class.forName("sun.misc.MIDPConfig");
                    Class args[] = {midpPath.getClass()};
                    Method newMIDPCL = midpconfig.getMethod(
			"newMIDPImplementationClassLoader", args);
                    Object args2[] = {midpPath};
                    midpClassLoader = (ClassLoader)newMIDPCL.invoke(null, args2);
		} catch(ClassNotFoundException ce) {
                    throw new IOException("Cannot find required classloader: " + clname);
		} catch(NoSuchMethodException me) {
                    me.printStackTrace();
                    throw new IOException("Cannot find required classloader: " + clname);
                } catch(IllegalAccessException ie) {
                    ie.printStackTrace();
                    throw new IOException("Cannot find required classloader: " + clname);
		} catch(InvocationTargetException ite) {
                    ite.printStackTrace();
                    throw new IOException("Cannot find required classloader: " + clname);
                }
	    }
            currClassLoader = midpClassLoader;
        } else {
            throw new IOException("unrecognized classloader: "+ clname);
        }

        if (verbose) {
            System.err.println("Set " + currClassLoader +
                               " as the current classloader");
        }
    }

    //
    // Read a list of elements and return it as a String[]
    //
    private static String[] readElements(BufferedReader inReader)
	throws IOException
    {
        /* Reset currClassLoader for the new list */
        currClassLoader = null;

	java.util.Vector v = new java.util.Vector();
	java.io.StreamTokenizer in;
	in = new StreamTokenizer(inReader);
	in.resetSyntax();
	in.eolIsSignificant( false );
	in.whitespaceChars( 0, 0x20 );
	in.wordChars( '!', '~' );
	in.commentChar('#');

	while (in.nextToken() != java.io.StreamTokenizer.TT_EOF){
            if (in.sval.startsWith("CLASSLOADER=")) {
                /* CLASSLOADER indicates which classloader should
                 * be used to load classes in the list.
                 */
                setCurrClassLoader(in.sval);
            } else {
                /* Add class name */
	        v.addElement(in.sval);
	    }
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
            c = Class.forName(className, init, currClassLoader);
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

    //
    // Read class or method list from file.
    //
    private static String[] readLists(String fileName)
    {
        String[] list = null;
        BufferedReader in;
	
	try {
            in = new BufferedReader(new FileReader(fileName));
	    list = readElements(in);
            if (verbose) {
	        System.err.println("read from " + fileName + " done...");
            }
            in.close();
            return list;
	} catch (IOException e) {
            if (verbose) {
                System.err.println("read from "+ fileName + " failed...");
            }
	    e.printStackTrace();
            return null;
	}
    }

    //
    // Initialize classes from the list.
    //
    private static boolean processClasses(String classes[])
    {
        if (verbose) {
	    System.err.println("CLASSES TO INITIALIZE");
        }

        if (classes != null) {
	    for (int i = 0; i < classes.length; i++) {
	        // Load and initialize class
	        Class cl = getClassFromName(classes[i], true);
                //DEBUG: System.err.println((i+1)+":\t "+cl);
	        if (cl == null) {
                    if (verbose) {
		        System.err.println("\nCould not find class "+classes[i]);
                    }
		    return false;
		}
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
    private static boolean processPrecompilation(String[] memberNameFiles)
    {
        int i;

	if (!CVM.isCompilerSupported()) {
            if (verbose) {
	        System.err.println("Compiler not supported, cannot precompile");
            }
	    return false;
	}

        // initialized AOT code
        CVM.initializeAOTCode();

        for (i = 0; i < memberNamesFiles.length; i++) {
            if (verbose) {
                System.err.println("membersFile=" + memberNamesFiles[i]);
                System.err.println("reading methods...");
            }
            String methods[] = readLists(memberNamesFiles[i]);

            // Parse and precompile
            if (methods != null) {
	        for (int j = 0; j < methods.length; j++) {
	            // Upon error, fail and return.
	            if (!parseAndPrecompileMethods(methods[j])) {
	                CVM.markCodeBuffer(); // mark shared codecache
			CVM.initializeJITPolicy();
		        return false;
	            }
                }
            }
        }

        // mark shared codecache
        CVM.markCodeBuffer();

        // initialize JIT policy
        CVM.initializeJITPolicy();
	return true;
    }
    
    static private int mLineNo = 1;
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
                //DEBUG: System.err.println(mLineNo+":\t"+m); 
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
	        System.err.println("Class "+className+" not found");
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

    //
    // Split path string into separate String components.
    //
    private static String[] split(String pathString)
    {
        if (pathString != null) {
            StringTokenizer components = new StringTokenizer(pathString,
                System.getProperty("path.separator", ":"));
            int num = components.countTokens();
            String paths[] = new String[num];
            for (int i = 0; i < num; i++) {
		 paths[i] = components.nextToken();
	    }
	    return paths;
        } else {
	    return null;
	}
    }

    public static void runit(String cnfs, String mnfs)
    {
        int i;

        // Class name lists
        if (cnfs != null && !cnfs.equals(classNamesFileString)) {
            classNamesFileString = cnfs;
            classNamesFiles = split(cnfs);
	}
        if (classNamesFiles != null) {
            String[] classlist;
            for (i = 0; i < classNamesFiles.length; i++) {
                if (verbose) {
                    System.err.println("classesFile=" + classNamesFiles[i]);
                    System.err.println("reading classes...");
                }
                classlist = readLists(classNamesFiles[i]);
                if (processClasses(classlist)) {
                    if (verbose) {
	                System.err.println("processing classes done...");
                    }
                } else {
                    if (verbose) {
                        System.err.println("processing classes failed...");
                    }
                }
	    }
	}
	    
        // Method lists
        if (mnfs != null && !mnfs.equals(memberNamesFileString)) {
            memberNamesFileString = mnfs;
            memberNamesFiles = split(mnfs);
	}
        if (memberNamesFiles != null) {
	    if (processPrecompilation(memberNamesFiles)) {
                if (verbose) {
	            System.err.println("processing methods done...");
                }
            } else {
                if (verbose) {
                    System.err.println("processing methods failed...");
                }
	    }
	}
    }
}

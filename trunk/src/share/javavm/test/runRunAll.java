/*
 * @(#)runRunAll.java	1.9 06/10/10
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

/* A class to run the kBench tests */

import sun.misc.CVM;
import java.lang.reflect.*;

public class runRunAll
{
    
    static String clinitClassNames[] = {
	"java.lang.StrictMath",
	"java.lang.Math",
	"java.lang.Character",
	"java.lang.Double",
	"java.lang.Long",
	"java.lang.Integer",
	"java.lang.ref.SoftReference",
	"java.util.Locale",
	"Strength",
	"BinaryConstraint",
	"Task"
    };

    public static void main(String[] args)
    {
	boolean doInterpRun = false;
	boolean doRun = true;
	Class  runAllClass;
	Method runAllMain = null;
	String[] argsToPass;
	
	String[] kbItems = {
	    "-f", "kBenchItems.txt"
	};
	
	//
	// -i means run interpreted first
	//     default is false
	// -n means compile only, but don't run
	//     default is true
	//
	if (args.length > 0) {
	    int i;
	    int numExtras = 0;
	    
	    for (i = 0; i < args.length; i++) {
		if (args[i].equals("-i")) {
		    doInterpRun = true;
		    numExtras++;
		} else if (args[i].equals("-n")) {
		    doRun = false;
		    numExtras++;
		}
	    }
		    
	    // Copy arguments, discard -i or -n
	    if (numExtras > 0) {
		argsToPass = new String[args.length - numExtras];
		for (int j = numExtras; j < args.length; j++) {
		    argsToPass[j - numExtras] = args[j];
		}
	    } else {
		argsToPass = args;
	    }
	} else {
	    argsToPass = args;
	}

	try {
	    runAllClass = Class.forName("MyRunAll");
	    runAllMain  = runAllClass.getDeclaredMethod("main",
		new Class[]{kbItems.getClass()});
	} catch (Exception e){
	    System.err.println("Lookup of MyRunAll.main got an exception:");
	    e.printStackTrace();
	    System.exit(1);
	}
	
	/*
	 * Run it once
	 */
	if (doInterpRun) {
	    System.err.println("Running MyRunAll interpreted\n");
	    try {
		runAllMain.invoke(null, new Object[]{argsToPass});
	    } catch (Exception e){
		System.err.println("MyRunAll.main got an exception:");
		e.printStackTrace();
		System.exit(1);
	    }
	} else {
	    // Just run the necessary clinit's
	    System.err.println("Running static initializers\n");
	    for (int i = 0; i < clinitClassNames.length; i++) {
		try {
		    Class c = Class.forName(clinitClassNames[i]);
		} catch (Throwable t) {
		    t.printStackTrace();
		}
	    }
	}
	

        /* Run GC to free up any unused memory. */
        System.err.println("Running GC\n");
        System.gc();

	/*
	 * Compile the associated classes
	 */
	System.err.println("Compiling named classes...\n");
	CompilerTest.main(kbItems);

        /* Run GC to free up any unused memory. */
        System.err.println("Running GC\n");
        System.gc();

	/*
	 * And run it again
	 */
	if (doRun) {
	    System.err.println("Running kBench compiled\n");
	    try{
		runAllMain.invoke(null, new Object[]{argsToPass});
	    } catch (Exception e){
		System.err.println("RunAll.main got an exception:");
		e.printStackTrace();
		System.exit(1);
	    }
	}
    }
}


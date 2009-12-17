/*
 * @(#)runNamedTest.java	1.8 06/10/10
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

public class runNamedTest
{
    
    static String[] shift(String[] in){
	String out[] = new String[in.length-1];
	System.arraycopy(in, 1, out, 0, out.length);
	return out;
    }

    public static void main(String[] args)
    {
	boolean doRun = true;
	boolean doRunCompiled = true;
	String targetName;
	Class  targetClass;
	Method targetMain = null;
	
	String[] kbItems = {
	    "-f", "classesToCompile.txt"
	};
	
	if (args.length > 0 && args[0].equals("-n")) {
	    doRun = false;
	    args = shift(args);
	}
	if (args.length > 0 && args[0].equals("-c")) {
	    doRunCompiled = false;
	    args = shift(args);
	}
	targetName = args[0];
	args = shift(args);

	try {
	    targetClass = Class.forName(targetName);
	    targetMain  = targetClass.getDeclaredMethod("main",
		new Class[]{kbItems.getClass()});
	} catch (Exception e){
	    System.err.println("Lookup of "+targetName+".main got an exception:");
	    e.printStackTrace();
	    System.exit(1);
	}
	
	/*
	 * Run it once
	 */
	if (doRun) {
	    System.err.println("Running "+targetName+" interpreted\n");
	    try {
		targetMain.invoke(null, new Object[]{args});
	    } catch (Exception e){
		System.err.println(targetName+".main got an exception:");
		e.printStackTrace();
		System.exit(1);
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
	if (doRunCompiled) {
	    System.err.println("Running "+targetName+" compiled\n");
	    try {
		targetMain.invoke(null, new Object[]{args});
	    } catch (Exception e){
		System.err.println(targetName+".main got an exception:");
		e.printStackTrace();
		System.exit(1);
	    }
	}
    }
}


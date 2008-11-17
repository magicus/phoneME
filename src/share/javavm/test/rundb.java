/*
 * @(#)rundb.java	1.7 06/10/10
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

/* A class to run DeltaBlue */

import sun.misc.CVM;

public class rundb
{
    
    public static void main(String[] args)
    {
	boolean doRun = true;
	
	String[] dbItems = {
	    "-f", "deltaBlueItems.txt"
	};
	
	if (args.length > 0 && args[0].equals("-n")) {
	    doRun = false;
	}
	
	/*
	 * Run it once
	 */
	if (doRun) {
	    System.err.println("Running DeltaBlue interpreted\n");
	    COM.sun.labs.kanban.DeltaBlue.DeltaBlue.main(args);
	}

        /* Run GC to free up any unused memory. */
        System.err.println("Running GC\n");
        System.gc();

	/*
	 * Compile the associated classes
	 */
	System.err.println("Compiling DeltaBlue classes...\n");
	CompilerTest.main(dbItems);

        /* Run GC to free up any unused memory. */
        System.err.println("Running GC\n");
        System.gc();

	/*
	 * And run it again
	 */
	if (doRun) {
            CVM.disableGC(); /* Disable GC because we don't handle stackmaps yet. */
	    System.err.println("Running DeltaBlue compiled\n");
	    COM.sun.labs.kanban.DeltaBlue.DeltaBlue.main(args);
            CVM.enableGC();  /* Re-enable GC. */
	}
    }
}


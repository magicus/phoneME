/*
 * @(#)MPStress.java	1.4 06/10/10
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

class MPStress extends Thread {
    static boolean verbose = false;

    static public void main(String args[]) {
	int threads = 20;
	int index = 0;
	while (index < args.length) {
	    if (args[index].equals("-verbose")) {
		verbose = true;
	    } else {
		break;
	    }
	    ++index;
	}
	if (index < args.length ) {
	    threads = Integer.parseInt(args[index]);
	}

	Object o = new Object();
	for (int i = 0; i < threads; ++i) {
	    new MPStress(o, i).start();
	}

	while (true) {
	    if (verbose) {
		System.err.println("MPStress performing GC...");
	    }
	    System.gc();

	    try {
		if (verbose) {
		    System.err.println("MPStress sleeping...");
		} else {
		    System.err.print(".");
		}
		Thread.sleep(10 * threads);
		if (verbose) {
		    System.err.println("MPStress done sleeping...");
		}
	    } catch (Exception e) {
	    }
	}
    }

    MPStress(Object o, int x) {
	mode = x;
	this.o = o;
    }

    public void run() {
	if (mode % 2 == 1) {
	    while (true) {
		synchronized (o) {
		    try {
			o.wait();
		    } catch (Exception e ) {
		    }
		}
	    }
	} else {
	    while (true) {
		synchronized (o) {
		    try {
			o.notify();
		    } catch (Exception e ) {
		    }
		}
	    }
	}
    }

    private int mode;
    private Object o;
}

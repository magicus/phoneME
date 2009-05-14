/*
 * @(#)MTGC.java	1.9 06/10/10
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

import java.util.Random;

class MTGC extends Thread {
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

	long delay = 1000;
	long total;
	while (true) {
	    long s = System.currentTimeMillis();
	    spin(delay);
	    total =  System.currentTimeMillis() - s;
	    if (total >= 500) {
		delay = (delay * 500) / total;
		break;
	    } else {
		delay = delay * 2;
	    }
	}
	long s = System.currentTimeMillis();
	spin(delay);
	total =  System.currentTimeMillis() - s;
	if (verbose) {
	    System.err.println("Delay of " + delay + " for " + total + " ms");
	}

	for (int i = 0; i < threads; ++i) {
	    new MTGC(delay).start();
	}

	while (true) {
	    if (verbose) {
		System.err.println("MTGC performing GC...");
	    }
	    System.gc();
	    try {
		if (verbose) {
		    System.err.println("MTGC sleeping...");
		}
		Thread.sleep(10 * threads);
		if (verbose) {
		    System.err.println("MTGC done sleeping...");
		}
	    } catch (Exception e) {
	    }
	}
    }

    private long delay;

    public MTGC(long delay) {
	this.delay = delay;
    }

    public static void spin(long v) {
	for (int i = 0; i < v; ++i) {
	    continue;
	}
    }

    public void run() {
	setPriority(5);
	Random r = new Random();
	while (true) {
	    if (r.nextBoolean()) {
		if (verbose) {
		    System.err.println("Thread " + this + " spinning...");
		}
		// spin (gc-unsafe)
		spin(delay);
	    } else {
		// sleep (gc-safe)
		if (verbose) {
		    System.err.println("Thread " + this + " sleeping...");
		}
		try {
		    Thread.sleep(500);
		} catch (Exception e) {
		}
	    }
	}
    }
}

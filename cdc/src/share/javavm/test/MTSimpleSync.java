/*
 * @(#)MTSimpleSync.java	1.4 06/10/10
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
 * Simple Test program to make sure unlocking of contended Simple
 * Sync locks is working ok. It is intended to be used to test
 * ports that use CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS.
 *
 * There is no output for this test, and it will run forever. In order
 * to monitor progress, you should enable the printf at the start of
 * CVMsimpleSyncUnlock. You should see it called frequently. Note
 * that it is only called if CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS.
 */

import java.util.*;
import java.lang.*;
import java.util.*;

public class MTSimpleSync implements Runnable {
    public static final int NUM_THRADS = 8;
    public static final int NUM_VECTORS = 23;
    public static final int primes[] = {1, 3, 5, 7, 11, 13, 17, 23};

    static Thread threads[] = new Thread[NUM_THRADS];
    static Vector vectors[] = new Vector[NUM_VECTORS];

    int threadIndex;
    int vectorIncrement;

    static {
    }
    public static void main(String[] args) {

	for (int i = 0; i < NUM_THRADS; i++) {
	    MTSimpleSync mtss = new MTSimpleSync(i, primes[i]);
	    threads[i] = new Thread(mtss);
	    threads[i].start();
	}
    }

    public MTSimpleSync(int threadIndex, int vectorIncrement) {
	this.threadIndex = threadIndex;
	this.vectorIncrement = vectorIncrement;
    }

    public void run() {
	System.out.println("Starting " + threads[threadIndex] +
			   " : vectorIncrement == " + vectorIncrement);
	if (vectorIncrement == 1) {
	    /* Repeatedly create Vector objects */
	    int i = 0;
	    while (true) {
		vectors[i] = new Vector();
		i += vectorIncrement;
		i = i % NUM_VECTORS;
		if (i == 0) {
		    /*
		    System.out.println("Iterating " + threads[threadIndex] +
			   " : vectorIncrement == " + vectorIncrement);
		    */
		    try {
			Thread.sleep(10);
		    } catch (InterruptedException e) {
		    }
		}
	    }
	} else {
	    /* Make sure we are done creating the Vector objects first */
	    while (vectors[NUM_VECTORS-1] == null) {
		try {
		    Thread.sleep(100);
		} catch (InterruptedException e) {
		}
	    }
	    /* Repeatedly call Vector.size() */
	    int i = 0;
	    while (true) {
		int size = vectors[i].size();
		i += vectorIncrement;
		i = i % NUM_VECTORS;
		if (i == 0) {
		    /*
		    System.out.println("Iterating " + threads[threadIndex] +
			   " : vectorIncrement == " + vectorIncrement);
		    */
		}
	    }
	}
    }

}


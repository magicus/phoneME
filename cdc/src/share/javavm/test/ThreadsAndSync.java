/*
 * @(#)ThreadsAndSync.java	1.9 06/10/10
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

public class ThreadsAndSync {

    public static void main(String argv[]) {
	System.out.print("Starting test1\n");
	test0();
	test1();
    }

    static void test0() {
        final Object lock = new Object();
        Thread thr = new Thread() {

            public void run() {
                while (true) {
                    synchronized (lock) {
                        Thread.yield();
                    }
                }
            }
        };

        thr.start();

        while (true) {
            synchronized (lock) {
                Thread.yield();
            }
        }
    }

    static void test1() {
	Thread thr = new Thread() {

	    public void run() {
		float y = (float)1.0;

		System.out.print("calling Thread.yield()\n");
		Thread.yield();

		y = (float)2.0;

		System.out.print("calling Thread.yield()\n");
		Thread.yield();

		y = (float)3.0;
	    }
	};

	thr.start();
	int x = 1;
	Thread.yield();
	x = 2;
	Thread.yield();
	x = 3;
	Thread.yield();
	Thread me = Thread.currentThread();
	if (thr == me) {
	    // impossible
	    throw new RuntimeException();
	}
	x = 4;
    }

}

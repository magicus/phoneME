/*
 * @(#)InterruptTest.java	1.9 06/10/10
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

public class InterruptTest {

    public static void main(String args[]) {
	test0(Integer.parseInt(args[0]));
    }

    static boolean abort = false;

    static void test0(int iterations) {
        final Object lock = new Object();
        Thread thr = new Thread() {

            public void run() {
		yield();
                while (!abort) {
                    synchronized (lock) {
			try {
			    lock.wait();
			} catch (InterruptedException ie) {
			    System.err.println("Interrupted! (" +
				isInterrupted() + ") (" +
				interrupted() + ")");
			}
                    }
                }
            }
        };

        thr.start();

        for (int i = 0; i < iterations; ++i) {
            synchronized (lock) {
                thr.interrupt();
		try {
		    Thread.sleep(100);
		} catch (InterruptedException ie) {
		}
            }
        }
	abort = true;
	thr.interrupt();
    }

}

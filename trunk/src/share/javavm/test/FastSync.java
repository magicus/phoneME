/*
 * @(#)FastSync.java	1.10 06/10/10
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

class FastSyncInner {
    public static synchronized void syncedMethod() { }
}

public class FastSync {

    private static void nullMethod() { }
    private static synchronized void syncedMethod() { }
    private static synchronized void syncedMethod2() { syncedMethod(); }
    private static synchronized void syncedMethod2Other() {
        FastSyncInner.syncedMethod();
    }
    private static synchronized void syncedMethodME1(Object lock) {
        synchronized (lock) {
        }
    }
    private static synchronized void
    syncedMethodME2(Object lock, Object lock2) {
        synchronized (lock) {
            synchronized (lock2) {
            }
        }
    }

    public static void main(String args[]) {
	int iterations = Integer.parseInt(args[0]);
	Object lock = new Object();
        Object lock2 = new Object();
        Object lock3 = new Object();
        Object lock4 = new Object();

        /* Warm up JIT: */
        for (int i = 0; i < iterations; ++i) {
            System.currentTimeMillis();
        }

	long t0 = System.currentTimeMillis();
	for (int i = 0; i < iterations; ++i) {
	}
	long t1 = System.currentTimeMillis();
	for (int i = 0; i < iterations; ++i) {
	    synchronized (lock) {
	    }
	}
	long t2 = System.currentTimeMillis();
	for (int i = 0; i < iterations; ++i) {
	    nullMethod();
	}
	long t3 = System.currentTimeMillis();
        for (int i = 0; i < iterations; ++i) {
            syncedMethod();
        }
        long t4 = System.currentTimeMillis();
        for (int i = 0; i < iterations; ++i) {
            synchronized (lock) {
                synchronized (lock) {
                }
            }
        }
        long t5 = System.currentTimeMillis();
        for (int i = 0; i < iterations; ++i) {
            syncedMethod2();
        }
        long t6 = System.currentTimeMillis();
        for (int i = 0; i < iterations; ++i) {
            synchronized (lock) {
                synchronized (lock2) {
                }
            }
        }
        long t7 = System.currentTimeMillis();
        for (int i = 0; i < iterations; ++i) {
            syncedMethod2Other();
        }
        long t8 = System.currentTimeMillis();

        long t9, t10, t11, t12;

        synchronized (lock) {
            t9 = System.currentTimeMillis();
            for (int i = 0; i < iterations; ++i) {
                synchronized (lock) {
                }
            }
            t10 = System.currentTimeMillis();
        }
        synchronized (lock) {
            t11 = System.currentTimeMillis();
            for (int i = 0; i < iterations; ++i) {
                synchronized (lock2) {
                }
            }
            t12 = System.currentTimeMillis();
        }

        long t13 = System.currentTimeMillis();
        for (int i = 0; i < iterations; ++i) {
            synchronized (lock) {
                synchronized (lock) {
                    synchronized (lock) {
                        synchronized (lock) {
                        }
                    }
                }
            }
        }
        long t14 = System.currentTimeMillis();
        for (int i = 0; i < iterations; ++i) {
            synchronized (lock) {
                synchronized (lock2) {
                    synchronized (lock3) {
                        synchronized (lock4) {
                        }
                    }
                }
            }
        }
        long t15 = System.currentTimeMillis();
        for (int i = 0; i < iterations; ++i) {
            syncedMethodME1(lock);
        }
        long t16 = System.currentTimeMillis();
        for (int i = 0; i < iterations; ++i) {
            syncedMethodME2(lock, lock2);
        }
        long t17 = System.currentTimeMillis();

        System.out.println("Time numbers has been adjusted for null loop and" +
                           " null method times respectively:");
        System.out.println("");
        System.out.println("null loop " + iterations + " iterations in " +
            (t1 - t0) + "ms");
        System.out.println("nullMethod " + iterations + " iterations in " +
            (t3 - t2 - (t1 - t0)) + "ms");
        System.out.println("monenter/exit o1 " + iterations +
            " iterations in " + (t2 - t1 - (t1 - t0)) + "ms");
        System.out.println("monenter/exit o1 + o1 " + iterations +
            " iterations in " + (t5 - t4 - (t1 - t0)) + "ms");
        System.out.println("monenter/exit o1 + o2 " + iterations +
            " iterations in " + (t7 - t6 - (t1 - t0)) + "ms");
        System.out.println("monenter/exit o1 + o1: 2nd o1 time " + iterations +
            " iterations in " + (t10 - t9 - (t1 - t0)) + "ms");
        System.out.println("monenter/exit o1 + o2: o2 time " + iterations +
            " iterations in " + (t12 - t11 - (t1 - t0)) + "ms");
        System.out.println("monenter/exit o1 + o1 + o1 + o1 " + iterations +
            " iterations in " + (t14 - t13 - (t1 - t0)) + "ms");
        System.out.println("monenter/exit o1 + o2 + o3 + o4 " + iterations +
            " iterations in " + (t15 - t14 - (t1 - t0)) + "ms");

        System.out.println("sync method c1 " + iterations + " iterations in " +
            (t4 - t3 - (t3 - t2)) + "ms");
        System.out.println("sync method c1 + c1 " + iterations +
            " iterations in " + (t6 - t5 - (t3 - t2)) + "ms");
        System.out.println("sync method c1 + c2 " + iterations +
            " iterations in " + (t8 - t7 - (t3 - t2)) + "ms");
        System.out.println("sync method c1 + mon o1 " + iterations +
            " iterations in " + (t16 - t15 - (t3 - t2)) + "ms");
        System.out.println("sync method c1 + mon o1 + o2 " + iterations +
            " iterations in " + (t17 - t16 - (t3 - t2)) + "ms");
        System.out.println("");
    }

}

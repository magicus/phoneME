/*
 * @(#)TestSync.java	1.10 06/10/10
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
/*
 * @(#)TestSync.java	1.4 03/11/04
 * The TestSync class tests miscellaneous uneven locking scenarios.
 */

class TestSyncT2Locker extends Thread
{
    TestSyncInflater inflater;
    boolean done;
    boolean ready;
    boolean waiting;
    Object objToLock;
    Object request;
    Object requestDone;
    int requestType;
    static final int LOCK = 1;
    static final int UNLOCK = 2;
    static final int SYNC = 3;
    static final int UNSYNC = 4;
    static final int INFLATE = 5;

    TestSyncT2Locker(TestSyncInflater _inflater) {
        inflater = _inflater;
        ready = false;
        request = new Object();
        requestDone = new Object();
        objToLock = null;
    }

    void init() {
        done = false;
        this.start();
    }

    void terminate() {
        done = true;
        synchronized(request) {
            request.notifyAll(); // Wake up locker thread.
        }
    }

    public void run() {
        while (!done) {
            doRun();
	}
    }

    private void doRun() {
            // Step 2: Lock the request lock and declare the locker ready.
            synchronized(request) {
                ready = true;
                try {
                    // Step 3: Wait for a request.
                    request.wait();
                    ready = false;
                } catch (InterruptedException e) {
                }

                if (!done) {
                    while (!waiting) {
                        try {
                            Thread.sleep(10);
                        } catch (InterruptedException e) {
                        }
                    }

                    // Step 7: Lock of unlock the object.
                    if (requestType == LOCK) {
                        TestSyncLocker.monitorEnter(objToLock);
		    } else if (requestType == UNLOCK) {
			TestSyncLocker.monitorExit(objToLock);
		    } else if (requestType == SYNC) {
                        synchronized(objToLock) {

                            // Now wait for the next request which should
                            // be for unsync:
                            synchronized(requestDone) {
                                requestDone.notifyAll();
		            }

                            doRun();

			}
                    } else if (requestType == UNSYNC) {

		    } else if (requestType == INFLATE) {
			inflater.inflate(objToLock);
		    }

                    // Step 8: Tell the requester that we're done.
                    synchronized(requestDone) {
                        requestDone.notifyAll();
		    }
                }
            }
    }

    void monitorEnter(Object o) {
	request(o, LOCK);
    }

    void monitorExit(Object o) {
	request(o, UNLOCK);
    }

    void sync(Object o) {
	request(o, SYNC);
    }

    void unsync(Object o) {
	request(o, UNSYNC);
    }

    void inflate(Object o) {
	request(o, INFLATE);
    }

    private void request(Object o, int requestVal) {
        // Step 1: Wait for the locker thread to get ready:
        while (!ready) {
            try {
                Thread.sleep(10);
            } catch (InterruptedException e) {
            }
        }
        // Step 4: Lock the request lock and make a request.
        synchronized(request) {
            waiting = false;
            objToLock = o;
            requestType = requestVal;
            // Step 5 : Tell the locker thread to wake up and handle the
            // request.
            request.notifyAll();
        }

        // Step 6: Lock the object to be locked and wait for
        // the requested to contend and notify it.
        synchronized(requestDone) {
            try {
                waiting = true;
                requestDone.wait();
            } catch (InterruptedException e) {
            }
        }

        // The only way we can get here is if the locked boolean has been
        // set to true.  The only way that can happen is if the locker
        // thread managed to wait on the object and the requester thread gets
        // to notify the locker thread to wake up from that wait.  Hence,
        // contention (and therefore inflation) must have happened.
    }
}

class TestSyncInflater extends Thread
{
    boolean done;
    boolean ready;
    boolean waiting;
    Object objToInflate;
    Object request;

    TestSyncInflater() {
        ready = false;
        request = new Object();
        objToInflate = null;
    }

    void init() {
        done = false;
        this.start();
    }

    void terminate() {
        done = true;
        synchronized(request) {
            request.notifyAll(); // Wake up inflater thread.
        }
    }

    public void run() {
        while (!done) {
            // Step 2: Lock the request lock and declare the inflater ready.
            synchronized(request) {
                ready = true;
                try {
                    // Step 3: Wait for a request.
                    request.wait();
                    ready = false;
                } catch (InterruptedException e) {
                }

                if (!done) {
                    while (!waiting) {
                        try {
                            Thread.sleep(10);
                        } catch (InterruptedException e) {
                        }
                    }

                    // Step 7: Contend on the object to be inflated and notify
                    // the inflater once we have contended on it.
                    synchronized(objToInflate) {
                        objToInflate.notifyAll();
                    }
                    try {
                        Thread.sleep(10);
                    } catch (InterruptedException e) {
                    }
                }
            }
        }
    }

    void inflate(Object o) {
        // Step 1: Wait for the inflater thread to get ready:
        while (!ready) {
            try {
                Thread.sleep(10);
            } catch (InterruptedException e) {
            }
        }
        // Step 4: Lock the request lock and make a request.
        synchronized(request) {
            waiting = false;
            objToInflate = o;
            // Step 5 : Tell the inflater thread to wake up and handle the
            // request.
            request.notifyAll();
        }

        // Step 6: Lock the object to be inflated and wait for
        // the requested to contend and notify it.
        synchronized(objToInflate) {
            try {
                waiting = true;
                objToInflate.wait();
            } catch (InterruptedException e) {
            }
        }

        // The only way we can get here is if the inflated boolean has been
        // set to true.  The only way that can happen is if the inflater
        // thread managed to wait on the object and the requester thread gets
        // to notify the inflater thread to wake up from that wait.  Hence,
        // contention (and therefore inflation) must have happened.
    }
}

public class TestSync
{
    static int level = 0;
    static int total = 0;
    static int failed = 0;
    static int passed = 0;
    static TestSyncInflater inflater =  new TestSyncInflater();
    static TestSyncT2Locker locker =  new TestSyncT2Locker(inflater);
    static TestSyncT2Locker t2 = locker;

    public static void main(String[] args) throws Exception {
        long maxReentryCount = 0;
        boolean runLargeReentryLockingTest = false;
	boolean needToTestUnstructuredLocks = false;
	int usedArg = -1;
        try {
	    // Check for the "testUnstructuredLocks" arg:
	    for (int i = 0; i < args.length; i++) {
		// NOTE: JDK 1.4's javac generate a catch block that catches
		// the same IllegalMonitorStateException that it throws. See
		// bug reports at
		// http://bugs.sun.com/bugdatabase/view_bug.do?bug_id=6271353 and
		// http://bugs.sun.com/bugdatabase/view_bug.do?bug_id=4414101.
		// However, the javac team has determined this to not be a bug
		// due to various reasons. But in CVM's case, this unfortunately
		// results in an infinite loop due to CVM supporting unstructured
		// locks.  For now, we disable these tests by default for
		// convenience.
		if (args[i].equals("testUnstructuredLocks")) {
		    needToTestUnstructuredLocks = true;
		    usedArg = i;
		    break;
		}
	    }
	    // Check for the "maxReentryCount" arg:
	    for (int i = 0; i < args.length; i++) {
		if (i != usedArg) {
		    maxReentryCount = Long.parseLong(args[0]);
		    runLargeReentryLockingTest = true;
		}
	    }

        } catch (Exception e) {
            maxReentryCount = 0;
            runLargeReentryLockingTest = false;
        }

        inflater.init();
        locker.init();

        testHoldsLock();
	if (needToTestUnstructuredLocks) {
	    testExcessMonitorExit();
	    testExcessMonitorEnter();
	}
        if (runLargeReentryLockingTest) {
            testLargeReentryLocking(maxReentryCount);
        }

        locker.terminate();
        inflater.terminate();

        System.gc();

        System.out.println();
        System.out.println("Passed: " + passed);
        System.out.println("Failed: " + failed);
        System.out.println("Total:  " + total);
    }

    static void monitorEnter(Object o) {
        TestSyncLocker.monitorEnter(o);
    }
    static void monitorExit(Object o) {
        TestSyncLocker.monitorExit(o);
    }
    static void inflate(Object o) {
        inflater.inflate(o);
    }

    static void
    passIf(boolean success, String message) {
        String status;
        if (success) {
            passed++;
            status = "PASSED";
        } else {
            failed++;
            status = "FAILED";
        }
        total++;
        System.out.println(status + " " + total + ": " + message);
    }

    static void
    passIf(boolean success, String message, int level, int expected) {
        passIf(success, message);
        if (!success || (level != expected)) {
            System.out.println("    Expected level = " + expected);
            System.out.println("    Actual level = " + level);
        }
    }

    static void decLevels(Object o) {
        for (int i = 0; i < 10; i++) {
            monitorExit(o);
            level--;
        }
    }

    //======================================================================

    synchronized void syncF() {
        level++;
        level--;
    }

    synchronized void syncFX() {
        level++;
        monitorExit(this);
        level--;
    }

    synchronized void syncFE() {
        level++;
        monitorEnter(this);
        level--;
    }

    synchronized void syncFIX() {
        level++;
        inflate(this);
        monitorExit(this);
        level--;
    }

    synchronized void syncFIE() {
        level++;
        inflate(this);
        monitorEnter(this);
        level--;
    }

    synchronized void syncFXI() {
        level++;
        monitorExit(this);
        inflate(this);
        level--;
    }

    synchronized void syncFEI() {
        level++;
        monitorEnter(this);
        inflate(this);
        level--;
    }

    synchronized void syncFFX() {
        level++;
        syncFX();
        level--;
    }

    synchronized void syncFFE() {
        level++;
        syncFE();
        level--;
    }

    synchronized void syncFFIX() {
        level++;
        syncFIX();
        level--;
    }

    synchronized void syncFFIE() {
        level++;
        syncFIE();
        level--;
    }

    synchronized void syncFFXI() {
        level++;
        syncFXI();
        level--;
    }

    synchronized void syncFFEI() {
        level++;
        syncFEI();
        level--;
    }

    synchronized void syncFIFX() {
        level++;
        inflate(this);
        syncFX();
        level--;
    }

    synchronized void syncFIFE() {
        level++;
        inflate(this);
        syncFE();
        level--;
    }

    synchronized void syncFXX() {
        level++;
        monitorExit(this);
        monitorExit(this);
        level--;
    }

    synchronized void syncFEE() {
        level++;
        monitorEnter(this);
        monitorEnter(this);
        level--;
    }

    synchronized void syncFIXX() {
        level++;
        inflate(this);
        monitorExit(this);
        monitorExit(this);
        level--;
    }

    synchronized void syncFIEE() {
        level++;
        inflate(this);
        monitorEnter(this);
        monitorEnter(this);
        level--;
    }

    synchronized void syncFXIX() {
        level++;
        monitorExit(this);
        inflate(this);
        monitorExit(this);
        level--;
    }

    synchronized void syncFEIE() {
        level++;
        monitorEnter(this);
        inflate(this);
        monitorEnter(this);
        level--;
    }

    synchronized void syncFFXX() {
        level++;
        syncFXX();
        level--;
    }

    synchronized void syncFFEE() {
        level++;
        syncFEE();
        level--;
    }

    synchronized void syncFFIXX() {
        level++;
        syncFIXX();
        level--;
    }

    synchronized void syncFFIEE() {
        level++;
        syncFIEE();
        level--;
    }

    synchronized void syncFFXIX() {
        level++;
        syncFXIX();
        level--;
    }

    synchronized void syncFFEIE() {
        level++;
        syncFEIE();
        level--;
    }

    synchronized void syncFIFXX() {
        level++;
        inflate(this);
        syncFXX();
        level--;
    }

    synchronized void syncFIFEE() {
        level++;
        inflate(this);
        syncFEE();
        level--;
    }

    synchronized void syncFFFX() {
        level++;
        syncFFX();
        level--;
    }

    synchronized void syncFFFE() {
        level++;
        syncFFE();
        level--;
    }

    synchronized void syncFFFIX() {
        level++;
        syncFFIX();
        level--;
    }

    synchronized void syncFFFIE() {
        level++;
        syncFFIE();
        level--;
    }

    synchronized void syncFFIFX() {
        level++;
        syncFIFX();
        level--;
    }

    synchronized void syncFFIFE() {
        level++;
        syncFIFE();
        level--;
    }

    synchronized void syncFIFFX() {
        level++;
        inflate(this);
        syncFFX();
        level--;
    }

    synchronized void syncFIFFE() {
        level++;
        inflate(this);
        syncFFE();
        level--;
    }

    synchronized void syncFFFXX() {
        level++;
        syncFFXX();
        level--;
    }

    synchronized void syncFFFEE() {
        level++;
        syncFFEE();
        level--;
    }

    synchronized void syncFFFXXX() {
        level++;
        syncFFXXX();
        level--;
    }

    synchronized void syncFFFEEE() {
        level++;
        syncFFEEE();
        level--;
    }

    synchronized void syncFFFXE() {
        level++;
        syncFFXE();
        level--;
    }

    synchronized void syncFFFEX() {
        level++;
        syncFFEX();
        level--;
    }

    synchronized void syncFFFX_E() {
        level++;
        syncFFX_E();
        level--;
    }

    synchronized void syncFFFE_X() {
        level++;
        syncFFE_X();
        level--;
    }

    synchronized void syncFFFX__E() {
        level++;
        syncFFX();
        monitorEnter(this);
        level--;
    }

    synchronized void syncFFFE__X() {
        level++;
        syncFFE();
        monitorExit(this);
        level--;
    }

    synchronized void syncFXXX() {
        level++;
        monitorExit(this);
        monitorExit(this);
        monitorExit(this);
        level--;
    }

    synchronized void syncFEEE() {
        level++;
        monitorEnter(this);
        monitorEnter(this);
        monitorEnter(this);
        level--;
    }

    synchronized void syncFIXXX() {
        level++;
        inflate(this);
        monitorExit(this);
        monitorExit(this);
        monitorExit(this);
        level--;
    }

    synchronized void syncFIEEE() {
        level++;
        inflate(this);
        monitorEnter(this);
        monitorEnter(this);
        monitorEnter(this);
        level--;
    }

    synchronized void syncFXIXX() {
        level++;
        monitorExit(this);
        inflate(this);
        monitorExit(this);
        monitorExit(this);
        level--;
    }

    synchronized void syncFEIEE() {
        level++;
        monitorEnter(this);
        inflate(this);
        monitorEnter(this);
        monitorEnter(this);
        level--;
    }

    synchronized void syncFXXIX() {
        level++;
        monitorExit(this);
        monitorExit(this);
        inflate(this);
        monitorExit(this);
        level--;
    }

    synchronized void syncFEEIE() {
        level++;
        monitorEnter(this);
        monitorEnter(this);
        inflate(this);
        monitorEnter(this);
        level--;
    }

    synchronized void syncFFXXX() {
        level++;
        syncFXXX();
        level--;
    }

    synchronized void syncFFEEE() {
        level++;
        syncFEEE();
        level--;
    }

    synchronized void syncFFIXXX() {
        level++;
        syncFIXXX();
        level--;
    }

    synchronized void syncFFIEEE() {
        level++;
        syncFIEEE();
        level--;
    }

    synchronized void syncFFXIXX() {
        level++;
        syncFXIXX();
        level--;
    }

    synchronized void syncFFEIEE() {
        level++;
        syncFEIEE();
        level--;
    }

    synchronized void syncFIFXXX() {
        level++;
        inflate(this);
        syncFXXX();
        level--;
    }

    synchronized void syncFIFEEE() {
        level++;
        inflate(this);
        syncFEEE();
        level--;
    }

    synchronized void syncFXE() {
        level++;
        monitorExit(this);
        monitorEnter(this);
        level--;
    }

    synchronized void syncFEX() {
        level++;
        monitorEnter(this);
        monitorExit(this);
        level--;
    }

    synchronized void syncFXEE() {
        level++;
        monitorExit(this);
        monitorEnter(this);
        monitorEnter(this);
        level--;
    }

    synchronized void syncFIXE() {
        level++;
        inflate(this);
        monitorExit(this);
        monitorEnter(this);
        level--;
    }

    synchronized void syncFIEX() {
        level++;
        inflate(this);
        monitorEnter(this);
        monitorExit(this);
        level--;
    }

    synchronized void syncFXIE() {
        level++;
        monitorExit(this);
        inflate(this);
        monitorEnter(this);
        level--;
    }

    synchronized void syncFEIX() {
        level++;
        monitorEnter(this);
        inflate(this);
        monitorExit(this);
        level--;
    }

    synchronized void syncFXEI() {
        level++;
        monitorExit(this);
        monitorEnter(this);
        inflate(this);
        level--;
    }

    synchronized void syncFEXI() {
        level++;
        monitorEnter(this);
        monitorExit(this);
        inflate(this);
        level--;
    }

    synchronized void syncFFXE() {
        level++;
        syncFXE();
        level--;
    }

    synchronized void syncFFEX() {
        level++;
        syncFEX();
        level--;
    }

    synchronized void syncFFXEE() {
        level++;
        syncFXEE();
        level--;
    }

    synchronized void syncFFIXE() {
        level++;
        syncFIXE();
        level--;
    }

    synchronized void syncFFIEX() {
        level++;
        syncFIEX();
        level--;
    }

    synchronized void syncFFIX_E() {
        level++;
        syncFIX();
        monitorEnter(this);
        level--;
    }

    synchronized void syncFFIE_X() {
        level++;
        syncFIE();
        monitorExit(this);
        level--;
    }

    synchronized void syncFFXI_E() {
        level++;
        syncFXI();
        monitorEnter(this);
        level--;
    }

    synchronized void syncFFEI_X() {
        level++;
        syncFEI();
        monitorExit(this);
        level--;
    }

    synchronized void syncFIFX_E() {
        level++;
        inflate(this);
        syncFX();
        monitorEnter(this);
        level--;
    }

    synchronized void syncFIFE_X() {
        level++;
        inflate(this);
        syncFE();
        monitorExit(this);
        level--;
    }

    synchronized void syncFFX_EI() {
        level++;
        syncFX();
        monitorEnter(this);
        inflate(this);
        level--;
    }

    synchronized void syncFFE_XI() {
        level++;
        syncFE();
        monitorExit(this);
        inflate(this);
        level--;
    }

    synchronized void syncFFXIE() {
        level++;
        syncFXIE();
        level--;
    }

    synchronized void syncFFEIX() {
        level++;
        syncFEIX();
        level--;
    }

    synchronized void syncFFXEI() {
        level++;
        syncFXEI();
        level--;
    }

    synchronized void syncFFEXI() {
        level++;
        syncFEXI();
        level--;
    }

    synchronized void syncFIFXE() {
        level++;
        inflate(this);
        syncFXE();
        level--;
    }

    synchronized void syncFIFEX() {
        level++;
        inflate(this);
        syncFEX();
        level--;
    }

    synchronized void syncFFXE_I() {
        level++;
        syncFXE();
        inflate(this);
        level--;
    }

    synchronized void syncFFEX_I() {
        level++;
        syncFEX();
        inflate(this);
        level--;
    }

    synchronized void syncFFX_E() {
        level++;
        syncFX();
        monitorEnter(this);
        level--;
    }

    synchronized void syncFFE_X() {
        level++;
        syncFE();
        monitorExit(this);
        level--;
    }

    synchronized void syncFFX_EE() {
        level++;
        syncFX();
        monitorEnter(this);
        monitorEnter(this);
        level--;
    }

    synchronized void syncFSX() {
        level++;
        synchronized(this) {
            level++;
            monitorExit(this);
            level--;
        }
        level--;
    }

    synchronized void syncFSE() {
        level++;
        synchronized(this) {
            level++;
            monitorEnter(this);
            level--;
        }
        level--;
    }

    synchronized void syncFSIX() {
        level++;
        synchronized(this) {
            level++;
            inflate(this);
            monitorExit(this);
            level--;
        }
        level--;
    }

    synchronized void syncFSIE() {
        level++;
        synchronized(this) {
            level++;
            inflate(this);
            monitorEnter(this);
            level--;
        }
        level--;
    }

    synchronized void syncFISX() {
        level++;
        inflate(this);
        synchronized(this) {
            level++;
            monitorExit(this);
            level--;
        }
        level--;
    }

    synchronized void syncFISE() {
        level++;
        inflate(this);
        synchronized(this) {
            level++;
            monitorEnter(this);
            level--;
        }
        level--;
    }

    synchronized void syncFSXX() {
        level++;
        synchronized(this) {
            level++;
            monitorExit(this);
            monitorExit(this);
            level--;
        }
        level--;
    }

    synchronized void syncFSEE() {
        level++;
        synchronized(this) {
            level++;
            monitorEnter(this);
            monitorEnter(this);
            level--;
        }
        level--;
    }

    synchronized void syncFSXXX() {
        level++;
        synchronized(this) {
            level++;
            monitorExit(this);
            monitorExit(this);
            monitorExit(this);
            level--;
        }
        level--;
    }

    synchronized void syncFSEEE() {
        level++;
        synchronized(this) {
            level++;
            monitorEnter(this);
            monitorEnter(this);
            monitorEnter(this);
            level--;
        }
        level--;
    }

    synchronized void syncFSXE() {
        level++;
        synchronized(this) {
            level++;
            monitorExit(this);
            monitorEnter(this);
            level--;
        }
        level--;
    }

    synchronized void syncFSEX() {
        level++;
        synchronized(this) {
            level++;
            monitorEnter(this);
            monitorExit(this);
            level--;
        }
        level--;
    }

    synchronized void syncFSX_E() {
        level++;
        synchronized(this){
            level++;
            monitorExit(this);
            level--;
        }
        monitorEnter(this);
        level--;
    }

    synchronized void syncFSE_X() {
        level++;
        synchronized(this){
            level++;
            monitorEnter(this);
            level--;
        }
        monitorExit(this);
        level--;
    }

    synchronized void syncFFSX() {
        level++;
        syncFSX();
        level--;
    }

    synchronized void syncFFSE() {
        level++;
        syncFSE();
        level--;
    }

    synchronized void syncFFSIX() {
        level++;
        syncFSIX();
        level--;
    }

    synchronized void syncFFSIE() {
        level++;
        syncFSIE();
        level--;
    }

    synchronized void syncFFISX() {
        level++;
        syncFISX();
        level--;
    }

    synchronized void syncFFISE() {
        level++;
        syncFISE();
        level--;
    }

    synchronized void syncFIFSX() {
        level++;
        inflate(this);
        syncFSX();
        level--;
    }

    synchronized void syncFIFSE() {
        level++;
        inflate(this);
        syncFSE();
        level--;
    }

    synchronized void syncFFSXE() {
        level++;
        syncFSXE();
        level--;
    }

    synchronized void syncFFSEX() {
        level++;
        syncFSEX();
        level--;
    }

    synchronized void syncFFSX_E() {
        level++;
        syncFSX_E();
        level--;
    }

    synchronized void syncFFSE_X() {
        level++;
        syncFSE_X();
        level--;
    }

    synchronized void syncFFSX__E() {
        level++;
        syncFSX();
        monitorEnter(this);
        level--;
    }

    synchronized void syncFFSE__X() {
        level++;
        syncFSE();
        monitorExit(this);
        level--;
    }

    synchronized void syncFSSX() {
        level++;
        synchronized(this) {
            level++;
            synchronized(this) {
                level++;
                monitorExit(this);
                level--;
            }
            level--;
        }
        level--;
    }

    synchronized void syncFSSE() {
        level++;
        synchronized(this) {
            level++;
            synchronized(this) {
                level++;
                monitorEnter(this);
                level--;
            }
            level--;
        }
        level--;
    }

    synchronized void syncFSSIX() {
        level++;
        synchronized(this) {
            level++;
            synchronized(this) {
                level++;
                inflate(this);
                monitorExit(this);
                level--;
            }
            level--;
        }
        level--;
    }

    synchronized void syncFSSIE() {
        level++;
        synchronized(this) {
            level++;
            synchronized(this) {
                level++;
                inflate(this);
                monitorEnter(this);
                level--;
            }
            level--;
        }
        level--;
    }

    synchronized void syncFSISX() {
        level++;
        synchronized(this) {
            level++;
            inflate(this);
            synchronized(this) {
                level++;
                monitorExit(this);
                level--;
            }
            level--;
        }
        level--;
    }

    synchronized void syncFSISE() {
        level++;
        synchronized(this) {
            level++;
            inflate(this);
            synchronized(this) {
                level++;
                monitorEnter(this);
                level--;
            }
            level--;
        }
        level--;
    }

    synchronized void syncFISSX() {
        level++;
        inflate(this);
        synchronized(this) {
            level++;
            synchronized(this) {
                level++;
                monitorExit(this);
                level--;
            }
            level--;
        }
        level--;
    }

    synchronized void syncFISSE() {
        level++;
        inflate(this);
        synchronized(this) {
            level++;
            synchronized(this) {
                level++;
                monitorEnter(this);
                level--;
            }
            level--;
        }
        level--;
    }

    synchronized void syncFSSXX() {
        level++;
        synchronized(this) {
            level++;
            synchronized(this) {
                level++;
                monitorExit(this);
                monitorExit(this);
                level--;
            }
            level--;
        }
        level--;
    }

    synchronized void syncFSSEE() {
        level++;
        synchronized(this) {
            level++;
            synchronized(this) {
                level++;
                monitorEnter(this);
                monitorEnter(this);
                level--;
            }
            level--;
        }
        level--;
    }

    synchronized void syncFSSXXX() {
        level++;
        synchronized(this) {
            level++;
            synchronized(this) {
                level++;
                monitorExit(this);
                monitorExit(this);
                monitorExit(this);
                level--;
            }
            level--;
        }
        level--;
    }

    synchronized void syncFSSEEE() {
        level++;
        synchronized(this) {
            level++;
            synchronized(this) {
                level++;
                monitorEnter(this);
                monitorEnter(this);
                monitorEnter(this);
                level--;
            }
            level--;
        }
        level--;
    }

    synchronized void syncFSSXE() {
        level++;
        synchronized(this) {
            level++;
            synchronized(this) {
                level++;
                monitorExit(this);
                monitorEnter(this);
                level--;
            }
            level--;
        }
        level--;
    }

    synchronized void syncFSSEX() {
        level++;
        synchronized(this) {
            level++;
            synchronized(this) {
                level++;
                monitorEnter(this);
                monitorExit(this);
                level--;
            }
            level--;
        }
        level--;
    }

    synchronized void syncFSSX_E() {
        level++;
        synchronized(this) {
            level++;
            synchronized(this) {
                level++;
                monitorExit(this);
                level--;
            }
            monitorEnter(this);
            level--;
        }
        level--;
    }

    synchronized void syncFSSE_X() {
        level++;
        synchronized(this) {
            level++;
            synchronized(this) {
                level++;
                monitorEnter(this);
                level--;
            }
            monitorExit(this);
            level--;
        }
        level--;
    }

    synchronized void syncFSSX__E() {
        level++;
        synchronized(this) {
            level++;
            synchronized(this) {
                level++;
                monitorExit(this);
                level--;
            }
            level--;
        }
        monitorEnter(this);
        level--;
    }

    synchronized void syncFSSE__X() {
        level++;
        synchronized(this) {
            level++;
            synchronized(this) {
                level++;
                monitorEnter(this);
                level--;
            }
            level--;
        }
        monitorExit(this);
        level--;
    }

    synchronized void syncFSFX() {
        level++;
        synchronized(this) {
            level++;
            syncFX();
            level--;
        }
        level--;
    }

    synchronized void syncFSFE() {
        level++;
        synchronized(this) {
            level++;
            syncFE();
            level--;
        }
        level--;
    }

    synchronized void syncFSFIX() {
        level++;
        synchronized(this) {
            level++;
            syncFIX();
            level--;
        }
        level--;
    }

    synchronized void syncFSFIE() {
        level++;
        synchronized(this) {
            level++;
            syncFIE();
            level--;
        }
        level--;
    }

    synchronized void syncFSIFX() {
        level++;
        synchronized(this) {
            level++;
            inflate(this);
            syncFX();
            level--;
        }
        level--;
    }

    synchronized void syncFSIFE() {
        level++;
        synchronized(this) {
            level++;
            inflate(this);
            syncFE();
            level--;
        }
        level--;
    }

    synchronized void syncFISFX() {
        level++;
        inflate(this);
        synchronized(this) {
            level++;
            syncFX();
            level--;
        }
        level--;
    }

    synchronized void syncFISFE() {
        level++;
        inflate(this);
        synchronized(this) {
            level++;
            syncFE();
            level--;
        }
        level--;
    }

    synchronized void syncFSFXX() {
        level++;
        synchronized(this) {
            level++;
            syncFXX();
            level--;
        }
        level--;
    }

    synchronized void syncFSFEE() {
        level++;
        synchronized(this) {
            level++;
            syncFEE();
            level--;
        }
        level--;
    }

    synchronized void syncFSFXXX() {
        level++;
        synchronized(this) {
            level++;
            syncFXXX();
            level--;
        }
        level--;
    }

    synchronized void syncFSFEEE() {
        level++;
        synchronized(this) {
            level++;
            syncFEEE();
            level--;
        }
        level--;
    }

    synchronized void syncFSFXE() {
        level++;
        synchronized(this) {
            level++;
            syncFXE();
            level--;
        }
        level--;
    }

    synchronized void syncFSFEX() {
        level++;
        synchronized(this) {
            level++;
            syncFEX();
            level--;
        }
        level--;
    }

    synchronized void syncFSFX_E() {
        level++;
        synchronized(this) {
            level++;
            syncFX();
            monitorEnter(this);
            level--;
        }
        level--;
    }

    synchronized void syncFSFE_X() {
        level++;
        synchronized(this) {
            level++;
            syncFE();
            monitorExit(this);
            level--;
        }
        level--;
    }

    synchronized void syncFSFX__E() {
        level++;
        synchronized(this) {
            level++;
            syncFX();
            level--;
        }
        monitorEnter(this);
        level--;
    }

    synchronized void syncFSFE__X() {
        level++;
        synchronized(this) {
            level++;
            syncFE();
            level--;
        }
        monitorExit(this);
        level--;
    }

    synchronized void syncFFSXX() {
        level++;
        syncFSXX();
        level--;
    }

    synchronized void syncFFSEE() {
        level++;
        syncFSEE();
        level--;
    }

    synchronized void syncFFSXXX() {
        level++;
        syncFSXXX();
        level--;
    }

    synchronized void syncFFSEEE() {
        level++;
        syncFSEEE();
        level--;
    }

    static void testHoldsLock() {
        boolean success;

        System.out.println("");
        System.out.println("Holds lock tests:");
        //====================================================================
        // Holds lock tests:

        // Case 0: holdsLock
	{
            Object o = new Object();
            success = !Thread.holdsLock(o);
        }
        passIf(success, "holdsLock");

        // Case 1: enter,holdsLock
	{
            Object o = new Object();
            monitorEnter(o);
            success = Thread.holdsLock(o);
            monitorExit(o);
        }
        passIf(success, "enter,holdsLock");

        // Case 2: enter,exit,holdsLock
	{
            Object o = new Object();
            monitorEnter(o);
            monitorExit(o);
            success = !Thread.holdsLock(o);
        }
        passIf(success, "enter,exit,holdsLock");

        // Case 3: enter,enter,holdsLock
	{
            Object o = new Object();
            monitorEnter(o);
            monitorEnter(o);
            success = Thread.holdsLock(o);
            monitorExit(o);
            monitorExit(o);
        }
        passIf(success, "enter,enter,holdsLock");

        // Case 4: enter,enter,exit,holdsLock
	{
            Object o = new Object();
            monitorEnter(o);
            monitorEnter(o);
            monitorExit(o);
            success = Thread.holdsLock(o);
            monitorExit(o);
        }
        passIf(success, "enter,enter,exit,holdsLock");

        // Case 5: enter,enter,exit,exit,holdsLock
	{
            Object o = new Object();
            monitorEnter(o);
            monitorEnter(o);
            monitorExit(o);
            monitorExit(o);
            success = !Thread.holdsLock(o);
        }
        passIf(success, "enter,enter,exit,exit,holdsLock");

        // Case 6: sync{holdsLock}
	{
            Object o = new Object();
            synchronized(o) {
                success = Thread.holdsLock(o);
            }
        }
        passIf(success, "sync{holdsLock}");

        // Case 7: sync{},holdsLock
	{
            Object o = new Object();
            synchronized(o) {}
            success = !Thread.holdsLock(o);
        }
        passIf(success, "sync{},holdsLock");

        // Case 8: sync{sync{holdsLock}}
	{
            Object o = new Object();
            synchronized(o) {
                synchronized(o) {
                    success = Thread.holdsLock(o);
                }
            }
        }
        passIf(success, "sync{sync{holdsLock}}");

        // Case 9: sync{sync{},holdsLock}
	{
            Object o = new Object();
            synchronized(o) {
                synchronized(o) {}
                success = Thread.holdsLock(o);
	    }
        }
        passIf(success, "sync{sync{},holdsLock}");

        // Case 10: sync{sync{}},holdsLock
	{
            Object o = new Object();
            synchronized(o) {
                synchronized(o) {}
	    }
            success = !Thread.holdsLock(o);
        }
        passIf(success, "sync{sync{}},holdsLock");

        // Case 11: enter,sync{holdsLock}
	{
            Object o = new Object();
            monitorEnter(o);
            synchronized(o) {
                success = Thread.holdsLock(o);
	    }
            monitorExit(o);
        }
        passIf(success, "enter,sync{holdsLock}");

        // Case 12: sync{enter,holdsLock,exit}
	{
            Object o = new Object();
            synchronized(o) {
                monitorEnter(o);
                success = Thread.holdsLock(o);
                monitorExit(o);
	    }
        }
        passIf(success, "sync{enter,holdsLock,exit}");

        // Case 13: inflate,holdsLock
	{
            Object o = new Object();
            inflate(o);
            success = !Thread.holdsLock(o);
        }
        passIf(success, "inflate,holdsLock");

        // Case 14: inflate,enter,holdsLock
	{
            Object o = new Object();
            inflate(o);
            monitorEnter(o);
            success = Thread.holdsLock(o);
            monitorExit(o);
        }
        passIf(success, "inflate,enter,holdsLock");

        // Case 15: enter,inflate,holdsLock
	{
            Object o = new Object();
            monitorEnter(o);
	    inflate(o);
            success = Thread.holdsLock(o);
            monitorExit(o);
        }
        passIf(success, "enter,inflate,holdsLock");

        // Case 16: enter,inflate,exit,holdsLock
	{
            Object o = new Object();
            monitorEnter(o);
            inflate(o);
            monitorExit(o);
            success = !Thread.holdsLock(o);
        }
        passIf(success, "enter,inflate,exit,holdsLock");

        // Case 17: enter,exit,inflate,holdsLock
	{
            Object o = new Object();
            monitorEnter(o);
            monitorExit(o);
	    inflate(o);
            success = !Thread.holdsLock(o);
        }
        passIf(success, "enter,exit,inflate,holdsLock");

        // Case 18: enter,inflate,enter,holdsLock
	{
            Object o = new Object();
            monitorEnter(o);
	    inflate(o);
            monitorEnter(o);
            success = Thread.holdsLock(o);
            monitorExit(o);
            monitorExit(o);
        }
        passIf(success, "enter,inflate,enter,holdsLock");

        // Case 19: enter,enter,inflate,holdsLock
	{
            Object o = new Object();
            monitorEnter(o);
            monitorEnter(o);
	    inflate(o);
            success = Thread.holdsLock(o);
            monitorExit(o);
            monitorExit(o);
        }
        passIf(success, "enter,enter,inflate,holdsLock");

        // Case 20: enter,inflate,enter,exit,holdsLock
	{
            Object o = new Object();
            monitorEnter(o);
	    inflate(o);
            monitorEnter(o);
            monitorExit(o);
            success = Thread.holdsLock(o);
            monitorExit(o);
        }
        passIf(success, "enter,inflate,enter,exit,holdsLock");

        // Case 21: enter,enter,inflate,exit,holdsLock
	{
            Object o = new Object();
            monitorEnter(o);
            monitorEnter(o);
	    inflate(o);
            monitorExit(o);
            success = Thread.holdsLock(o);
            monitorExit(o);
        }
        passIf(success, "enter,enter,inflate,exit,holdsLock");

        // Case 22: enter,enter,exit,inflate,holdsLock
	{
            Object o = new Object();
            monitorEnter(o);
            monitorEnter(o);
            monitorExit(o);
	    inflate(o);
            success = Thread.holdsLock(o);
            monitorExit(o);
        }
        passIf(success, "enter,enter,exit,inflate,holdsLock");

        // Case 23: enter,enter,exit,exit,inflate,holdsLock
	{
            Object o = new Object();
            monitorEnter(o);
            monitorEnter(o);
            monitorExit(o);
            monitorExit(o);
	    inflate(o);
            success = !Thread.holdsLock(o);
        }
        passIf(success, "enter,enter,exit,exit,inflate,holdsLock");

        // Case 24: inflate,sync{holdsLock}
	{
            Object o = new Object();
	    inflate(o);
            synchronized(o) {
                success = Thread.holdsLock(o);
            }
        }
        passIf(success, "inflate,sync{holdsLock}");

        // Case 25: sync{inflate,holdsLock}
	{
            Object o = new Object();
            synchronized(o) {
		inflate(o);
                success = Thread.holdsLock(o);
            }
        }
        passIf(success, "sync{inflate,holdsLock}");

        // Case 26: sync{},inflate,holdsLock
	{
            Object o = new Object();
            synchronized(o) {}
	    inflate(o);
            success = !Thread.holdsLock(o);
        }
        passIf(success, "sync{},inflate,holdsLock");

        // Case 27: sync{inflate,sync{holdsLock}}
	{
            Object o = new Object();
            synchronized(o) {
		inflate(o);
                synchronized(o) {
                    success = Thread.holdsLock(o);
                }
            }
        }
        passIf(success, "sync{inflate,sync{holdsLock}}");

        // Case 28: sync{sync{inflate,holdsLock}}
	{
            Object o = new Object();
            synchronized(o) {
                synchronized(o) {
		    inflate(o);
                    success = Thread.holdsLock(o);
                }
            }
        }
        passIf(success, "sync{sync{holdsLock}}");

        // Case 29: sync{sync{},inflate,holdsLock}
	{
            Object o = new Object();
            synchronized(o) {
                synchronized(o) {}
		inflate(o);
                success = Thread.holdsLock(o);
	    }
        }
        passIf(success, "sync{sync{},inflate,holdsLock}");

        // Case 30: sync{sync{}},inflate,holdsLock
	{
            Object o = new Object();
            synchronized(o) {
                synchronized(o) {}
	    }
	    inflate(o);
            success = !Thread.holdsLock(o);
        }
        passIf(success, "sync{sync{}},inflate,holdsLock");

        // Case 31: enter,inflate,sync{holdsLock}
	{
            Object o = new Object();
            monitorEnter(o);
	    inflate(o);
            synchronized(o) {
                success = Thread.holdsLock(o);
	    }
            monitorExit(o);
        }
        passIf(success, "enter,inflate,sync{holdsLock}");

        // Case 32: enter,sync{inflate,holdsLock}
	{
            Object o = new Object();
            monitorEnter(o);
            synchronized(o) {
		inflate(o);
                success = Thread.holdsLock(o);
	    }
            monitorExit(o);
        }
        passIf(success, "enter,sync{holdsLock}");

        // Case 33: sync{inflate,enter,holdsLock,exit}
	{
            Object o = new Object();
            synchronized(o) {
		inflate(o);
                monitorEnter(o);
                success = Thread.holdsLock(o);
                monitorExit(o);
	    }
        }
        passIf(success, "sync{inflate,enter,holdsLock,exit}");

        // Case 34: sync{enter,inflate,holdsLock,exit}
	{
            Object o = new Object();
            synchronized(o) {
                monitorEnter(o);
		inflate(o);
                success = Thread.holdsLock(o);
                monitorExit(o);
	    }
        }
        passIf(success, "sync{enter,inflate,holdsLock,exit}");

        ///////////////////////////////////////////

        // Case 1: t2.enter,holdsLock
	{
            Object o = new Object();
            t2.monitorEnter(o);
            success = !Thread.holdsLock(o);
            t2.monitorExit(o);
        }
        passIf(success, "t2.enter,holdsLock");

        // Case 2: t2.enter,t2.exit,holdsLock
	{
            Object o = new Object();
            t2.monitorEnter(o);
            t2.monitorExit(o);
            success = !Thread.holdsLock(o);
        }
        passIf(success, "t2.lock,t2.exit,holdsLock");

        // Case 3: t2.enter,t2.enter,holdsLock
	{
            Object o = new Object();
            t2.monitorEnter(o);
            t2.monitorEnter(o);
            success = !Thread.holdsLock(o);
            t2.monitorExit(o);
            t2.monitorExit(o);
        }
        passIf(success, "t2.enter,t2.enter,holdsLock");

        // Case 4: t2.enter,t2.enter,t2.exit,holdsLock
	{
            Object o = new Object();
            t2.monitorEnter(o);
            t2.monitorEnter(o);
            t2.monitorExit(o);
            success = !Thread.holdsLock(o);
            t2.monitorExit(o);
        }
        passIf(success, "t2.enter,t2.enter,t2.exit,holdsLock");

        // Case 5: t2.enter,t2.enter,t2.exit,t2.exit,holdsLock
	{
            Object o = new Object();
            t2.monitorEnter(o);
            t2.monitorEnter(o);
            t2.monitorExit(o);
            t2.monitorExit(o);
            success = !Thread.holdsLock(o);
        }
        passIf(success, "t2.enter,t2.enter,t2.exit,t2.exit,holdsLock");

        // Case 6: t2.sync{holdsLock}
	{
            Object o = new Object();
            t2.sync(o);
            success = !Thread.holdsLock(o);
            t2.unsync(o);
        }
        passIf(success, "t2.sync{holdsLock}");

        // Case 7: t2.sync{},holdsLock
	{
            Object o = new Object();
            t2.sync(o);
	    t2.unsync(o);
            success = !Thread.holdsLock(o);
        }
        passIf(success, "t2.sync{},holdsLock");

        // Case 8: t2.sync{t2.sync{holdsLock}}
	{
            Object o = new Object();
            t2.sync(o);
                t2.sync(o);
                    success = !Thread.holdsLock(o);
		t2.unsync(o);
	    t2.unsync(o);
        }
        passIf(success, "t2.sync{t2.sync{holdsLock}}");

        // Case 9: t2.sync{t2.sync{},holdsLock}
	{
            Object o = new Object();
            t2.sync(o);
	    t2.sync(o);
	    t2.unsync(o);
            success = !Thread.holdsLock(o);
	    t2.unsync(o);
        }
        passIf(success, "t2.sync{t2.sync{},holdsLock}");

        // Case 10: t2.sync{t2.sync{}},holdsLock
	{
            Object o = new Object();
            t2.sync(o);
	    t2.sync(o);
	    t2.unsync(o);
	    t2.unsync(o);
            success = !Thread.holdsLock(o);
        }
        passIf(success, "t2.sync{t2.sync{}},holdsLock");

        // Case 11: t2.enter,t2.sync{holdsLock}
	{
            Object o = new Object();
            t2.monitorEnter(o);
            t2.sync(o);
            success = !Thread.holdsLock(o);
	    t2.unsync(o);
            t2.monitorExit(o);
        }
        passIf(success, "t2.enter,t2.sync{holdsLock}");

        // Case 12: t2.sync{t2.enter,holdsLock,t2.exit}
	{
            Object o = new Object();
            t2.sync(o);
                t2.monitorEnter(o);
                success = !Thread.holdsLock(o);
                t2.monitorExit(o);
	    t2.unsync(o);
        }
        passIf(success, "t2.sync{t2.enter,holdsLock,t2.exit}");

        // Case 13: t2.inflate,holdsLock
	{
            Object o = new Object();
            t2.inflate(o);
            success = !Thread.holdsLock(o);
        }
        passIf(success, "t2.inflate,holdsLock");

        // Case 14: t2.inflate,t2.enter,holdsLock
	{
            Object o = new Object();
            t2.inflate(o);
            t2.monitorEnter(o);
            success = !Thread.holdsLock(o);
            t2.monitorExit(o);
        }
        passIf(success, "t2.inflate,t2.enter,holdsLock");

        // Case 15: t2.enter,t2.inflate,holdsLock
	{
            Object o = new Object();
            t2.monitorEnter(o);
	    t2.inflate(o);
            success = !Thread.holdsLock(o);
            t2.monitorExit(o);
        }
        passIf(success, "t2.enter,t2.inflate,holdsLock");

        // Case 16: t2.enter,t2.inflate,t2.exit,holdsLock
	{
            Object o = new Object();
            t2.monitorEnter(o);
            t2.inflate(o);
            t2.monitorExit(o);
            success = !Thread.holdsLock(o);
        }
        passIf(success, "t2.enter,t2.inflate,t2.exit,holdsLock");

        // Case 17: t2.enter,t2.exit,t2.inflate,holdsLock
	{
            Object o = new Object();
            monitorEnter(o);
            monitorExit(o);
	    inflate(o);
            success = !Thread.holdsLock(o);
        }
        passIf(success, "enter,exit,inflate,holdsLock");

        // Case 18: t2.enter,t2.inflate,t2.enter,holdsLock
	{
            Object o = new Object();
            t2.monitorEnter(o);
	    t2.inflate(o);
            t2.monitorEnter(o);
            success = !Thread.holdsLock(o);
            t2.monitorExit(o);
            t2.monitorExit(o);
        }
        passIf(success, "t2.enter,t2.inflate,t2.enter,holdsLock");

        // Case 19: t2.enter,t2.enter,t2.inflate,holdsLock
	{
            Object o = new Object();
            t2.monitorEnter(o);
            t2.monitorEnter(o);
	    t2.inflate(o);
            success = !Thread.holdsLock(o);
            t2.monitorExit(o);
            t2.monitorExit(o);
        }
        passIf(success, "t2.enter,t2.enter,t2.inflate,holdsLock");

        // Case 20: t2.enter,t2.inflate,t2.enter,t2.exit,holdsLock
	{
            Object o = new Object();
            t2.monitorEnter(o);
	    t2.inflate(o);
            t2.monitorEnter(o);
            t2.monitorExit(o);
            success = !Thread.holdsLock(o);
            t2.monitorExit(o);
        }
        passIf(success, "t2.enter,t2.inflate,t2.enter,t2.exit,holdsLock");

        // Case 21: t2.enter,t2.enter,t2.inflate,t2.exit,holdsLock
	{
            Object o = new Object();
            t2.monitorEnter(o);
            t2.monitorEnter(o);
	    t2.inflate(o);
            t2.monitorExit(o);
            success = !Thread.holdsLock(o);
            t2.monitorExit(o);
        }
        passIf(success, "t2.enter,t2.enter,t2.inflate,t2.exit,holdsLock");

        // Case 22: t2.enter,t2.enter,t2.exit,t2.inflate,holdsLock
	{
            Object o = new Object();
            t2.monitorEnter(o);
            t2.monitorEnter(o);
            t2.monitorExit(o);
	    t2.inflate(o);
            success = !Thread.holdsLock(o);
            t2.monitorExit(o);
        }
        passIf(success, "t2.enter,t2.enter,t2.exit,t2.inflate,holdsLock");

        // Case 23: t2.enter,t2.enter,t2.exit,t2.exit,t2.inflate,holdsLock
	{
            Object o = new Object();
            t2.monitorEnter(o);
            t2.monitorEnter(o);
            t2.monitorExit(o);
            t2.monitorExit(o);
	    t2.inflate(o);
            success = !Thread.holdsLock(o);
        }
        passIf(success, "t2.enter,t2.enter,t2.exit,t2.exit,t2.inflate,holdsLock");

        // Case 24: t2.inflate,t2.sync{holdsLock}
	{
            Object o = new Object();
	    t2.inflate(o);
            t2.sync(o);
            success = !Thread.holdsLock(o);
            t2.unsync(o);
        }
        passIf(success, "t2.inflate,t2.sync{holdsLock}");

        // Case 25: t2.sync{t2.inflate,holdsLock}
	{
            Object o = new Object();
            t2.sync(o);
	    t2.inflate(o);
            success = !Thread.holdsLock(o);
            t2.unsync(o);
        }
        passIf(success, "t2.sync{t2.inflate,holdsLock}");

        // Case 26: t2.sync{},t2.inflate,holdsLock
	{
            Object o = new Object();
            t2.sync(o);
	    t2.unsync(o);
	    t2.inflate(o);
            success = !Thread.holdsLock(o);
        }
        passIf(success, "t2.sync{},t2.inflate,holdsLock");

        // Case 27: t2.sync{t2.inflate,t2.sync{holdsLock}}
	{
            Object o = new Object();
            t2.sync(o);
	    t2.inflate(o);
            t2.sync(o);
            success = !Thread.holdsLock(o);
	    t2.unsync(o);
            t2.unsync(o);
        }
        passIf(success, "t2.sync{t2.inflate,t2.sync{holdsLock}}");

        // Case 28: t2.sync{t2.sync{t2.inflate,holdsLock}}
	{
            Object o = new Object();
            t2.sync(o);
            t2.sync(o);
	    t2.inflate(o);
            success = !Thread.holdsLock(o);
            t2.unsync(o);
            t2.unsync(o);
        }
        passIf(success, "t2.sync{t2.sync{holdsLock}}");

        // Case 29: t2.sync{t2.sync{},t2.inflate,holdsLock}
	{
            Object o = new Object();
            t2.sync(o);
            t2.sync(o);
	    t2.unsync(o);
	    t2.inflate(o);
            success = !Thread.holdsLock(o);
	    t2.unsync(o);
        }
        passIf(success, "t2.sync{t2.sync{},t2.inflate,holdsLock}");

        // Case 30: t2.sync{t2.sync{}},t2.inflate,holdsLock
	{
            Object o = new Object();
            t2.sync(o);
            t2.sync(o);
	    t2.unsync(o);
	    t2.unsync(o);
	    t2.inflate(o);
            success = !Thread.holdsLock(o);
        }
        passIf(success, "t2.sync{t2.sync{}},t2.inflate,holdsLock");

        // Case 31: t2.enter,t2.inflate,t2.sync{holdsLock}
	{
            Object o = new Object();
            t2.monitorEnter(o);
	    t2.inflate(o);
            t2.sync(o);
            success = !Thread.holdsLock(o);
	    t2.unsync(o);
            t2.monitorExit(o);
        }
        passIf(success, "t2.enter,t2.inflate,t2.sync{holdsLock}");

        // Case 32: t2.enter,t2.sync{t2.inflate,holdsLock}
	{
            Object o = new Object();
            t2.monitorEnter(o);
            t2.sync(o);
	    t2.inflate(o);
            success = !Thread.holdsLock(o);
	    t2.unsync(o);
            t2.monitorExit(o);
        }
        passIf(success, "t2.enter,t2.sync{holdsLock}");

        // Case 33: t2.sync{t2.inflate,t2.enter,holdsLock,t2.exit}
	{
            Object o = new Object();
            t2.sync(o);
	    t2.inflate(o);
            t2.monitorEnter(o);
            success = !Thread.holdsLock(o);
            t2.monitorExit(o);
	    t2.unsync(o);
        }
        passIf(success, "t2.sync{t2.inflate,t2.enter,holdsLock,t2.exit}");

        // Case 34: t2.sync{t2.enter,t2.inflate,holdsLock,t2.exit}
	{
            Object o = new Object();
            t2.sync(o);
            t2.monitorEnter(o);
	    t2.inflate(o);
            success = !Thread.holdsLock(o);
            t2.monitorExit(o);
	    t2.unsync(o);
        }
        passIf(success, "t2.sync{t2.enter,t2.inflate,holdsLock,t2.exit}");
    }

    static void testLargeReentryLocking(long maxReentryCount) {
        System.out.println("");
        System.out.println("Large monitorenter reentry tests:");

        {
            long i = 0;
            Object o = new Object();
            boolean success = false;
            System.out.print("Testing large monenter reentry ");
            try {
                for (i = 0; i < maxReentryCount; i++) {
                    //System.out.println("    monenter " + i);
                    if (i % 10000 == 1) {
                        System.out.print(".");
                    }
                    monitorEnter(o);
                }
            } catch (IllegalMonitorStateException e) {
                System.out.println("");
                System.out.println("    Caught " + e + " at the " + i +
                                   "th entry");
                // Clean up the uneven monitorEnters above:
                for (long j = 0; j < i; j++) {
                    monitorExit(o);
                }
                success = true;
            }
            if (!success) {
                System.out.println("");
            }
            passIf(success, "large monitor reentry");
        }

        {
            long i = 0;
            Object o = new Object();
            boolean success = false;
            System.out.print("Testing large inflated monenter test ");
            inflate(o);
            try {
                for (i = 0; i < maxReentryCount; i++) {
                    //System.out.println("    monenter " + i);
                    if (i % 10000 == 1) {
                        System.out.print(".");
                    }
                    monitorEnter(o);
                }
            } catch (IllegalMonitorStateException e) {
                System.out.println("");
                System.out.println("    Caught " + e + " at the " + i +
                                   "th entry");
                // Clean up the uneven monitorEnters above:
                for (long j = 0; j < i; j++) {
                    monitorExit(o);
                }
                success = true;
            }
            if (!success) {
                System.out.println("");
            }
            passIf(success, "inflated large monitor reentry");
        }
    }

    static void testExcessMonitorEnter() {
        boolean success;

        System.out.println("");
        System.out.println("Excess monitorenter tests:");
        //====================================================================
        // Excess monitorexit on synchronized blocks:

        // Case 0: enter,sync{}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            monitorEnter(o);
            level++;
            synchronized(o) {
                level++;
                level--;
            }
            level--;
            decLevels(o);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "enter,sync{}", level, -1);

        // Case 1: sync{enter}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                monitorEnter(o);
                level--;
            }
            level--;
            decLevels(o);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "sync{enter}", level, -1);

        // Case 2: sync{sync{enter}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    monitorEnter(o);
                    level--;
                }
                level--;
            }
            level--;
            decLevels(o);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "sync{sync{enter}}", level, -1);

        // Case 3: sync{sync{enter,enter}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    monitorEnter(o);
                    monitorEnter(o);
                    level--;
                }
                level--;
            }
            level--;
            decLevels(o);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "sync{sync{enter,enter}}", level, -2);

        // Case 4: sync{sync{enter,enter,enter}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    monitorEnter(o);
                    monitorEnter(o);
                    monitorEnter(o);
                    level--;
                }
                level--;
            }
            level--;
            decLevels(o);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "sync{sync{enter,enter,enter}}", level, -3);

        // Case 5: sync{exit,enter,enter}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                monitorExit(o);
                monitorEnter(o);
                monitorEnter(o);
                level--;
            }
            level--;
            decLevels(o);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "sync{exit,enter,enter}", level, -1);

        // Case 6: sync{sync{exit,enter,enter}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    monitorExit(o);
                    monitorEnter(o);
                    monitorEnter(o);
                    level--;
                }
                level--;
            }
            level--;
            decLevels(o);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "sync{sync{exit,enter,enter}}", level, -1);

        // Case 7: sync{sync{exit},enter,enter}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    monitorExit(o);
                    level--;
                }
                monitorEnter(o);
                monitorEnter(o);
                level--;
            }
            level--;
            decLevels(o);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "sync{sync{exit},enter,enter}", level, -1);

        //====================================================================
        // Excess monitorenter on synchronized methods:

        // Case 0: enter,fsync{}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            monitorEnter(t);
            level++;
            t.syncF();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "enter,fsync{}", level, -1);

        // Case 1: fsync{enter}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{enter}", level, -1);

        // Case 2: fsync{fsync{enter}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{enter}}", level, -1);

        // Case 3: fsync{fsync{enter,enter}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFEE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{enter,enter}}", level, -2);

        // Case 4: fsync{fsync{enter,enter,enter}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFEEE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{enter,enter,enter}}", level, -3);

        // Case 5: fsync{exit,enter,enter}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFXEE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{exit,enter,enter}", level, -1);

        // Case 5: fsync{enter,exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFEX();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{enter,exit}", level, 0);

        // Case 6: fsync{fsync{exit,enter,enter}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFXEE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{exit,enter,enter}}", level, -1);

        // Case 6: fsync{fsync{enter,exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFEX();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{enter,exit}}", level, 0);

        // Case 7: fsync{fsync{exit},enter,enter}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFX_EE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{exit},enter,enter}", level, -1);

        // Case 7: fsync{fsync{enter},exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFE_X();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{enter},exit}", level, 0);

        //====================================================================
        // Excess monitorenter on a mixture of synchronized methods and blocks:

        // Case 1: sync{fsync{enter}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{enter}}", level, -1);

        // Case 2: fsync{sync{enter}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{enter}}", level, -1);

        // Case 3: sync{fsync{enter,enter}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFEE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{enter,enter}}", level, -2);

        // Case 4: fsync{sync{enter,enter}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSEE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{enter,entre}}", level, -2);

        // Case 5: sync{fsync{enter,enter,enter}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFEEE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{enter,enter,enter}}", level, -3);

        // Case 6: fsync{sync{enter,enter,enter}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSEEE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{enter,enter,enter}}", level, -3);

        // Case 7: sync{fsync{enter,exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFEX();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{enter,exit}}", level, 0);

        // Case 8: fsync{sync{enter,exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSXE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{enter,exit}}", level, 0);

        // Case 9: sync{fsync{enter},exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFE();
                monitorExit(t);
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{enter},exit}", level, 0);

        // Case 10: fsync{sync{enter},exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSE_X();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{exit},enter}", level, 0);

        //====================================================================
        // Excess monitorexit on a mixture of synchronized methods and blocks
        // with re-entrant syncs:

        // Case 1: sync{sync{sync{enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    synchronized(t) {
                        level++;
                        monitorEnter(t);
                        level--;
                    }
                    level--;
                }
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "sync{sync{sync{enter}}}", level, -1);

        // Case 2: sync{sync{fsync{enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    t.syncFE();
                    level--;
                }
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "sync{sync{fsync{enter}}}", level, -1);

        // Case 3: sync{fync{sync{enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{sync{enter}}}", level, -1);

        // Case 4: sync{fync{fsync{enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFFE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{fsync{enter}}}", level, -1);

        // Case 5: fsync{sync{sync{enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSSE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{sync{enter}}}", level, -1);

        // Case 6: fsync{sync{fsync{enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSFE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{fsync{enter}}}", level, -1);

        // Case 7: fsync{fsync{sync{enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFSE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{sync{enter}}}", level, -1);

        // Case 8: fsync{fsync{fsync{enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFFE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{fsync{enter}}}", level, -1);

        // Case 9: sync{sync{sync{enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    synchronized(t) {
                        level++;
                        monitorEnter(t);
                        monitorEnter(t);
                        level--;
                    }
                    level--;
                }
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "sync{sync{sync{enter,enter}}}", level, -2);

        // Case 10: sync{sync{fsync{enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    t.syncFEE();
                    level--;
                }
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "sync{sync{fsync{enter,enter}}}", level, -2);

        // Case 11: sync{fync{sync{enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSEE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{sync{enter,enter}}}", level, -2);

        // Case 12: sync{fync{fsync{enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFFEE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{fsync{enter,enter}}}", level, -2);

        // Case 13: fsync{sync{sync{enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSSEE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{sync{enter,enter}}}", level, -2);

        // Case 14: fsync{sync{fsync{enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSFEE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{fsync{enter,enter}}}", level, -2);

        // Case 15: fsync{fsync{sync{enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFSEE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{sync{enter,enter}}}", level, -2);

        // Case 16: fsync{fsync{fsync{enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFFEE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{fsync{enter,enter}}}", level, -2);

        // Case 17: sync{sync{sync{enter,enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    synchronized(t) {
                        level++;
                        monitorEnter(t);
                        monitorEnter(t);
                        monitorEnter(t);
                        level--;
                    }
                    level--;
                }
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "sync{sync{sync{enter,enter,enter}}}", level, -3);

        // Case 18: sync{sync{fsync{enter,enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    t.syncFEEE();
                    level--;
                }
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "sync{sync{fsync{enter,enter,enter}}}", level, -3);

        // Case 19: sync{fync{sync{enter,enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSEEE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{sync{enter,enter,enter}}}", level, -3);

        // Case 20: sync{fync{fsync{enter,enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFFEEE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{fsync{enter,enter,enter}}}", level, -3);

        // Case 21: fsync{sync{sync{enter,enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSSEEE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{sync{enter,enter,enter}}}", level, -3);

        // Case 22: fsync{sync{fsync{enter,enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSFEEE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{fsync{enter,enter,enter}}}", level, -3);

        // Case 23: fsync{fsync{sync{enter,enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFSEEE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{sync{enter,enter,enter}}}", level, -3);

        // Case 24: fsync{fsync{fsync{enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFFEEE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{fsync{enter,enter,enter}}}", level, -3);

        // Case 25: sync{sync{sync{enter,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    synchronized(t) {
                        level++;
                        monitorEnter(t);
                        monitorExit(t);
                        level--;
                    }
                    level--;
                }
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{sync{sync{enter,exit}}}", level, 0);

        // Case 26: sync{sync{fsync{enter,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    t.syncFEX();
                    level--;
                }
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{sync{fsync{enter,exit}}}", level, 0);

        // Case 27: sync{fync{sync{enter,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSEX();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{sync{enter,exit}}}", level, 0);

        // Case 28: sync{fync{fsync{enter,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFFEX();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{fsync{enter,exit}}}", level, 0);

        // Case 29: fsync{sync{sync{enter,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSSEX();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{sync{enter,exit}}}", level, 0);

        // Case 30: fsync{sync{fsync{enter,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSFEX();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{fsync{enter,exit}}}", level, 0);

        // Case 31: fsync{fsync{sync{enter,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFSEX();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{sync{enter,exit}}}", level, 0);

        // Case 32: fsync{fsync{fsync{enter,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFFEX();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{fsync{enter,exit}}}", level, 0);

        // Case 33: sync{sync{sync{enter},exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    synchronized(t) {
                        level++;
                        monitorEnter(t);
                        level--;
                    }
                    monitorExit(t);
                    level--;
                }
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{sync{sync{enter},exit}}", level, 0);

        // Case 34: sync{sync{fsync{enter},exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    t.syncFE();
                    level--;
                }
                monitorExit(t);
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{sync{fsync{enter},exit}}", level, 0);

        // Case 35: sync{fync{sync{enter},exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSE_X();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{sync{enter},exit}}", level, 0);

        // Case 36: sync{fync{fsync{enter},exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFFE_X();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{fsync{enter},exit}}", level, 0);

        // Case 37: fsync{sync{sync{enter},exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSSE_X();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{sync{enter},exit}}", level, 0);

        // Case 38: fsync{sync{fsync{enter},exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSFE_X();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{fsync{enter},exit}}", level, 0);

        // Case 39: fsync{fsync{sync{enter},exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFSE_X();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{sync{enter},exit}}", level, 0);

        // Case 40: fsync{fsync{fsync{enter},exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFFE_X();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{fsync{enter},exit}}", level, 0);

        // Case 41: sync{sync{sync{enter}},exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    synchronized(t) {
                        level++;
                        monitorEnter(t);
                        level--;
                    }
                    level--;
                }
                monitorExit(t);
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
           }
        }
        passIf(success, "sync{sync{sync{enter}},exit}", level, 0);

        // Case 42: sync{sync{fsync{enter}},exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    t.syncFE();
                    level--;
                }
                monitorExit(t);
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{sync{fsync{enter}},exit}", level, 0);

        // Case 43: sync{fync{sync{enter}},exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSE();
                monitorExit(t);
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{sync{enter}},exit}", level, 0);

        // Case 44: sync{fync{fsync{enter}},exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFFE();
                monitorExit(t);
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                 success = true;
            }
        }
        passIf(success, "sync{fsync{fsync{enter}},exit}", level, 0);

        // Case 45: fsync{sync{sync{enter}},exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSSE__X();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{sync{enter}},exit}", level, 0);

        // Case 46: fsync{sync{fsync{enter}},exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSFE__X();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{fsync{enter}},exit}", level, 0);

        // Case 47: fsync{fsync{sync{enter}},exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFSE__X();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{sync{enter}},exit}", level, 0);

        // Case 48: fsync{fsync{fsync{enter}},exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFFE__X();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{fsync{enter}},exit}", level, 0);

        //====================================================================
        // Inflation Test:

        //====================================================================
        // Excess monitorexit on synchronized blocks:

        // Case 1: sync{inflate,enter}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                inflate(o);
                monitorEnter(o);
                level--;
            }
            level--;
            decLevels(o);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "sync{inflate,enter}", level, -1);

        // Case 2: sync{sync{inflate,enter}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    inflate(o);
                    monitorEnter(o);
                    level--;
                }
                level--;
            }
            level--;
            decLevels(o);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "sync{sync{inflate,enter}}", level, -1);

        // Case 2a: sync{inflate,sync{enter}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                inflate(o);
                synchronized(o) {
                    level++;
                    monitorEnter(o);
                    level--;
                }
                level--;
            }
            level--;
            decLevels(o);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "sync{inflate,sync{enter}}", level, -1);

        // Case 3: sync{sync{inflate,enter,enter}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    inflate(o);
                    monitorEnter(o);
                    monitorEnter(o);
                    level--;
                }
                level--;
            }
            level--;
            decLevels(o);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "sync{sync{inflate,enter,enter}}", level, -2);

        // Case 3a: sync{sync{enter,inflate,enter}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    monitorEnter(o);
                    inflate(o);
                    monitorEnter(o);
                    level--;
                }
                level--;
            }
            level--;
            decLevels(o);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "sync{sync{enter,inflate,enter}}", level, -2);

        // Case 3b: sync{inflate,sync{enter,enter}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                inflate(o);
                synchronized(o) {
                    level++;
                    monitorEnter(o);
                    monitorEnter(o);
                    level--;
                }
                level--;
            }
            level--;
            decLevels(o);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "sync{inflate,sync{enter,enter}}", level, -2);

        // Case 4: sync{sync{inflate,enter,enter,enter}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    inflate(o);
                    monitorEnter(o);
                    monitorEnter(o);
                    monitorEnter(o);
                    level--;
                }
                level--;
            }
            level--;
            decLevels(o);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "sync{sync{inflate,enter,enter,enter}}", level, -3);

        // Case 4: sync{sync{enter,inflate,enter,enter}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    monitorEnter(o);
                    inflate(o);
                    monitorEnter(o);
                    monitorEnter(o);
                    level--;
                }
                level--;
            }
            level--;
            decLevels(o);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "sync{sync{enter,inflate,enter,enter}}", level, -3);

        // Case 4: sync{sync{enter,inflate,enter,enter}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                inflate(o);
                synchronized(o) {
                    level++;
                    monitorEnter(o);
                    monitorEnter(o);
                    monitorEnter(o);
                    level--;
                }
                level--;
            }
            level--;
            decLevels(o);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "sync{inflate,sync{enter,enter,enter}}", level, -3);

        // Case 5: sync{inflate,enter,exit}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                inflate(o);
                monitorEnter(o);
                monitorExit(o);
                level--;
            }
            level--;
            decLevels(o);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{inflate,enter,exit}", level, 0);

        // Case 6: sync{sync{inflate,enter,exit}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    inflate(o);
                    monitorEnter(o);
                    monitorExit(o);
                    level--;
                }
                level--;
            }
            level--;
            decLevels(o);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{sync{inflate,enter,exit}}", level, 0);

        // Case 6: sync{sync{enter,inflate,exit}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    monitorEnter(o);
                    inflate(o);
                    monitorExit(o);
                    level--;
                }
                level--;
            }
            level--;
            decLevels(o);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{sync{enter,inflate,exit}}", level, 0);

        // Case 6: sync{sync{enter,exit,inflate}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    monitorEnter(o);
                    monitorExit(o);
                    inflate(o);
                    level--;
                }
                level--;
            }
            level--;
            decLevels(o);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{sync{enter,exit,inflate}}", level, 0);

        // Case 6: sync{inflate,sync{enter,exit}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                inflate(o);
                synchronized(o) {
                    level++;
                    monitorEnter(o);
                    monitorExit(o);
                    level--;
                }
                level--;
            }
            level--;
            decLevels(o);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{inflate,sync{enter,exit}}", level, 0);

        // Case 6: sync{sync{enter,exit},inflate}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    monitorEnter(o);
                    monitorExit(o);
                    level--;
                }
                inflate(o);
                level--;
            }
            level--;
            decLevels(o);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{sync{enter,exit},inflate}", level, 0);

        // Case 7: sync{sync{inflated,enter},exit}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    inflate(o);
                    monitorEnter(o);
                    level--;
                }
                monitorExit(o);
                level--;
            }
            level--;
            decLevels(o);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{sync{inflated,enter},exit}", level, 0);

        // Case 7: sync{inflated,sync{enter},exit}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                inflate(o);
                synchronized(o) {
                    level++;
                    monitorEnter(o);
                    level--;
                }
                monitorExit(o);
                level--;
            }
            level--;
            decLevels(o);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{inflated,sync{enter},exit}", level, 0);

        // Case 7: sync{sync{enter},exit,inflated}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    monitorEnter(o);
                    level--;
                }
                monitorExit(o);
                inflate(o);
                level--;
            }
            level--;
            decLevels(o);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{sync{enter},exit,inflated}", level, 0);

        //====================================================================
        // Excess monitorenter on synchronized methods:

        // Case 1: fsync{inflate,enter}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFIE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{inflate,enter}", level, -1);

        // Case 2: fsync{fsync{inflate,enter}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFIE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{inflate,enter}}", level, -1);

        // Case 2: fsync{fsync{enter,inflate}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFEI();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{enter,inflate}}", level, -1);

        // Case 2: fsync{inflate,fsync{enter}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFIFE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{inflate,fsync{enter}}", level, -1);

        // Case 3: fsync{fsync{inflate,enter,enter}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFIEE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{inflate,enter,enter}}", level, -2);

        // Case 3: fsync{fsync{enter,inflate,enter}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFEIE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{enter,inflate,enter}}", level, -2);

        // Case 3: fsync{inflate,fsync{enter,enter}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFIFEE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "fsync{inflate,fsync{enter,enter}}", level, -2);

        // Case 4: fsync{fsync{inflate,enter,enter,enter}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFIEEE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{inflate,enter,enter,enter}}", level, -3);

        // Case 4: fsync{fsync{enter,inflate,enter,enter}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFEIEE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{enter,inflate,enter,enter}}", level, -3);

        // Case 4: fsync{inflate,fsync{enter,enter,enter}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFIFEEE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "fsync{inflate,fsync{enter,enter,enter}}", level, -3);

        // Case 5: fsync{inflate,enter,exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFIEX();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{inflate,enter,exit}", level, 0);

        // Case 5: fsync{enter,exit,inflate}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFEXI();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{enter,exit,inflate}", level, 0);

        // Case 6: fsync{fsync{inflate,enter,exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFIEX();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{inflate,enter,exit}}", level, 0);

        // Case 6: fsync{fsync{enter,inflate,exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFEIX();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{enter,inflate,exit}}", level, 0);

        // Case 6: fsync{fsync{enter,exit,inflate}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFEXI();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{enter,exit,inflate}}", level, 0);

        // Case 6: fsync{inflate,fsync{enter,exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFIFEX();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{inflate,fsync{enter,exit}}", level, 0);

        // Case 6: fsync{fsync{enter,exit},inflate}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFEX_I();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{enter,exit},inflate}", level, 0);

        // Case 7: fsync{fsync{inflate,enter},exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFIE_X();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{inflate,enter},exit}", level, 0);

        // Case 7: fsync{fsync{enter,inflate},exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFEI_X();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{enter,inflate},exit}", level, 0);

        // Case 7: fsync{inflate,fsync{enter},exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFIFE_X();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{inflate,fsync{enter},exit}", level, 0);

        // Case 7: fsync{fsync{enter},exit,inflate}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFE_XI();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{enter},exit,inflate}", level, 0);

/*
        //====================================================================
        // Excess monitorexit on a mixture of synchronized methods and blocks:

        // Case 1: sync{fsync{enter}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{enter}}", level, -1);

        // Case 2: fsync{sync{enter}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{enter}}", level, -1);

        // Case 3: sync{fsync{enter,enter}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFEE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{enter,enter}}", level, -2);

        // Case 4: fsync{sync{enter,enter}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSEE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{enter,enter}}", level, -2);

        // Case 5: sync{fsync{enter,enter,enter}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFEEE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{enter,enter,enter}}", level, -3);

        // Case 6: fsync{sync{enter,enter,enter}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSEEE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{enter,enter,enter}}", level, -3);

        // Case 7: sync{fsync{enter,exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFEX();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{enter,exit}}", level, 0);

        // Case 8: fsync{sync{enter,exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSEX();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{enter,exit}}", level, 0);

        // Case 9: sync{fsync{enter},exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFE();
                monitorExit(t);
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{enter},exit}", level, 0);

        // Case 10: fsync{sync{enter},exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSE_X();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{enter},exit}", level, 0);
*/
        //====================================================================
        // Excess monitorenter on a mixture of synchronized methods and blocks
        // with re-entrant syncs:

        // Case 1: sync{sync{sync{inflate,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    synchronized(t) {
                        level++;
                        inflate(t);
                        monitorEnter(t);
                        level--;
                    }
                    level--;
                }
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "sync{sync{sync{inflate,enter}}}", level, -1);

        // Case 2: sync{sync{fsync{inflate,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    t.syncFIE();
                    level--;
                }
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "sync{sync{fsync{inflate,enter}}}", level, -1);

        // Case 3: sync{fync{sync{inflate,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSIE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{sync{inflate,enter}}}", level, -1);

        // Case 4: sync{fync{fsync{inflate,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFFIE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{fsync{inflate,enter}}}", level, -1);

        // Case 5: fsync{sync{sync{inflate,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSSIE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{sync{inflate,enter}}}", level, -1);

        // Case 6: fsync{sync{fsync{inflate,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSFIE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{fsync{inflate,enter}}}", level, -1);

        // Case 7: fsync{fsync{sync{inflate,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFSIE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{sync{inflate,enter}}}", level, -1);

        // Case 8: fsync{fsync{fsync{inflate,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFFIE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{fsync{inflate,enter}}}", level, -1);

        // Case 1: sync{sync{inflate,sync{enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    inflate(t);
                    synchronized(t) {
                        level++;
                        monitorEnter(t);
                        level--;
                    }
                    level--;
                }
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "sync{sync{inflate,sync{enter}}}", level, -1);

        // Case 2: sync{sync{inflate,fsync{enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    inflate(t);
                    t.syncFE();
                    level--;
                }
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "sync{sync{inflate,fsync{enter}}}", level, -1);

        // Case 3: sync{fync{inflate,sync{enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFISE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{inflate,sync{enter}}}", level, -1);

        // Case 4: sync{fync{inflate,fsync{enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFIFE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{inflate,fsync{enter}}}", level, -1);

        // Case 5: fsync{sync{inflate,sync{enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSISE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{inflate,sync{enter}}}", level, -1);

        // Case 6: fsync{sync{inflate,fsync{enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSIFE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{inflate,fsync{enter}}}", level, -1);

        // Case 7: fsync{fsync{inflate,sync{enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFISE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{inflate,sync{enter}}}", level, -1);

        // Case 8: fsync{fsync{inflate,fsync{enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFIFE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{inflate,fsync{enter}}}", level, -1);

        // Case 1: sync{inflate,sync{sync{enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                inflate(t);
                synchronized(t) {
                    level++;
                    synchronized(t) {
                        level++;
                        monitorEnter(t);
                        level--;
                    }
                    level--;
                }
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "sync{inflate,sync{sync{enter}}}", level, -1);

        // Case 2: sync{inflate,sync{fsync{enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                inflate(t);
                synchronized(t) {
                    level++;
                    t.syncFE();
                    level--;
                }
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "sync{inflate,sync{fsync{enter}}}", level, -1);

        // Case 3: sync{inflate,fync{sync{enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                inflate(t);
                t.syncFSE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "sync{inflate,fsync{sync{enter}}}", level, -1);

        // Case 4: sync{inflate,fync{fsync{enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                inflate(t);
                t.syncFIFE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "sync{inflate,fsync{fsync{enter}}}", level, -1);

        // Case 5: fsync{inflate,sync{sync{enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFISSE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{inflate,sync{sync{enter}}}", level, -1);

        // Case 6: fsync{inflate,sync{fsync{enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFISFE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{inflate,sync{fsync{enter}}}", level, -1);

        // Case 7: fsync{inflate,fsync{sync{enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFIFSE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{inflate,fsync{sync{enter}}}", level, -1);

        // Case 8: fsync{inflate,fsync{fsync{enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFIFFE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -1) {
                success = true;
            }
        }
        passIf(success, "fsync{inflate,fsync{fsync{enter}}}", level, -1);

/*
        // Case 9: sync{sync{sync{enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    synchronized(t) {
                        level++;
                        monitorEnter(t);
                        monitorEnter(t);
                        level--;
                    }
                    level--;
                }
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "sync{sync{sync{enter,enter}}}", level, -2);

        // Case 10: sync{sync{fsync{enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    t.syncFEE();
                    level--;
                }
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "sync{sync{fsync{enter,enter}}}", level, -2);

        // Case 11: sync{fync{sync{enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSEE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{sync{enter,enter}}}", level, -2);

        // Case 12: sync{fync{fsync{enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFFEE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{fsync{enter,enter}}}", level, -2);

        // Case 13: fsync{sync{sync{enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSSEE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{sync{enter,enter}}}", level, -2);

        // Case 14: fsync{sync{fsync{enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSFEE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{fsync{enter,enter}}}", level, -2);

        // Case 15: fsync{fsync{sync{enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFSEE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{sync{enter,enter}}}", level, -2);

        // Case 16: fsync{fsync{fsync{enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFFEE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -2) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{fsync{enter,enter}}}", level, -2);

        // Case 17: sync{sync{sync{enter,enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    synchronized(t) {
                        level++;
                        monitorEnter(t);
                        monitorEnter(t);
                        monitorEnter(t);
                        level--;
                    }
                    level--;
                }
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "sync{sync{sync{enter,enter,enter}}}", level, -3);

        // Case 18: sync{sync{fsync{enter,enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    t.syncFEEE();
                    level--;
                }
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "sync{sync{fsync{enter,enter,enter}}}", level, -3);

        // Case 19: sync{fync{sync{enter,enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSEEE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{sync{enter,enter,enter}}}", level, -3);

        // Case 20: sync{fync{fsync{enter,enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFFEEE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{fsync{enter,enter,enter}}}", level, -3);

        // Case 21: fsync{sync{sync{enter,enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSSEEE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{sync{enter,enter,enter}}}", level, -3);

        // Case 22: fsync{sync{fsync{enter,enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSFEEE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{fsync{enter,enter,enter}}}", level, -3);

        // Case 23: fsync{fsync{sync{enter,enter,enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFSEEE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{sync{enter,enter,enter}}}", level, -3);

        // Case 24: fsync{fsync{fsync{enter}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFFEEE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == -3) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{fsync{enter,enter,enter}}}", level, -3);

        // Case 25: sync{sync{sync{enter,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    synchronized(t) {
                        level++;
                        monitorEnter(t);
                        monitorExit(t);
                        level--;
                    }
                    level--;
                }
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{sync{sync{enter,exit}}}", level, 0);

        // Case 26: sync{sync{fsync{enter,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    t.syncFXE();
                    level--;
                }
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{sync{fsync{enter,exit}}}", level, 0);

        // Case 27: sync{fync{sync{enter,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSXE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{sync{enter,exit}}}", level, 0);

        // Case 28: sync{fync{fsync{enter,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFFXE();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{fsync{enter,exit}}}", level, 0);

        // Case 29: fsync{sync{sync{enter,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSSXE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{sync{enter,exit}}}", level, 0);

        // Case 30: fsync{sync{fsync{enter,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSFXE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{fsync{enter,exit}}}", level, 0);

        // Case 31: fsync{fsync{sync{enter,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFSXE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{sync{enter,exit}}}", level, 0);

        // Case 32: fsync{fsync{fsync{enter,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFFXE();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{fsync{enter,exit}}}", level, 0);

        // Case 33: sync{sync{sync{enter},exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    synchronized(t) {
                        level++;
                        monitorEnter(t);
                        level--;
                    }
                    monitorExit(t);
                    level--;
                }
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{sync{sync{enter},exit}}", level, 0);

        // Case 34: sync{sync{fsync{enter},exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    t.syncFX();
                    level--;
                }
                monitorExit(t);
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{sync{fsync{enter},exit}}", level, 0);

        // Case 35: sync{fync{sync{enter},exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSX_E();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{sync{enter},exit}}", level, 0);

        // Case 36: sync{fync{fsync{enter},exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFFX_E();
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{fsync{enter},exit}}", level, 0);

        // Case 37: fsync{sync{sync{enter},exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSSX_E();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{sync{enter},exit}}", level, 0);

        // Case 38: fsync{sync{fsync{enter},exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSFX_E();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{fsync{enter},exit}}", level, 0);

        // Case 39: fsync{fsync{sync{enter},exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFSX_E();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{sync{enter},exit}}", level, 0);

        // Case 40: fsync{fsync{fsync{enter},exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFFX_E();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{fsync{enter},exit}}", level, 0);

        // Case 41: sync{sync{sync{enter}},exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    synchronized(t) {
                        level++;
                        monitorEnter(t);
                        level--;
                    }
                    level--;
                }
                monitorExit(t);
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{sync{sync{enter}},exit}", level, 0);

        // Case 42: sync{sync{fsync{enter}},exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    t.syncFX();
                    level--;
                }
                monitorExit(t);
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{sync{fsync{enter}},exit}", level, 0);

        // Case 43: sync{fync{sync{enter}},exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSX();
                monitorExit(t);
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{sync{enter}},exit}", level, 0);

        // Case 44: sync{fync{fsync{enter}},exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFFX();
                monitorExit(t);
                level--;
            }
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{fsync{enter}},exit}", level, 0);

        // Case 45: fsync{sync{sync{enter}},exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSSX__E();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{sync{enter}},exit}", level, 0);

        // Case 46: fsync{sync{fsync{enter}},exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSFX__E();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{fsync{enter}},exit}", level, 0);

        // Case 47: fsync{fsync{sync{enter}},exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFSX__E();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{sync{enter}},exit}", level, 0);

        // Case 48: fsync{fsync{fsync{enter}},exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFFX__E();
            level--;
            decLevels(t);
        } catch (IllegalMonitorStateException e) {
            if (level == 0) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{fsync{enter}},exit}", level, 0);
*/
        //
    }

    static void testExcessMonitorExit() {
        boolean success;

        System.out.println("");
        System.out.println("Excess monitorexit tests:");
        //====================================================================
        // Excess monitorexit on synchronized blocks:

        // Case 1: sync{exit}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                monitorExit(o);
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "sync{exit}", level, 1);

        // Case 2: sync{sync{exit}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    monitorExit(o);
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "sync{sync{exit}}", level, 1);

        // Case 3: sync{sync{exit,exit}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    monitorExit(o);
                    monitorExit(o);
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "sync{sync{exit,exit}}", level, 2);

        // Case 4: sync{sync{exit,exit,exit}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    monitorExit(o);
                    monitorExit(o);
                    monitorExit(o);
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "sync{sync{exit,exit,exit}}", level, 3);

        // Case 5: sync{exit,enter}
        success = true;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                monitorExit(o);
                monitorEnter(o);
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{exit,enter}", level, 0);

        // Case 6: sync{sync{exit,enter}}
        success = true;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    monitorExit(o);
                    monitorEnter(o);
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{sync{exit,enter}}", level, 0);

        // Case 7: sync{sync{exit},enter}
        success = true;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    monitorExit(o);
                    level--;
                }
                monitorEnter(o);
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{sync{exit},enter}", level, 0);

        //====================================================================
        // Excess monitorexit on synchronized methods:

        // Case 1: fsync{exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "fsync{exit}", level, 1);

        // Case 2: fsync{fsync{exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{exit}}", level, 1);

        // Case 3: fsync{fsync{exit,exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFXX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{exit,exit}}", level, 2);

        // Case 4: fsync{fsync{exit,exit,exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFXXX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{exit,exit,exit}}", level, 3);

        // Case 5: fsync{exit,enter}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFXE();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{exit,enter}", level, 0);

        // Case 6: fsync{fsync{exit,enter}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFXE();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{fsync{exit,enter}}", level, 0);

        // Case 7: fsync{fsync{exit},enter}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFX_E();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{fsync{exit},enter}", level, 0);

        //====================================================================
        // Excess monitorexit on a mixture of synchronized methods and blocks:

        // Case 1: sync{fsync{exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFX();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{exit}}", level, 1);

        // Case 2: fsync{sync{exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{exit}}", level, 1);

        // Case 3: sync{fsync{exit,exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFXX();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{exit,exit}}", level, 2);

        // Case 4: fsync{sync{exit,exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSXX();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{exit,exit}}", level, 2);

        // Case 5: sync{fsync{exit,exit,exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFXXX();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{exit,exit,exit}}", level, 3);

        // Case 6: fsync{sync{exit,exit,exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSXXX();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{exit,exit,exit}}", level, 3);

        // Case 7: sync{fsync{exit,enter}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFXE();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{fsync{exit,enter}}", level, 0);

        // Case 8: fsync{sync{exit,enter}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSXE();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{sync{exit,enter}}", level, 0);

        // Case 9: sync{fsync{exit},enter}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFX();
                monitorEnter(t);
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{fsync{exit},enter}", level, 0);

        // Case 10: fsync{sync{exit},enter}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSX_E();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{sync{exit},enter}", level, 0);

        //====================================================================
        // Excess monitorexit on a mixture of synchronized methods and blocks
        // with re-entrant syncs:

        // Case 1: sync{sync{sync{exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    synchronized(t) {
                        level++;
                        monitorExit(t);
                        level--;
                    }
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "sync{sync{sync{exit}}}", level, 1);

        // Case 2: sync{sync{fsync{exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    t.syncFX();
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "sync{sync{fsync{exit}}}", level, 1);

        // Case 3: sync{fync{sync{exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSX();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{sync{exit}}}", level, 1);

        // Case 4: sync{fync{fsync{exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFFX();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{fsync{exit}}}", level, 1);

        // Case 5: fsync{sync{sync{exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSSX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{sync{exit}}}", level, 1);

        // Case 6: fsync{sync{fsync{exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSFX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{fsync{exit}}}", level, 1);

        // Case 7: fsync{fsync{sync{exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFSX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{sync{exit}}}", level, 1);

        // Case 8: fsync{fsync{fsync{exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFFX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{fsync{exit}}}", level, 1);

        // Case 9: sync{sync{sync{exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    synchronized(t) {
                        level++;
                        monitorExit(t);
                        monitorExit(t);
                        level--;
                    }
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "sync{sync{sync{exit,exit}}}", level, 2);

        // Case 10: sync{sync{fsync{exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    t.syncFXX();
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "sync{sync{fsync{exit,exit}}}", level, 2);

        // Case 11: sync{fync{sync{exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSXX();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{sync{exit,exit}}}", level, 2);

        // Case 12: sync{fync{fsync{exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFFXX();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{fsync{exit,exit}}}", level, 2);

        // Case 13: fsync{sync{sync{exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSSXX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{sync{exit,exit}}}", level, 2);

        // Case 14: fsync{sync{fsync{exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSFXX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{fsync{exit,exit}}}", level, 2);

        // Case 15: fsync{fsync{sync{exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFSXX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{sync{exit,exit}}}", level, 2);

        // Case 16: fsync{fsync{fsync{exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFFXX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{fsync{exit,exit}}}", level, 2);

        // Case 17: sync{sync{sync{exit,exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    synchronized(t) {
                        level++;
                        monitorExit(t);
                        monitorExit(t);
                        monitorExit(t);
                        level--;
                    }
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "sync{sync{sync{exit,exit,exit}}}", level, 3);

        // Case 18: sync{sync{fsync{exit,exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    t.syncFXXX();
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "sync{sync{fsync{exit,exit,exit}}}", level, 3);

        // Case 19: sync{fync{sync{exit,exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSXXX();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{sync{exit,exit,exit}}}", level, 3);

        // Case 20: sync{fync{fsync{exit,exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFFXXX();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{fsync{exit,exit,exit}}}", level, 3);

        // Case 21: fsync{sync{sync{exit,exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSSXXX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{sync{exit,exit,exit}}}", level, 3);

        // Case 22: fsync{sync{fsync{exit,exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSFXXX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{fsync{exit,exit,exit}}}", level, 3);

        // Case 23: fsync{fsync{sync{exit,exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFSXXX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{sync{exit,exit,exit}}}", level, 3);

        // Case 24: fsync{fsync{fsync{exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFFXXX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{fsync{exit,exit,exit}}}", level, 3);

        // Case 25: sync{sync{sync{exit,enter}}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    synchronized(t) {
                        level++;
                        monitorExit(t);
                        monitorEnter(t);
                        level--;
                    }
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{sync{sync{exit,enter}}}", level, 0);

        // Case 26: sync{sync{fsync{exit,enter}}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    t.syncFXE();
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{sync{fsync{exit,enter}}}", level, 0);

        // Case 27: sync{fync{sync{exit,enter}}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSXE();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{fsync{sync{exit,enter}}}", level, 0);

        // Case 28: sync{fync{fsync{exit,enter}}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFFXE();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{fsync{fsync{exit,enter}}}", level, 0);

        // Case 29: fsync{sync{sync{exit,enter}}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSSXE();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{sync{sync{exit,enter}}}", level, 0);

        // Case 30: fsync{sync{fsync{exit,enter}}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSFXE();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{sync{fsync{exit,enter}}}", level, 0);

        // Case 31: fsync{fsync{sync{exit,enter}}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFSXE();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{fsync{sync{exit,enter}}}", level, 0);

        // Case 32: fsync{fsync{fsync{exit,enter}}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFFXE();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{fsync{fsync{exit,enter}}}", level, 0);

        // Case 33: sync{sync{sync{exit},enter}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    synchronized(t) {
                        level++;
                        monitorExit(t);
                        level--;
                    }
                    monitorEnter(t);
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{sync{sync{exit},enter}}", level, 0);

        // Case 34: sync{sync{fsync{exit},enter}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    t.syncFX();
                    level--;
                }
                monitorEnter(t);
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{sync{fsync{exit},enter}}", level, 0);

        // Case 35: sync{fync{sync{exit},enter}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSX_E();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{fsync{sync{exit},enter}}", level, 0);

        // Case 36: sync{fync{fsync{exit},enter}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFFX_E();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{fsync{fsync{exit},enter}}", level, 0);

        // Case 37: fsync{sync{sync{exit},enter}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSSX_E();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{sync{sync{exit},enter}}", level, 0);

        // Case 38: fsync{sync{fsync{exit},enter}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSFX_E();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{sync{fsync{exit},enter}}", level, 0);

        // Case 39: fsync{fsync{sync{exit},enter}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFSX_E();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{fsync{sync{exit},enter}}", level, 0);

        // Case 40: fsync{fsync{fsync{exit},enter}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFFX_E();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{fsync{fsync{exit},enter}}", level, 0);

        // Case 41: sync{sync{sync{exit}},enter}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    synchronized(t) {
                        level++;
                        monitorExit(t);
                        level--;
                    }
                    level--;
                }
                monitorEnter(t);
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{sync{sync{exit}},enter}", level, 0);

        // Case 42: sync{sync{fsync{exit}},enter}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    t.syncFX();
                    level--;
                }
                monitorEnter(t);
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{sync{fsync{exit}},enter}", level, 0);

        // Case 43: sync{fync{sync{exit}},enter}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSX();
                monitorEnter(t);
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{fsync{sync{exit}},enter}", level, 0);

        // Case 44: sync{fync{fsync{exit}},enter}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFFX();
                monitorEnter(t);
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{fsync{fsync{exit}},enter}", level, 0);

        // Case 45: fsync{sync{sync{exit}},enter}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSSX__E();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{sync{sync{exit}},enter}", level, 0);

        // Case 46: fsync{sync{fsync{exit}},enter}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSFX__E();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{sync{fsync{exit}},enter}", level, 0);

        // Case 47: fsync{fsync{sync{exit}},enter}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFSX__E();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{fsync{sync{exit}},enter}", level, 0);

        // Case 48: fsync{fsync{fsync{exit}},enter}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFFX__E();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{fsync{fsync{exit}},enter}", level, 0);

        //====================================================================
        // Inflation Test:

        //====================================================================
        // Excess monitorexit on synchronized blocks:

        // Case 1: sync{inflate,exit}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                inflate(o);
                monitorExit(o);
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "sync{inflate,exit}", level, 1);

        // Case 2: sync{sync{inflate,exit}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    inflate(o);
                    monitorExit(o);
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "sync{sync{inflate,exit}}", level, 1);

        // Case 2a: sync{inflate,sync{exit}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                inflate(o);
                synchronized(o) {
                    level++;
                    monitorExit(o);
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "sync{inflate,sync{exit}}", level, 1);

        // Case 3: sync{sync{inflate,exit,exit}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    inflate(o);
                    monitorExit(o);
                    monitorExit(o);
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "sync{sync{inflate,exit,exit}}", level, 2);

        // Case 3a: sync{sync{exit,inflate,exit}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    monitorExit(o);
                    inflate(o);
                    monitorExit(o);
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "sync{sync{exit,inflate,exit}}", level, 2);

        // Case 3b: sync{inflate,sync{exit,exit}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                inflate(o);
                synchronized(o) {
                    level++;
                    monitorExit(o);
                    monitorExit(o);
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "sync{inflate,sync{exit,exit}}", level, 2);

        // Case 4: sync{sync{inflate,exit,exit,exit}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    inflate(o);
                    monitorExit(o);
                    monitorExit(o);
                    monitorExit(o);
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "sync{sync{inflate,exit,exit,exit}}", level, 3);

        // Case 4: sync{sync{exit,inflate,exit,exit}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    monitorExit(o);
                    inflate(o);
                    monitorExit(o);
                    monitorExit(o);
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "sync{sync{exit,inflate,exit,exit}}", level, 3);

        // Case 4: sync{sync{exit,inflate,exit,exit}}
        success = false;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                inflate(o);
                synchronized(o) {
                    level++;
                    monitorExit(o);
                    monitorExit(o);
                    monitorExit(o);
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "sync{inflate,sync{exit,exit,exit}}", level, 3);

        // Case 5: sync{inflate,exit,enter}
        success = true;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                inflate(o);
                monitorExit(o);
                monitorEnter(o);
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{inflate,exit,enter}", level, 0);

        // Case 6: sync{sync{inflate,exit,enter}}
        success = true;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    inflate(o);
                    monitorExit(o);
                    monitorEnter(o);
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{sync{inflate,exit,enter}}", level, 0);

        // Case 6: sync{sync{exit,inflate,enter}}
        success = true;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    monitorExit(o);
                    inflate(o);
                    monitorEnter(o);
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{sync{exit,inflate,enter}}", level, 0);

        // Case 6: sync{sync{exit,enter,inflate}}
        success = true;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    monitorExit(o);
                    monitorEnter(o);
                    inflate(o);
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{sync{exit,enter,inflate}}", level, 0);

        // Case 6: sync{inflate,sync{exit,enter}}
        success = true;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                inflate(o);
                synchronized(o) {
                    level++;
                    monitorExit(o);
                    monitorEnter(o);
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{inflate,sync{exit,enter}}", level, 0);

        // Case 6: sync{sync{exit,enter},inflate}
        success = true;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    monitorExit(o);
                    monitorEnter(o);
                    level--;
                }
                inflate(o);
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{sync{exit,enter},inflate}", level, 0);

        // Case 7: sync{sync{inflated,exit},enter}
        success = true;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    inflate(o);
                    monitorExit(o);
                    level--;
                }
                monitorEnter(o);
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{sync{inflated,exit},enter}", level, 0);

        // Case 7: sync{inflated,sync{exit},enter}
        success = true;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                inflate(o);
                synchronized(o) {
                    level++;
                    monitorExit(o);
                    level--;
                }
                monitorEnter(o);
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{inflated,sync{exit},enter}", level, 0);

        // Case 7: sync{sync{exit},enter,inflated}
        success = true;
        level = 0;
        try {
            Object o = new Object();
            level++;
            synchronized(o) {
                level++;
                synchronized(o) {
                    level++;
                    monitorExit(o);
                    level--;
                }
                monitorEnter(o);
                inflate(o);
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{sync{exit},enter,inflated}", level, 0);

        //====================================================================
        // Excess monitorexit on synchronized methods:

        // Case 1: fsync{inflate,exit}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFIX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "fsync{inflate,exit}", level, 1);

        // Case 2: fsync{fsync{inflate,exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFIX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{inflate,exit}}", level, 1);

        // Case 2: fsync{fsync{exit,inflate}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFXI();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{exit,inflate}}", level, 1);

        // Case 2: fsync{inflate,fsync{exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFIFX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "fsync{inflate,fsync{exit}}", level, 1);

        // Case 3: fsync{fsync{inflate,exit,exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFIXX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{inflate,exit,exit}}", level, 2);

        // Case 3: fsync{fsync{exit,inflate,exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFXIX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{exit,inflate,exit}}", level, 2);

        // Case 3: fsync{inflate,fsync{exit,exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFIFXX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "fsync{inflate,fsync{exit,exit}}", level, 2);

        // Case 4: fsync{fsync{inflate,exit,exit,exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFIXXX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{inflate,exit,exit,exit}}", level, 3);

        // Case 4: fsync{fsync{exit,inflate,exit,exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFXIXX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{exit,inflate,exit,exit}}", level, 3);

        // Case 4: fsync{inflate,fsync{exit,exit,exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFIFXXX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "fsync{inflate,fsync{exit,exit,exit}}", level, 3);

        // Case 5: fsync{inflate,exit,enter}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFIXE();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{inflate,exit,enter}", level, 0);

        // Case 5: fsync{exit,enter,inflate}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFXEI();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{exit,enter,inflate}", level, 0);

        // Case 6: fsync{fsync{inflate,exit,enter}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFIXE();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{fsync{inflate,exit,enter}}", level, 0);

        // Case 6: fsync{fsync{exit,inflate,enter}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFXIE();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{fsync{exit,inflate,enter}}", level, 0);

        // Case 6: fsync{fsync{exit,enter,inflate}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFXEI();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{fsync{exit,enter,inflate}}", level, 0);

        // Case 6: fsync{inflate,fsync{exit,enter}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFIFXE();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{inflate,fsync{exit,enter}}", level, 0);

        // Case 6: fsync{fsync{exit,enter},inflate}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFXE_I();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{fsync{exit,enter},inflate}", level, 0);

        // Case 7: fsync{fsync{inflate,exit},enter}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFIX_E();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{fsync{inflate,exit},enter}", level, 0);

        // Case 7: fsync{fsync{exit,inflate},enter}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFXI_E();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{fsync{exit,inflate},enter}", level, 0);

        // Case 7: fsync{inflate,fsync{exit},enter}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFIFX_E();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{inflate,fsync{exit},enter}", level, 0);

        // Case 7: fsync{fsync{exit},enter,inflate}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFX_EI();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{fsync{exit},enter,inflate}", level, 0);

/*
        //====================================================================
        // Excess monitorexit on a mixture of synchronized methods and blocks:

        // Case 1: sync{fsync{exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFX();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{exit}}", level, 1);

        // Case 2: fsync{sync{exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{exit}}", level, 1);

        // Case 3: sync{fsync{exit,exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFXX();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{exit,exit}}", level, 2);

        // Case 4: fsync{sync{exit,exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSXX();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{exit,exit}}", level, 2);

        // Case 5: sync{fsync{exit,exit,exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFXXX();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{exit,exit,exit}}", level, 3);

        // Case 6: fsync{sync{exit,exit,exit}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSXXX();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{exit,exit,exit}}", level, 3);

        // Case 7: sync{fsync{exit,enter}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFXE();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{fsync{exit,enter}}", level, 0);

        // Case 8: fsync{sync{exit,enter}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSXE();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{sync{exit,enter}}", level, 0);

        // Case 9: sync{fsync{exit},enter}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFX();
                monitorEnter(t);
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{fsync{exit},enter}", level, 0);

        // Case 10: fsync{sync{exit},enter}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSX_E();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{sync{exit},enter}", level, 0);
*/
        //====================================================================
        // Excess monitorexit on a mixture of synchronized methods and blocks
        // with re-entrant syncs:

        // Case 1: sync{sync{sync{inflate,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    synchronized(t) {
                        level++;
                        inflate(t);
                        monitorExit(t);
                        level--;
                    }
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "sync{sync{sync{inflate,exit}}}", level, 1);

        // Case 2: sync{sync{fsync{inflate,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    t.syncFIX();
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "sync{sync{fsync{inflate,exit}}}", level, 1);

        // Case 3: sync{fync{sync{inflate,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSIX();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{sync{inflate,exit}}}", level, 1);

        // Case 4: sync{fync{fsync{inflate,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFFIX();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{fsync{inflate,exit}}}", level, 1);

        // Case 5: fsync{sync{sync{inflate,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSSIX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{sync{inflate,exit}}}", level, 1);

        // Case 6: fsync{sync{fsync{inflate,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSFIX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{fsync{inflate,exit}}}", level, 1);

        // Case 7: fsync{fsync{sync{inflate,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFSIX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{sync{inflate,exit}}}", level, 1);

        // Case 8: fsync{fsync{fsync{inflate,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFFIX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{fsync{inflate,exit}}}", level, 1);

        // Case 1: sync{sync{inflate,sync{exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    inflate(t);
                    synchronized(t) {
                        level++;
                        monitorExit(t);
                        level--;
                    }
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "sync{sync{inflate,sync{exit}}}", level, 1);

        // Case 2: sync{sync{inflate,fsync{exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    inflate(t);
                    t.syncFX();
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "sync{sync{inflate,fsync{exit}}}", level, 1);

        // Case 3: sync{fync{inflate,sync{exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFISX();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{inflate,sync{exit}}}", level, 1);

        // Case 4: sync{fync{inflate,fsync{exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFIFX();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{inflate,fsync{exit}}}", level, 1);

        // Case 5: fsync{sync{inflate,sync{exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSISX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{inflate,sync{exit}}}", level, 1);

        // Case 6: fsync{sync{inflate,fsync{exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSIFX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{inflate,fsync{exit}}}", level, 1);

        // Case 7: fsync{fsync{inflate,sync{exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFISX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{inflate,sync{exit}}}", level, 1);

        // Case 8: fsync{fsync{inflate,fsync{exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFIFX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{inflate,fsync{exit}}}", level, 1);

        // Case 1: sync{inflate,sync{sync{exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                inflate(t);
                synchronized(t) {
                    level++;
                    synchronized(t) {
                        level++;
                        monitorExit(t);
                        level--;
                    }
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "sync{inflate,sync{sync{exit}}}", level, 1);

        // Case 2: sync{inflate,sync{fsync{exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                inflate(t);
                synchronized(t) {
                    level++;
                    t.syncFX();
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "sync{inflate,sync{fsync{exit}}}", level, 1);

        // Case 3: sync{inflate,fync{sync{exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                inflate(t);
                t.syncFSX();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "sync{inflate,fsync{sync{exit}}}", level, 1);

        // Case 4: sync{inflate,fync{fsync{exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                inflate(t);
                t.syncFIFX();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "sync{inflate,fsync{fsync{exit}}}", level, 1);

        // Case 5: fsync{inflate,sync{sync{exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFISSX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "fsync{inflate,sync{sync{exit}}}", level, 1);

        // Case 6: fsync{inflate,sync{fsync{exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFISFX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "fsync{inflate,sync{fsync{exit}}}", level, 1);

        // Case 7: fsync{inflate,fsync{sync{exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFIFSX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "fsync{inflate,fsync{sync{exit}}}", level, 1);

        // Case 8: fsync{inflate,fsync{fsync{exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFIFFX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 1) {
                success = true;
            }
        }
        passIf(success, "fsync{inflate,fsync{fsync{exit}}}", level, 1);

/*
        // Case 9: sync{sync{sync{exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    synchronized(t) {
                        level++;
                        monitorExit(t);
                        monitorExit(t);
                        level--;
                    }
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "sync{sync{sync{exit,exit}}}", level, 2);

        // Case 10: sync{sync{fsync{exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    t.syncFXX();
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "sync{sync{fsync{exit,exit}}}", level, 2);

        // Case 11: sync{fync{sync{exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSXX();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{sync{exit,exit}}}", level, 2);

        // Case 12: sync{fync{fsync{exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFFXX();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{fsync{exit,exit}}}", level, 2);

        // Case 13: fsync{sync{sync{exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSSXX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{sync{exit,exit}}}", level, 2);

        // Case 14: fsync{sync{fsync{exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSFXX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{fsync{exit,exit}}}", level, 2);

        // Case 15: fsync{fsync{sync{exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFSXX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{sync{exit,exit}}}", level, 2);

        // Case 16: fsync{fsync{fsync{exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFFXX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 2) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{fsync{exit,exit}}}", level, 2);

        // Case 17: sync{sync{sync{exit,exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    synchronized(t) {
                        level++;
                        monitorExit(t);
                        monitorExit(t);
                        monitorExit(t);
                        level--;
                    }
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "sync{sync{sync{exit,exit,exit}}}", level, 3);

        // Case 18: sync{sync{fsync{exit,exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    t.syncFXXX();
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "sync{sync{fsync{exit,exit,exit}}}", level, 3);

        // Case 19: sync{fync{sync{exit,exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSXXX();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{sync{exit,exit,exit}}}", level, 3);

        // Case 20: sync{fync{fsync{exit,exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFFXXX();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "sync{fsync{fsync{exit,exit,exit}}}", level, 3);

        // Case 21: fsync{sync{sync{exit,exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSSXXX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{sync{exit,exit,exit}}}", level, 3);

        // Case 22: fsync{sync{fsync{exit,exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSFXXX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "fsync{sync{fsync{exit,exit,exit}}}", level, 3);

        // Case 23: fsync{fsync{sync{exit,exit,exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFSXXX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{sync{exit,exit,exit}}}", level, 3);

        // Case 24: fsync{fsync{fsync{exit}}}
        success = false;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFFXXX();
            level--;
        } catch (IllegalMonitorStateException e) {
            if (level == 3) {
                success = true;
            }
        }
        passIf(success, "fsync{fsync{fsync{exit,exit,exit}}}", level, 3);

        // Case 25: sync{sync{sync{exit,enter}}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    synchronized(t) {
                        level++;
                        monitorExit(t);
                        monitorEnter(t);
                        level--;
                    }
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{sync{sync{exit,enter}}}", level, 0);

        // Case 26: sync{sync{fsync{exit,enter}}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    t.syncFXE();
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{sync{fsync{exit,enter}}}", level, 0);

        // Case 27: sync{fync{sync{exit,enter}}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSXE();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{fsync{sync{exit,enter}}}", level, 0);

        // Case 28: sync{fync{fsync{exit,enter}}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFFXE();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{fsync{fsync{exit,enter}}}", level, 0);

        // Case 29: fsync{sync{sync{exit,enter}}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSSXE();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{sync{sync{exit,enter}}}", level, 0);

        // Case 30: fsync{sync{fsync{exit,enter}}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSFXE();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{sync{fsync{exit,enter}}}", level, 0);

        // Case 31: fsync{fsync{sync{exit,enter}}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFSXE();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{fsync{sync{exit,enter}}}", level, 0);

        // Case 32: fsync{fsync{fsync{exit,enter}}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFFXE();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{fsync{fsync{exit,enter}}}", level, 0);

        // Case 33: sync{sync{sync{exit},enter}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    synchronized(t) {
                        level++;
                        monitorExit(t);
                        level--;
                    }
                    monitorEnter(t);
                    level--;
                }
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{sync{sync{exit},enter}}", level, 0);

        // Case 34: sync{sync{fsync{exit},enter}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    t.syncFX();
                    level--;
                }
                monitorEnter(t);
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{sync{fsync{exit},enter}}", level, 0);

        // Case 35: sync{fync{sync{exit},enter}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSX_E();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{fsync{sync{exit},enter}}", level, 0);

        // Case 36: sync{fync{fsync{exit},enter}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFFX_E();
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{fsync{fsync{exit},enter}}", level, 0);

        // Case 37: fsync{sync{sync{exit},enter}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSSX_E();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{sync{sync{exit},enter}}", level, 0);

        // Case 38: fsync{sync{fsync{exit},enter}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSFX_E();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{sync{fsync{exit},enter}}", level, 0);

        // Case 39: fsync{fsync{sync{exit},enter}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFSX_E();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{fsync{sync{exit},enter}}", level, 0);

        // Case 40: fsync{fsync{fsync{exit},enter}}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFFX_E();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{fsync{fsync{exit},enter}}", level, 0);

        // Case 41: sync{sync{sync{exit}},enter}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    synchronized(t) {
                        level++;
                        monitorExit(t);
                        level--;
                    }
                    level--;
                }
                monitorEnter(t);
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{sync{sync{exit}},enter}", level, 0);

        // Case 42: sync{sync{fsync{exit}},enter}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                synchronized(t) {
                    level++;
                    t.syncFX();
                    level--;
                }
                monitorEnter(t);
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{sync{fsync{exit}},enter}", level, 0);

        // Case 43: sync{fync{sync{exit}},enter}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFSX();
                monitorEnter(t);
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{fsync{sync{exit}},enter}", level, 0);

        // Case 44: sync{fync{fsync{exit}},enter}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            synchronized(t) {
                level++;
                t.syncFFX();
                monitorEnter(t);
                level--;
            }
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "sync{fsync{fsync{exit}},enter}", level, 0);

        // Case 45: fsync{sync{sync{exit}},enter}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSSX__E();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{sync{sync{exit}},enter}", level, 0);

        // Case 46: fsync{sync{fsync{exit}},enter}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFSFX__E();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{sync{fsync{exit}},enter}", level, 0);

        // Case 47: fsync{fsync{sync{exit}},enter}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFSX__E();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{fsync{sync{exit}},enter}", level, 0);

        // Case 48: fsync{fsync{fsync{exit}},enter}
        success = true;
        level = 0;
        try {
            TestSync t = new TestSync();
            level++;
            t.syncFFFX__E();
            level--;
        } catch (IllegalMonitorStateException e) {
            success = false;
        }
        passIf(success, "fsync{fsync{fsync{exit}},enter}", level, 0);
*/
        //
    }
}

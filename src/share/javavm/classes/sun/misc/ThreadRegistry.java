/*
 * @(#)ThreadRegistry.java	1.19 06/10/10
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

package sun.misc;

import java.util.Vector;
import java.security.AccessController;
import java.security.PrivilegedAction;

public class ThreadRegistry {

    public static void add(Thread t) {
	synchronized (staticLock) {
	    threadList.addElement(t);
	    dprintln("New thread " + t.getName());
	}
    }

    public static void remove(Thread t) {
	synchronized (staticLock) {
	    threadList.removeElement(t);
            // ThreadRegistry.remove() is called when the thread is about to
            // die, at the point after which no exceptions are to be expected.
            // Hence, we have to make sure that the following (which allocate
            // objects) do not result in a Throwable.
            try {
                dprintln("Thread " + t.getName() + " removed");
            } catch (Throwable throwable) {
                // Just discard the exception.
            }
	}
    }

    public static boolean threadCreationAllowed() {
	return !threadCreationDisabled;
    }

    public static void waitAllUserThreadsExit() {
	dprintln("Waiting for all user threads to exit");
	waitThreadsExit(false);
    }

    public static void waitAllSystemThreadsExit() {
	dprintln("Asking all SystemThreads to exit");
	allExitRequestedFlag = true;
	waitThreadsExit(true);
    }

    private static void waitThreadsExit(boolean systemThread) {
	Thread current = Thread.currentThread();
	boolean tryAgain;

	do {
	    Vector snapshot;
	    tryAgain = false;

	    synchronized (staticLock) {
		snapshot = (Vector)threadList.clone();
	    }
	    int n = snapshot.size();
	    if (systemThread) {
		// ask all the system thread to exit
		for (int i = 0; i < n; ++i) {
		    final Thread t = (Thread)snapshot.elementAt(i);
		    if (t == current) {
			continue;
		    }
		    // shouldn't be a user thread, but check it just in case
		    dprintln("Asking " + (t.isDaemon() ? "system" : "user") +
			" thread " + t.getName() + " to exit");
		    AccessController.doPrivileged(new PrivilegedAction() {
			public Object run() {
			    t.interrupt();
			    return null;
			}
		    });
		}
	    }
	    for (int i = 0; i < n; ++i) {
		Thread t = (Thread)snapshot.elementAt(i);
		if (t == current || (!systemThread && t.isDaemon())) {
		    continue;
		}
		dprintln("Waiting for " + (t.isDaemon() ? "system" : "user") + 
		    " thread " + t.getName() + " to exit");

		tryAgain = true;
		while (t.isAlive()) {
		    try {
			t.join();
			break;
		    } catch (InterruptedException ie) {
		    }
		}
	    }
	} while (tryAgain);
    }

    public static boolean exitRequested() {
	if (allExitRequestedFlag) {
	    dprintln("System thread " + Thread.currentThread().getName() +
		" has recognized the request to exit");
	}
	return allExitRequestedFlag;
    }

    public static void resetExitRequest() {
	allExitRequestedFlag = false;
    }

    private static synchronized void dprintln(String s) {
	if (CVM.checkDebugFlags(CVM.DEBUGFLAG_TRACE_MISC) != 0) {
	    // Be careful of initialization order problems
	    if (System.err != null) {
		if (debugBuffer != null) {
		    int n = debugBuffer.size();
		    for (int i = 0; i < n; ++i) {
			String str = (String)debugBuffer.elementAt(i);
			System.err.println(str);
		    }
		}
		debugBuffer = null;
		System.err.println(s);
	    } else {
		if (debugBuffer == null) {
		    debugBuffer = new Vector();
		}
		debugBuffer.addElement(s);
	    }
	}
    }

    private static Object staticLock = new Object();
    private static boolean allExitRequestedFlag = false;
    private static Vector threadList = new Vector();
    private static Vector debugBuffer;
    // %begin lvm
    // The following flag gets set from native code in LVM implementation.
    // %end lvm
    private static boolean threadCreationDisabled = false;

    /*
     * 4952560: These locks must always have an inflated monitor associated
     * with them so we never have to worry about ThreadRegistry.remove
     * failing under low memory conditions. Otherwise the VM will never
     * shutdown.
     */
    static {
	if (!sun.misc.CVM.objectInflatePermanently(staticLock)) {
	    throw new OutOfMemoryError();
	}
	if (!sun.misc.CVM.objectInflatePermanently(threadList)) {
	    throw new OutOfMemoryError();
	}
    }
}

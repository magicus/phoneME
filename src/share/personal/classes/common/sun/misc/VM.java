/*
 * @(#)VM.java	1.21 06/10/10
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

/**
 * The Sun.misc.vm class provides an interface to the memory
 * management system of the Virtual Machine so that applications
 * and applets can be informed of the state of memory system.
 * Memory States :
 * <li>Green 
 * When the memory state is "Green", the vm has sufficient memory
 * to run normally.
 * <li>Yellow
 * When the memory state is "Yellow", the vm still has sufficient
 * memory to run, but large allocations may fail, so an application
 * may wish to free unnecessary resources or make other space saving trade-offs.
 * <li>Red
 * When the memory state is Red, the vm is critically low on memory.
 * All unnecessary memory should be freed and unnecessary applications should exit.
 * <p>
 * These states are represented with the constants
 * <code>VM.STATE_GREEN, VM.STATE_YELLOW, and VM.STATE_RED.</code>
 * <h3>Compatibility</h3>
 * Some PersonalJava implementations support the Sun.misc.vm class.
 * It is not supported in the J2SE.
 */
public class VM implements Runnable {
    public static final int STATE_GREEN = 1;
    public static final int STATE_YELLOW = 2;
    public static final int STATE_RED = 3;
    private byte memoryAdvice = STATE_GREEN;	// Updated by GC
    private Object memoryAdviceLock = new Object();
    private Vector callbacks = new Vector(1);
    private Object[] callbacksCopy = new Object[1];
    private Thread callbackThread;
    private static VM singleton = null;
    /**
     * Compute the current memory state.  This may be an expensive operation,
     * because a GC might be triggered to produce an accurate result.
     * @return the current memory state
     *
     */
    //    public static final native int getState();

    /**
     * Report the (approximate) memory state at the end of the last GC.  This will
     * be a quick operation that should give a useful approximation of the
     * real state of the system.
     *
     * @return the current memory state
     */
    public static final int getStateQuick() {
        return getSingleton().memoryAdvice;
    }

    /**
     * Register a notifier to be informed when availability of
     * memory in the system changes state.
     * <p>
     * After calling the <code>registerVMNotification()</code> method, the
     * notifier will be invoked when the specified changes in memory state occur.
     * @see sun.misc.VMNotification
     */
    public static void registerVMNotification(VMNotification n) {
        getSingleton().registerCallback(n);
    }
    
    //
    // Don't let anyone else instantiate this class
    //
    private VM() {//	registerWithGC();
    }

    private static VM getSingleton() {
        if (singleton == null) {
            createSingleton();
        }
        return singleton;
    }

    private static synchronized void createSingleton() {
        if (singleton == null) {  // We must re-test this within synchronized
            singleton = new VM();
        }
    }

    //    private native void registerWithGC();

    private void registerCallback(VMNotification n) {
        synchronized (this) {
            if (callbackThread == null) {
                callbackThread = new Thread(singleton, "RYG Callback Thread");
                callbackThread.setPriority(Thread.MAX_PRIORITY - 1);
                callbackThread.start();
            }
            // we update callbacksCopy first in case the memory allocation
            // triggers a state change.  The gc will attempt to acquire
            // the lock that we already own, which is ok since it's
            // only going to update fields we're not playing with.
            synchronized (callbacks) {
                if (callbacks.size() + 1 > callbacksCopy.length) {
                    callbacksCopy = new Object[callbacks.size() + 1];
                }
                callbacks.addElement(n);
            }
        }
    }

    /**
     * User code is not meant to call this.
     */
    // It's public because that's a requirement of runnable.  This isn't 
    // a security hole, because untrusted code can't call into sun.*.
    public void run() {
        int previousState = STATE_GREEN;
        int currentState;
        while (true) {
            try {
                while (true) {
                    synchronized (memoryAdviceLock) {
                        currentState = memoryAdvice;
                        if (currentState != previousState) {
                            break;
                        }
                        memoryAdviceLock.wait();
                    }
                }
                // We go to some effort to avoid doing callbacks while holding
                // locks.  Doing otherwise is just asking for trouble.
                int copySize;
                Object[] cbc;
                synchronized (callbacks) {
                    // guaranteed to fit by registerCallback
                    callbacks.copyInto(callbacksCopy);
                    copySize = callbacks.size();
                    // callbacksCopy (the variable, not the object)
                    // may get stored into from registerCallback, so
                    // make a copy of the reference.
                    cbc = callbacksCopy;
                }
                // do the callbacks outside of the synchronized block to
                // avoid locking problems
                for (int i = 0; i < copySize; i++) {
                    VMNotification n = (VMNotification) cbc[i];
                    try {
                        n.newAllocState(previousState, currentState, true);
                    } catch (Throwable t) {
                        try {
                            t.printStackTrace();
                        } catch (Throwable ignored) {}
                    }
                }
                previousState = currentState;
            } catch (Throwable t) {
                try {
                    // don't let the RYG Callback thread exit for any reason.
                    t.printStackTrace();
                } catch (Throwable e) {}
            }
        }
    }
}

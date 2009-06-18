/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
 */

package com.sun.midp.automation;

/**
 * Utility class for tracking states.
 */
class AutoStateTracker {
    /*
     * Protected by 'this' lock
     */
    
    /** Current state */
    private Object currentState;

    /** Number of waiters currently waiting */
    private int totalWaiters;
    
    /** flag, indicating that wait must be interrupted */
    private boolean interruptWait;
    
    
    /*
     * Protected by wakeWaitersLock
     */

    /** Lock used to wait for waiters to awake */
    private Object wakeWaitersLock;

    /** Number of waiters to awake */
    private int waitersToWake;
    

    /**
     * Constructor
     *
     * @param initialState initial state
     */
    AutoStateTracker(Object initialState) {
        currentState = initialState;
        totalWaiters = 0;
        waitersToWake = 0;
        interruptWait = false;
        wakeWaitersLock = new Object();
    }
    

    /**
     * Returns current state.
     *
     * @return current state
     */
    final Object getCurrentState() {
        synchronized (this) {
            return currentState;
        }
    }
    
    /** 
     * Sets a state. 
     * Assumption: only one thread sets a state, 
     * 'this' lock shouldn't be already taken.
     *
     * @param state state to set as current
     * @param interrupt if true, interrupt all waiters 
     * after setting a state.
     */
    final void setCurrentState(Object state, boolean interrupt) {
        synchronized (this) {
            if (currentState == state) {
                return;
            }

            interruptWait = interrupt;
            currentState = state;
            waitersToWake = totalWaiters;
            notifyAll();
        }
        
        // wait until all waiters have been awaken, so they have 
        // a chance to recheck condition they are waiting for
        synchronized (wakeWaitersLock) {
            while (waitersToWake != 0) {
                try {
                    wakeWaitersLock.wait();
                } catch (InterruptedException e) {
                    // just ignore
                }
            }
        }
    }
    
    /**
     * Interrupts waiters
     */
    final void interruptWait() {
        synchronized (this) {
            interruptWait = true;
            notifyAll();
        }
    }

    /**
     * Checks condition waiters are waiting for:
     * specified state has been reached.
     * Assumption: 'this' lock is already taken.
     *
     * @param state state to check againts
     * @return true, if specified state has been reached
     */
    protected boolean isStateReached(Object state) {
        return (currentState == state);
    }
    

    /**
     * Waits until specified state has been reached.
     *
     * @param state state to wait for
     */
    final void waitFor(Object state) {
        synchronized (this) {
            while (!isStateReached(state) && !interruptWait) {
                try {
                    totalWaiters++;
                    wait();
                    totalWaiters--;
                
                    synchronized (wakeWaitersLock) {
                        waitersToWake--;
                        // if all threads affected by setCurrentState 
                        // notification have waken up, notify setCurrentState
                        // about it
                        if (waitersToWake == 0) {
                            wakeWaitersLock.notify();
                        }
                    }
                } catch (InterruptedException e) {
                    // just ignore
                }
            }
        }
    }
}

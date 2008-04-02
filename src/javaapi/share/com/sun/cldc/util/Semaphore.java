/*
 *   
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
 */

package com.sun.cldc.util;

public class Semaphore {
    private SemaphoreLock lock;

    /**
     * Creates a Semaphore with the given number of permits. 
     */
    public Semaphore(int permits) {
        lock = new SemaphoreLock(permits);
    }

    /**
     * Acquires a permit from this semaphore, blocking until one is
     * available.
     */
    public void acquire() {
        lock.acquire();
    }

    /**
     * Releases a permit, returning it to the semaphore. If any
     * threads are blocking trying to acquire a permit, then one is
     * selected and given the permit that was just released. That
     * thread is re-enabled for thread scheduling purposes.
     */
    public void release() {
        lock.release();
    }
}

/**
 * This class implements the behavior of a Semaphore. Note that the
 * synchronized methods are placed in this class, instead of in the
 * public Semaphore class. This makes sure that an application cannot
 * affect the behavior of a Semaphore by synchronizing on the
 * Semaphore object itself.
 */
class SemaphoreLock {
    private int permits;

    SemaphoreLock(int permits) {
        this.permits = permits;
    }
    synchronized native void acquire();
                 native void release();
}

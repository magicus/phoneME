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

package com.sun.midp.i3test;

import com.sun.cldc.util.Semaphore;

/**
 * A simple cross-isolate synchronization mechanism.
 * Handles the simple case of synchronizing between two
 * isolates, performing "master" and "slave" roles.
 * This is useful for cross-isolate unit testing.
 * This is a utility class, containing only static methods.
 * No instances of this class are ever created.
 * 
 * <p>This class must be initialized once (presumably the by master),
 * by calling init(), before starting the slave isolate.  The master isolate 
 * can call awaitReady() to wait for the slave isolate to become ready.
 * The slave isolate performs whatever initialization and testing it wants and 
 * then calls pause().  This blocks the slave and
 * unblocks the master's call to awaitReady(), giving the master the 
 * opportunity to inspect the slave's state.  The master can then call 
 * signalContinue() to unblock the slave.  Finally, the fini() call will 
 * destroy the shared objects created by init().
 *
 * <p>Example code for master isolate:
 * <pre>
 * void testAnotherIsolate() {
 *     IsolateSynch.init();
 *     Isolate iso = new Isolate(start_class, args);
 *     iso.start();
 *     IsolateSynch.awaitReady();
 *     // assertions can be made safely here about the state of the slave
 *     IsolateSynch.signalContinue();
 *     IsolateSynch.fini();
 * }
 * </pre>
 * <p>Example code for slave isolate:
 * <pre>
 * public static void main(Strings args[]) {
 *     // perform initialization and execute code under test
 *     IsolateSynch.pause();
 *     // perform any cleanup here
 * }
 * </pre>
 */
public class IsolateSynch {

    static Semaphore semReady;
    static Semaphore semContinue;
    
    static native Semaphore getSemReady0();
    static native Semaphore getSemContinue0();

    private static native void init0(Semaphore semReady, Semaphore semCont);
    private static native void fini0();

    private IsolateSynch() {
        // disallow creation of instances
    }

    /**
     * Initialize shared objects.
     * @throws IllegalStateException if already initialized.
     */
    public static void init() {
        semReady = new Semaphore(0);
        semContinue = new Semaphore(0);
        init0(semReady, semContinue);
    }

    /**
     * Blocks until the slave pauses, indicating that it's ready.  Intended to
     * be called by the master isolate.
     */
    public static void awaitReady() {
        getSemReady0().acquire();
    }

    /**
     * Informs the slave that it can continue after having paused.  Intended 
     * to be called by the master isolate.
     */
    public static void signalContinue() {
        getSemContinue0().release();
    }

    /**
     * Blocks the caller and unblocks the master.  Intended to be called by 
     * the slave.
     */
    public static void pause() {
        getSemReady0().release();
        getSemContinue0().acquire();
    }

    /**
     * Cleans up shared objects.  Does nothing if there is
     * nothing to clean up.
     */
    public static void fini() {
        semReady = null;
        semContinue = null;
        fini0();
    }

}

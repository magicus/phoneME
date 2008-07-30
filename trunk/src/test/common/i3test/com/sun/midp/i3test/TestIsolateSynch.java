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

public class TestIsolateSynch extends TestCase {

    boolean isBlocked;

    /**
     * Tests semaphores are null prior to initialization.
     */
    void testNull() {
        // no initialization here!
        Semaphore s1 = IsolateSynch.getSemReady0();
        Semaphore s2 = IsolateSynch.getSemContinue0();
        assertNull(s1);
        assertNull(s2);
    }

    /**
     * Checks semaphore identities after init, null after fini.
     */
    void testInitFini() {
        IsolateSynch.init();

        Semaphore s1 = IsolateSynch.getSemReady0();
        Semaphore s2 = IsolateSynch.getSemContinue0();
        assertSame("semReady", IsolateSynch.semReady, s1);
        assertSame("semContinue", IsolateSynch.semContinue, s2);

        IsolateSynch.fini();

        assertNull(IsolateSynch.semReady);
        assertNull(IsolateSynch.semContinue);
        assertNull(IsolateSynch.getSemReady0());
        assertNull(IsolateSynch.getSemContinue0());
    }

    /**
     * Synchronizes with another thread within this isolate.
     */
    void testSynch() {
        IsolateSynch.init();

        Thread t = new Thread() {
            public void run() {
                isBlocked = true;
                IsolateSynch.pause();
                isBlocked = false;
            }
        };

        isBlocked = false;
        t.start();
        IsolateSynch.awaitReady();
        assertTrue(isBlocked);
        IsolateSynch.signalContinue();
        try {
            t.join();
        } catch (InterruptedException ignore) { }
        assertTrue(!isBlocked);

        IsolateSynch.fini();
    }

    /**
     * Tests what happens if initialization is done twice.
     */
    void testDoubleInit() {
        boolean thrown = false;

        IsolateSynch.init();
        try {
            IsolateSynch.init();
        } catch (IllegalStateException ise) {
            thrown = true;
        }
        assertTrue("IllegalStateException not thrown", thrown);
        IsolateSynch.fini();
    }

    /**
     * Runs all the tests.
     */
    public void runTests() {
        declare("testNull");
        testNull();
        declare("testInitFini");
        testInitFini();
        declare("testSynch");
        testSynch();
        declare("testDoubleInit");
        testDoubleInit();
    }

}

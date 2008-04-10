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

package com.sun.midp.test;

import com.sun.cldc.util.Semaphore;
import com.sun.midp.i3test.*;

public class TestSemaphore extends TestCase {

    Semaphore sema;

    // local methods and classes

    void sleep(int s) {
        try {
            Thread.sleep(s);
        } catch (InterruptedException ignore) { }
    }

    class Blocker extends Thread {
        Semaphore sema;
        boolean isBlocked = false;

        Blocker(Semaphore newSema) {
            sema = newSema;
        }

        public void run() {
            isBlocked = true;
            sema.acquire();
            isBlocked = false;
        }
    }

    // the tests

    void testOne() {
        Semaphore sema = new Semaphore(1);
        sema.acquire();
        assertTrue(true);
    }

    void testTwo() {
        Semaphore sema = new Semaphore(0);
        sema.release();
        sema.acquire();
        assertTrue(true);
    }

    void testBlock() {
        sema = new Semaphore(0);

        Blocker t1 = new Blocker(sema);

        assertTrue(! t1.isBlocked);
        t1.start();
        sleep(100);
        assertTrue(t1.isBlocked);
        sema.release();
        try {
            t1.join();
        } catch (InterruptedException ignore) { }
        assertTrue(! t1.isBlocked);

        sema = null;
    }

    void testManyBlock() {
        sema = new Semaphore(0);
        final int NTHREADS = 10;

        Blocker ta[] = new Blocker[NTHREADS];
        for (int i = 0; i < NTHREADS; i++) {
            ta[i] = new Blocker(sema);
        }

        for (int i = 0; i < NTHREADS; i++) {
            assertTrue("blocked initially", !ta[i].isBlocked);
        }

        for (int i = 0; i < NTHREADS; i++) {
            ta[i].start();
        }
        
        sleep(100);

        for (int i = 0; i < NTHREADS; i++) {
            assertTrue("not blocked after start", ta[i].isBlocked);
        }

        sema.release();
        sema.release();
        sema.release();

        sleep(100);

        int count = 0;
        for (int i = 0; i < NTHREADS; i++) {
            if (ta[i].isBlocked)
                ++count;
        }

        assertEquals("blocked " + count + "instead of 7", 7, count);

        sema.release();
        sema.release();
        sema.release();
        sema.release();
        sema.release();
        sema.release();
        sema.release();

        sleep(100);

        count = 0;
        for (int i = 0; i < NTHREADS; i++) {
            if (ta[i].isBlocked)
                ++count;
        }

        assertEquals("all not unblocked", 0, count);

        for (int i = 0; i < NTHREADS; i++) {
            try {
                ta[i].join();
            } catch (InterruptedException ignore) { }
        }
    }

    public void runTests() {
        declare("testOne");
        testOne();
        declare("testTwo");
        testTwo();
        declare("testBlock");
        testBlock();
        declare("testManyBlock");
        testManyBlock();
    }

}

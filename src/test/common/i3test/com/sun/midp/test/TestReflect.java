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

import com.sun.cldc.isolate.Isolate;
import com.sun.cldc.isolate.IsolateStartupException;
import com.sun.cldchi.test.Reflect;
import com.sun.midp.i3test.*;

public class TestReflect extends TestCase {

    static final String slave = "com.sun.midp.test.ReflectSlave";

    Isolate iso;

    void setUp() {

        // Make sure we can find the slave class.  This isn't strictly
        // necessary, but it serves as a useful cross-check to ensure
        // that the class is visible to both the master and slave
        // isolates.  It also helps if the classname is wrong for
        // some reason: you get a useful error message instead of
        // an obscure isolate startup error.

        try {
            Class.forName(slave);
        } catch (ClassNotFoundException cnfe) {
            throw new RuntimeException(
                "Class.forName() can't find slave class " + slave);
        }

        // Initialize the isolate synchronization mechanism.
        // This must be done before the other isolate is started.
        // This must be called ONLY in the "master" isolate.

        IsolateSynch.init();

        // Create the slave isolate and start it.

        try {
            iso = new Isolate(slave, new String[0]);
            iso.start();
        } catch (IsolateStartupException ise) {
            throw new RuntimeException("can't start isolate");
        }

        // Wait for the slave isolate to become ready.

        IsolateSynch.awaitReady();
    }

    void tearDown() {
        // Tell the other isolate to continue.

        IsolateSynch.signalContinue();

        // Alternatively, kill the other isolate instead of
        // telling it to continue.  Do one or the other, but not both.
        /* iso.exit(0); */

        // Wait for the other isolate to exit.

        iso.waitForExit();

        // Clean up isolate synchronization mechanism.

        IsolateSynch.fini();
    }

    void testOne() {
        setUp();
        int x = Reflect.getStaticIntValue(iso, slave, "integerValue");
        assertEquals(17, ReflectSlave.integerValue);
        assertEquals(42, x);
        tearDown();
    }

    public void runTests() {
        declare("one");
        testOne();
    }
}

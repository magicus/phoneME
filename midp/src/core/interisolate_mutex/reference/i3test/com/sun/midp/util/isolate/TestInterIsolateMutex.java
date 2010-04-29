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

package com.sun.midp.util.isolate;
import com.sun.midp.i3test.*;
import com.sun.midp.security.*;
import com.sun.cldc.isolate.*;

public class TestInterIsolateMutex extends TestCase {
    /**
     * Test failures
     */
    void testFailures() {
        boolean exceptionThrown;

        declare("Obtain InterIsolateMutex instance");
        SecurityToken t = SecurityTokenProvider.getToken();
        InterIsolateMutex m = InterIsolateMutex.getInstance(t, "i3test");
        assertNotNull("Failed to obtain InterIsolateMutex instance", m);

        declare("Unlock not locked mutex");
        exceptionThrown = false;
        try {
            m.unlock();
        } catch (Exception e) {
            exceptionThrown = true;
        }
        assertTrue("Unlocked not locked mutex", exceptionThrown);

        declare("Lock mutex twice");
        exceptionThrown = false;
        try {
            m.lock();
            m.lock();
        } catch (Exception e) {
            exceptionThrown = true;
        } finally {
            m.unlock();
        }
        assertTrue("Locked mutex twice", exceptionThrown);
 
        declare("Unlock the mutex locked by another Isolate");
        exceptionThrown = false;
        Isolate iso = null;
        try {
            // start slave Isolate
            IsolateSynch.init();
            iso = new Isolate("com.sun.midp.util.isolate.InterIsolateMutexTestIsolate", null);
            iso.start();

            // wait till slave Isolate locks the mutex
            IsolateSynch.awaitReady();

            // try to unlock the mutex
            m.unlock();
        } catch (Exception e) {
            exceptionThrown = true;
        } finally {
            // unblock slave Isolate to let it unblock the mutex and exit
            IsolateSynch.signalContinue();
            iso.waitForExit();
            IsolateSynch.fini();
        }
        assertTrue("Unlocked the mutex locked by another Isolate", 
                exceptionThrown);
    }

    /**
     * Run tests
     */
    public void runTests() {
        testFailures();
    }
}

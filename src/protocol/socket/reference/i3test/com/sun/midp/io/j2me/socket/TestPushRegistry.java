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

package com.sun.midp.io.j2me.socket;

import java.io.*;
import javax.microedition.io.*;
import javax.microedition.midlet.MIDlet;
import com.sun.midp.i3test.*;
import com.sun.midp.io.j2me.push.*;

/**
 * Test of SocketConnection function that unblocks a pending read
 * when the connection is closed.
 */
public class TestPushRegistry extends TestCase {
    boolean exceptionThrownAsExpected;
    String negativeFilters[] = { "", "...", "?.?.?", ".1.2.3", "1.2.3.", "*.*.*.*.*" };
    String positiveFilters[] = { "1.2.3.4", "?.?.?.?", "1.1.1.1", "1.1.1.*", "1.1.1.**", "1.**.1.**", "1.1.1.1*", "1.1.1.*1", "1.1.1.*1", "1.1.1.*1", "1.1.1.*?1", "1.1.1.*??", "1.1.1.*1?", "1.1.*.1", "1.1.1*.1", "1.1.*1.1", "1.1.*1.1", "1.1.*1.1", "1.1.*?1.1", "1.1.*??.1", "1.1.*1?.1", "1.1.*.*", "1.1.1*.1*", "1.1.*1.*1", "1.1.*1.*1", "1.1.*1.*1", "1.1.*?1.*1", "1.1.*??.1??", "1.1.*1?.*1?", "*1?.1.1.1", "*1?.1.1.*1?", "1.*.?1.1", "*" };
    String connection = "socket://:1223";

    void testPushFilters() {
        ProtocolPushImpl pushImpl = null;
        int i = 0;

        exceptionThrownAsExpected = true;

        try {
            pushImpl = (ProtocolPushImpl)ProtocolPush.getInstance(connection);
        }
        catch (Exception cce) {
            exceptionThrownAsExpected = false;
        }

        if (exceptionThrownAsExpected) {
            for (i = negativeFilters.length; --i >= 0 && exceptionThrownAsExpected; ) {
                exceptionThrownAsExpected = false;
                try {
                    pushImpl.checkRegistration(connection,
                                               "com.sun.midp.io.j2me.socket.TestPushRegistry.MyTestMIDlet",
                                               negativeFilters[i]);
                    System.err.println("failed with no exception at " + negativeFilters[i]);
                }
                catch (IllegalArgumentException iae) {
                    exceptionThrownAsExpected = true;
                }
                catch (Exception ex) {
                    System.err.println("failed with " + ex + " at " + negativeFilters[i]);
                }
            }
        }

        if (exceptionThrownAsExpected) {
            try {
                for (i = positiveFilters.length; --i >= 0 && exceptionThrownAsExpected; ) {
                    pushImpl.checkRegistration(connection,
                                               "com.sun.midp.io.j2me.socket.TestPushRegistry.MyTestMIDlet",
                                               positiveFilters[i]);
                }
            }
            catch (Exception ex) {
                System.err.println("failed with " + ex + " at " + positiveFilters[i]);
                exceptionThrownAsExpected = false;
            }
        }

        assertTrue("Verify PushRegistry for socket connection", exceptionThrownAsExpected);
    }

    /**
     * Start PushRegistry tests for socket connection
     */
    public void runTests() {
        declare("Test PushRegistry for socket connection");

        testPushFilters();
    }

}

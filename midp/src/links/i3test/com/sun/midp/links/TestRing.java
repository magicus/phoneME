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

package com.sun.midp.links;

import com.sun.cldc.isolate.Isolate;
import com.sun.cldc.isolate.IsolateStartupException;
import com.sun.midp.i3test.TestCase;
import java.io.IOException;

/**
 * Tests sending messages around a ring of three isolates.
 */
public class TestRing extends TestCase {

    void testRing() throws IOException, IsolateStartupException {
        Isolate zero = Isolate.currentIsolate();
        Isolate one = new Isolate("com.sun.midp.links.Echo", null);
        Isolate two = new Isolate("com.sun.midp.links.Echo", null);

        one.start();
        two.start();

        Link link01 = Link.newLink(zero, one);
        Link link12 = Link.newLink(one, two);
        Link link20 = Link.newLink(two, zero);

        LinkPortal.setLinks(one, new Link[] { link01, link12 });
        LinkPortal.setLinks(two, new Link[] { link12, link20 });

        String ss = "Mr. Watson, come here. I want you!";
        link01.send(LinkMessage.newStringMessage(ss));
        LinkMessage lm = link20.receive();

        assertTrue("contains string", lm.containsString());
        String rs = lm.extractString();
        assertNotSame("strings not same", ss, rs);
        assertEquals("strings equal", ss, rs);

        LinkPortal.setLinks(one, null);
        LinkPortal.setLinks(two, null);
        one.exit(0);
        two.exit(0);
        one.waitForExit();
        two.waitForExit();
    }


    /**
     * Runs all tests.
     */
    public void runTests() throws IOException, IsolateStartupException {
        declare("testRing");
        testRing();
    }
}

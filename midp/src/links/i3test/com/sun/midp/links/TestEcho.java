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
 * Tests sending messages to and receiving messages from another isolate.
 * Starts the Echo program in another isolate and tests echoing of string,
 * data, data range, and link messages.
 */
public class TestEcho extends TestCase {

    Isolate us;
    Isolate them;
    Link to;
    Link from;

    void setUp() throws IOException, IsolateStartupException {
        us = Isolate.currentIsolate();
        them = new Isolate("com.sun.midp.links.Echo", null);
        them.start();
        to = Link.newLink(us, them);
        from = Link.newLink(them, us);
        LinkPortal.setLinks(them, new Link[] { to, from });
    }

    void tearDown() {
        LinkPortal.setLinks(them, null);
        them.exit(0);
        them.waitForExit();

        us = null;
        them = null;
        to = null;
        from = null;
    }


    /**
     * Tests echoing of a string message.
     */
    void testString() throws IOException, IsolateStartupException {
        String rs, ss;
        LinkMessage lm;

        ss = "What hath God wrought?";
        to.send(LinkMessage.newStringMessage(ss));
        lm = from.receive();

        assertTrue("contains string", lm.containsString());
        rs = lm.extractString();
        assertNotSame("strings not same", ss, rs);
        assertEquals("strings equal", ss, rs);
    }


    /**
     * Tests echoing of a data message.
     */
    void testData() throws IOException, IsolateStartupException {
        byte[] rdata;
        byte[] sdata;
        LinkMessage lm;

        sdata = new byte[50];
        Utils.fillRandom(sdata);
        to.send(LinkMessage.newDataMessage(sdata));
        lm = from.receive();

        assertTrue("contains data", lm.containsData());
        rdata = lm.extractData();
        assertNotSame("data not same", sdata, rdata);
        assertTrue("data equal", Utils.bytesEqual(sdata, rdata));
    }


    /**
     * Tests echoing of a data subrange message.
     */
    void testDataRange() throws IOException, IsolateStartupException {
        byte[] rdata;
        byte[] sdata;
        LinkMessage lm;

        sdata = new byte[60];
        Utils.fillRandom(sdata);
        to.send(LinkMessage.newDataMessage(sdata, 12, 23));
        lm = from.receive();

        assertTrue("contains data", lm.containsData());
        rdata = lm.extractData();
        assertNotSame("data not same", sdata, rdata);
        assertTrue("data equal", Utils.bytesEqual(sdata, 12, 23, rdata));
    }


    /**
     * Tests echoing of a link message.
     */
    void testLink() throws IOException, IsolateStartupException {
        Link rlink, slink;
        LinkMessage lm;

        slink = Link.newLink(us, us);
        to.send(LinkMessage.newLinkMessage(slink));
        lm = from.receive();

        assertTrue("contains link", lm.containsLink());
        rlink = lm.extractLink();
        assertNotSame("links not same", slink, rlink);
        assertEquals("links equal", slink, rlink);
    }


    /**
     * Runs all tests.
     */
    public void runTests() throws IOException, IsolateStartupException {

        declare("testString");
        setUp();
        testString();
        tearDown();

        declare("testData");
        setUp();
        testData();
        tearDown();

        declare("testDataRange");
        setUp();
        testDataRange();
        tearDown();

        declare("testLink");
        setUp();
        testLink();
        tearDown();

    }
}

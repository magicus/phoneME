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
import java.io.InterruptedIOException;

/**
 * Tests proper link state after an isolate is killed.
 */
public class TestKill extends TestCase {

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
        us = them = null;
        to = from = null;
    }

    void checkClosedLinks() throws IOException {
        boolean thrown = false;

        try {
            to.send(LinkMessage.newStringMessage("Hello, world!"));
        } catch (ClosedLinkException cle) {
            thrown = true;
        }
        assertTrue("send should throw CLE", thrown);

        thrown = false;
        try {
            LinkMessage msg = from.receive();
        } catch (ClosedLinkException cle) {
            thrown = true;
        }
        assertTrue("receive should throw CLE", thrown);
    }


    void testKillReceive() throws IOException {
        Receiver receiver = new Receiver(from);
        assertFalse("receiver blocked", receiver.done);

        them.exit(0);
        them.waitForExit();
        receiver.await();

        assertTrue("receiver got IIOE",
            receiver.exception instanceof InterruptedIOException);
        checkClosedLinks();
    }


    void testKillSend() throws IOException {
        to.send(LinkMessage.newStringMessage("one"));
        Sender sender = new Sender(to, LinkMessage.newStringMessage("two"));
        assertFalse("sender blocked", sender.done);

        them.exit(0);
        them.waitForExit();
        sender.await();

        assertTrue("sender got IIOE",
            sender.exception instanceof InterruptedIOException);
        checkClosedLinks();
    }


    /**
     * Tests have the sender strand a message in a link.
     */
    void testStrandMessage() throws IOException {
        byte[] data = new byte[20];
        to.send(LinkMessage.newDataMessage(data));

        // wait until echo is blocked sending reply back to us
        Utils.sleep(100);

        them.exit(0);
        them.waitForExit();
        checkClosedLinks();
    }


    /**
     * Runs all tests.
     */
    public void runTests() throws IOException, IsolateStartupException {

        declare("testKillReceive");
        setUp();
        testKillReceive();
        tearDown();

        declare("testKillSend");
        setUp();
        testKillSend();
        tearDown();

        declare("testStrandMessage");
        setUp();
        testStrandMessage();
        tearDown();
    }
}

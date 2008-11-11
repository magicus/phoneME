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
 * Tests basic operations on the Link class.
 */
public class TestLink extends TestCase {


    /**
     * Tests creation, isOpen(), and close().
     */
    void testCreate() {
        Isolate i = Isolate.currentIsolate();
        Link link = Link.newLink(i, i);

        assertTrue("link should be open", link.isOpen());

        link.close();
        assertFalse("link should be closed", link.isOpen());
    }


    /**
     * Tests nulls.
     */
    void testNulls() {
        Isolate i = Isolate.currentIsolate();
        boolean thrown;

        thrown = false;
        try {
            Link link = Link.newLink(null, null);
        } catch (NullPointerException npe) {
            thrown = true;
        }
        assertTrue("null,null should throw NPE", thrown);

        thrown = false;
        try {
            Link link = Link.newLink(null, i);
        } catch (NullPointerException npe) {
            thrown = true;
        }
        assertTrue("null,i should throw NPE", thrown);

        thrown = false;
        try {
            Link link = Link.newLink(i, null);
        } catch (NullPointerException npe) {
            thrown = true;
        }
        assertTrue("i,null should throw NPE", thrown);

        thrown = false;
        try {
            Link link = Link.newLink(i, i);
        } catch (NullPointerException npe) {
            thrown = true;
        }
        assertFalse("i,i should not throw NPE", thrown);
    }


    /**
     * Tests for equals() and hashCode().
     */
    void testEquals() {
        Isolate i = Isolate.currentIsolate();
        Link link = Link.newLink(i, i);
        Link link2 = Link.newLink(i, i);
        assertFalse("must not equal null", link.equals(null));
        assertTrue("must equal itself", link.equals(link));
        assertFalse("must not equal other obj", link.equals("foobar"));
        assertFalse("must not equal other link", link.equals(link2));

        link.close();
        assertEquals("hashCode of closed should be zero", 0, link.hashCode());
        link2.close();
        assertTrue("two closed links should be equal", link.equals(link2));
    }


    /**
     * Tests receive() on a closed link.
     */
    void testReceiveClosed() throws IOException {
        Isolate i = Isolate.currentIsolate();
        Link link = Link.newLink(i, i);
        link.close();

        boolean thrown = false;
        try {
            LinkMessage lm = link.receive();
        } catch (ClosedLinkException cle) {
            thrown = true;
        }

        assertTrue("exception should be thrown", thrown);
    }


    /**
     * Tests send() on a closed link.
     */
    void testSendClosed() throws IOException {
        Isolate i = Isolate.currentIsolate();
        Link link = Link.newLink(i, i);
        link.close();

        boolean thrown = false;
        try {
            link.send(LinkMessage.newStringMessage("foo"));
        } catch (ClosedLinkException cle) {
            thrown = true;
        }

        assertTrue("exception should be thrown", thrown);
    }


    /**
     * Tests receive() followed by a send().
     */
    void testReceiveSend() throws IOException {
        Isolate i = Isolate.currentIsolate();
        Link link = Link.newLink(i, i);
        Receiver receiver = new Receiver(link);

        assertFalse("receiver should be blocked", receiver.done);
        assertNull("receiver should have no exceptions", receiver.exception);

        String sendstr = "bar";
        link.send(LinkMessage.newStringMessage(sendstr));

        receiver.await();
        assertTrue("receiver should be done", receiver.done);
        assertNull("receiver should have no exceptions", receiver.exception);
        assertNotNull("receiver should have received a message", receiver.msg);

        String recvstr = receiver.msg.extractString();
        assertTrue("strings shouldn't be identical", sendstr != recvstr);
        assertEquals("strings should be equal", sendstr, recvstr);
    }


    /**
     * Tests send() followed by a receive().
     */
    void testSendReceive() throws IOException {
        String sendstr = "foo";
        Isolate i = Isolate.currentIsolate();
        Link link = Link.newLink(i, i);
        Sender sender = new Sender(link,
            LinkMessage.newStringMessage(sendstr));

        assertFalse("sender should be blocked", sender.done);
        assertNull("sender should have no exceptions", sender.exception);

        LinkMessage lm = link.receive();

        sender.await();
        assertTrue("sender should be done", sender.done);
        assertNull("sender should have no exceptions", sender.exception);

        String recvstr = lm.extractString();
        assertTrue("strings shouldn't be identical", sendstr != recvstr);
        assertEquals("strings should be equal", sendstr, recvstr);
    }


    /**
     * Tests that close() will unblock a thread blocked in receive().
     */
    void testReceiveClose() throws IOException {
        Isolate i = Isolate.currentIsolate();
        Link link = Link.newLink(i, i);
        Receiver receiver = new Receiver(link);

        assertFalse("receiver should be blocked", receiver.done);
        assertNull("receiver should have no exceptions", receiver.exception);

        link.close();
        receiver.await();
        assertTrue("receiver should be done", receiver.done);

        boolean wasIIOE =
            receiver.exception instanceof InterruptedIOException;
        assertTrue("receiver should have gotten InterruptedIOException",
            wasIIOE);
        if (!wasIIOE) {
            System.out.println("### receiver got " + receiver.exception);
        }
    }


    /**
     * Tests that close() will unblock a thread blocked in send().
     */
    void testSendClose() throws IOException {
        Isolate i = Isolate.currentIsolate();
        Link link = Link.newLink(i, i);
        Sender sender = new Sender(link,
            LinkMessage.newStringMessage("foobar"));

        assertFalse("sender should be blocked", sender.done);
        assertNull("sender should have no exceptions", sender.exception);

        link.close();
        sender.await();
        assertTrue("sender should be done", sender.done);

        boolean wasIIOE =
            sender.exception instanceof InterruptedIOException;
        assertTrue("sender should have gotten InterruptedIOException",
            wasIIOE);
        if (!wasIIOE) {
            System.out.println("### sender got " + sender.exception);
        }
    }


    /**
     * Tests that the native rendezvous point structure is getting cleaned up 
     * properly by the finalizer.
     */
    void testCleanup() {
        Isolate i = Isolate.currentIsolate();
        Link link = Link.newLink(i, i);
        int rp = link.hashCode();

        assertEquals("refcount must be 1", 1, Utils.getRefCount(link));

        Utils.forceGC();
        /* ignored */ Utils.getFreedRendezvousPoints();
        link = null;
        Utils.forceGC();
        int[] freed = Utils.getFreedRendezvousPoints();
        assertEquals("length must be one", 1, freed.length);
        if (freed.length == 1) {
            assertEquals("freed rp must match", rp, freed[0]);
        }
    }


    /**
     * Tests link creation with isolates in different states.
     */
    void testIsolateStates() throws IOException, IsolateStartupException {
        boolean thrown;
        Isolate us = Isolate.currentIsolate();
        Isolate them = new Isolate("com.sun.midp.links.Empty", null);

        thrown = false;
        try {
            Link link = Link.newLink(us, them);
        } catch (IllegalStateException ise) {
            thrown = true;
        }
        assertTrue("started,new should throw ISE", thrown);

        thrown = false;
        try {
            Link link = Link.newLink(them, us);
        } catch (IllegalStateException ise) {
            thrown = true;
        }
        assertTrue("new,started should throw ISE", thrown);

        them.start();

        thrown = false;
        try {
            Link link = Link.newLink(us, them);
        } catch (IllegalStateException ise) {
            thrown = true;
        }
        assertFalse("started,started should not throw ISE", thrown);

        thrown = false;
        try {
            Link link = Link.newLink(them, us);
        } catch (IllegalStateException ise) {
            thrown = true;
        }
        assertFalse("started,started should not throw ISE", thrown);

        them.exit(0);
        them.waitForExit();

        thrown = false;
        try {
            Link link = Link.newLink(us, them);
        } catch (IllegalStateException ise) {
            thrown = true;
        }
        assertTrue("started,terminated should throw ISE", thrown);

        thrown = false;
        try {
            Link link = Link.newLink(them, us);
        } catch (IllegalStateException ise) {
            thrown = true;
        }
        assertTrue("terminated,started should throw ISE", thrown);
    }


    /**
     * Tests receiver and sender isolate access for a link.
     */
    void testAccess() throws IOException, IsolateStartupException {
        Isolate i1 = Isolate.currentIsolate();
        Isolate i2 = new Isolate("com.sun.midp.links.Empty", null);
        i2.start();
        Link link1 = Link.newLink(i1, i2);
        Link link2 = Link.newLink(i2, i1);
        boolean thrown;

        thrown = false;
        try {
            LinkMessage lm = link1.receive();
        } catch (IllegalArgumentException iae) {
            thrown = true;
        }
        assertTrue("receive should catch IAE", thrown);

        thrown = false;
        try {
            link2.send(LinkMessage.newStringMessage("hello"));
        } catch (IllegalArgumentException iae) {
            thrown = true;
        }
        assertTrue("send should catch IAE", thrown);

        i2.exit(0);
    }


    /**
     * Runs all tests.
     */
    public void runTests() throws IOException, IsolateStartupException {
        declare("testCreate");
        testCreate();

        declare("testNulls");
        testNulls();

        declare("testEquals");
        testEquals();

        declare("testReceiveClosed");
        testReceiveClosed();

        declare("testSendClosed");
        testSendClosed();

        declare("testReceiveSend");
        testReceiveSend();

        declare("testSendReceive");
        testSendReceive();

        declare("testReceiveClose");
        testReceiveClose();

        declare("testSendClose");
        testSendClose();

        declare("testCleanup");
        testCleanup();

        declare("testIsolateStates");
        testIsolateStates();

        declare("testAccess");
        testAccess();
    }
}

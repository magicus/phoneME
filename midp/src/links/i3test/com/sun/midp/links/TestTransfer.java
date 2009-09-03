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
import com.sun.midp.i3test.TestCase;
import java.io.IOException;
import java.io.InterruptedIOException;

/**
 * Tests simple exchange of different types of messages (data, data subrange,
 * and String) within the same isolate. Covers simple cases only, with 
 * one receiver thread and one sender thread. Covers receive-then-send as well 
 * as send-then-receive, for each kind of message.
 */
public class TestTransfer extends TestCase {


    /**
     * Tests receive-then-send of a String.
     */
    void testReceiveString() throws IOException {
        Isolate i = Isolate.currentIsolate();
        Link link = Link.newLink(i, i);
        Receiver receiver = new Receiver(link);
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
     * Tests send-then-receive of a String.
     */
    void testSendString() throws IOException {
        Isolate i = Isolate.currentIsolate();
        Link link = Link.newLink(i, i);
        String sendstr = "foo";
        Sender sender = new Sender(link,
            LinkMessage.newStringMessage(sendstr));

        LinkMessage lm = link.receive();
        sender.await();

        assertTrue("sender should be done", sender.done);
        assertNull("sender should have no exceptions", sender.exception);

        String recvstr = lm.extractString();
        assertTrue("strings shouldn't be identical", sendstr != recvstr);
        assertEquals("strings should be equal", sendstr, recvstr);
    }


    /**
     * Tests receive-then-send of a byte array.
     */
    void testReceiveData() throws IOException {
        Isolate i = Isolate.currentIsolate();
        Link link = Link.newLink(i, i);
        Receiver receiver = new Receiver(link);
        byte[] sendarr = new byte[100];
        Utils.fillRandom(sendarr);

        link.send(LinkMessage.newDataMessage(sendarr));
        receiver.await();

        assertTrue("receiver should be done", receiver.done);
        assertNull("receiver should have no exceptions", receiver.exception);
        assertNotNull("receiver should have received a message", receiver.msg);

        byte[] recvarr = receiver.msg.extractData();
        assertTrue("arrays shouldn't be identical", sendarr != recvarr);
        assertTrue("arrays should be equal",
            Utils.bytesEqual(sendarr, recvarr));
    }


    /**
     * Tests send-then-receive of a byte array.
     */
    void testSendData() throws IOException {
        Isolate i = Isolate.currentIsolate();
        Link link = Link.newLink(i, i);
        byte[] sendarr = new byte[100];
        Utils.fillRandom(sendarr);
        Sender sender = new Sender(link,
            LinkMessage.newDataMessage(sendarr));

        LinkMessage lm = link.receive();
        sender.await();

        assertTrue("sender should be done", sender.done);
        assertNull("sender should have no exceptions", sender.exception);

        byte[] recvarr = lm.extractData();
        assertTrue("arrays shouldn't be identical", sendarr != recvarr);
        assertTrue("arrays should be equal",
            Utils.bytesEqual(sendarr, recvarr));
    }


    /**
     * Tests receive-then-send of a byte array subrange.
     */
    void testReceiveDataRange() throws IOException {
        Isolate i = Isolate.currentIsolate();
        Link link = Link.newLink(i, i);
        Receiver receiver = new Receiver(link);
        byte[] sendarr = new byte[100];
        Utils.fillRandom(sendarr);

        link.send(LinkMessage.newDataMessage(sendarr, 17, 32));
        receiver.await();

        assertTrue("receiver should be done", receiver.done);
        assertNull("receiver should have no exceptions", receiver.exception);
        assertNotNull("receiver should have received a message", receiver.msg);

        byte[] recvarr = receiver.msg.extractData();
        assertTrue("arrays shouldn't be identical", sendarr != recvarr);
        assertTrue("arrays should be equal",
            Utils.bytesEqual(sendarr, 17, 32, recvarr));
    }


    /**
     * Tests send-then-receive of a byte array subrange.
     */
    void testSendDataRange() throws IOException {
        Isolate i = Isolate.currentIsolate();
        Link link = Link.newLink(i, i);
        byte[] sendarr = new byte[100];
        Utils.fillRandom(sendarr);
        Sender sender = new Sender(link,
            LinkMessage.newDataMessage(sendarr, 47, 22));

        LinkMessage lm = link.receive();
        sender.await();

        assertTrue("sender should be done", sender.done);
        assertNull("sender should have no exceptions", sender.exception);

        byte[] recvarr = lm.extractData();
        assertTrue("arrays shouldn't be identical", sendarr != recvarr);
        assertTrue("arrays should be equal",
            Utils.bytesEqual(sendarr, 47, 22, recvarr));
    }


    /**
     * Runs all tests.
     */
    public void runTests() throws IOException {
        declare("testReceiveString");
        testReceiveString();

        declare("testSendString");
        testSendString();

        declare("testReceiveData");
        testReceiveData();

        declare("testSendData");
        testSendData();

        declare("testReceiveDataRange");
        testReceiveDataRange();

        declare("testSendDataRange");
        testSendDataRange();
    }

}

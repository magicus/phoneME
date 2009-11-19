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


/**
 * Tests simple exchange of a message containing a Link and also tests for 
 * proper cleanup. Covers simple cases only, with one receiver thread and one 
 * sender thread. Covers receive-then-send as well as send-then-receive.
 */
public class TestLinkTransfer extends TestCase {


    /**
     * Tests the transfer of a message containing a link, where the receiver 
     * thread blocks first.
     */
    void testReceiveLink() throws IOException {
        Isolate i = Isolate.currentIsolate();
        Link link = Link.newLink(i, i);
        Link sentlink = Link.newLink(i, i);
        Receiver receiver = new Receiver(link);

        link.send(LinkMessage.newLinkMessage(sentlink));
        receiver.await();

        assertTrue("receiver should be done", receiver.done);
        assertNull("receiver should have no exceptions", receiver.exception);
        assertNotNull("receiver should have received a message", receiver.msg);
        assertTrue("received message should contain link",
            receiver.msg.containsLink());

        Link recvlink = receiver.msg.extractLink();
        assertTrue("links must not be identical", recvlink != sentlink);
        assertTrue("links must be equal", sentlink.equals(recvlink));
        assertEquals("refcount must be 2", 2, Utils.getRefCount(sentlink));

        /* clean out the free list */

        Utils.forceGC();
        Utils.getFreedRendezvousPoints();

        /* free one reference */

        int[] freed;
        int rp;

        receiver.msg = null;
        recvlink = null;
        Utils.forceGC();
        freed = Utils.getFreedRendezvousPoints();
        assertEquals("length must be zero", 0, freed.length);
        assertEquals("refcount must be 1", 1, Utils.getRefCount(sentlink));

        /* free the second reference */

        rp = sentlink.hashCode();
        sentlink = null;
        Utils.forceGC();
        freed = Utils.getFreedRendezvousPoints();
        assertEquals("length must be one", 1, freed.length);
        if (freed.length == 1) {
            assertEquals("freed rp must match", rp, freed[0]);
        }
    }


    /**
     * Tests the transfer of a message containing a link, where the sender
     * thread blocks first.
     */
    void testSendLink() throws IOException {
        Isolate i = Isolate.currentIsolate();
        Link link = Link.newLink(i, i);
        Link sentlink = Link.newLink(i, i);
        Sender sender = new Sender(link,
            LinkMessage.newLinkMessage(sentlink));

        LinkMessage msg = link.receive();
        sender.await();

        assertTrue("sender should be done", sender.done);
        assertNull("sender should have no exceptions", sender.exception);
        assertTrue("received message should contain link",
            msg.containsLink());

        Link recvlink = msg.extractLink();
        assertTrue("links must not be identical", recvlink != sentlink);
        assertTrue("links must be equal", sentlink.equals(recvlink));
        assertEquals("refcount must be 2", 2, Utils.getRefCount(sentlink));

        /* clean out the free list */

        Utils.forceGC();
        Utils.getFreedRendezvousPoints();

        /* free one reference */

        int[] freed;
        int rp;

        sender.msg = null;
        sentlink = null;
        Utils.forceGC();
        freed = Utils.getFreedRendezvousPoints();
        assertEquals("length must be zero", 0, freed.length);
        assertEquals("refcount must be 1", 1, Utils.getRefCount(recvlink));

        /* free the second reference */

        rp = recvlink.hashCode();
        recvlink = null;
        msg = null;
        Utils.forceGC();
        freed = Utils.getFreedRendezvousPoints();
        assertEquals("length must be one", 1, freed.length);
        if (freed.length == 1) {
            assertEquals("freed rp must match", rp, freed[0]);
        }
    }


    /**
     * Runs all tests.
     */
    public void runTests() throws IOException {
        declare("testReceiveLink");
        testReceiveLink();

        declare("testSendLink");
        testSendLink();
    }

}

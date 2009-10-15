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


/**
 * Tests for the LinkPortal class.
 */
public class TestLinkPortal extends TestCase {

    class Getter extends Thread {
        boolean done = false;
        Object result = null;

        Getter() {
            this.start();
            Utils.sleep(50);
        }

        Object await() {
            try {
                synchronized (this) {
                    while (!done) {
                        wait();
                    }
                }
            } catch (InterruptedException ignore) { }

            return result;
        }

        public void run() {
            try {
                result = LinkPortal.getLinks();
            } catch (Throwable t) {
                result = t;
            }

            synchronized (this) {
                done = true;
                notifyAll();
            }
        }
    }

    /**
     * Tests passing null for arguments.
     */
    void testNulls() {
        boolean thrown;
        Isolate is = Isolate.currentIsolate();
        Link[] la = new Link[3];
        la[0] = Link.newLink(is, is);
        la[1] = Link.newLink(is, is);
        la[2] = Link.newLink(is, is);

        thrown = false;
        try {
            LinkPortal.setLinks(null, la);
        } catch (NullPointerException npe) {
            thrown = true;
        }
        assertTrue("null isolate should throw NPE", thrown);

        thrown = false;
        try {
            LinkPortal.setLinks(is, null);
        } catch (NullPointerException npe) {
            thrown = true;
        }
        assertFalse("null link array shouldn't throw NPE", thrown);

        la[1] = null;
        thrown = false;
        try {
            LinkPortal.setLinks(is, la);
        } catch (NullPointerException npe) {
            thrown = true;
        }
        assertTrue("link array with nulls should throw NPE", thrown);
    }


    /**
     * Tests zero-length setting and getting operations.
     */
    void testZero() {
        Link[] la;
        Isolate is = Isolate.currentIsolate();

        LinkPortal.setLinks(is, new Link[0]);
        la = LinkPortal.getLinks();
        assertNotNull("shouldn't be null", la);
        assertEquals("length zero", 0, la.length);
    }


    /**
     * Tests setting a closed link.
     */
    void testClosed() {
        boolean thrown;
        Link[] la;
        Isolate is = Isolate.currentIsolate();

        la = new Link[3];
        la[0] = Link.newLink(is, is);
        la[1] = Link.newLink(is, is);
        la[2] = Link.newLink(is, is);

        la[1].close();
        thrown = false;
        try {
            LinkPortal.setLinks(is, la);
        } catch (IllegalArgumentException iae) {
            thrown = true;
        }
        assertTrue("closed link should throw IAE", thrown);
    }


    /**
     * Tests blocking getLinks(), followed by setLinks().
     */
    void testBlockedGet() {
        Isolate is = Isolate.currentIsolate();

        LinkPortal.setLinks(is, new Link[] { Link.newLink(is, is) });
        LinkPortal.setLinks(is, null);

        Getter get1 = new Getter();
        assertFalse("get1 should be blocked", get1.done);

        LinkPortal.setLinks(is, new Link[] { Link.newLink(is, is) });
        Object result = get1.await();

        assertTrue("get1 should return Link[]", result instanceof Link[]);
        Link[] gotten = (Link[])result;
        assertEquals("length 1", 1, gotten.length);

        Getter get2 = new Getter();
        assertFalse("get2 should be blocked", get2.done);

        LinkPortal.setLinks(is, new Link[0]);
        result = get2.await();
        assertTrue("get2 should return Link[]", result instanceof Link[]);
        gotten = (Link[])result;
        assertEquals("length 0", 0, gotten.length);
    }


    /**
     * Tests setting and getting of actual data.
     */
    void testActual() {
        Link[] la1;
        Link[] la2;
        Isolate is = Isolate.currentIsolate();

        la1 = new Link[3];
        la1[0] = Link.newLink(is, is);
        la1[1] = Link.newLink(is, is);
        la1[2] = Link.newLink(is, is);

        LinkPortal.setLinks(is, la1);
        la2 = LinkPortal.getLinks();

        assertEquals("lengths should be equal", la1.length, la2.length);
        for (int i = 0; i < la1.length; i++) {
            assertNotSame("link not same", la1[i], la2[i]);
            assertTrue("links equal", la1[i].equals(la2[i]));
            assertEquals("refcount 2", 2, Utils.getRefCount(la2[i]));
        }

        for (int i = 0; i < la1.length; i++) {
            Utils.forceGC();
            Utils.getFreedRendezvousPoints();

            int hash = la1[i].hashCode();
            la1[i] = la2[i] = null;
            Utils.forceGC();
            int[] ia = Utils.getFreedRendezvousPoints();
            assertEquals("one freed", 1, ia.length);
            assertEquals("freed one matches", hash, ia[0]);
        }
    }


    /**
     * Tests replacement of a link array and proper cleanup.
     */
    void testReplace() {
        Isolate is = Isolate.currentIsolate();
        Link[] la = new Link[3];

        la[0] = Link.newLink(is, is);
        la[1] = Link.newLink(is, is);
        la[2] = Link.newLink(is, is);

        LinkPortal.setLinks(is, la);

        assertEquals("refcount 2", 2, Utils.getRefCount(la[0]));
        assertEquals("refcount 2", 2, Utils.getRefCount(la[1]));
        assertEquals("refcount 2", 2, Utils.getRefCount(la[2]));

        LinkPortal.setLinks(is, new Link[0]);

        assertEquals("refcount 1", 1, Utils.getRefCount(la[0]));
        assertEquals("refcount 1", 1, Utils.getRefCount(la[1]));
        assertEquals("refcount 1", 1, Utils.getRefCount(la[2]));

        for (int i = 0; i < la.length; i++) {
            Utils.forceGC();
            Utils.getFreedRendezvousPoints();

            int hash = la[i].hashCode();
            la[i] = null;
            Utils.forceGC();
            int[] ia = Utils.getFreedRendezvousPoints();
            assertEquals("one freed", 1, ia.length);
            assertEquals("freed one matches", hash, ia[0]);
        }

        // clean up
        LinkPortal.setLinks(is, null);
    }


    /**
     * Tests whether setLinks does proper checking on the isolate's state.
     */
    void testIsolateState() throws IsolateStartupException {
        Isolate us = Isolate.currentIsolate();
        Isolate them = new Isolate("com.sun.midp.links.Empty", null);
        Link[] la = new Link[1];
        boolean thrown;

        la[0] = Link.newLink(us, us);

        thrown = false;
        try {
            LinkPortal.setLinks(them, la);
        } catch (IllegalStateException ise) {
            thrown = true;
        }
        assertTrue("not started: setLinks should throw ISE", thrown);

        them.start();
        thrown = false;
        try {
            LinkPortal.setLinks(them, la);
        } catch (IllegalStateException ise) {
            thrown = true;
        } finally {
            LinkPortal.setLinks(them, null);  // clean up
        }            
        assertFalse("started: setLinks shouldn't throw ISE", thrown);

        them.exit(0);
        them.waitForExit();
        thrown = false;
        try {
            LinkPortal.setLinks(them, la);
        } catch (IllegalStateException ise) {
            thrown = true;
        }
        assertTrue("exited: setLinks should throw ISE", thrown);
    }


    /**
     * Runs all tests.
     */
    public void runTests() throws IsolateStartupException {
        declare("testNulls");
        testNulls();
        declare("testZero");
        testZero();
        declare("testClosed");
        testClosed();
        declare("testBlockedGet");
        testBlockedGet();
        declare("testActual");
        testActual();
        declare("testReplace");
        testReplace();
        declare("testIsolateState");
        testIsolateState();
    }

}

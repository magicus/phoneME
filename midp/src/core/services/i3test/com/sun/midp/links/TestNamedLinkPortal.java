/*
 *
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

import com.sun.midp.services.SystemServiceLinkPortal;
import java.util.Hashtable;
import java.util.Enumeration;
import java.io.IOException;
import java.io.InterruptedIOException;

import com.sun.cldc.isolate.Isolate;
import com.sun.cldc.isolate.IsolateStartupException;
import com.sun.midp.i3test.TestCase;


/**
 * Tests for the LinkPortal class.
 */
public class TestNamedLinkPortal extends TestCase {

    class LinksReceiver extends Thread {
        Link receiveLink = null;
        boolean done = false;
        Object result = null;

        LinksReceiver(Link receiveLink) {
            this.receiveLink = receiveLink;
            this.start();
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
                SystemServiceLinkPortal.receiveLinks(receiveLink);
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
     * Tests passing illegal arguments.
     */
    void testIllegals1() throws InterruptedIOException, IOException {
        boolean thrown;

        Isolate is = Isolate.currentIsolate();
        Link sendLink = Link.newLink(is, is);
        Link[] links = new Link[3];
        links[0] = Link.newLink(is, is);
        links[1] = Link.newLink(is, is);
        links[2] = Link.newLink(is, is);

        thrown = false;
        try {
            SystemServiceLinkPortal.putLink(null, links[0]);
        } catch (IllegalArgumentException iae) {
            thrown = true;
        }
        assertTrue("putting link with null name should throw IAE", thrown);

        thrown = false;
        try {
            SystemServiceLinkPortal.putLink("link1", null);
        } catch (IllegalArgumentException iae) {
            thrown = true;
        }
        assertTrue("putting null link should throw IAE", thrown);

        thrown = false;
        try {
            SystemServiceLinkPortal.sendLinks(null);
        } catch (IllegalArgumentException iae) {
            thrown = true;
        }
        assertTrue("null send link should throw IAE", thrown);

        SystemServiceLinkPortal.putLink("link0", links[0]);
        SystemServiceLinkPortal.putLink("link1", links[1]);
        SystemServiceLinkPortal.putLink("link2", links[2]);

        thrown = false;
        try {
            Link l = SystemServiceLinkPortal.getLink("link1");
        } catch (IllegalStateException ise) {
            thrown = true;
        }
        assertTrue("getting link before links have been received should" + 
                " throw ISE", thrown);

        thrown = false;
        links[1].close();
        try {
            SystemServiceLinkPortal.sendLinks(sendLink);
        } catch (IllegalStateException ise) {
            thrown = true;
        }
        assertTrue("trying to send closed link should throw ISE", thrown);

    }

    /**
     * Tests setting and getting of actual data.
     */
    void testActual() throws InterruptedIOException, IOException {
        Isolate is = Isolate.currentIsolate();

        Link sendLink = Link.newLink(is, is); 
        Link[] links = new Link[3];
        links[0] = Link.newLink(is, is);
        links[1] = Link.newLink(is, is);
        links[2] = Link.newLink(is, is);

        LinksReceiver lr = new LinksReceiver(sendLink);
        assertFalse("LinksReceiver should be blocked", lr.done);

        SystemServiceLinkPortal.putLink("link0", links[0]);
        SystemServiceLinkPortal.putLink("link1", links[1]);
        SystemServiceLinkPortal.putLink("link2", links[2]);     
        SystemServiceLinkPortal.sendLinks(sendLink);

        Object result = lr.await();
        assertTrue("links received without error", result == null);

        for (int i = 0; i < 3; i++) {
            Link l = SystemServiceLinkPortal.getLink("link" + i);

            assertNotSame("link not same", l, links[i]);
            assertTrue("links equal", l.equals(links[i]));
        }
    }
    
    /**
     * Tests passing illegal arguments.
     */
    void testIllegals2() throws InterruptedIOException, IOException {
        boolean thrown;
        Isolate is = Isolate.currentIsolate();
        Link sendLink = Link.newLink(is, is); 
        Link l = Link.newLink(is, is);

        thrown = false;
        try {
            SystemServiceLinkPortal.putLink("link", l);
        } catch (IllegalStateException iae) {
            thrown = true;
        }
        assertTrue("trying to put link after links have been sent should" + 
                " throw ISE", thrown);

        thrown = false;
        try {
            SystemServiceLinkPortal.sendLinks(sendLink);
        } catch (IllegalStateException iae) {
            thrown = true;
        }
        assertTrue("trying to send links again should throw ISE", thrown);

        LinksReceiver lr = new LinksReceiver(sendLink);
        Object result = lr.await();
        assertTrue("trying to receive links again should produce error", 
                result != null);
    }

    /**
     * Runs all tests.
     */
    public void runTests() throws InterruptedIOException, IOException {
        declare("testIllegals1");
        testIllegals1();

        declare("testActual");
        testActual();

        declare("testIllegals2");
        testIllegals2();
    }
}

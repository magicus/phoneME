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
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Random;


/**
 * Tests multiple receiving or sending threads blocked on the same link.
 */
public class TestMultiple extends TestCase {

    /**
     * The number of senders or receivers.
     */
    public static final int NUM_THREADS = 10;

    /**
     * Amount of time to wait for threads to complete after sending or 
     * receiving a message.
     */
    public static final long TIMEOUT = 100L;

    Random rand = new Random();
    Object lock = new Object();

    Hashtable doneset;    // Set<Thread>
    Hashtable messages;   // Map<Thread, LinkMessage>
    Hashtable throwables; // Map<Thread, Throwable>

    /**
     * Common code for completion callbacks for LoggingReceiver and 
     * LoggingSender. Adds the thread to the done set; if non-null, adds msg 
     * to the map of received messages; if non-null, adds any throwables 
     * caught to the map of caught throwables.
     */
    public void completed(Thread thr, LinkMessage msg, Throwable exc) {
        synchronized (lock) {
            assertFalse("shouldn't be done already",
                doneset.containsKey(thr));
            doneset.put(thr, thr);

            if (msg != null) {
                messages.put(thr, msg);
            }

            if (exc != null) {
                throwables.put(thr, exc);
            }
        }
    }


    /** 
     * Adds a completion callback to a Receiver thread.
     */
    class LoggingReceiver extends Receiver {
        public LoggingReceiver(Link newlink) {
            super(newlink);
        }

        public void completed(LinkMessage msg, Throwable exc) {
            TestMultiple.this.completed(this, msg, exc);
        }
    }


    /** 
     * Adds a completion callback to a Sender thread.
     */
    class LoggingSender extends Sender {
        public LoggingSender(Link newlink, LinkMessage lm) {
            super(newlink, lm);
        }

        public void completed(LinkMessage msg, Throwable exc) {
            TestMultiple.this.completed(this, msg, exc);
        }
    }


    /**
     * Tests multiple receivers blocked on the same link.  Creates NUM_THREADS
     * threads to receive messages. Then, sends messages one by one, and
     * ensures that each time exactly one thread is unblocked and has received
     * the message just sent. Finally, after sending enough messages, ensures
     * that all threads have been accounted for.
     */
    void testReceivers() throws IOException {
        Isolate is = Isolate.currentIsolate();
        Link link = Link.newLink(is, is);
        Receiver ra[] = new LoggingReceiver[NUM_THREADS];

        doneset = new Hashtable(NUM_THREADS);
        messages = new Hashtable(NUM_THREADS);
        throwables = new Hashtable(NUM_THREADS);

        for (int i = 0; i < NUM_THREADS; i++) {
            ra[i] = new LoggingReceiver(link);
        }
        Utils.sleep(2*TIMEOUT);

        for (int i = 0; i < NUM_THREADS; i++) {
            String s = Integer.toString(rand.nextInt());
            
            messages.clear();
            throwables.clear();
            link.send(LinkMessage.newStringMessage(s));
            Utils.sleep(TIMEOUT);

            assertEquals("iteration " + i, i + 1, doneset.size());

            assertEquals("one message", 1, messages.size());
            for (Enumeration e = messages.elements() ;
                    e.hasMoreElements() ; ) {
                LinkMessage msg = (LinkMessage)e.nextElement();
                assertTrue("message should contain a string",
                    msg.containsString());
                String rs = msg.extractString();
                assertEquals("strings should be equal", s, rs);
            }

            assertEquals("zero throwables", 0, throwables.size());
            for (Enumeration e = throwables.elements() ;
                    e.hasMoreElements() ; ) {
                System.out.println("### " + e.nextElement());
            }
        }

        for (int i = 0; i < NUM_THREADS; i++) {
            assertTrue("should be done", doneset.containsKey(ra[i]));
            doneset.remove(ra[i]);
        }
    }


    /**
     * Tests multiple senders blocked on the same link.  Creates NUM_THREADS
     * threads to send messages. Then, receives messages one by one, and
     * ensures that each time exactly one thread is unblocked and had sent the
     * message just received. Finally, after receiving enough messages,
     * ensures that all threads have been accounted for.
     */
    void testSenders() throws IOException {
        Isolate is = Isolate.currentIsolate();
        Link link = Link.newLink(is, is);
        LoggingSender sa[] = new LoggingSender[NUM_THREADS];

        for (int i = 0; i < NUM_THREADS; i++) {
            String s = Integer.toString(rand.nextInt());
            sa[i] = new LoggingSender(link, LinkMessage.newStringMessage(s));
        }
        Utils.sleep(2*TIMEOUT);

        doneset = new Hashtable(NUM_THREADS);
        throwables = new Hashtable(NUM_THREADS);

        for (int i = 0; i < NUM_THREADS; i++) {
            LinkMessage rm;
            String rs;

            messages.clear();
            throwables.clear();

            rm = link.receive();
            Utils.sleep(TIMEOUT);

            assertEquals("iteration " + i, i + 1, doneset.size());

            assertTrue("message should contain a string",
                rm.containsString());
            rs = rm.extractString();

            assertEquals("one message", 1, messages.size());
            for (Enumeration e = messages.elements() ;
                    e.hasMoreElements() ; ) {
                LinkMessage msg = (LinkMessage)e.nextElement();
                String ss = msg.extractString();
                assertEquals("strings should be equal", ss, rs);
            }

            assertEquals("zero throwables", 0, throwables.size());
            for (Enumeration e = throwables.elements() ;
                    e.hasMoreElements() ; ) {
                System.out.println("### " + e.nextElement());
            }
        }

        for (int i = 0; i < NUM_THREADS; i++) {
            assertTrue("should be done", doneset.containsKey(sa[i]));
            doneset.remove(sa[i]);
        }
    }


    /**
     * Runs all tests.
     */
    public void runTests() throws IOException {
        declare("testReceivers");
        testReceivers();

        declare("testSenders");
        testSenders();
    }
}

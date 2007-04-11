/*
 *
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.j2me.content;

import com.sun.midp.i3test.TestCase;

import java.util.Random;

import com.sun.j2me.content.InvocationStore;
import com.sun.j2me.content.InvocationImpl;

import javax.microedition.content.Invocation;


/**
 * A test thread to pound on the InvocationStore.
 */
class InvocStoreStress implements Runnable {
    /** The thread that is running this Stress case. */
    Thread thread;
    /** The application ID consumed by this thread. */
    int appID;
    /** The classname consumed by this thread. */
    String classname;
    /** The number of applicationIDs to target. */
    int nappIDs;
    /** The number of content handlers per applicationID. */
    int nclassnames;
    /** Flag to stop this thread. */
    boolean stopping;
    /** The maximum number of iterations. */
    int maxInvokes = 5;
    /** Number of invocations queued by this test. */
    int numInvokes;
    /** Number of responses to invocations. */
    int numResponses;
    /** The TestCase to handle the assertions. */
    TestCase testcase;
    /** The random number generator. */
    Random random;
    /** Counters for received responses. */
    int[] scorecard = new int[maxInvokes];
    /** Number of terminations pending to be received; zero = done. */
    int numTerminations;
    /** The application to stress. */
    AppProxy appl;

    /**
     * Construct a new test thread with parameters.
     * @param appNdx index of this application's id
     * @param handlerNdx index of this application's classname
     * @param nappids number of applications ids to generate
     * @param nclassnames number of classnames to generate
     * @param testcase to use for asserts
     */
    InvocStoreStress(int appNdx, int handlerNdx, int nappids, int nclassnames,
             TestCase testcase, AppProxy appl ) {
    this.appID = getAppID(appNdx);
    this.classname = getClassname(handlerNdx);
        this.nappIDs = nappids;
        this.nclassnames = nclassnames;
        this.testcase = testcase;
    this.appl = appl;
        random = new Random(47);  // To get a consistent start for tests
    }

    /**
     * Stop this thread and cleanup.
     */
    void stop() {
    stopping = true;
    }

    /**
     * Run the test.
     */
    public void run() {
    try {
        if (thread == null) {
        thread = Thread.currentThread();
        numTerminations = nappIDs * nclassnames;
        // Start a thread for the responder side
        new Thread(this).start();
        doInvoker();
        } else {
        doResponder();
        }
    } catch (Throwable t) {
        testcase.assertNull("Unexpected exception", t);
        t.printStackTrace();
    }
    }

    /**
     * Generate new requests as needed and tally the response.
     * The final invoke is the termination request/response.
     */
    void doInvoker() {
        InvocationImpl request = new InvocationImpl();
    while (numTerminations > 0) {
            int rand = nextRandom(10);
            if (numInvokes < maxInvokes &&
                (numResponses == numInvokes || (rand < 5))) {
                // produce a new INIT request for a random app, class
                int targetid = nextRandom(nappIDs);
                int targetcn = nextRandom(nclassnames);
                request.suiteId = getAppID(targetid);
                request.classname = getClassname(targetcn);
                request.invokingSuiteId = appID;
                request.invokingClassname = classname;
                request.responseRequired = true;
        request.ID = Integer.toString(numInvokes);
                request.status = Invocation.INIT;
                int wait = nextRandom(500);
                sleep(wait);
                InvocationStore.put(request);
        ++numInvokes;
                println("invoke: +" + wait + " ", request);
            }

        // Consume a response; block until some
        InvocationImpl response = new InvocationImpl();
            response = InvocationStore.getResponse(response,
                           appID, classname, true);
            if (response != null) {
                if (response.status == Invocation.OK) {
            testcase.assertEquals(appID +
                      " verify target appID",
                      appID, response.suiteId);
            testcase.assertEquals(appID +
                      " verify target classname",
                      classname, response.classname);
            println("response", response);

            if ("terminate".equals(response.action)) {
            numTerminations--;
            } else {
            // Keep track of responses
            scorecard[Integer.parseInt(response.ID)] += 1;
            ++numResponses;

            /*
             * If just finished receiving the max responses;
             * send the terminations
             */
            if (numResponses == maxInvokes) {
                sendAll(Invocation.INIT, "terminate");
            }
            }
        } else {
            testcase.assertNull(appID + " illegal response:",
                    response);
                }
        }
    }

        sleep(2000L);
        do {
            request = InvocationStore.getResponse(new InvocationImpl(),
                          appID, classname, false);
        } while (request != null);

    // Terminate responder thread
    stop();

        // Verify that every invocation received a response.
        testcase.assertEquals(appID + " verify each invoke got a response",
                  numInvokes, numResponses);
        for (int i = 0; i < maxInvokes; i++) {
            testcase.assertEquals(appID + " verify received a response " + i,
                  1, scorecard[i]);
        }
    }


    /**
     * Handle and respond to any response that comes back.
     */
    void doResponder() {
    InvocationImpl request;
    while (!stopping) {

            // consume any request and send a response
            request = InvocationStore.getRequest(appID, classname, true);
            if (request != null) {
        testcase.assertEquals("verify only ACTIVE requests",
                      Invocation.ACTIVE, request.status);
        testcase.assertTrue("verify responseRequired",
                    request.responseRequired);

        // An ACTIVE request; send a reply
        request.status = Invocation.OK;
        InvocationStore.setStatus(request);
        println("reply:    ", request);
            }
    }
    }

    /**
     * Send a request to every other consumer.
     * @param status a status to send
     * @param action an action string to send
     */
    void sendAll(int status, String action) {
        InvocationImpl request = new InvocationImpl();
        request.invokingSuiteId = appID;
        request.invokingClassname = classname;
        request.status = status;
    request.action = action;
    request.ID = Integer.toString(numInvokes);

        for (int i = 0; i < nappIDs; i++) {
            for (int j = 0; j < nclassnames; j++) {
                request.suiteId = getAppID(i);
                request.classname = getClassname(j);
                InvocationStore.put(request);
        println("sending terminate", request);
            }
        }
    }

    /**
     * Sleep a bit.
     * @param millis millseconds to sleep
     */
    void sleep(long millis) {
        try {
            Thread.sleep(millis);
        } catch (InterruptedException ie) {
        }
    }

    /**
     * Generate the application ID for app(n).
     * @param n the index of the applicationid.
     * @return the string application id.
     */
    int getAppID(int n) {
        return n + 1000;
    }
    /**
     * Generate the classname for content handler (n).
     * @param n the index of the content handler.
     * @return the string classname
     */
    String getClassname(int n) {
        return "class-".concat(Integer.toString(n));
    }
    /**
     * Print the interesting fields of an Invocation.
     * @param msg a message to print before the invocation
     * @param invoc an Invocation
     */
    void println(String msg, InvocationImpl invoc) {
        if (false) {
            System.out.println(appID + " " + msg +
                   ": status = " + invoc.status +
                   ", tid: " + invoc.tid +
                   ", target " + invoc.suiteId +
                   ", action = " + invoc.action);
        }
    }

    /**
     * Generate the next random number between 0 (inclusive)
     * and max (exclusive).
     * @param n limit numbers from 0 to n-1.
     * @return a random number
     */
    private int nextRandom(int n) {
        int val = random.nextInt() % n;
        if (val < 0) {
            val = -val;
        }
        return val;
    }

}

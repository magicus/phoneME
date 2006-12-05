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

package com.sun.midp.content;

import com.sun.midp.i3test.TestCase;

import javax.microedition.content.Invocation;

import java.util.Vector;

/**
 * Test that InvocationImpl instances with a wide range of values
 * can be stored in the InvocationStore and retrieved.
 * <p>
 * The tested functions of Invocation are:
 * <UL>
 * <LI>The settable fields of Invocation can be retrieved and verified.</LI>
 * </UL>
 */
public class TestInvocStore extends ExtendedTestCase {

    /** The application. */
    AppProxy appl;

    /** Number of application ids to test in stress test. */
    static final int NUM_APPLICATION_IDS = 3;
    /** Number of content handlers per application to stress test. */
    static final int NUM_CONTENT_HANDLERS = 2;
    /** A Long string to test against. */
    static final String LONG_STRING =
	"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" +
	"BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB" +
	"CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC" +
	"DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD";

    /** A suite Id to test against. */
    static final int SUITE_ID = 65535;
    /** First test string; empty. */
    static final String STRING1 = "";
    /** Second test string; short. */
    static final String STRING2 = "1";
    /** Third test string; longer. */
    static final String STRING3 = "12";
    /** Fourth test string; equal length. */
    static final String STRING4 = "1*";
    /** Fifth test string; shorter (forced to be different from string2.) */
    static final String STRING5 = new String("1");
    /** Sixth test string; shorter (forced to be different from string1.) */
    static final String STRING6 = new String("");

    /** First args array; empty. */
    static final String[] ARGS1 = new String[0];
    /** First args array; 1 string. */
    static final String[] ARGS2 = new String[1];
    /** First args array; longer, 2 strings. */
    static final String[] ARGS3 = new String[2];
    /** First args array; different two. */
    static final String[] ARGS4 = new String[2];
    /** First args array; shorter, 1 string. */
    static final String[] ARGS5 = new String[1];
    /** First args array; empty again. */
    static final String[] ARGS6 = new String[0];


    /** Maximum number of ARGUMENTS supported. */
    static final int MAX_ARGUMENTS = 10;
    /** Long argument list to test with. */
    static String[] LONG_ARGUMENTS;

    static {
	LONG_ARGUMENTS = new String[MAX_ARGUMENTS];
	for (int i = 0; i < MAX_ARGUMENTS; i++) {
	    LONG_ARGUMENTS[i] = LONG_STRING;
	}
    }

    /**
     * Run the tests.
     */
    public void runTests() {
	try {
	    appl = AppProxy.getCurrent().forClass(classname);
	} catch (ClassNotFoundException cnfe) {
	    fail("Unexpected exception");
	    cnfe.printStackTrace();
	}

	test001();
	test002();
	test003();
	// test004();  // OutOfMemory is unpredictable
	test005();
	test006();
	test007();
	test008();
	test009();
	test010();
    }

    /**
     * Create a new Invocation test case.
     */
    public TestInvocStore() {

    }

    /**
     * Verify that each field of an {@link com.sun.midp.content.InvocationImpl}
     * can be set,
     * put into the store and retrieved and verified.
     * The only test is if the field is saved and restored
     * correctly for values of <code>null</code> and <code>non-null</code>.
     */
    void test001() {
	declare("InvocationStore put/get");
	String[] args = {null, "", "arg1", "arg2"};
	byte[] data = new byte[47];


	InvocationImpl get;
	InvocationImpl put;

	for (int i = 0; i < data.length; i++) {
	    data[i] = (byte)i;
	}

	// Test with most fields empty
	put = new InvocationImpl();
	put.suiteId = appl.getStorageId();
	put.classname = classname;
	put.responseRequired = false;
	InvocationStore.put(put);
	assertTrue("Verify tid assigned", put.tid != 0);

	get = InvocationStore.getRequest(appl.getStorageId(),
					 classname, false);
	assertEquals("Verify active status",
		     Invocation.ACTIVE, get.getStatus());
	assertEquals("Verify get request equals put request", put, get);

	// Finalize the status to have the Invocation discarded
	put.status = Invocation.OK;
	InvocationStore.setStatus(put);



	// Try one with non-null fields
	put = new InvocationImpl();
	put.suiteId = appl.getStorageId();
	put.classname = classname;
	put.setID("ID");
	put.setType("type");
	put.setURL("URL");
	put.setAction("ACTION");
	put.setArgs(args);
	put.setData(data);
	put.setResponseRequired(true);
	put.setCredentials("USERNAME", "PASSWORD".toCharArray());

	put.status = Invocation.OK;
	put.suiteId = appl.getStorageId();
	put.classname = classname;
	put.tid = 0x80808080;
	put.previousTid = 999;
	put.invokingSuiteId = 60000;
	put.invokingClassname = "invokingClassname";
	put.invokingAuthority = "invokingAuthority";
	put.invokingID = "invokingID";
	put.invokingAppName = "invokingAppName";
	put.argsLen = 0x52525252;
	InvocationStore.put(put);
	assertTrue("Verify tid assigned", get.tid != 0);

	get = InvocationStore.getResponse(new InvocationImpl(),
					  appl.getStorageId(), appl.getClassname(),
					  false);
	assertEquals("Non-empty", put, get);
	assertEquals("Verify OK status",
		     Invocation.OK, get.getStatus());

	get = InvocationStore.getResponse(new InvocationImpl(),
					  appl.getStorageId(), appl.getClassname(),
					  false);
	assertNull("Verify nothing lingering", get);

    }


    /**
     * Verify that each field of an
     * {@link com.sun.midp.content.InvocationImpl} can be set,
     * put into the store and retrieved and verified.
     * The only test is if the field is saved and restored
     * correctly for values of <code>null</code> and <code>non-null</code>.
     */
    void test002() {
	declare("Test mark and cleanup");

	/**
	 * Generate and post a request for each status value.
	 * Mark all of those and generate another bunch after the mark.
	 */
	InvocationImpl[] invoc = genEachStatus();
	InvocationStore.setCleanup(appl.getStorageId(), appl.getClassname(), true);
	// InvocationImpl[] invoc2 = genEachStatus();

	// Cycle on cleanup and Verify which ones should be returned
	InvocationImpl get;
	for (int i = Invocation.INIT; i <= Invocation.ACTIVE; i++) {

	    /*
	     * Should get back an INIT, ACTIVE; those need an ERROR Response
	     * OK, CANCELLED, INITIATED, ERROR, WAITING should have
	     * been discarded.
	     */
	    get = InvocationStore.getCleanup(appl.getStorageId(),
					     appl.getClassname());
	    assertEquals("Verify cleanup needed", invoc[i], get);
	    if (get != null) {
		assertEquals("Verify status", invoc[i].status, get.status);
		invoc[i].status = Invocation.ERROR;
		InvocationStore.setStatus(invoc[i]);

	    }
	}

	// Verify that no others are returned
	get = InvocationStore.getCleanup(appl.getStorageId(), appl.getClassname());
	assertNull("Verify no more cleanup needed", get);

	// Now the only invocations in the queue should be the responses
	for (int i = Invocation.INIT; i <= Invocation.ACTIVE; i++) {

            // Retrieve the response and Verify
            get = InvocationStore.getResponse(new InvocationImpl(),
					      appl.getStorageId(),
					      appl.getClassname(), false);
            assertEquals("Verify error response delivered", invoc[i], get);
            if (get != null) {
                assertEquals("Verify error response status",
                             Invocation.ERROR, get.getStatus());
            }
	}

        /*
	 * What should remain is a HOLD invocation
	 * that needs to be changed before they can be removed.
	 */
	for (int i = Invocation.HOLD; i <= Invocation.HOLD; i++) {
	    invoc[i].status = Invocation.OK;
	    InvocationStore.setStatus(invoc[i]);
	    get = InvocationStore.getResponse(new InvocationImpl(),
					      appl.getStorageId(),
					      appl.getClassname(), false);
	    assertEquals("Verify received response", invoc[i], get);
	    if (get != null) {
		assertEquals("Verify response status",
			     invoc[i].status, get.status);
	    }
	}

	assertEmpty();
    }

    /**
     * Test that for a large number of inserts the order
     * is maintained when multiple entries have the same ID, classname.
     * A dataset is generated with only the TID differing.
     * The verification fetches them and verifies the order.
     */
    void test003() {
	declare("Maintaining order");
	int NUM = 50;
	Vector v = new Vector(NUM);
	InvocationImpl put;
	for (int i = 0; i < NUM; i++) {
	    put = new InvocationImpl();
	    put.suiteId = appl.getStorageId();
	    put.classname = classname;
	    put.status = Invocation.OK;	// Use ok so they are removed later
	    v.addElement(put);
	    InvocationStore.put(put);
	}
	InvocationImpl get;
	for (int i = 0; i < NUM; i++) {
	    get = InvocationStore.getResponse(new InvocationImpl(),
					      appl.getStorageId(),
					      appl.getClassname(), false);
	    put = (InvocationImpl)v.elementAt(i);
	    assertEquals("Non-empty", put, get);
	}
    }

    /**
     * Test exhausting the native heap storage with Invocations.
     * Max size invocations are created, saved and put in the Invocation
     * store until put throws OutOfMemoryError.
     * All of the Invocations are read back and compared.
     */
    void test004() {
	declare("Force out of memory and recovery");
	int maxInvocations = 10000;
	InvocationImpl get = null;
	InvocationImpl put = null;
	InvocationImpl last = null;
	Vector v = new Vector(maxInvocations);
        byte[] space = new byte[64000]; /* Space to free on out of memory */

	try {
	for (int i = 0; i < maxInvocations; i++) {
	    try {
		put = newMaxInvocation();
		put.status = Invocation.OK;
		v.addElement(put);
	    } catch (OutOfMemoryError mex) {
		System.out.println("exhausted Java heap " +
				   i + " iterations");
                // Free some heap so the test can continue
                space = null;
		break;
	    }
	    try {
		InvocationStore.put(put);
	    } catch (OutOfMemoryError ex) {
		// exhausted native heap
        	System.out.println("exhausted native heap " +
				   i + " iterations");
		// Remove the final element that was not put and save
		v.removeElement(put);
		last = put;
		break;
	    }
	}
	try {
	    // Verify that all the stored Invocations are correct
	    for (int i = 0; i < v.size(); i++) {
		get = InvocationStore.getResponse(new InvocationImpl(),
						  appl.getStorageId(),
						  appl.getClassname(), false);
		put = (InvocationImpl)v.elementAt(i);
		assertEquals("Check invocations before OutOfMemory", put, get);
		if (get == null) {
		    break;
		}
	    }
	} catch (OutOfMemoryError mex) {
	    System.out.println("exhausted Java heap on get ");
	    System.out.println("available memory = " +
			       Runtime.getRuntime().freeMemory());
	}

	try {
	    System.out.println("v.size(): " + v.size());
	    System.out.println("get.tid: " + ((get != null) ? get.tid : -1));
	    if (last != null) {
		// Now store and Verify the failed Invocation
		put = last;
		InvocationStore.put(put);
		get = InvocationStore.getResponse(new InvocationImpl(),
						  appl.getStorageId(),
						  appl.getClassname(), false);
		assertEquals("Check store/Verify after OutOfMemory", put, get);
	    }
	} catch (OutOfMemoryError mex) {
	    System.out.println("exhausted Java heap on 2nd get ");
	    System.out.println("available memory = " +
			       Runtime.getRuntime().freeMemory());
	}

	assertEmpty();

	} catch (Throwable t) {
	    space = null;
	    t.printStackTrace();
	}
    }

    /**
     * Test that the selection based on request/response values works.
     * Both the {@link com.sun.midp.content.InvocationStore#get} and
     * {@link com.sun.midp.content.InvocationStore#listen}
     * are tested at the same time.
     * Every status is put into the queue and then the response
     * status values are retrieved and verified. Then the active
     * status value is verified.
     *
     * Each test expects the results to come back in a particular
     * order and verifies it.
     */
    void test005() {
	declare("Selection based on status");
	InvocationImpl get;
	InvocationImpl put;
        boolean pending;

	assertEmpty();

	/**
	 * Listen and Get work by status
	 */
	InvocationImpl[] invoc = genEachStatus();

        // Verify that the responses can be found
        for (int i = Invocation.OK; i <= Invocation.INITIATED; i++) {
            pending = InvocationStore.listen(appl.getStorageId(),
					     appl.getClassname(),
                                             false, false);
            assertTrue("1. listen for pending response", pending);

            get = InvocationStore.getResponse(new InvocationImpl(),
					      appl.getStorageId(),
					      appl.getClassname(), false);
            assertEquals("2. fetch a response", invoc[i], get);
        }

        // Try again to Verify nothing there
        get = InvocationStore.getResponse(new InvocationImpl(),
					  appl.getStorageId(),
					  appl.getClassname(), false);
        assertNull("3. Verify no pending response ", get);


        // Verify that the one ACTIVE request can be found
        pending = InvocationStore.listen(appl.getStorageId(), appl.getClassname(),
					 true, false);
        assertTrue("4. listen for pending request", pending);

        put = InvocationStore.getRequest(appl.getStorageId(),
					 appl.getClassname(),
					 false);
        assertEquals("5. fetch a request", invoc[Invocation.INIT], put);
	if (put != null) {
	    assertEquals("Verify state is ACTIVE",
			 Invocation.ACTIVE, put.getStatus());
	    assertTrue("Verify responseRequired is true",
			 put.getResponseRequired());
	}

        // Try again to Verify nothing there
        get = InvocationStore.getRequest(appl.getStorageId(),
					 appl.getClassname(),
					 false);
        assertNull("6. Verify no extra response", get);

        // Set its status to OK and discard it
        put.status = Invocation.OK;
        InvocationStore.setStatus(put);

	// ResponseRequired is true so expect a response
	get = InvocationStore.getResponse(new InvocationImpl(),
					  appl.getStorageId(),
					  appl.getClassname(), false);
	assertEquals("7. fetch a response", put, get);

        // Verify nothing there
        get = InvocationStore.getResponse(new InvocationImpl(),
					  appl.getStorageId(),
					  appl.getClassname(), false);
        assertNull("8. Verify no extra response", get);


	// Change status of ACTIVE, WAITING, HOLD to OK to discard
	for (int i = Invocation.ACTIVE; i <= Invocation.HOLD; i++) {
	    put = invoc[i];
	    put.status = Invocation.OK;
	    InvocationStore.setStatus(put);
	}

        // Verify nothing there
        get = InvocationStore.getRequest(appl.getStorageId(),
					 appl.getClassname(),
					 false);
        assertNull("11. extra Invocation", get);
    }

    /**
     * Test that get requests can be interrupted with the cancel method.
     * Check that cancel doesn't corrupt the queue or loose requests.
     * drain(); cancel(); get(); cancel(); put(); get(); cancel();
     */
    void test006() {
	declare("Cancelling of get and listen");

	InvocStoreCancel tracker = new InvocStoreCancel(true, appl);

	// Start the get processing
	tracker.reset();

	// Do a cancel without nothing pending
	InvocationStore.cancel();

	assertEquals("Check the initial condition", 0, tracker.check());

	Thread thread = new Thread(tracker);
	thread.start();

	sleep(500);

	// Do a cancel and see if a get was cancelled
	InvocationStore.cancel();

	sleep(500);

	assertEquals("Check the get was cancelled", 1, tracker.numNotPending);

	// Stop the tracking thread and wait for the thread to terminate
	tracker.stop();
	InvocationStore.cancel(); // Unblock
	assertEquals("Check the get was unblocked", 1, tracker.numNotPending);

	sleep(500);

	try {
	    thread.join();
	} catch (InterruptedException ie) {
	    assertNull("Verify InterruptedException on join", ie);
	}
    }

    /**
     * Test that get by <code>tid</code> works.
     * An instance of each status is created and then they are
     * fetched by tid.
     */
    void test007() {
	declare("Get by TID");
	InvocationImpl get;
	InvocationImpl put;

	assertEmpty();

        /*
	 * Create one of each status and Verify that they can be
	 * found by {@link com.sun.midp.content.InvocationStore#getByTid}.
	 * GetByTid does not remove or modify the Invocations.
	 */
	InvocationImpl[] invoc = genEachStatus();
        for (int i = Invocation.OK; i <= Invocation.INITIATED; i++) {
            get = InvocationStore.getByTid(invoc[i].tid, 0);
	    assertEquals("Verify getByTid matches put", invoc[i], get);
        }

        // Verify 1 active request pending
        put = invoc[Invocation.INIT];
        get = InvocationStore.getRequest(appl.getStorageId(),
					 appl.getClassname(),
					 false);
        assertEquals("Verify a pending request after getByTid", put, get);
        if (get != null) {
            put.status = Invocation.OK;
            InvocationStore.setStatus(put);
            get = InvocationStore.getResponse(new InvocationImpl(),
					      appl.getStorageId(),
					      appl.getClassname(), false);
            assertEquals("Verify response matches request", put, get);
        }
           // Verify no active request pending
        get = InvocationStore.getRequest(appl.getStorageId(),
					 appl.getClassname(),
					 false);
        assertNull("Verify no pending request after getByTid", get);

	// Change status of INIT, ACTIVE, WAITING, HOLD to OK to discard
	for (int i = Invocation.ACTIVE; i <= Invocation.INITIATED; i++) {
	    put = invoc[i];
	    put.status = Invocation.OK;
	    InvocationStore.setStatus(put);

	    // Get and discard the response generated by setStatus
	    get = InvocationStore.getResponse(new InvocationImpl(),
					      appl.getStorageId(),
					      appl.getClassname(), false);
	    assertEquals("Verify response matches put", put, get);
	}

	assertEmpty();
    }

    /**
     * Test that setParams can correctly set and reset all
     * parameters and not result in and not result in any leaks.
     * <br>
     * An invocation is put into the store and then a series of
     * setParams calls are used to modify the parameters.
     * After each modification the request is retrieved and
     * compared with the expected modified request.
     */
    void test008() {
	declare("setParams integrity");

	// Create an invocation a put it in the native store
	InvocationImpl invoc = new InvocationImpl();
	invoc.suiteId = appl.getStorageId();
	invoc.classname = classname;
	invoc.status = Invocation.ACTIVE;
        invoc.responseRequired = false;
	InvocationStore.put(invoc);

	String[] args = null;
	do {
	    /*
	     * For each argument case.
	     */
	    String string = null;
	    do {
		/*
		 * For each string value; check the type, url, ID, and action.
		 * Call setParams to put the values to native and then getByTid
		 * to get back a copy of the request and compare them.
		 */
		fillInvocation(invoc, string, args);
		InvocationStore.setParams(invoc);

		InvocationImpl req = InvocationStore.getByTid(invoc.tid, 0);
		assertNotNull("Verify the request was returned", req);
		if (req != null) {
		    assertEquals("Verify parameter values", invoc, req);
		}

		// Generate the next set of string values
		string = nextString(string);
	    } while (string != null);
	    args = nextArgs(args);
	} while (args != null);

	// Discard the pending request
        invoc.status = Invocation.OK;
        InvocationStore.setStatus(invoc);
	assertEmpty();
    }

    /**
     * Fill in the invocation with the next data case.
     * The same data is used in every field in the Invocation.
     * The args are initialized with the string as appropriate.
     * The data is filled from the string if non-null.
     *
     * @param invoc Invocation
     * @param string to fill into Invocation fields and args and data
     * @param args a prototype args array to fill
     */
    void fillInvocation(InvocationImpl invoc,
			String string,
			String[] args)
    {
	invoc.setID(string);
	invoc.setURL(string);
	invoc.setType(string);
	invoc.setAction(string);
	invoc.setArgs(args);
	if (args != null) {
	    for (int i = 0; i < args.length; i++) {
		args[i] = string;
	    }
	}
        invoc.setData((string == null) ? null : string.getBytes());
    }

    /**
     * Sequence through the test strings; can start anywhere
     * but typically start/end with null.
     * @param string the current string in the sequence
     * @return the nextt string in the sequence
     */
    String nextString(String string) {
	if (string == null) {
	    return STRING1;
	} else if (string == STRING1) {
	    return STRING2;
	} else if (string == STRING2) {
	    return STRING3;
	} else if (string == STRING3) {
	    return STRING4;
	} else if (string == STRING4) {
	    return STRING5;
	} else if (string == STRING5) {
	    return STRING6;
	} else {
	    // Start over
	    return null;
	}
    }

    /**
     * Generate the next array to test.
     * The sequence is null, 0, 1, 2, 2, 1, 0; back to null.
     * @param args an String array; may be null.
     * @return the next args array; not filled with anything in particular
     */
    String[] nextArgs(String[] args) {
	if (args == null) {
	    return ARGS1;
	} else if (args == ARGS1) {
	    return ARGS2;
	} else if (args == ARGS2) {
	    return ARGS3;
	} else if (args == ARGS3) {
	    return ARGS4;
	} else if (args == ARGS4) {
	    return ARGS5;
	} else if (args == ARGS5) {
	    return ARGS6;
	} else {
	    return null;
	}
    }

    /**
     * Test that getByTid handles next, equals, and previous correctly.
     */
    void test009() {
	declare("getByTid testing next, previous, same");
	assertEmpty();

	// Test that when empty there is no next, previous, equal.
	InvocationImpl invoc = null;
	invoc = InvocationStore.getByTid(0, -1);
	assertNull("Verify no previous on empty queue", invoc);
	invoc = InvocationStore.getByTid(0, 0);
	assertNull("Verify no zeroth on empty queue", invoc);
	invoc = InvocationStore.getByTid(0, +1);
	assertNull("Verify no next on empty queue", invoc);

	// Insert a single Invocation
	InvocationImpl invoc1 = new InvocationImpl();
	invoc1.suiteId = appl.getStorageId();
	invoc1.classname = classname;
	invoc1.status = Invocation.ACTIVE;
        invoc1.responseRequired = false;
	InvocationStore.put(invoc1);

	invoc = InvocationStore.getByTid(invoc1.tid, -1);
	assertNull("Verify no previous on single entry", invoc);
	invoc = InvocationStore.getByTid(invoc1.tid, 0);
	assertEquals("Verify equal on single entry",
		     invoc1.tid, invoc.tid);
	invoc = InvocationStore.getByTid(invoc1.tid, +1);
	assertNull("Verify no next in single entry", invoc);

	// Insert a second Invocation
	InvocationImpl invoc2 = new InvocationImpl();
	invoc2.suiteId = appl.getStorageId();
	invoc2.classname = classname;
	invoc2.status = Invocation.ACTIVE;
        invoc2.responseRequired = false;
	InvocationStore.put(invoc2);

	invoc = InvocationStore.getByTid(invoc2.tid, -1);
	assertEquals("Verify previous on double entry",
		    invoc1.tid, invoc.tid);
	invoc = InvocationStore.getByTid(invoc2.tid, 0);
	assertEquals("Verify equal on double entry",
		     invoc2.tid, invoc.tid);
	invoc = InvocationStore.getByTid(invoc2.tid, +1);
	assertNull("Verify no next in double entry", invoc);

	// Discard the pending request(s)
        invoc1.status = Invocation.OK;
        InvocationStore.setStatus(invoc1);
        invoc2.status = Invocation.OK;
        InvocationStore.setStatus(invoc2);

	assertEmpty();
    }

    /**
     * Stress test using multiple threads to pound on the queue.
     * Each thread performs a series of gets and puts with
     * pseudo random choices of target id, classname, and status
     */
    void test010() {
	declare("Multithread Stress Test");
        InvocStoreStress[][] stress =
                new InvocStoreStress[NUM_APPLICATION_IDS][NUM_CONTENT_HANDLERS];
        for (int i = 0; i < NUM_APPLICATION_IDS; i++) {
            for (int j = 0; j < NUM_CONTENT_HANDLERS; j++) {
                InvocStoreStress s =
		    new InvocStoreStress(i, j,
					 NUM_APPLICATION_IDS,
					 NUM_CONTENT_HANDLERS, this, appl);
                stress[i][j] = s;
                new Thread(s).start();
            }
        }

        // Join all the threads to make sure they are done
        for (int i = 0; i < NUM_APPLICATION_IDS; i++) {
            for (int j = 0; j < NUM_CONTENT_HANDLERS; j++) {
                InvocStoreStress s = stress[i][j];
                try {
		    while (s.thread == null) {
			sleep(1000L);
		    }
                    s.thread.join();
                } catch (InterruptedException ii) {
                    ii.printStackTrace();
                }
            }
        }
    }

    /**
     * Setup and put an Invocation with each status value
     * from Invocation.INIT (1) to Invocation.INITIATED (8).
     * @return Vector of posted InvocationImpls
     */
    InvocationImpl[] genEachStatus() {
	InvocationImpl[] invoc = new InvocationImpl[Invocation.INITIATED + 1];
	for (int i = Invocation.INIT; i <= Invocation.INITIATED; i++) {
	    InvocationImpl put = new InvocationImpl();
	    put.suiteId = appl.getStorageId();
	    put.classname = classname;
	    put.invokingSuiteId = appl.getStorageId();
	    put.invokingClassname = classname;
	    put.status = i;
	    invoc[i] = put;
	    InvocationStore.put(put);
	}
	return invoc;
    }

    /**
     * Check that there are no Invocations pending;
     * none should be.
     */
    void assertEmpty() {
	InvocationImpl get;
	do {
	    get = InvocationStore.getRequest(appl.getStorageId(), appl.getClassname(),
					     false);
	    assertNull("Verify request queue is empty", get);
	} while (get != null);

	do {
	    get = InvocationStore.getResponse(new InvocationImpl(),
					      appl.getStorageId(),
					      appl.getClassname(), false);
	    assertNull("Verify response queue is empty", get);
	} while (get != null);

	assertEquals("Verify invocation queue is empty",
		     0, InvocationStore.size());
    }

    /**
     * Make a new maximum size Invocation.
     * @return a new InvocationImpl
     */
    InvocationImpl newMaxInvocation() {
	InvocationImpl put = new InvocationImpl();
	put.setID(LONG_STRING);
	put.setType(LONG_STRING);
	put.setURL(LONG_STRING);
	put.setAction(LONG_STRING);
	put.setArgs(LONG_ARGUMENTS);
	put.setResponseRequired(true);
	put.suiteId = appl.getStorageId();
	put.classname = classname;
	put.invokingSuiteId = SUITE_ID;
	put.invokingClassname = LONG_STRING;
	put.invokingAuthority = LONG_STRING;
	put.invokingID = LONG_STRING;
	return put;
    }

}

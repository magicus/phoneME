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
import com.sun.midp.security.SecurityToken;
import com.sun.midp.midlet.MIDletStateHandler;
import com.sun.midp.midlet.MIDletSuite;


/**
 * Extension to the basic TestCase class to add ContentHandler
 * specific assert methods.
 */
public abstract class ExtendedTestCase extends TestCase {

    /** Classname of the MIDlet. */
    String classname = "com.sun.midp.i3test.Framework";

    /**
     * Initialize the registry for this application.
     * @return the registry for the Framework class
     */
    RegistryImpl getRegistry() {
	try {
	    SecurityToken token = getSecurityToken();
	    MIDletStateHandler mstate =
		MIDletStateHandler.getMidletStateHandler();
            MIDletSuite msuite = mstate.getMIDletSuite();
	    msuite.setTempProperty(token, "MIDlet-1",
				   "CHAPI Tests,," + classname);

	    return RegistryImpl.getRegistryImpl(classname, token);
	} catch (SecurityException sx) {
	    fail("SecurityException: can't setTempProperty");
	} catch (Throwable t) {
	    assertNull("i3test can't get registry for class " + classname, t);
	    t.printStackTrace();
	}
	return null;
    }

    /**
     * Override assertEquals to allow null pointers.
     * @param message message
     * @param expected the expected object
     * @param actual the actual object.
     */
    public void assertEquals(String message, Object expected, Object actual) {
	if (expected == actual) {
	    // The identity and the null == null
	    return;
	}
	if (expected != null) {
	    assertTrue(message, expected.equals(actual));
	} else {
	    assertTrue(message, actual.equals(expected));
	}
    }

    /**
     * Assert that all of the fields of the two InvocationImpls match
     * exactly.
     * @param msg the message to print for assert failures
     * @param expected the InvocationImpl containing the expected values
     * @param actual the InvocationImpl containing the actual values
     */
    public void assertEquals(String msg, InvocationImpl expected,
			      InvocationImpl actual) {
	if (expected == actual) {
	    return;
	}
	assertNotNull(msg + " expected: ", actual);
	if (actual == null) {
	    return;
	}

        assertEquals("verify getID", expected.getID(), actual.getID());
        assertEquals("verify getType", expected.getType(), actual.getType());
        assertEquals("verify getURL", expected.getURL(), actual.getURL());
        assertTrue("verify responseRequired",
            expected.getResponseRequired() == actual.getResponseRequired());
	assertEquals("verify action",
		     expected.getAction(), actual.getAction());
        assertEquals("verify classname",
		     expected.classname, actual.classname);
        assertEquals("verify invokingSuiteId", expected.invokingSuiteId,
		     actual.invokingSuiteId);
        assertEquals("verify invokingClassname",
		     expected.invokingClassname,
		     actual.invokingClassname);
        assertEquals("verify tid", expected.tid, actual.tid);
        assertEquals("verify invokingAuthority",
		     expected.invokingAuthority,
		     actual.invokingAuthority);
        assertEquals("verify invokingID", expected.invokingID,
		     actual.invokingID);
	assertEquals("verify previousTid",
		     expected.previousTid, actual.previousTid);
	assertEquals("verify arguments",
		     expected.getArgs(), actual.getArgs());
	String[] args = expected.getArgs();
	int argsLen = args != null ? args.length : 0;
	assertEquals("verify argsLen", argsLen, actual.argsLen);
	assertEquals("verify data", expected.data, actual.data);
    }

    /**
     * Compare two arrays of strings, must be the same length
     * and contents to match.
     * @param msg the message to print for assert failures
     * @param s1 an array of strings
     * @param s2 another array of strings
     */
    public void assertEquals(String msg, String[] s1, String[] s2) {
	// Identical arrays are equals
	if (s1 == s2) {
	    return;
	}
	// If either array is null then they don't match
	if (s1 == null) {
	    fail("mismatched arg arrays; expected is null");
	    return;
	}
	if (s2 == null) {
	    fail("mismatched arg arrays; actual is null");
	    return;
	}
	// If the lengths are unequal then they are not equal
	assertEquals("mismatched arg lengths", s1.length, s2.length);

	if (s1.length == s2.length) {
	    // Compare the strings in each array
	    for (int i = 0; i < s1.length; i++) {
		// Two strings, non-null of the same length
		assertEquals("arg " + i, s1[i], s2[i]);
	    }
	}
    }


    /**
     * Compare two byte arrays; must be same length.
     * @param msg the description of the array
     * @param expected the expected data array
     * @param actual the actual data array
     */
    public void assertEquals(String msg, byte[] expected, byte[] actual) {
        if (expected == actual) {
            return;
        }
        if (expected == null) {
            fail("expected string array is null");
            return;
        }
        if (actual == null) {
            fail("actual string array is null");
            return;
        }
        assertEquals(msg + " length", expected.length, actual.length);
	int badbytes = 0;
	if (expected.length == actual.length) {
	    for (int i = 0; i < expected.length; i++) {
		if (expected[i] != actual[i]) {
		    if (0 == badbytes++) {
			fail(msg + " data does not match");
		    }
		    assertEquals("  " + i + ": ", expected[i], actual[i]);
		}
	    }
	}
    }

    /**
     * Sleep a bit.
     * @param millis millseconds to sleep
     */
    public static void sleep(long millis) {
        try {
            Thread.sleep(millis);
        } catch (InterruptedException ie) {
        }
    }
}

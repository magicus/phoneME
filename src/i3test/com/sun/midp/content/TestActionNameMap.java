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

package com.sun.midp.content;

import com.sun.midp.i3test.TestCase;

import javax.microedition.content.ActionNameMap;

/**
 * Tests for creation of ActionNameMaps.
 */
public class TestActionNameMap extends ExtendedTestCase {
    
    /** Valid locale. */
    static final String locale = "en_US";

    /** Empty string arrays. */
    static final String[] zeroStrings = new String[0];

    /** Length 1 string array. */
    static final String[] oneString = { "1" };

    /** Length 1 name string array. */
    static final String[] oneNameString = { "N1" };

    /** Length 5 string array. */
    static final String[] fiveStrings = { "1", "2", "3", "4", "5" };

    /** Length 5 names string array. */
    static final String[] fiveNameStrings = { "N1", "N2", "N3", "N4", "N5" };

    /** String array containing a null. */
    static final String[] nullString = { "n1", "n2", "n3", "n4", null };
 
    /** String array containing an empty string. */
    static final String[] emptyString = { "e1", "e2", "e3", "e4", ""};
 
    /** String array containing a duplicate string. */
    static final String[] dupString = { "dup", "d2", "d3", "dup", "d5"};
 

    /**
     * Run the tests of the Listener.
     */
    public void runTests() {
	test001();
	test002();
	test003();
    }

    /**
     * Test that the listener is notified when a new Invocation
     * is queued.
     */
    private void test001() {
	declare("Create Valid maps");

	assertLegal("one action/name pair",
		    oneString, oneNameString, locale);
	assertLegal("five action/name pairs",
		    fiveStrings, fiveNameStrings, locale);
    }

    /**
     * Test for all the IllegalArgument combinations.
     */
    private void test002() {
	declare("Create Illegal cases");
	assertIllegal("empty arrays", 
		      zeroStrings, zeroStrings, locale,
		      new IllegalArgumentException());
	assertIllegal("null actions", null, oneNameString, locale,
		      new NullPointerException());

	assertIllegal("null actionname", oneString, null, locale,
		      new NullPointerException());

	assertIllegal("unequal action/name pairs", oneString, fiveNameStrings,
		      locale, new IllegalArgumentException());

	assertIllegal("null locale", oneString, oneNameString, null,
		      new NullPointerException());

	assertIllegal("empty locale", oneString, oneNameString,
		      "", new IllegalArgumentException());

	assertIllegal("null action string", nullString, fiveNameStrings,
		      locale, new NullPointerException());

	assertIllegal("empty action string", emptyString, fiveNameStrings,
		      locale, new IllegalArgumentException());

	assertIllegal("null action name string", fiveStrings, nullString,
		      locale, new NullPointerException());

	assertIllegal("empty action name string", fiveStrings, emptyString,
		      locale, new IllegalArgumentException());

	assertIllegal("duplicate action string", dupString, fiveNameStrings,
		      locale, new IllegalArgumentException());
    }

    /**
     * Test NullPointerException cases.
     */
    void test003() {


	ActionNameMap map;
	map = new ActionNameMap(oneString, oneNameString, locale);
	
	try {
	    String a = map.getActionName(null);
	    fail("getActionName(null) must throw npe");
	} catch (NullPointerException npe) {
	    assertNotNull("verify exception occurred", npe);
	}
	try {
	    String a = map.getAction(null);
	    fail("getActionName(null) must throw npe");
	} catch (NullPointerException npe) {
	    assertNotNull("verify exception occurred", npe);
	}

    }

    /**
     * Create an actionNameMap from the args and verify it was created
     * correctly.  Each of the method of the resulting map is 
     * checked with the test case asserts.  Exceptions are not handled.
     * @param message the message for the errors
     * @param actions the array of actions
     * @param actionnames the array of actionnames
     * @param locale the locale
     */
    void assertLegal(String message,
		String[] actions, String[]actionnames, String locale) {
	ActionNameMap map;
	map = new ActionNameMap(actions, actionnames, locale);

	int size = map.size();
	assertEquals(message, actions.length, size);
	assertEquals(message, actionnames.length, size);
	assertEquals(message, locale, map.getLocale());

	for (int i = 0; i < size; i++) {
	    assertEquals(message, actions[i], map.getAction(i));
	}

	for (int i = 0; i < size; i++) {
	    assertEquals(message, actionnames[i], map.getActionName(i));
	}

	for (int i = 0; i < size; i++) {
	    assertEquals(message, actionnames[i], 
			 map.getActionName(actions[i]));
	}

	for (int i = 0; i < size; i++) {
	    assertEquals(message, actions[i], map.getAction(actionnames[i]));
	}
    }

    /**
     * Create an actionNameMap from the args and verify that
     * ane xception is thrown.
     * @param message the message for the errors
     * @param actions the array of actions
     * @param actionnames the array of actionnames
     * @param locale the locale
     * @param expected the expected exception
     */
    void assertIllegal(String message,
		       String[] actions, String[]actionnames, String locale,
		       Exception expected) {
	ActionNameMap map;
	try {
	    map = new ActionNameMap(actions, actionnames, locale);
	    // Always report an error
	    assertEquals("MUST throw an exception", null, map);
 	} catch (IllegalArgumentException ill) {
	    assertEquals(message, expected.getClass().getName(),
			ill.getClass().getName());
	} catch (NullPointerException npe) {
	    assertEquals(message, expected.getClass().getName(),
			npe.getClass().getName());
	}
    }
}

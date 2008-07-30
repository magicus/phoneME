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

/**
 * Tests basic operations on the LinkMessage class.
 */
public class TestLinkMessage extends TestCase {


    /**
     * Checks the creation of a data message with subrange arguments to 
     * determine whether it throws IndexOutOfBoundsException. Returns a 
     * boolean indicating whether IOOBE was thrown.
     */
    boolean checkRange(byte[] data, int offset, int length) {
        boolean thrown = false;

        try {
            LinkMessage.newDataMessage(data, offset, length);
        } catch (IndexOutOfBoundsException ioobe) {
            thrown = true;
        }

        return thrown;
    }


    /**
     * Tests a LinkMessage containing a basic byte array (that is, not a 
     * subrange).
     */
    void testData() {
        byte[] data = Utils.extractBytes("this is a test string");
        LinkMessage lm = LinkMessage.newDataMessage(data);

        assertTrue("containsData must return true", lm.containsData());
        assertFalse("containsLink must return false", lm.containsLink());
        assertFalse("containsString must return false", lm.containsString());
        assertSame("extract must return same object", data, lm.extract());

        boolean thrown;
        byte[] retData = null;

        thrown = false;
        try {
            retData = lm.extractData();
        } catch (IllegalStateException ise) {
            thrown = true;
        }
        assertFalse("extractData must not throw exception", thrown);
        assertTrue("extractData must return equal data",
            Utils.bytesEqual(data, retData));

        thrown = false;
        Object obj;
        try {
            obj = lm.extractLink();
        } catch (IllegalStateException ise) {
            thrown = true;
        }
        assertTrue("extractLink must throw exception", thrown);

        thrown = false;
        try {
            obj = lm.extractString();
        } catch (IllegalStateException ise) {
            thrown = true;
        }
        assertTrue("extractString must throw exception", thrown);
    }


    /**
     * Tests a LinkMessage that contains a byte array subrange.
     */
    void testDataSubrange() {
        byte[] data = Utils.extractBytes("this is test data fodder");

        assertTrue("offset -7 must throw ioobe", checkRange(data, -7, 10));
        assertTrue("offset 9853 must throw ioobe", checkRange(data, 9853, 3));
        assertTrue("length 2387 must throw ioobe", checkRange(data, 0, 2387));
        assertTrue("length -2 must throw ioobe", checkRange(data, 5, -2));
        assertTrue("20, 20 must throw ioobe", checkRange(data, 20, 20));

        assertFalse("0,0 ok",   checkRange(data, 0, 0));
        assertFalse("0,len ok", checkRange(data, 0, data.length));
        assertFalse("len,0 ok", checkRange(data, data.length, 0));
        assertFalse("3,6 ok",   checkRange(data, 3, 6));

        LinkMessage lm;
        byte[] retData;

        lm = LinkMessage.newDataMessage(data, 0, data.length);
        retData = lm.extractData();
        assertTrue("full data", Utils.bytesEqual(data, retData));

        byte[] nullData = new byte[0];

        lm = LinkMessage.newDataMessage(data, 0, 0);
        retData = lm.extractData();
        assertTrue("zero null", Utils.bytesEqual(nullData, retData));

        lm = LinkMessage.newDataMessage(data, data.length, 0);
        retData = lm.extractData();
        assertTrue("end null", Utils.bytesEqual(nullData, retData));

        lm = LinkMessage.newDataMessage(data, 5, 9);
        retData = lm.extractData();
        assertTrue("subrange", Utils.bytesEqual(data, 5, 9, retData));
    }


    /**
     * Tests a LinkMessage containing a Link.
     */
    void testLink() {
        Isolate isolate = Isolate.currentIsolate();
        Link link = Link.newLink(isolate, isolate);
        LinkMessage lm = LinkMessage.newLinkMessage(link);

        assertFalse("containsData must return false", lm.containsData());
        assertTrue("containsLink must return true", lm.containsLink());
        assertFalse("containsString must return false", lm.containsString());
        assertSame("extract must return same object", link, lm.extract());

        boolean thrown;
        Object obj;

        thrown = false;
        try {
            obj = lm.extractData();
        } catch (IllegalStateException ise) {
            thrown = true;
        }
        assertTrue("extractData must throw exception", thrown);

        thrown = false;
        Link retLink = null;
        try {
            retLink = lm.extractLink();
        } catch (IllegalStateException ise) {
            thrown = true;
        }
        assertFalse("extractLink must not throw exception", thrown);
        assertSame("extractLink must return same link",
            link, retLink);

        thrown = false;
        try {
            obj = lm.extractString();
        } catch (IllegalStateException ise) {
            thrown = true;
        }
        assertTrue("extractString must throw exception", thrown);
    }


    /**
     * Tests a LinkMessage containing a String.
     */
    void testString() {
        String msg = "this is a string message";
        LinkMessage lm = LinkMessage.newStringMessage(msg);

        assertFalse("containsData must return false", lm.containsData());
        assertFalse("containsLink must return false", lm.containsLink());
        assertTrue("containsString must return true", lm.containsString());
        assertSame("extract must return same object", msg, lm.extract());

        boolean thrown;
        Object obj;

        thrown = false;
        try {
            obj = lm.extractData();
        } catch (IllegalStateException ise) {
            thrown = true;
        }
        assertTrue("extractData must throw exception", thrown);

        thrown = false;
        try {
            obj = lm.extractLink();
        } catch (IllegalStateException ise) {
            thrown = true;
        }
        assertTrue("extractLink must throw exception", thrown);

        thrown = false;
        String str = null;
        try {
            str = lm.extractString();
        } catch (IllegalStateException ise) {
            thrown = true;
        }
        assertFalse("extractString must not thrown exception", thrown);
        assertTrue("strings must match", msg.equals(str));
    }


    /**
     * Runs all tests.
     */
    public void runTests() {
        declare("data");
        testData();
        declare("data subrange");
        testDataSubrange();
        declare("link");
        testLink();
        declare("string");
        testString();
    }

}

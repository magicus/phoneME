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

package com.sun.midp.i3test;

import com.sun.midp.security.SecurityToken;
import com.sun.midp.security.ImplicitlyTrustedClass;
import com.sun.midp.security.SecurityInitializer;

class SelfTest {

    static final String pkgname = "com.sun.midp.i3test";

    static int internalErrorCount = 0;
    static int selfTestCount = 0;
    static String errorLog = "";

    static void check(
        String label,
        int eTotalCases,
        int eTotalTests,
        int eTotalAsserts,
        int eTotalFailures,
        int eErrorClassNotFound,
        int eErrorConstructorException,
        int eErrorNotTestCase,
        int eErrorTestRunThrows,
        int eErrorNoTests,
        int eErrorAssertWithoutTest,
        int eErrorTestWithoutAssert)
    {
        String buf = "";

        if (eTotalCases != TestCase.totalCases) {
            buf += "  totalCases: expected " + eTotalCases +
                ", got " + TestCase.totalCases + "\n";
        }

        if (eTotalTests != TestCase.totalTests) {
            buf += "  totalTests: expected " + eTotalTests +
                ", got " + TestCase.totalTests + "\n";
        }

        if (eTotalAsserts != TestCase.totalAsserts) {
            buf += "  totalAsserts: expected " + eTotalAsserts +
                ", got " + TestCase.totalAsserts + "\n";
        }

        if (eTotalFailures != TestCase.totalFailures) {
            buf += "  totalFailures: expected " + eTotalFailures +
                ", got " + TestCase.totalFailures + "\n";
        }

        if (eErrorClassNotFound != TestCase.errorClassNotFound) {
            buf += "  errorClassNotFound: expected " + eErrorClassNotFound +
                ", got " + TestCase.errorClassNotFound + "\n";
        }

        if (eErrorConstructorException != TestCase.errorConstructorException) {
            buf += "  errorConstructorException: expected " +
                eErrorConstructorException +
                ", got " + TestCase.errorConstructorException + "\n";
        }

        if (eErrorNotTestCase != TestCase.errorNotTestCase) {
            buf += "  errorNotTestCase: expected " + eErrorNotTestCase +
                ", got " + TestCase.errorNotTestCase + "\n";
        }

        if (eErrorTestRunThrows != TestCase.errorTestRunThrows) {
            buf += "  errorTestRunThrows: expected " + eErrorTestRunThrows +
                ", got " + TestCase.errorTestRunThrows + "\n";
        }

        if (eErrorNoTests != TestCase.errorNoTests) {
            buf += "  errorNoTests: expected " + eErrorNoTests +
                ", got " + TestCase.errorNoTests + "\n";
        }

        if (eErrorAssertWithoutTest != TestCase.errorAssertWithoutTest) {
            buf += "  errorAssertWithoutTest: expected " +
                eErrorAssertWithoutTest +
                ", got " + TestCase.errorAssertWithoutTest + "\n";
        }

        if (eErrorTestWithoutAssert != TestCase.errorTestWithoutAssert) {
            buf += "  errorTestWithoutAssert: expected " +
                eErrorTestWithoutAssert +
                ", got " + TestCase.errorTestWithoutAssert + "\n";
        }

        if (! "".equals(buf)) {
            internalErrorCount++;
            errorLog += "INTERNAL ERROR in " + label + "\n" + buf;
        }
    }

    static void runOneTest(String clname) {
        TestCase.reset();
        TestCase.runTestCase(pkgname + "." + clname);
        selfTestCount++;
    }

    static void run() {
        System.out.println();
        System.out.println("Starting self test.  Ignore messages below.");
        System.out.println();
        System.out.println("========================================");
        System.out.println();

        runOneTest("xyzzy.plugh");
        check("NotFound",                   0, 0,  0,  0, 1, 0, 0, 0, 0, 0, 0);

        runOneTest("SelfTest$BadConstructor");
        check("ConstructorException",       1, 0,  0,  0, 0, 1, 0, 0, 0, 0, 0);

        runOneTest("SelfTest$NotTestCase");
        check("NotTestCase",                0, 0,  0,  0, 0, 0, 1, 0, 0, 0, 0);

        runOneTest("SelfTest$TestRunThrows1");
        check("TestRunThrows1",             1, 0,  0,  0, 0, 0, 0, 1, 1, 0, 0);

        runOneTest("SelfTest$TestRunThrows2");
        check("TestRunThrows2",             1, 1,  0,  0, 0, 0, 0, 1, 0, 0, 1);

        runOneTest("SelfTest$NoTests");
        check("NoTests",                    1, 0,  0,  0, 0, 0, 0, 0, 1, 0, 0);

        runOneTest("SelfTest$AssertWithoutTest");
        check("AssertWithoutTest",          1, 0,  0,  0, 0, 0, 0, 0, 1, 1, 0);

        runOneTest("SelfTest$TestWithoutAssert1");
        check("TestWithoutAssert1",         1, 1,  0,  0, 0, 0, 0, 0, 0, 0, 1);

        runOneTest("SelfTest$TestWithoutAssert2");
        check("TestWithoutAssert2",         1, 2,  1,  0, 0, 0, 0, 0, 0, 0, 1);

        runOneTest("SelfTest$TestWithoutAssert3");
        check("TestWithoutAssert3",         1, 2,  1,  0, 0, 0, 0, 0, 0, 0, 1);

        runOneTest("SelfTest$TestAllSuccess");
        check("TestAllSuccess",             1, 1, 24,  0, 0, 0, 0, 0, 0, 0, 0);

        runOneTest("SelfTest$TestAllFail");
        check("TestAllFail",                1, 1, 24, 24, 0, 0, 0, 0, 0, 0, 0);

        runOneTest("SelfTest$TestRequestSecurityToken");
        check("TestRequestSecurityToken",       1, 1,  1,  0, 0, 0, 0, 0, 0, 0, 0);

        System.out.println();
        System.out.println("========================================");
        System.out.println();
        if (! "".equals(errorLog)) {
            System.out.print(errorLog);
        }
        System.out.println();
        System.out.println(
            "Self test run complete: " +
            selfTestCount + " test(s), " + internalErrorCount + " error(s).");
    }

    // the actual self-test cases

    static class BadConstructor extends TestCase {
        BadConstructor() {
            throw new RuntimeException("foo");
        }
        public void runTests() { }
    }

    static class NotTestCase {
    }

    static class TestRunThrows1 extends TestCase {
        public void runTests() {
            throw new RuntimeException("bar");
        }
    }

    static class TestRunThrows2 extends TestCase {
        public void runTests() {
            declare("bazz");
            throw new RuntimeException("bletch");
        }
    }

    static class NoTests extends TestCase {
        public void runTests() {
        }
    }

    static class AssertWithoutTest extends TestCase {
        public void runTests() {
            assertTrue(true);
        }
    }

    static class TestWithoutAssert1 extends TestCase {
        public void runTests() {
            declare("noAssertsFollow");
        }
    }

    static class TestWithoutAssert2 extends TestCase {
        public void runTests() {
            declare("test1");
            declare("test2");
            assertTrue(true);
        }
    }

    static class TestWithoutAssert3 extends TestCase {
        public void runTests() {
            declare("test1");
            assertTrue(true);
            declare("test2");
        }
    }

    static class TestAllSuccess extends TestCase {
        public void runTests() {
            declare("AllSuccess");
            assertEquals(new Integer(5), new Integer(5));
            assertEquals("this should succeed",
                new Integer(5), new Integer(5));
            assertEquals(null, null);
            assertEquals("this should succeed", null, null);
            Integer iobj = new Integer(7);
            assertEquals(iobj, iobj);
            assertEquals("this should succeed", iobj, iobj);
            assertEquals(17, 17);
            assertEquals("this should succeed", 18, 18);
            assertFalse(false);
            assertFalse("this should succeed", false);
            assertNotNull(this);
            assertNotNull("this should succeed", this);
            assertNotSame(new Integer(8), new Integer(8));
            assertNotSame("this should succeed",
                new Integer(9), new Integer(9));
            assertNotSame(new Integer(10), null);
            assertNotSame("this should succeed", new Integer(10), null);
            assertNotSame(null, new Integer(11));
            assertNotSame("this should succeed", null, new Integer(11));
            assertNull(null);
            assertNull("this should succeed", null);
            assertSame(this, this);
            assertSame("this should succeed", this, this);
            assertTrue(true);
            assertTrue("this should succeed", true);
        }
    }

    static class TestAllFail extends TestCase {
        public void runTests() {
            declare("AllFail");
            assertEquals(null, new Integer(1));
            assertEquals("this should fail", null, new Integer(2));
            assertEquals(new Integer(3), null);
            assertEquals("this should fail", new Integer(4), null);
            assertEquals(new Integer(5), new Integer(6));
            assertEquals("this should fail", new Integer(5), new Integer(6));
            assertEquals(1, 2);
            assertEquals("this should fail", 3, 4);
            assertFalse(true);
            assertFalse("this should fail", true);
            assertNotNull(null);
            assertNotNull("this should fail", null);
            Integer iobj = new Integer(7);
            assertNotSame(iobj, iobj);
            assertNotSame("this should fail", iobj, iobj);
            assertNotSame(null, null);
            assertNotSame("this should fail", null, null);
            assertNull(this);
            assertNull("this should fail", this);
            assertSame(new Integer(5), this);
            assertSame("this should fail", new Integer(5), this);
            assertTrue(false);
            assertTrue("this should fail", false);
            fail();
            fail("this should fail");
        }
    }

    static class TestRequestSecurityToken extends TestCase {
        /**
         * Inner class to request security token from SecurityInitializer.
         * SecurityInitializer should be able to check this inner class name.
         */
        static private class SecurityTrusted
            implements ImplicitlyTrustedClass {};

        public void runTests() {
            declare("RequestSecurityToken");
            SecurityToken tok =
                SecurityInitializer.requestToken(new SecurityTrusted());
            assertNotNull("token is null", tok);
        }
    }
}

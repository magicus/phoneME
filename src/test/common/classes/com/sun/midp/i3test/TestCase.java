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

import com.sun.midp.security.SecurityInitializer;
import com.sun.midp.security.SecurityToken;
import com.sun.midp.security.ImplicitlyTrustedClass;

/**
 * The main class for writing integrated internal interface (i3) tests.  Tests
 * should create a subclass of TestCase and implement the runTests() method.
 * This method is responsible for running all of the tests.  Tests consist of
 * a test declaration followed by one or more assertions.  The declaration is
 * a call to the declare() method, which simply establishes a name for the set
 * of assertions to follow.  The assertions are made through calls to the
 * assert*() family of methods.  The framework considers it an error if any 
 * assertions are made prior to a call to declare(), or if there is a call to 
 * declare() with no subsequent calls to assert().
 *
 * <p>The TestCase class and its assert() methods are loosely based on the
 * JUnit TestCase class.  JUnit uses reflection to find test methods.  Since
 * reflection isn't present in CLDC, we need to have a runTests() method that
 * calls them explicitly.
 *
 * <p>JUnit, in contrast to other test frameworks, doesn't return a result 
 * from each test.  Instead, each test simply makes assertions, and only 
 * assertions that fail are flagged.  The i3 tests follow the JUnit approach.
 * For this reason, there is no pass() method one might expect to see 
 * corresponding to the fail() method.
 *
 * <p>While it's not a requirement, it's conventional for the TestCase 
 * subclass to reside in the same package as the code under test.  This will 
 * allow the tests access to package-private variables and methods.
 * 
 * <p>Each of the different assert() calls has an optional
 * <code>message</code> parameter.  This message is emitted only when the 
 * assertion fails and should be written with this in mind.
 *
 * <p>For "negative tests" that require an exception to be thrown, a suggested 
 * style is to record the fact that the correct exception was thrown in a 
 * boolean variable.  This variable can then be tested using one of the 
 * assert() calls.  This is probably preferable to placing calls to fail() in 
 * the midst of the test method.  See testThree() in the example below.
 *
 * <p>The framework will catch any Throwable thrown by the tests and will log
 * a message and count the occurrence as an error.  This allows tests to be
 * written more simply, because they can rely on the framework to catch
 * everything. In support of this, the runTests() method has been defined with
 * a 'throws Throwable' clause.
 *
 * <p>A general rule is that tests should run independently of each other. 
 * That is, tests should not leave any side effects that other tests rely on 
 * or that could cause them to fail.  Typically, tests should create fresh 
 * objects for testing instead of reusing objects from other tests.  If a test
 * has any external side effects, such as creating threads or opening files, 
 * the test should be coded with a 'finally' clause that attempts to clean up 
 * after itself.  See testFour() in the example below.
 *
 * <p>Example:
 * <pre><code>
 * import com.sun.midp.i3test;
 * package java.lang;
 *
 * public class SampleTest extends TestCase {
 *     void testOne() {
 *         int i = 1 + 1;
 *         assertEquals("integer addition failed", 2, i);
 *     }
 * 
 *     void testTwo() {
 *         Object x = new Object();
 *         assertNotNull("Object constructor returned null", x);
 *     }
 * 
 *     void testThree() {
 *         StringBuffer sb;
 *         boolean caught = false;
 *         try {
 *             sb = new StringBuffer(-1);
 *         } catch (NegativeArraySizeException e) {
 *             caught = true;
 *         }
 *         assertTrue("no exception thrown", caught);
 *     }
 *
 *     void testFour() {
 *         Thread thr = new Thread(...).start();
 *         try {
 *             // ...
 *         } finally {
 *             thr.interrupt();
 *         }
 *     }
 * 
 *     public void runTests() {
 *         declare("testOne");
 *         testOne();
 *         declare("testTwo");
 *         testTwo();
 *         declare("testThree");
 *         testThree();
 *         declare("testFour");
 *         testFour();
 *     }
 * }
 * </code></pre>
 */
public abstract class TestCase {

    // the lone constructor

    /**
     * Constructs a TestCase.  Since TestCase is an abstract class, this 
     * constructor is called only at the time a TestCase subclass is being 
     * constructed.
     */
    public TestCase() {
        if (verbose) {
            p("## TestCase " + this.getClass().getName());
        }
        currentTestCase = this;
        currentTest = null;
        totalCases++;
        currentNumAsserts = 0;
    }

    // ==================== public methods ====================

    /**
     * Runs all the tests for this TestCase.  This is an abstract method that
     * must be implemented by the subclass.  The implementation of this method
     * should run all of the tests provided in this TestCase.  This method can
     * also be used to initialize and tear down any state that might need to
     * be shared among all the tests.
     * 
     * <p>A suggested organization is to have runTests() alternate calling
     * declare() and an internal method that implements the test.  One could
     * write the declare() method as part of the internal test method, but
     * consolidating all calls in runTests() makes it a little easier to
     * ensure that each test has its own call to declare().  See the example
     * in the class documentation.
     */
    public abstract void runTests() throws Throwable;

    /**
     * Declares that the set of assertions that follow this call belong to a
     * test named <code>testName</code>.  The framework considers it an error
     * if no calls to any of the assert() methods follow a call to declare().
     *
     * @param testName the name of the test
     */
    public void declare(String testName) {
        if (testName == null) {
            throw new NullPointerException("test name is null");
        }

        if (verbose) {
            p("## test " + testName);
        }

        if (currentNumAsserts == 0 && currentTest != null) {
            p("ERROR no asserts in test case " + currentTestCaseName() +
                " test " + currentTest);
            errorTestWithoutAssert++;
        }

        currentTest = testName;
        totalTests++;
        currentNumAsserts = 0;
    }

    /**
     * Tests the assertion that the boolean <code>condition</code> is true.
     * If the condition is true, this method simply updates some statistics
     * and returns.  If the condition is false, the failure is noted and the
     * message is emitted, along with an indication of the current TestCase
     * and the name of the test currently being run.  The
     * <code>message</code> string should be phrased in a way that makes sense 
     * when the test fails.  See the example in the class documentation.
     *
     * @param message the message to be emitted if the assertion fails
     * @param condition the condition asserted to be true
     */
    public void assertTrue(String message, boolean condition) {
        if (currentTest == null) {
            p("ERROR assert \"" + message + "\" not part of any test");
            errorAssertWithoutTest++;
            return;
        }

        currentNumAsserts++;
        totalAsserts++;

        if (verbose) {
            p("## " + totalAsserts + ": " + message + (condition?" OK":" FAILURE"));
        }

        if (!condition) {
            totalFailures++;
            String m = "FAIL " + currentTestCaseName();
            if (currentTest != null) {
                m += "#" + currentTest;
            }
            if (message != null) {
                m += ": " + message;
            }
            p(m);
        }
    }

    /**
     * Asserts that <code>condition</code> is true.
     *
     * @param condition the condition to be tested
     */
    public void assertTrue(boolean condition) {
        assertTrue(null, condition);
    }
    
    /**
     * Asserts that two objects are equal according to the equals() method.
     *
     * @param expected an object containing the expected value
     * @param actual an object containing the actual value
     */
    public void assertEquals(Object expected, Object actual) {
        assertEquals(null, expected, actual);
    }

    /**
     * Asserts that two objects are equal according to the equals() method.
     * Two null references are considered to be equal.
     *
     * @param message the message to be emitted if the assertion fails
     * @param expected an object containing the expected value
     * @param actual an object containing the actual value
     */
    public void assertEquals(String message, Object expected, Object actual) {
	boolean isequal = (expected == actual);
        
        // If references aren't identical, call equals() only when
        // both references are non-null.

	if (!isequal && expected != null && actual != null) {
	    isequal = expected.equals(actual);
	}
	assertTrue(message, isequal);
	if (!isequal) {
	    p("  expected: " + expected + "; actual: " + actual);
	}
    }

    /**
     * Asserts that two integer values are equal.
     *
     * @param expected the expected value
     * @param actual the actual value
     */
    public void assertEquals(int expected, int actual) {
        assertEquals(null, expected, actual);
    }

    /**
     * Asserts that two integer values are equal.
     *
     * @param message the message to be emitted if the assertion fails
     * @param expected the expected value
     * @param actual the actual value
     */
    public void assertEquals(String message, int expected, int actual) {
        assertTrue(message, expected == actual);
	if (expected != actual) {
	    p("  expected: " + expected + "; actual: " + actual);
	}
    }

    /**
     * Asserts that <code>condition</code> is false.
     * 
     * @param condition the condition asserted to be false
     */
    public void assertFalse(boolean condition) {
        assertTrue(null, !condition);
    }

    /**
     * Asserts that <code>condition</code> is false.
     * 
     * @param message the message to be emitted if the assertion fails
     * @param condition the condition asserted to be false
     */
    public void assertFalse(String message, boolean condition) {
        assertTrue(message, !condition);
    }

    /**
     * Asserts that the object reference is not null.
     *
     * @param object the reference to be tested
     */
    public void assertNotNull(Object object) {
        assertTrue(null, object != null);
    }

    /**
     * Asserts that the object reference is not null.
     *
     * @param message the message to be emitted if the assertion fails
     * @param object the reference to be tested
     */
    public void assertNotNull(String message, Object object) {
        assertTrue(message, object != null);
    }

    /**
     * Asserts that two references do not point to the same object,
     * using the == operator.
     *
     * @param expected a reference to the expected object
     * @param actual a reference to the actual object
     */
    public void assertNotSame(Object expected, Object actual) {
        assertNotSame(null, expected, actual);
    }

    /**
     * Asserts that two references do not point to the same object,
     * using the == operator.
     *
     * @param message the message to be emitted if the assertion fails
     * @param expected a reference to the expected object
     * @param actual a reference to the actual object
     */
    public void assertNotSame(String message, Object expected, Object actual) {
        assertTrue(message, expected != actual);
	if (expected == actual) {
	    p("  expected: " + expected + "; actual: " + actual);
	}
    }

    /**
     * Asserts that the object reference is null.
     *
     * @param object the reference to be tested
     */
    public void assertNull(Object object) {
        assertNull(null, object);
    }

    /**
     * Asserts that the object reference is null.
     *
     * @param message the message to be emitted if the assertion fails
     * @param object the reference to be tested
     */
    public void assertNull(String message, Object object) {
        assertTrue(message, object == null);
	if (object != null) {
	    p("  actual: " + object);
	}
    }

    /**
     * Asserts that two references point to the same object, using the
     * == operator.
     *
     * @param expected a reference to the expected object
     * @param actual a reference to the actual object
     */
    public void assertSame(Object expected, Object actual) {
        assertSame(null, expected, actual);
    }

    /**
     * Asserts that two references point to the same object, using the
     * == operator.
     *
     * @param message the message to be emitted if the assertion fails
     * @param expected a reference to the expected object
     * @param actual a reference to the actual object
     */
    public void assertSame(String message, Object expected, Object actual) {
        assertTrue(message, expected == actual);
	if (expected != actual) {
	    p("  expected: " + expected + "; actual: " + actual);
	}
    }

    /**
     * Signals an unconditional assertion failure.
     */
    public void fail() {
        assertTrue(null, false);
    }

    /**
     * Signals an unconditional assertion failure.
     *
     * @param message the message to be emitted, explaining the failure
     */
    public void fail(String message) {
        assertTrue(message, false);
    }

    /**
     * Gets the system's internal security token. This is useful for testing 
     * sensitive interfaces that require a security token parameter.  This 
     * returns a valid security token only when called within the context of a 
     * call to runTests() inside a TestCase.  At other times it will throw a 
     * SecurityException.
     *
     * @return the internal security token
     * @throws SecurityException if not called from within runTests()
     */
    protected SecurityToken getSecurityToken() {
        if (tokenEnabled) {
            return internalSecurityToken;
        } else {
            throw new SecurityException();
        }
    }

    // ==================== static variables ====================

    static boolean verbose = false;

    static private class SecurityTrusted
        implements ImplicitlyTrustedClass {};
    private static SecurityToken internalSecurityToken =
        SecurityInitializer.requestToken(new SecurityTrusted());

    static boolean tokenEnabled = false;

    // general statistics

    static int totalCases = 0;    // total number of test cases
    static int totalTests = 0;    // total number of tests declared
    static int totalAsserts = 0;  // total number of asserts called
    static int totalFailures = 0; // total number of assertion failures

    // error counts

    static int errorClassNotFound = 0;
    static int errorConstructorException = 0;
    static int errorNotTestCase = 0;
    static int errorTestRunThrows = 0;
    static int errorNoTests = 0;
    static int errorAssertWithoutTest = 0;
    static int errorTestWithoutAssert = 0;

    // state information about the currently running test

    static TestCase currentTestCase;
    static String currentTest;
    static int currentNumAsserts;

    // ==================== static implementation ====================

    /**
     * Run the named test case.
     * @param testCaseName the class name of the test case
     */
    static void runTestCase(String testCaseName) {
        Class clazz;
        Object obj;
        TestCase tc;

        if (verbose) {
            System.out.println("## running test case " + testCaseName);
        }

        try {
            clazz = Class.forName(testCaseName);
        } catch (ClassNotFoundException cnfe) {
            System.out.println(
                "ERROR test class " + testCaseName + " not found");
            errorClassNotFound++;
            return;
        }

        try {
            obj = clazz.newInstance();
        } catch (Throwable t) {
            System.out.println("ERROR test class " + testCaseName +
                " constructor threw " + t);
            errorConstructorException++;
            return;
        }

        try {
            tc = (TestCase)obj;
        } catch (ClassCastException cce) {
            System.out.println("ERROR test class " + testCaseName +
                " not a subclass of TestCase");
            errorNotTestCase++;
            return;
        }

        try {
            tokenEnabled = true;
            tc.runTests();
        } catch (Throwable t) {
            String m = "ERROR " + currentTestCaseName();
            if (currentTest != null) {
                m += "#" + currentTest;
            }
            m += " threw " + t;
            p(m);
            t.printStackTrace();
            errorTestRunThrows++;
        }

        tokenEnabled = false;
        cleanup();
    }

    /**
     * Return the name of the current test case.
     * @return the name of the class for the current test case.
     */
    static String currentTestCaseName() {
        return currentTestCase.getClass().getName();
    }

    /**
     * Cleanup after running a test.
     */
    static void cleanup() {
        if (currentTest == null) {
            p("ERROR " + currentTestCaseName() + " has no tests");
            errorNoTests++;
        } else if (currentNumAsserts == 0) {
            p("ERROR no asserts in test case " + currentTestCaseName() +
                    " test " + currentTest);
            errorTestWithoutAssert++;
        }

        currentTestCase = null;
        currentTest = null;
    }

    /**
     * Set the verbose output flag.
     * @param v true to enable verbose output
     */ 
    static void setVerbose(boolean v) {
        verbose = v;
    }

    /**
     * Print the string.
     * @param s the string to print on a new line.
     */
    static void p(String s) {
        System.out.println(s);
    }

    /**
     * Print the number of errors with the category.
     * @param nerr the number of errors in the category
     * @param errstr the description of the errors
     */
    static void perror(int nerr, String errstr) {
        if (nerr != 0) {
            System.out.println("  " + nerr + " " + errstr);
        }
    }

    // print out a report of all statistics

    static void report() {
        System.out.print("Test run complete: ");
        System.out.print(totalCases
            + (totalCases == 1 ? " testcase " : " testcases "));
        System.out.print(totalTests
            + (totalTests == 1 ? " test " : " tests "));
        System.out.println(totalAsserts
            + (totalAsserts == 1 ? " assertion" : " assertions"));

        if (totalFailures != 0) {
            System.out.println(totalFailures
               + (totalFailures == 1 ? " FAILURE!!!" : " FAILURES!!!"));
        }

        int totalErrors = 
            errorClassNotFound +
            errorNotTestCase +
            errorConstructorException +
            errorTestRunThrows +
            errorNoTests +
            errorAssertWithoutTest +
            errorTestWithoutAssert;

        if (totalErrors != 0) {
            System.out.println(totalErrors
                + (totalErrors == 1 ? " ERROR!!!" : " ERRORS!!!"));
            perror(errorClassNotFound, "test class not found");
            perror(errorConstructorException, "constructor threw exception");
            perror(errorNotTestCase, "class not subclass of TestCase");
            perror(errorTestRunThrows, "test run threw exception");
            perror(errorNoTests, "no tests declared");
            perror(errorAssertWithoutTest, "asserts outside of any test");
            perror(errorTestWithoutAssert, "tests with no asserts");
        }
    }

    /**
     * Reset the counters of all errors and exceptions.
     */
    static void reset() {
        totalCases = 0;
        totalTests = 0;
        totalAsserts = 0;
        totalFailures = 0;
        currentNumAsserts = 0;

        errorClassNotFound = 0;
        errorConstructorException = 0;
        errorNotTestCase = 0;
        errorTestRunThrows = 0;
        errorNoTests = 0;
        errorAssertWithoutTest = 0;
        errorTestWithoutAssert = 0;
    }

    /**
     * If in verbose mode, print information.
     */
    public void info(String s) {
        if (verbose) {
            p(".# "+s);
        }
    }

    /**
     * Reads the verbose flag.
     * @return true if in verbose mode
     */
    protected boolean getVerbose() {
        return verbose;
    }
}

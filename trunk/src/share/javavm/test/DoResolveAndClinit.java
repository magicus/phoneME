/*
 * @(#)DoResolveAndClinit.java	1.8 06/10/10
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
 *
 */

/*
    NOTE: This module compiles some methods which are specifically designed
    to operate on some bytecodes which require runtime class initializations,
    and exercises lazy resolution.  This module will test each of those cases
    to see if the desired behavior is executed.

    To run, use:
    cvm -Xjit:compile=none -cp testclasses.zip DoResolveAndClinit

    NOTE: compile=none is important because this testsuite needs to control
    which and when methods get compiled.  It also uses the CompilerTest class
    in testclasses.zip to do the controlled JIT compilation.
*/
/*
    Cases to test:
    =============
    opc_checkcast:
    opc_instanceof:
    opc_new:
    opc_anewarray:
    opc_multianewarray:
    opc_invokestatic:
    opc_invokevirtual:
    opc_invokespecial:
    opc_invokeinterface:
    opc_putstatic:
    opc_getstatic:
    opc_putfield:
    opc_getfield:
*/

public class DoResolveAndClinit
{
    static int totalTests = 0;
    static int totalFailures = 0;

    static String usageStr[] = {
	"=========================================================================",
	"TestSuite: DoResolveAndClinit",
	"",
	"To run, use:",
	"cvm -Xjit:compile=none -cp testclasses.zip DoResolveAndClinit",
	"",
	"NOTE: compile=none is important because this testsuite needs to control",
	"which and when methods get compiled.  It also uses the CompilerTest class",
	"in testclasses.zip to do the controlled JIT compilation.",
	"",
	"If run successfully, there should be 0 failures reported at the end of",
	"the run.",
	"=========================================================================",
	"",
    };

    static void printUsage() {
	for (int i = 0; i < usageStr.length; i++) {
	    System.out.println(usageStr[i]);
	}
    }

    public static void main(String[] args) {

	printUsage();

        totalTests = 0;
        totalFailures = 0;

        // Do setup:
        DoCheckCast.doSetup();
        DoInstanceOf.doSetup();
        DoNew.doSetup();
        DoANewArray.doSetup();
        DoMultiANewArray.doSetup();
        DoInvokeStatic.doSetup();
        DoInvokeVirtual.doSetup();
//        DoInvokeSpecial.doSetup();
        DoInvokeInterface.doSetup();
        DoPutStatic.doSetup();
        DoGetStatic.doSetup();
        DoPutField.doSetup();
        DoGetField.doSetup();

        // Do tests:
        DoCheckCast.doTest();
        DoInstanceOf.doTest();
        DoNew.doTest();
        DoANewArray.doTest();
        DoMultiANewArray.doTest();
        DoInvokeStatic.doTest();
        DoInvokeVirtual.doTest();
//        DoInvokeSpecial.doTest();
        DoInvokeInterface.doTest();
        DoPutStatic.doTest();
        DoGetStatic.doTest();
        DoPutField.doTest();
        DoGetField.doTest();

        // Report the total number of failures:
        System.out.println("Tests ran: " + totalTests + ", failures: " +
                           totalFailures);

    }

    public static void reportPassIf(String testName, boolean success) {
        System.out.println((success ? "PASSED" : "FAILED") + " Test " +
                           testName);
        totalTests++;
        if (!success) {
            totalFailures++;
        }
    }

    public static void reportPassIf(String testName, boolean actual, boolean expected) {
	boolean success = (actual == expected);
        System.out.println((success ? "PASSED" : "FAILED") + " Test " +
                           testName);
        totalTests++;
        if (!success) {
	    System.out.println("  expecting = " + expected +
			       ", actual = " + actual);
            totalFailures++;
        }
    }

    public static void reportPassIf(String testName, byte actual, byte expected) {
	boolean success = (actual == expected);
        System.out.println((success ? "PASSED" : "FAILED") + " Test " +
                           testName);
        totalTests++;
        if (!success) {
	    System.out.println("  expecting = " + expected +
			       ", actual = " + actual);
            totalFailures++;
        }
    }

    public static void reportPassIf(String testName, char actual, char expected) {
	boolean success = (actual == expected);
        System.out.println((success ? "PASSED" : "FAILED") + " Test " +
                           testName);
        totalTests++;
        if (!success) {
	    System.out.println("  expecting = " + expected +
			       ", actual = " + actual);
            totalFailures++;
        }
    }

    public static void reportPassIf(String testName, short actual, short expected) {
	boolean success = (actual == expected);
        System.out.println((success ? "PASSED" : "FAILED") + " Test " +
                           testName);
        totalTests++;
        if (!success) {
	    System.out.println("  expecting = " + expected +
			       ", actual = " + actual);
            totalFailures++;
        }
    }

    public static void reportPassIf(String testName, int actual, int expected) {
	boolean success = (actual == expected);
        System.out.println((success ? "PASSED" : "FAILED") + " Test " +
                           testName);
        totalTests++;
        if (!success) {
	    System.out.println("  expecting = " + expected +
			       ", actual = " + actual);
            totalFailures++;
        }
    }

    public static void reportPassIf(String testName, float actual, float expected) {
	boolean success = (actual == expected);
        System.out.println((success ? "PASSED" : "FAILED") + " Test " +
                           testName);
        totalTests++;
        if (!success) {
	    System.out.println("  expecting = " + expected +
			       ", actual = " + actual);
            totalFailures++;
        }
    }

    public static void reportPassIf(String testName, long actual, long expected) {
	boolean success = (actual == expected);
        System.out.println((success ? "PASSED" : "FAILED") + " Test " +
                           testName);
        totalTests++;
        if (!success) {
	    System.out.println("  expecting = " + expected +
			       ", actual = " + actual);
            totalFailures++;
        }
    }

    public static void reportPassIf(String testName, double actual, double expected) {
	boolean success = (actual == expected);
        System.out.println((success ? "PASSED" : "FAILED") + " Test " +
                           testName);
        totalTests++;
        if (!success) {
	    System.out.println("  expecting = " + expected +
			       ", actual = " + actual);
            totalFailures++;
        }
    }

    public static void reportPassIf(String testName, Object actual, Object expected) {
	boolean success = (actual == expected);
        System.out.println((success ? "PASSED" : "FAILED") + " Test " +
                           testName);
        totalTests++;
        if (!success) {
	    System.out.println("  expecting = " + expected +
			       ", actual = " + actual);
            totalFailures++;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Testing opcode checkcast:
// Note: Resolves to a CB pointer.

class DoCheckCastResolved {}
class DoCheckCastUnresolved {}
class DoCheckCastUnresolvedNull {}
class DoCheckCastResolved2 extends DoCheckCastResolved {}
class DoCheckCastUnresolved2 extends DoCheckCastUnresolved {}

class DoCheckCast
{
    static final String[] compileItems = {
        "DoCheckCast.doResolved(Ljava/lang/Object;)LDoCheckCastResolved;",
        "DoCheckCast.doUnresolved(Ljava/lang/Object;)LDoCheckCastUnresolved;",
        "DoCheckCast.doUnresolvedNull(Ljava/lang/Object;)LDoCheckCastUnresolved;",
    };

    public static void doSetup() {
        // Setup initial conditions:
        DoCheckCastResolved r = new DoCheckCastResolved();  // Resolve it.

        // Do compilation:
        CompilerTest.main(compileItems);
    }

    public static void doTest() {
        Object o = new Object();
        boolean success;

        // 1. Resolved CheckCast against a NULL object.
        try {
            doResolved(null);
            success = true;  // Should pass this checkcast.
        } catch (ClassCastException cce) {
            success = false;
        }
        DoResolveAndClinit.reportPassIf("CheckCastResolved(NULL)", success);

        // 2. Resolved CheckCast against an invalid object.
        try {
            doResolved(o);
            success = false;
        } catch (ClassCastException cce) {
            success = true; // Should fail this checkcast.
        }
        DoResolveAndClinit.reportPassIf("CheckCastResolved(Invalid)", success);

        // 3. Resolved CheckCast against a valid object.
        try {
            doResolved(new DoCheckCastResolved());
            success = true; // Should pass this checkcast.
        } catch (ClassCastException cce) {
            success = false;
        }
        DoResolveAndClinit.reportPassIf("CheckCastResolved(Valid)", success);

        // 4. Resolved CheckCast against a valid object w/ guess.
        //    Guess initialized in 3.
        try {
            doResolved(new DoCheckCastResolved());
            success = true; // Should pass this checkcast.
        } catch (ClassCastException cce) {
            success = false;
        }
        DoResolveAndClinit.reportPassIf(
            "CheckCastResolved(Valid + Guess)", success);

        // 5. Resolved CheckCast against a valid sub-object w/o guess.
        //    Guess initialized in 3.
        try {
            doResolved(new DoCheckCastResolved2());
            success = true; // Should pass this checkcast.
        } catch (ClassCastException cce) {
            success = false;
        }
        DoResolveAndClinit.reportPassIf(
            "CheckCastResolved(ValidSubClass)", success);

        // 6. Resolved CheckCast against a valid sub-object w/ guess.
        //    Guess initialized in 5.
        try {
            doResolved(new DoCheckCastResolved2());
            success = true; // Should pass this checkcast.
        } catch (ClassCastException cce) {
            success = false;
        }
        DoResolveAndClinit.reportPassIf(
            "CheckCastResolved(ValidSubClass + Guess)", success);

        // 7. Unresolved CheckCast against a NULL object.
        //    Not yet resolved ... 1st use.
        try {
            doUnresolvedNull(null);
            success = true;
        } catch (ClassCastException cce) {
            success = false;
        }
        DoResolveAndClinit.reportPassIf(
            "CheckCastUnresolvedNull(NULL, unresolved)", success);

        // 8. Unresolved CheckCast against an invalid object.
        //    Resolved by step 7.
        try {
            doUnresolvedNull(o);
            success = false;
        } catch (ClassCastException cce) {
            success = true;
        }
        DoResolveAndClinit.reportPassIf(
            "CheckCastUnresolvedNull(Invalid, unresolved)", success);

        // 9. Unresolved CheckCast against an invalid object.
        //    Not yet resolved because NULL is check inline in step 7.
        try {
            doUnresolved(o);
            success = false;
        } catch (ClassCastException cce) {
            success = true;
        }
        DoResolveAndClinit.reportPassIf(
            "CheckCastUnresolved(Invalid, unresolved)", success);

        // 10. Unresolved CheckCast against a NULL object.
        //     Resolved by step 9.
        try {
            doUnresolved(null);
            success = true;
        } catch (ClassCastException cce) {
            success = false;
        }
        DoResolveAndClinit.reportPassIf(
            "CheckCastUnresolved(NULL, resolved)", success);

        // 11. Unresolved CheckCast against an invalid object.
        //     Resolved by step 9.
        try {
            doUnresolved(o);
            success = false;
        } catch (ClassCastException cce) {
            success = true;
        }
        DoResolveAndClinit.reportPassIf(
            "CheckCastUnresolved(Invalid, resolved)", success);

        // 12. Unresolved CheckCast against a valid object.
        //     Resolved by step 9.  Sets Guess.
        try {
            doUnresolved(new DoCheckCastUnresolved());
            success = true;
        } catch (ClassCastException cce) {
            success = false;
        }
        DoResolveAndClinit.reportPassIf(
            "CheckCastUnresolved(Valid, resolved)", success);

        // 13. Unresolved CheckCast against a valid object by guess.
        //     Resolved by step 9.  Guess set in step 12.
        try {
            doUnresolved(new DoCheckCastUnresolved());
            success = true;
        } catch (ClassCastException cce) {
            success = false;
        }
        DoResolveAndClinit.reportPassIf(
            "CheckCastUnresolved(Valid + Guess, resolved)", success);

        // 14. Unresolved CheckCast against a valid sub-class.
        //     Resolved by step 9.  Guess set in step 12.
        try {
            doUnresolved(new DoCheckCastUnresolved2());
            success = true;
        } catch (ClassCastException cce) {
            success = false;
        }
        DoResolveAndClinit.reportPassIf(
            "CheckCastUnresolved(ValidSubClass, resolved)", success);

        // 15. Unresolved CheckCast against a valid sub-class.
        //     Resolved by step 9.  Guess set in step 14.
        try {
            doUnresolved(new DoCheckCastUnresolved2());
            success = true;
        } catch (ClassCastException cce) {
            success = false;
        }
        DoResolveAndClinit.reportPassIf(
            "CheckCastUnresolved(ValidSubClass + Guess, resolved)", success);
    }

    static DoCheckCastResolved doResolved(Object o)
        throws ClassCastException {
        return (DoCheckCastResolved) o;
    }
    static DoCheckCastUnresolved doUnresolved(Object o)
        throws ClassCastException {
        return (DoCheckCastUnresolved) o;
    }
    static DoCheckCastUnresolvedNull doUnresolvedNull(Object o)
        throws ClassCastException {
        return (DoCheckCastUnresolvedNull) o;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Testing opcode instanceof:
// Note: Resolves to a CB pointer.

class DoInstanceOfResolved {}
class DoInstanceOfUnresolved {}
class DoInstanceOfUnresolvedNull {}
class DoInstanceOfResolved2 extends DoInstanceOfResolved {}
class DoInstanceOfUnresolved2 extends DoInstanceOfUnresolved {}

class DoInstanceOf
{

    static final String[] compileItems = {
        "DoInstanceOf.doResolved(Ljava/lang/Object;)Z",
        "DoInstanceOf.doUnresolved(Ljava/lang/Object;)Z",
        "DoInstanceOf.doUnresolvedNull(Ljava/lang/Object;)Z",
    };

    public static void doSetup() {
        // Setup initial conditions:
        DoInstanceOfResolved r = new DoInstanceOfResolved();  // Resolve it.

        // Do compilation:
        CompilerTest.main(compileItems);
    }

    public static void doTest() {
        Object o = new Object();
        boolean success;

        // 1. Resolved InstanceOf against a NULL object.
        success = doResolved(null);
        DoResolveAndClinit.reportPassIf(
	    "InstanceOfResolved(NULL)", success, false);

        // 2. Resolved InstanceOf against an invalid object.
        success = doResolved(o);
        DoResolveAndClinit.reportPassIf(
	    "InstanceOfResolved(Invalid)", success, false);

        // 3. Resolved InstanceOf against a valid object.
        success = doResolved(new DoInstanceOfResolved());
        DoResolveAndClinit.reportPassIf(
            "InstanceOfResolved(Valid)", success, true);

        // 4. Resolved InstanceOf against a valid object w/ guess.
        //    Guess initialized in 3.
        success = doResolved(new DoInstanceOfResolved());
        DoResolveAndClinit.reportPassIf(
            "InstanceOfResolved(Valid + Guess)", success, true);

        // 5. Resolved InstanceOf against a valid sub-object w/o guess.
        //    Guess initialized in 3.
        success = doResolved(new DoInstanceOfResolved2());
        DoResolveAndClinit.reportPassIf(
            "InstanceOfResolved(ValidSubClass)", success, true);

        // 6. Resolved InstanceOf against a valid sub-object w/ guess.
        //    Guess initialized in 5.
        success = doResolved(new DoInstanceOfResolved2());
        DoResolveAndClinit.reportPassIf(
            "InstanceOfResolved(ValidSubClass + Guess)", success, true);

        // 7. Unresolved InstanceOf against a NULL object.
        //    Not yet resolved ... 1st use.
        success = doUnresolvedNull(null);
        DoResolveAndClinit.reportPassIf(
            "InstanceOfUnresolvedNull(NULL, unresolved)", success, false);

        // 8. Unresolved InstanceOf against an invalid object.
        //    Not yet resolved.
        success = doUnresolved(o);
        DoResolveAndClinit.reportPassIf(
            "InstanceOfUnresolved(Invalid, unresolved)", success, false);

        // 9. Unresolved InstanceOf against a NULL object.
        //    Resolved by step 8.
        success = doUnresolved(null);
        DoResolveAndClinit.reportPassIf(
            "InstanceOfUnresolved(NULL, resolved)", success, false) ;

        // 10. Unresolved InstanceOf against an invalid object.
        //     Resolved by step 8.
        success = doUnresolved(o);
        DoResolveAndClinit.reportPassIf(
            "InstanceOfUnresolved(Invalid, resolved)", success, false);

        // 11. Unresolved InstanceOf against a valid object.
        //     Resolved by step 8.  Sets Guess.
        success = doUnresolved(new DoInstanceOfUnresolved());
        DoResolveAndClinit.reportPassIf(
            "InstanceOfUnresolved(Valid, resolved)", success, true);

        // 12. Unresolved InstanceOf against a valid object by guess.
        //     Resolved by step 8.  Guess set in step 11.
        success = doUnresolved(new DoInstanceOfUnresolved());
        DoResolveAndClinit.reportPassIf(
            "InstanceOfUnresolved(Valid + Guess, resolved)", success, true);

        // 13. Unresolved InstanceOf against a valid sub-class.
        //     Resolved by step 8.  Guess set in step 11.
        success = doUnresolved(new DoInstanceOfUnresolved2());
        DoResolveAndClinit.reportPassIf(
            "InstanceOfUnresolved(ValidSubClass, resolved)", success, true);

        // 14. Unresolved InstanceOf against a valid sub-class.
        //     Resolved by step 8.  Guess set in step 13.
        success = doUnresolved(new DoInstanceOfUnresolved2());
        DoResolveAndClinit.reportPassIf(
            "InstanceOfUnresolved(ValidSubClass + Guess, resolved)",
            success, true);
    }

    static boolean doResolved(Object o) {
        return o instanceof DoInstanceOfResolved;
    }
    static boolean doUnresolved(Object o) {
        return o instanceof DoInstanceOfUnresolved;
    }
    static boolean doUnresolvedNull(Object o) {
        return o instanceof DoInstanceOfUnresolvedNull;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Testing opcode new:
// Note: Resolves to a CB pointer.

class DoNewResolved {}
class DoNewResolvedClinit {
    static Object so;
    static {
        System.out.println("\tDoNewResolvedClinit.<clinit>()");
        so = new Object();
    }
}
class DoNewUnresolved {}
class DoNewUnresolvedClinit {
    static Object so;
    static {
        System.out.println("\tDoNewUnresolvedClinit.<clinit>()");
        so = new Object();
    }
    DoNewUnresolvedClinit() {
        System.out.println("\tDoNewUnresolvedClinit.<init>()");
    }
}
class DoNewUnresolvedClinit1 {
    public static Object so;
    static {
        DoNew.DoNewUnresolvedClinit1_inited = true;
        System.out.println("\tDoNewUnresolvedClinit1.<clinit>()");
        so = new Object();
    }
}

class DoNewInitUnresolvedClinit1 {
    public static void doUnresolvedClinit1() {
        Object so = DoNewUnresolvedClinit1.so;
    }
};

class DoNewUnresolvedClinit2 {
    static Object so;
    static {
        System.out.println("\tDoNewUnresolvedClinit2.<clinit>()");
        so = new Object();
    }
}

class DoNewUnresolvedClinit12a {
    static Object so;
    static {
        System.out.println("\tDoNewUnresolvedClinit12a.<clinit>()");
        so = new Object();
    }
}
class DoNewUnresolvedClinit12b {
    static Object so;
    DoNewUnresolvedClinit12a d1;
    static {
        System.out.println("\tDoNewUnresolvedClinit12b.<clinit>()");
        so = new Object();
    }
    public DoNewUnresolvedClinit12b(DoNewUnresolvedClinit12a d1) {
        this.d1 = d1;
    }
}

class DoNew
{
    static final String[] compileItems = {
        "DoNew.doResolved()LDoNewResolved;",
        "DoNew.doUnresolved()LDoNewUnresolved;",
        "DoNew.doResolvedClinit()LDoNewResolvedClinit;",
        "DoNew.doUnresolvedClinit()LDoNewUnresolvedClinit;",
        "DoNew.doUnresolvedClinit1()LDoNewUnresolvedClinit1;",
        "DoNew.doUnresolvedClinit2()LDoNewUnresolvedClinit2;",
        "DoNew.doUnresolvedClinit12()LDoNewUnresolvedClinit12b;",
    };

    public static boolean DoNewUnresolvedClinit1_inited = false;

    static DoNewResolvedClinit getNullObject() { return null; }

    public static void doSetup() {
        // Setup initial conditions:
        Object o = new Object();
        DoNewResolved r = new DoNewResolved();  // Resolve it.
        DoNewResolvedClinit rc = getNullObject();

        // Resolve ResolvedClinit without calling its clinit:
        System.out.println("\tDoNew: Forcing resolution of ResolvedClinit: " +
                           ((rc instanceof DoNewResolvedClinit) == false));

        DoNewInitUnresolvedClinit1.doUnresolvedClinit1();
        if (!DoNewUnresolvedClinit1_inited) {
            System.out.println("\tDoNew: ERROR: Unable to run clinit of " +
                               "DoNewUnresolvedClinit1");
        }

        // Do compilation:
        CompilerTest.main(compileItems);
    }

    public static void doTest() {

        // 1. Resolved New.
        {
            DoNewResolved r = doResolved();
            DoResolveAndClinit.reportPassIf(
                "NewResolved()", ((r != null) && (r instanceof DoNewResolved)));
        }

        // 2. Unresolved New.  Will resolve.
        {
            DoNewUnresolved u = doUnresolved();
            DoResolveAndClinit.reportPassIf(
                "NewUnresolved(unresolved)",
                ((u != null) && (u instanceof DoNewUnresolved)));
        }

        // 3. Unresolved New.  Resolved in step 2.
        {
            DoNewUnresolved u = doUnresolved();
            DoResolveAndClinit.reportPassIf(
                "NewUnresolved(resolved)",
                ((u != null) && (u instanceof DoNewUnresolved)));
        }

        // 4. Resolved New w/ Clinit.
        {
            DoNewResolvedClinit r = doResolvedClinit();
            DoResolveAndClinit.reportPassIf(
                "NewResolvedClinit()",
                ((r != null) && (r instanceof DoNewResolvedClinit)));
        }

        // 5. Unresolved New w/ Clinit.  Will resolve & clinit.
        {
            DoNewUnresolvedClinit u = doUnresolvedClinit();
            DoResolveAndClinit.reportPassIf(
                "NewUnresolvedClinit(unresolved, !clinit)",
                ((u != null) && (u instanceof DoNewUnresolvedClinit)));
        }

        // 6. Unresolved New Resolved & Clinited.
        {
            DoNewUnresolvedClinit u = doUnresolvedClinit();
            DoResolveAndClinit.reportPassIf(
                "NewUnresolvedClinit(resolved, clinit)",
                ((u != null) && (u instanceof DoNewUnresolvedClinit)));
        }

        // 7. Unresolved New !Resolved & Clinited.  Will attempt clinit but
        //    find it unnecessary.
        {
            DoNewUnresolvedClinit1 u = doUnresolvedClinit1();
            DoResolveAndClinit.reportPassIf(
                "NewUnresolvedClinit1(unresolved, clinit)",
                ((u != null) && (u instanceof DoNewUnresolvedClinit1)));
        }

        // 8. Unresolved New Resolved & Clinited.  Will attempt resolution &
        //    clinit but find it unnecessary.
        {
            Object o = new Object();
            // Resolve UnresolvedClinit1 without calling its clinit:
            System.out.println(
                "\tDoNew: Forcing resolution of UnresolvedClinit2: " +
                ((o instanceof DoNewUnresolvedClinit2) == false));

            DoNewUnresolvedClinit2 u = doUnresolvedClinit2();
            DoResolveAndClinit.reportPassIf(
                "NewUnresolvedClinit2(unresolved, clinit)",
                ((u != null) && (u instanceof DoNewUnresolvedClinit2)));
        }

        // 9. Unresolved New of 2 classes.  Will resolve both.
        {
            DoNewUnresolvedClinit12b u = doUnresolvedClinit12();
            DoResolveAndClinit.reportPassIf(
                "NewUnresolvedClinit12(unresolved x 2, clinit x 2)",
                ((u != null) && (u instanceof DoNewUnresolvedClinit12b)));
        }

    }

    static DoNewResolved doResolved() {
        return new DoNewResolved();
    }
    static DoNewUnresolved doUnresolved() {
        return new DoNewUnresolved();
    }
    static DoNewResolvedClinit doResolvedClinit() {
        return new DoNewResolvedClinit();
    }
    static DoNewUnresolvedClinit doUnresolvedClinit() {
        return new DoNewUnresolvedClinit();
    }
    static DoNewUnresolvedClinit1 doUnresolvedClinit1() {
        return new DoNewUnresolvedClinit1();
    }
    static DoNewUnresolvedClinit2 doUnresolvedClinit2() {
        return new DoNewUnresolvedClinit2();
    }
    static DoNewUnresolvedClinit12b doUnresolvedClinit12() {
        return new DoNewUnresolvedClinit12b(new DoNewUnresolvedClinit12a());
    }
}

///////////////////////////////////////////////////////////////////////////////
// Testing opcode anewarray:
// Note: Resolves to a CB pointer.

class DoANewArrayResolved {}
class DoANewArrayResolvedClinit {
    static Object so;
    static {
        System.out.println("\tDoANewArrayResolvedClinit.<clinit>()");
        so = new Object();
    }
}
class DoANewArrayUnresolved {}
class DoANewArrayUnresolvedClinit {
    static Object so;
    static {
        System.out.println("\tDoANewArrayUnresolvedClinit.<clinit>()");
        so = new Object();
    }
    DoANewArrayUnresolvedClinit() {
        System.out.println("\tDoANewArrayUnresolvedClinit.<init>()");
    }
}

class DoANewArray
{
    static final String[] compileItems = {
        "DoANewArray.doResolved(I)[LDoANewArrayResolved;",
        "DoANewArray.doUnresolved(I)[LDoANewArrayUnresolved;",
        "DoANewArray.doResolvedClinit(I)[LDoANewArrayResolvedClinit;",
        "DoANewArray.doUnresolvedClinit(I)[LDoANewArrayUnresolvedClinit;",
    };

    static DoANewArrayResolvedClinit getNullObject() { return null; }

    public static void doSetup() {
        // Setup initial conditions:
        Object o = new Object();
        DoANewArrayResolved r = new DoANewArrayResolved();  // Resolve it.
        DoANewArrayResolvedClinit rc = getNullObject();

        // Resolve ResolvedClinit without calling its clinit:
        System.out.println(
            "\tDoANewArrayNew: Forcing resolution of ResolvedClinit: " +
            ((rc instanceof DoANewArrayResolvedClinit) == false));

        // Do compilation:
        CompilerTest.main(compileItems);
    }

    public static void doTest() {

        // 1. Resolved New.
        {
            DoANewArrayResolved[] r = doResolved(5);
            DoResolveAndClinit.reportPassIf(
                "DoANewArrayResolved()", 
                ((r != null) && (r instanceof DoANewArrayResolved[])));
        }

        // 2. Unresolved New.  Will resolve.
        {
            DoANewArrayUnresolved[] u = doUnresolved(5);
            DoResolveAndClinit.reportPassIf(
                "DoANewArrayUnresolved(unresolved)",
                ((u != null) && (u instanceof DoANewArrayUnresolved[])));
        }

        // 3. Unresolved New.  Resolved in step 2.
        {
            DoANewArrayUnresolved[] u = doUnresolved(5);
            DoResolveAndClinit.reportPassIf(
                "DoANewArrayUnresolved(resolved)",
                ((u != null) && (u instanceof DoANewArrayUnresolved[])));
        }

        // 4. Resolved New w/ Clinit.
        {
            DoANewArrayResolvedClinit[] r = doResolvedClinit(5);
            DoResolveAndClinit.reportPassIf(
                "DoANewArrayResolvedClinit()",
                ((r != null) && (r instanceof DoANewArrayResolvedClinit[])));
        }

        // 5. Unresolved New w/ Clinit.  Will resolve & clinit.
        {
            DoANewArrayUnresolvedClinit[] u = doUnresolvedClinit(5);
            DoResolveAndClinit.reportPassIf(
                "DoANewArrayUnresolvedClinit(unresolved, !clinit)",
                ((u != null) && (u instanceof DoANewArrayUnresolvedClinit[])));
        }

        // 6. Unresolved New Resolved & Clinited.
        {
            DoANewArrayUnresolvedClinit[] u = doUnresolvedClinit(5);
            DoResolveAndClinit.reportPassIf(
                "DoANewArrayUnresolvedClinit(resolved, clinit)",
                ((u != null) && (u instanceof DoANewArrayUnresolvedClinit[])));
        }
    }

    static DoANewArrayResolved[] doResolved(int length) {
        return new DoANewArrayResolved[length];
    }
    static DoANewArrayUnresolved[] doUnresolved(int length) {
        return new DoANewArrayUnresolved[length];
    }
    static DoANewArrayResolvedClinit[] doResolvedClinit(int length) {
        return new DoANewArrayResolvedClinit[length];
    }
    static DoANewArrayUnresolvedClinit[] doUnresolvedClinit(int length) {
        return new DoANewArrayUnresolvedClinit[length];
    }
}

///////////////////////////////////////////////////////////////////////////////
// Testing opcode multianewarray:
// Note: Resolves to a CB pointer.

class DoMultiANewArrayResolved {
    DoMultiANewArrayResolved() {
        System.out.println("\tDoMultiANewArrayResolved.<init>()");
    }
}
class DoMultiANewArrayResolvedClinit {
    static Object so;
    static {
        System.out.println("\tDoMultiANewArrayResolvedClinit.<clinit>()");
        so = new Object();
    }
    DoMultiANewArrayResolvedClinit() {
        System.out.println("\tDoMultiANewArrayResolvedClinit.<init>()");
    }
}
class DoMultiANewArrayUnresolved {}
class DoMultiANewArrayUnresolvedClinit {
    static Object so;
    static {
        System.out.println("\tDoMultiANewArrayUnresolvedClinit.<clinit>()");
        so = new Object();
    }
    DoMultiANewArrayUnresolvedClinit() {
        System.out.println("\tDoMultiANewArrayUnresolvedClinit.<init>()");
    }
}

class DoMultiANewArray
{
    static final String[] compileItems = {
        "DoMultiANewArray.doResolved(II)[[LDoMultiANewArrayResolved;",
        "DoMultiANewArray.doUnresolved(II)[[LDoMultiANewArrayUnresolved;",
        "DoMultiANewArray.doResolvedClinit(II)[[LDoMultiANewArrayResolvedClinit;",
        "DoMultiANewArray.doUnresolvedClinit(II)[[LDoMultiANewArrayUnresolvedClinit;",
    };

    static DoMultiANewArrayResolvedClinit getNullObject() { return null; }

    public static void doSetup() {
        // Setup initial conditions:
        Object o = new Object();
        DoMultiANewArrayResolved r = new DoMultiANewArrayResolved();  // Resolve it.
        DoMultiANewArrayResolvedClinit rc = getNullObject();

        // Resolve ResolvedClinit without calling its clinit:
        System.out.println(
            "\tDoMultiANewArrayNew: Forcing resolution of ResolvedClinit: " +
            ((rc instanceof DoMultiANewArrayResolvedClinit) == false));

        // Do compilation:
        CompilerTest.main(compileItems);
    }

    public static void doTest() {

        // 1. Resolved New.
        {
            DoMultiANewArrayResolved[][] r = doResolved(5, 3);
            DoResolveAndClinit.reportPassIf(
                "DoMultiANewArrayResolved()", 
                ((r != null) &&
                 (r instanceof DoMultiANewArrayResolved[][])));
        }

        // 2. Unresolved New.  Will resolve.
        {
            DoMultiANewArrayUnresolved[][] u = doUnresolved(5, 3);
            DoResolveAndClinit.reportPassIf(
                "DoMultiANewArrayUnresolved(unresolved)",
                ((u != null) &&
                 (u instanceof DoMultiANewArrayUnresolved[][])));
        }

        // 3. Unresolved New.  Resolved in step 2.
        {
            DoMultiANewArrayUnresolved[][] u = doUnresolved(5, 3);
            DoResolveAndClinit.reportPassIf(
                "DoMultiANewArrayUnresolved(resolved)",
                ((u != null) &&
                 (u instanceof DoMultiANewArrayUnresolved[][])));
        }

        // 4. Resolved New w/ Clinit.
        {
            DoMultiANewArrayResolvedClinit[][] r = doResolvedClinit(5, 3);
            DoResolveAndClinit.reportPassIf(
                "DoMultiANewArrayResolvedClinit()",
                ((r != null) &&
                 (r instanceof DoMultiANewArrayResolvedClinit[][])));
        }

        // 5. Unresolved New w/ Clinit.  Will resolve & clinit.
        {
            DoMultiANewArrayUnresolvedClinit[][] u = doUnresolvedClinit(5, 3);
            DoResolveAndClinit.reportPassIf(
                "DoMultiANewArrayUnresolvedClinit(unresolved, !clinit)",
                ((u != null) &&
                 (u instanceof DoMultiANewArrayUnresolvedClinit[][])));
        }

        // 6. Unresolved New Resolved & Clinited.
        {
            DoMultiANewArrayUnresolvedClinit[][] u = doUnresolvedClinit(5, 3);
            DoResolveAndClinit.reportPassIf(
                "DoMultiANewArrayUnresolvedClinit(resolved, clinit)",
                ((u != null) &&
                 (u instanceof DoMultiANewArrayUnresolvedClinit[][])));
        }
    }

    static DoMultiANewArrayResolved[][]
    doResolved(int length, int width) {
        return new DoMultiANewArrayResolved[length][width];
    }
    static DoMultiANewArrayUnresolved[][]
    doUnresolved(int length, int width) {
        return new DoMultiANewArrayUnresolved[length][width];
    }
    static DoMultiANewArrayResolvedClinit[][]
    doResolvedClinit(int length, int width) {
        return new DoMultiANewArrayResolvedClinit[length][width];
    }
    static DoMultiANewArrayUnresolvedClinit[][]
    doUnresolvedClinit(int length, int width) {
        return new DoMultiANewArrayUnresolvedClinit[length][width];
    }
}

///////////////////////////////////////////////////////////////////////////////
// Testing opcode invokestatic:
// Note: Resolves to a MB pointer.

class DoInvokeStaticResolved {
    DoInvokeStaticResolved() {
        System.out.println("\tDoInvokeStaticResolved.<init>()");
    }
    public static DoInvokeStaticResolved staticMethod() {
        System.out.println("\tDoInvokeStaticResolved.staticMethod()");
        return new DoInvokeStaticResolved();
    }
}
class DoInvokeStaticResolvedClinit {
    static Object so;
    static {
        System.out.println("\tDoInvokeStaticResolvedClinit.<clinit>()");
        so = new Object();
    }
    DoInvokeStaticResolvedClinit() {
        System.out.println("\tDoInvokeStaticResolvedClinit.<init>()");
    }
    public static DoInvokeStaticResolvedClinit staticMethod() {
        System.out.println("\tDoInvokeStaticResolvedClinit.staticMethod()");
        return new DoInvokeStaticResolvedClinit();
    }
}
class DoInvokeStaticUnresolved {
    DoInvokeStaticUnresolved() {
        System.out.println("\tDoInvokeStaticUnresolved.<init>()");
    }
    public static DoInvokeStaticUnresolved staticMethod() {
        System.out.println("\tDoInvokeStaticUnresolved.staticMethod()");
        return new DoInvokeStaticUnresolved();
    }
}
class DoInvokeStaticUnresolvedClinit {
    static Object so;
    static {
        System.out.println("\tDoInvokeStaticUnresolvedClinit.<clinit>()");
        so = new Object();
    }
    DoInvokeStaticUnresolvedClinit() {
        System.out.println("\tDoInvokeStaticUnresolvedClinit.<init>()");
    }
    public static DoInvokeStaticUnresolvedClinit staticMethod() {
        System.out.println("\tDoInvokeStaticUnresolvedClinit.staticMethod()");
        return new DoInvokeStaticUnresolvedClinit();
    }
}

class DoInvokeStatic
{
    static final String[] compileItems = {
        "DoInvokeStatic.doResolved()LDoInvokeStaticResolved;",
        "DoInvokeStatic.doUnresolved()LDoInvokeStaticUnresolved;",
        "DoInvokeStatic.doResolvedClinit()LDoInvokeStaticResolvedClinit;",
        "DoInvokeStatic.doUnresolvedClinit()LDoInvokeStaticUnresolvedClinit;",
    };

    static DoInvokeStaticResolvedClinit getNullObject() { return null; }

    public static void doSetup() {
        // Setup initial conditions:
        Object o = new Object();
        DoInvokeStaticResolved r = DoInvokeStaticResolved.staticMethod(); // Resolve it.
        DoInvokeStaticResolvedClinit rc = getNullObject();

        // Resolve ResolvedClinit without calling its clinit:
        System.out.println(
            "\tDoInvokeStatic: Forcing resolution of ResolvedClinit: " +
            ((rc instanceof DoInvokeStaticResolvedClinit) == false));

        // Do compilation:
        CompilerTest.main(compileItems);
    }

    public static void doTest() {

        // 1. Resolved New.
        {
            DoInvokeStaticResolved r = doResolved();
            DoResolveAndClinit.reportPassIf(
                "DoInvokeStaticResolved()", 
                ((r != null) && (r instanceof DoInvokeStaticResolved)));
        }

        // 2. Unresolved New.  Will resolve.
        {
            DoInvokeStaticUnresolved u = doUnresolved();
            DoResolveAndClinit.reportPassIf(
                "DoInvokeStaticUnresolved(unresolved)",
                ((u != null) && (u instanceof DoInvokeStaticUnresolved)));
        }

        // 3. Unresolved New.  Resolved in step 2.
        {
            DoInvokeStaticUnresolved u = doUnresolved();
            DoResolveAndClinit.reportPassIf(
                "DoInvokeStaticUnresolved(resolved)",
                ((u != null) && (u instanceof DoInvokeStaticUnresolved)));
        }

        // 4. Resolved New w/ Clinit.
        {
            DoInvokeStaticResolvedClinit r = doResolvedClinit();
            DoResolveAndClinit.reportPassIf(
                "DoInvokeStaticResolvedClinit()",
                ((r != null) && (r instanceof DoInvokeStaticResolvedClinit)));
        }

        // 5. Unresolved New w/ Clinit.  Will resolve & clinit.
        {
            DoInvokeStaticUnresolvedClinit u = doUnresolvedClinit();
            DoResolveAndClinit.reportPassIf(
                "DoInvokeStaticUnresolvedClinit(unresolved, !clinit)",
                ((u != null) && (u instanceof DoInvokeStaticUnresolvedClinit)));
        }

        // 6. Unresolved New Resolved & Clinited.
        {
            DoInvokeStaticUnresolvedClinit u = doUnresolvedClinit();
            DoResolveAndClinit.reportPassIf(
                "DoInvokeStaticUnresolvedClinit(resolved, clinit)",
                ((u != null) && (u instanceof DoInvokeStaticUnresolvedClinit)));
        }
    }

    static DoInvokeStaticResolved doResolved() {
        return DoInvokeStaticResolved.staticMethod();
    }
    static DoInvokeStaticUnresolved doUnresolved() {
        return DoInvokeStaticUnresolved.staticMethod();
    }
    static DoInvokeStaticResolvedClinit doResolvedClinit() {
        return DoInvokeStaticResolvedClinit.staticMethod();
    }
    static DoInvokeStaticUnresolvedClinit doUnresolvedClinit() {
        return DoInvokeStaticUnresolvedClinit.staticMethod();
    }
}

///////////////////////////////////////////////////////////////////////////////
// Testing opcode invokevirtual:
// Note: Resolves to a MB pointer.

class DoInvokeVirtualResolved {
    DoInvokeVirtualResolved() {
        System.out.println("\tDoInvokeVirtualResolved.<init>()");
    }
    public DoInvokeVirtualResolved virtualMethod() {
        System.out.println("\tDoInvokeVirtualResolved.virtualMethod()");
        return new DoInvokeVirtualResolved();
    }
}
class DoInvokeVirtualResolvedClinit {
    static Object so;
    static {
        System.out.println("\tDoInvokeVirtualResolvedClinit.<clinit>()");
        so = new Object();
    }
    DoInvokeVirtualResolvedClinit() {
        System.out.println("\tDoInvokeVirtualResolvedClinit.<init>()");
    }
    public DoInvokeVirtualResolvedClinit virtualMethod() {
        System.out.println("\tDoInvokeVirtualResolvedClinit.virtualMethod()");
        return new DoInvokeVirtualResolvedClinit();
    }
}
class DoInvokeVirtualUnresolved {
    DoInvokeVirtualUnresolved() {
        System.out.println("\tDoInvokeVirtualUnresolved.<init>()");
    }
    public DoInvokeVirtualUnresolved virtualMethod() {
        System.out.println("\tDoInvokeVirtualUnresolved.virtualMethod()");
        return new DoInvokeVirtualUnresolved();
    }
}
class DoInvokeVirtualUnresolvedClinit {
    static Object so;
    static {
        System.out.println("\tDoInvokeVirtualUnresolvedClinit.<clinit>()");
        so = new Object();
    }
    DoInvokeVirtualUnresolvedClinit() {
        System.out.println("\tDoInvokeVirtualUnresolvedClinit.<init>()");
    }
    public DoInvokeVirtualUnresolvedClinit virtualMethod() {
        System.out.println("\tDoInvokeVirtualUnresolvedClinit.virtualMethod()");
        return new DoInvokeVirtualUnresolvedClinit();
    }
}

class DoInvokeVirtualFactory {
    public static DoInvokeVirtualResolved getResolved() {
        return new DoInvokeVirtualResolved();
    }
    public static DoInvokeVirtualResolvedClinit getResolvedClinit() {
        return new DoInvokeVirtualResolvedClinit();
    }
    public static DoInvokeVirtualUnresolved getUnresolved() {
        return new DoInvokeVirtualUnresolved();
    }
    public static DoInvokeVirtualUnresolvedClinit getUnresolvedClinit() {
        return new DoInvokeVirtualUnresolvedClinit();
    }
}

class DoInvokeVirtual
{
    static final String[] compileItems = {
        "DoInvokeVirtual.doResolved(LDoInvokeVirtualResolved;)LDoInvokeVirtualResolved;",
        "DoInvokeVirtual.doUnresolved(LDoInvokeVirtualUnresolved;)LDoInvokeVirtualUnresolved;",
        "DoInvokeVirtual.doResolvedClinit(LDoInvokeVirtualResolvedClinit;)LDoInvokeVirtualResolvedClinit;",
        "DoInvokeVirtual.doUnresolvedClinit(LDoInvokeVirtualUnresolvedClinit;)LDoInvokeVirtualUnresolvedClinit;",
    };

    static DoInvokeVirtualResolvedClinit getNullObject() { return null; }

    public static void doSetup() {
        // Setup initial conditions:
        Object o = new Object();
        DoInvokeVirtualResolved r = DoInvokeVirtualFactory.getResolved();
        r = r.virtualMethod();  // resolve it.

        // Do compilation:
        CompilerTest.main(compileItems);
    }

    public static void doTest() {

        // 1. Resolved New.
        {
            DoInvokeVirtualResolved r =
                doResolved(DoInvokeVirtualFactory.getResolved());
            DoResolveAndClinit.reportPassIf(
                "DoInvokeVirtualResolved()", 
                ((r != null) && (r instanceof DoInvokeVirtualResolved)));
        }

        // 2. Unresolved New.  Will resolve.
        {
            DoInvokeVirtualUnresolved u =
                doUnresolved(DoInvokeVirtualFactory.getUnresolved());
            DoResolveAndClinit.reportPassIf(
                "DoInvokeVirtualUnresolved(unresolved)",
                ((u != null) && (u instanceof DoInvokeVirtualUnresolved)));
        }

        // 3. Unresolved New.  Resolved in step 2.
        {
            DoInvokeVirtualUnresolved u =
                doUnresolved(DoInvokeVirtualFactory.getUnresolved());
            DoResolveAndClinit.reportPassIf(
                "DoInvokeVirtualUnresolved(resolved)",
                ((u != null) && (u instanceof DoInvokeVirtualUnresolved)));
        }

        // 4. Resolved New w/ Clinit.
        {
            DoInvokeVirtualResolvedClinit r =
                doResolvedClinit(DoInvokeVirtualFactory.getResolvedClinit());
            DoResolveAndClinit.reportPassIf(
                "DoInvokeVirtualResolvedClinit()",
                ((r != null) && (r instanceof DoInvokeVirtualResolvedClinit)));
        }

        // 5. Unresolved New w/ Clinit.  Will resolve & clinit.
        {
            DoInvokeVirtualUnresolvedClinit u =
                doUnresolvedClinit(DoInvokeVirtualFactory.getUnresolvedClinit());
            DoResolveAndClinit.reportPassIf(
                "DoInvokeVirtualUnresolvedClinit(unresolved, !clinit)",
                ((u != null) && (u instanceof DoInvokeVirtualUnresolvedClinit)));
        }

        // 6. Unresolved New Resolved & Clinited.
        {
            DoInvokeVirtualUnresolvedClinit u =
                doUnresolvedClinit(DoInvokeVirtualFactory.getUnresolvedClinit());
            DoResolveAndClinit.reportPassIf(
                "DoInvokeVirtualUnresolvedClinit(resolved, clinit)",
                ((u != null) && (u instanceof DoInvokeVirtualUnresolvedClinit)));
        }
    }

    static DoInvokeVirtualResolved
    doResolved(DoInvokeVirtualResolved o) {
        return o.virtualMethod();
    }
    static DoInvokeVirtualUnresolved
    doUnresolved(DoInvokeVirtualUnresolved o) {
        return o.virtualMethod();
    }
    static DoInvokeVirtualResolvedClinit
    doResolvedClinit(DoInvokeVirtualResolvedClinit o) {
        return o.virtualMethod();
    }
    static DoInvokeVirtualUnresolvedClinit
    doUnresolvedClinit(DoInvokeVirtualUnresolvedClinit o) {
        return o.virtualMethod();
    }
}

//        DoInvokeSpecial.doTest();

///////////////////////////////////////////////////////////////////////////////
// Testing opcode invokeinterface:
// Note: Resolves to a MB pointer.

interface DoInvokeInterfaceResolved {
    public DoInvokeInterfaceResolved interfaceMethod();
}
interface DoInvokeInterfaceResolvedClinit {
    public DoInvokeInterfaceResolvedClinit interfaceMethod();
}
interface DoInvokeInterfaceUnresolved {
    public DoInvokeInterfaceUnresolved interfaceMethod();
}
interface DoInvokeInterfaceUnresolvedClinit {
    public DoInvokeInterfaceUnresolvedClinit interfaceMethod();
}

class DoInvokeInterfaceResolvedImpl implements DoInvokeInterfaceResolved {
    DoInvokeInterfaceResolvedImpl() {
        System.out.println("\tDoInvokeInterfaceResolvedImpl.<init>()");
    }
    public DoInvokeInterfaceResolved interfaceMethod() {
        System.out.println("\tDoInvokeInterfaceResolvedImpl.interfaceMethod()");
        return new DoInvokeInterfaceResolvedImpl();
    }
}
class DoInvokeInterfaceResolvedClinitImpl
    implements DoInvokeInterfaceResolvedClinit {
    static Object so;
    static {
        System.out.println("\tDoInvokeInterfaceResolvedClinitImpl.<clinit>()");
        so = new Object();
    }
    DoInvokeInterfaceResolvedClinitImpl() {
        System.out.println("\tDoInvokeInterfaceResolvedClinitImpl.<init>()");
    }
    public DoInvokeInterfaceResolvedClinit interfaceMethod() {
        System.out.println("\tDoInvokeInterfaceResolvedClinitImpl.interfaceMethod()");
        return new DoInvokeInterfaceResolvedClinitImpl();
    }
}
class DoInvokeInterfaceUnresolvedImpl implements DoInvokeInterfaceUnresolved {
    DoInvokeInterfaceUnresolvedImpl() {
        System.out.println("\tDoInvokeInterfaceUnresolvedImpl.<init>()");
    }
    public DoInvokeInterfaceUnresolved interfaceMethod() {
        System.out.println("\tDoInvokeInterfaceUnresolvedImpl.interfaceMethod()");
        return new DoInvokeInterfaceUnresolvedImpl();
    }
}
class DoInvokeInterfaceUnresolvedClinitImpl
    implements DoInvokeInterfaceUnresolvedClinit {
    static Object so;
    static {
        System.out.println("\tDoInvokeInterfaceUnresolvedClinitImpl.<clinit>()");
        so = new Object();
    }
    DoInvokeInterfaceUnresolvedClinitImpl() {
        System.out.println("\tDoInvokeInterfaceUnresolvedClinitImpl.<init>()");
    }
    public DoInvokeInterfaceUnresolvedClinit interfaceMethod() {
        System.out.println("\tDoInvokeInterfaceUnresolvedClinitImpl.interfaceMethod()");
        return new DoInvokeInterfaceUnresolvedClinitImpl();
    }
}

class DoInvokeInterfaceFactory {
    public static DoInvokeInterfaceResolved getResolved() {
        return new DoInvokeInterfaceResolvedImpl();
    }
    public static DoInvokeInterfaceResolvedClinit getResolvedClinit() {
        return new DoInvokeInterfaceResolvedClinitImpl();
    }
    public static DoInvokeInterfaceUnresolved getUnresolved() {
        return new DoInvokeInterfaceUnresolvedImpl();
    }
    public static DoInvokeInterfaceUnresolvedClinit getUnresolvedClinit() {
        return new DoInvokeInterfaceUnresolvedClinitImpl();
    }
}

class DoInvokeInterface
{
    static final String[] compileItems = {
        "DoInvokeInterface.doResolved(LDoInvokeInterfaceResolved;)LDoInvokeInterfaceResolved;",
        "DoInvokeInterface.doUnresolved(LDoInvokeInterfaceUnresolved;)LDoInvokeInterfaceUnresolved;",
        "DoInvokeInterface.doResolvedClinit(LDoInvokeInterfaceResolvedClinit;)LDoInvokeInterfaceResolvedClinit;",
        "DoInvokeInterface.doUnresolvedClinit(LDoInvokeInterfaceUnresolvedClinit;)LDoInvokeInterfaceUnresolvedClinit;",
    };

    public static void doSetup() {
        // Setup initial conditions:
        Object o = new Object();
        DoInvokeInterfaceResolved r = DoInvokeInterfaceFactory.getResolved();
        r = r.interfaceMethod();  // resolve it.

        // Do compilation:
        CompilerTest.main(compileItems);
    }

    public static void doTest() {

        // 1. Resolved New.
        {
            DoInvokeInterfaceResolved r =
                doResolved(DoInvokeInterfaceFactory.getResolved());
            DoResolveAndClinit.reportPassIf(
                "DoInvokeInterfaceResolved()", 
                ((r != null) && (r instanceof DoInvokeInterfaceResolved)));
        }

        // 2. Unresolved New.  Will resolve.
        {
            DoInvokeInterfaceUnresolved u =
                doUnresolved(DoInvokeInterfaceFactory.getUnresolved());
            DoResolveAndClinit.reportPassIf(
                "DoInvokeInterfaceUnresolved(unresolved)",
                ((u != null) && (u instanceof DoInvokeInterfaceUnresolved)));
        }

        // 3. Unresolved New.  Resolved in step 2.
        {
            DoInvokeInterfaceUnresolved u =
                doUnresolved(DoInvokeInterfaceFactory.getUnresolved());
            DoResolveAndClinit.reportPassIf(
                "DoInvokeInterfaceUnresolved(resolved)",
                ((u != null) && (u instanceof DoInvokeInterfaceUnresolved)));
        }

        // 4. Resolved New w/ Clinit.
        {
            DoInvokeInterfaceResolvedClinit r =
                doResolvedClinit(DoInvokeInterfaceFactory.getResolvedClinit());
            DoResolveAndClinit.reportPassIf(
                "DoInvokeInterfaceResolvedClinit()",
                ((r != null) && (r instanceof DoInvokeInterfaceResolvedClinit)));
        }

        // 5. Unresolved New w/ Clinit.  Will resolve & clinit.
        {
            DoInvokeInterfaceUnresolvedClinit u =
                doUnresolvedClinit(DoInvokeInterfaceFactory.getUnresolvedClinit());
            DoResolveAndClinit.reportPassIf(
                "DoInvokeInterfaceUnresolvedClinit(unresolved, !clinit)",
                ((u != null) && (u instanceof DoInvokeInterfaceUnresolvedClinit)));
        }

        // 6. Unresolved New Resolved & Clinited.
        {
            DoInvokeInterfaceUnresolvedClinit u =
                doUnresolvedClinit(DoInvokeInterfaceFactory.getUnresolvedClinit());
            DoResolveAndClinit.reportPassIf(
                "DoInvokeInterfaceUnresolvedClinit(resolved, clinit)",
                ((u != null) && (u instanceof DoInvokeInterfaceUnresolvedClinit)));
        }
    }

    static DoInvokeInterfaceResolved
    doResolved(DoInvokeInterfaceResolved o) {
        return o.interfaceMethod();
    }
    static DoInvokeInterfaceUnresolved
    doUnresolved(DoInvokeInterfaceUnresolved o) {
        return o.interfaceMethod();
    }
    static DoInvokeInterfaceResolvedClinit
    doResolvedClinit(DoInvokeInterfaceResolvedClinit o) {
        return o.interfaceMethod();
    }
    static DoInvokeInterfaceUnresolvedClinit
    doUnresolvedClinit(DoInvokeInterfaceUnresolvedClinit o) {
        return o.interfaceMethod();
    }
}

///////////////////////////////////////////////////////////////////////////////
// Testing opcode putstatic:
// Note: Resolves to a field address.

/* Boolean: */
class DoPutStaticResolvedClinitedBoolean {
    public static boolean staticField = false;
    static {
        System.out.println("\tDoPutStaticResolvedClinitedBoolean.<clinit>()");
    }
}

class DoPutStaticResolvedBoolean {
    public static boolean staticField = false;
    static {
        System.out.println("\tDoPutStaticResolvedBoolean.<clinit>()");
    }
}
class DoPutStaticUnresolvedBoolean {
    public static boolean staticField = false;
    static {
        System.out.println("\tDoPutStaticUnresolvedBoolean.<clinit>()");
    }
}

/* Byte: */
class DoPutStaticResolvedClinitedByte {
    public static byte staticField = 0;
    static {
        System.out.println("\tDoPutStaticResolvedClinitedByte.<clinit>()");
    }
}

class DoPutStaticResolvedByte {
    public static byte staticField = 0;
    static {
        System.out.println("\tDoPutStaticResolvedByte.<clinit>()");
    }
}
class DoPutStaticUnresolvedByte {
    public static byte staticField = 0;
    static {
        System.out.println("\tDoPutStaticUnresolvedByte.<clinit>()");
    }
}

/* Char: */
class DoPutStaticResolvedClinitedChar {
    public static char staticField = '\0';
    static {
        System.out.println("\tDoPutStaticResolvedClinitedChar.<clinit>()");
    }
}

class DoPutStaticResolvedChar {
    public static char staticField = '\0';
    static {
        System.out.println("\tDoPutStaticResolvedChar.<clinit>()");
    }
}
class DoPutStaticUnresolvedChar {
    public static char staticField = '\0';
    static {
        System.out.println("\tDoPutStaticUnresolvedChar.<clinit>()");
    }
}

/* Short: */
class DoPutStaticResolvedClinitedShort {
    public static short staticField = 0;
    static {
        System.out.println("\tDoPutStaticResolvedClinitedShort.<clinit>()");
    }
}

class DoPutStaticResolvedShort {
    public static short staticField = 0;
    static {
        System.out.println("\tDoPutStaticResolvedShort.<clinit>()");
    }
}
class DoPutStaticUnresolvedShort {
    public static short staticField = 0;
    static {
        System.out.println("\tDoPutStaticUnresolvedShort.<clinit>()");
    }
}

/* Int: */
class DoPutStaticResolvedClinitedInt {
    public static int staticField = 0;
    static {
        System.out.println("\tDoPutStaticResolvedClinitedInt.<clinit>()");
    }
}

class DoPutStaticResolvedInt {
    public static int staticField = 0;
    static {
        System.out.println("\tDoPutStaticResolvedInt.<clinit>()");
    }
}
class DoPutStaticUnresolvedInt {
    public static int staticField = 0;
    static {
        System.out.println("\tDoPutStaticUnresolvedInt.<clinit>()");
    }
}

/* Float: */
class DoPutStaticResolvedClinitedFloat {
    public static float staticField = 0.0f;
    static {
        System.out.println("\tDoPutStaticResolvedClinitedFloat.<clinit>()");
    }
}

class DoPutStaticResolvedFloat {
    public static float staticField = 0.0f;
    static {
        System.out.println("\tDoPutStaticResolvedFloat.<clinit>()");
    }
}
class DoPutStaticUnresolvedFloat {
    public static float staticField = 0.0f;
    static {
        System.out.println("\tDoPutStaticUnresolvedFloat.<clinit>()");
    }
}

/* Long: */
class DoPutStaticResolvedClinitedLong {
    public static long staticField = 0l;
    static {
        System.out.println("\tDoPutStaticResolvedClinitedLong.<clinit>()");
    }
}

class DoPutStaticResolvedLong {
    public static long staticField = 0l;
    static {
        System.out.println("\tDoPutStaticResolvedLong.<clinit>()");
    }
}
class DoPutStaticUnresolvedLong {
    public static long staticField = 0l;
    static {
        System.out.println("\tDoPutStaticUnresolvedLong.<clinit>()");
    }
}

/* Double: */
class DoPutStaticResolvedClinitedDouble {
    public static double staticField = 0.0d;
    static {
        System.out.println("\tDoPutStaticResolvedClinitedDouble.<clinit>()");
    }
}

class DoPutStaticResolvedDouble {
    public static double staticField = 0.0d;
    static {
        System.out.println("\tDoPutStaticResolvedDouble.<clinit>()");
    }
}
class DoPutStaticUnresolvedDouble {
    public static double staticField = 0.0d;
    static {
        System.out.println("\tDoPutStaticUnresolvedDouble.<clinit>()");
    }
}

/* Object: */
class DoPutStaticResolvedClinitedObject {
    public static Object staticField = null;
    static {
        System.out.println("\tDoPutStaticResolvedClinitedObject.<clinit>()");
    }
}

class DoPutStaticResolvedObject {
    public static Object staticField = null;
    static {
        System.out.println("\tDoPutStaticResolvedObject.<clinit>()");
    }
}
class DoPutStaticUnresolvedObject {
    public static Object staticField = null;
    static {
        System.out.println("\tDoPutStaticUnresolvedObject.<clinit>()");
    }
}

/* Volatile Boolean: */
class DoPutStaticResolvedClinitedBooleanVolatile {
    public static volatile boolean staticField = false;
    static {
        System.out.println("\tDoPutStaticResolvedClinitedBooleanVolatile.<clinit>()");
    }
}

class DoPutStaticResolvedBooleanVolatile {
    public static volatile boolean staticField = false;
    static {
        System.out.println("\tDoPutStaticResolvedBooleanVolatile.<clinit>()");
    }
}
class DoPutStaticUnresolvedBooleanVolatile {
    public static volatile boolean staticField = false;
    static {
        System.out.println("\tDoPutStaticUnresolvedBooleanVolatile.<clinit>()");
    }
}

/* Volatile Byte: */
class DoPutStaticResolvedClinitedByteVolatile {
    public static volatile byte staticField = 0;
    static {
        System.out.println("\tDoPutStaticResolvedClinitedByteVolatile.<clinit>()");
    }
}

class DoPutStaticResolvedByteVolatile {
    public static volatile byte staticField = 0;
    static {
        System.out.println("\tDoPutStaticResolvedByteVolatile.<clinit>()");
    }
}
class DoPutStaticUnresolvedByteVolatile {
    public static volatile byte staticField = 0;
    static {
        System.out.println("\tDoPutStaticUnresolvedByteVolatile.<clinit>()");
    }
}

/* Volatile Char: */
class DoPutStaticResolvedClinitedCharVolatile {
    public static volatile char staticField = '\0';
    static {
        System.out.println("\tDoPutStaticResolvedClinitedCharVolatile.<clinit>()");
    }
}

class DoPutStaticResolvedCharVolatile {
    public static volatile char staticField = '\0';
    static {
        System.out.println("\tDoPutStaticResolvedCharVolatile.<clinit>()");
    }
}
class DoPutStaticUnresolvedCharVolatile {
    public static volatile char staticField = '\0';
    static {
        System.out.println("\tDoPutStaticUnresolvedCharVolatile.<clinit>()");
    }
}

/* Volatile Short: */
class DoPutStaticResolvedClinitedShortVolatile {
    public static volatile short staticField = 0;
    static {
        System.out.println("\tDoPutStaticResolvedClinitedShortVolatile.<clinit>()");
    }
}

class DoPutStaticResolvedShortVolatile {
    public static volatile short staticField = 0;
    static {
        System.out.println("\tDoPutStaticResolvedShortVolatile.<clinit>()");
    }
}
class DoPutStaticUnresolvedShortVolatile {
    public static volatile short staticField = 0;
    static {
        System.out.println("\tDoPutStaticUnresolvedShortVolatile.<clinit>()");
    }
}

/* Volatile Int: */
class DoPutStaticResolvedClinitedIntVolatile {
    public static volatile int staticField = 0;
    static {
        System.out.println("\tDoPutStaticResolvedClinitedIntVolatile.<clinit>()");
    }
}

class DoPutStaticResolvedIntVolatile {
    public static volatile int staticField = 0;
    static {
        System.out.println("\tDoPutStaticResolvedIntVolatile.<clinit>()");
    }
}
class DoPutStaticUnresolvedIntVolatile {
    public static volatile int staticField = 0;
    static {
        System.out.println("\tDoPutStaticUnresolvedIntVolatile.<clinit>()");
    }
}

/* Volatile Float: */
class DoPutStaticResolvedClinitedFloatVolatile {
    public static volatile float staticField = 0.0f;
    static {
        System.out.println("\tDoPutStaticResolvedClinitedFloatVolatile.<clinit>()");
    }
}

class DoPutStaticResolvedFloatVolatile {
    public static volatile float staticField = 0.0f;
    static {
        System.out.println("\tDoPutStaticResolvedFloatVolatile.<clinit>()");
    }
}
class DoPutStaticUnresolvedFloatVolatile {
    public static volatile float staticField = 0.0f;
    static {
        System.out.println("\tDoPutStaticUnresolvedFloatVolatile.<clinit>()");
    }
}

/* Volatile Long: */
class DoPutStaticResolvedClinitedLongVolatile {
    public static volatile long staticField = 0l;
    static {
        System.out.println("\tDoPutStaticResolvedClinitedLongVolatile.<clinit>()");
    }
}

class DoPutStaticResolvedLongVolatile {
    public static volatile long staticField = 0l;
    static {
        System.out.println("\tDoPutStaticResolvedLongVolatile.<clinit>()");
    }
}
class DoPutStaticUnresolvedLongVolatile {
    public static volatile long staticField = 0l;
    static {
        System.out.println("\tDoPutStaticUnresolvedLongVolatile.<clinit>()");
    }
}

/* Volatile Double: */
class DoPutStaticResolvedClinitedDoubleVolatile {
    public static volatile double staticField = 0.0d;
    static {
        System.out.println("\tDoPutStaticResolvedClinitedDoubleVolatile.<clinit>()");
    }
}

class DoPutStaticResolvedDoubleVolatile {
    public static volatile double staticField = 0.0d;
    static {
        System.out.println("\tDoPutStaticResolvedDoubleVolatile.<clinit>()");
    }
}
class DoPutStaticUnresolvedDoubleVolatile {
    public static volatile double staticField = 0.0d;
    static {
        System.out.println("\tDoPutStaticUnresolvedDoubleVolatile.<clinit>()");
    }
}

/* Volatile Object: */
class DoPutStaticResolvedClinitedObjectVolatile {
    public static volatile Object staticField = null;
    static {
        System.out.println("\tDoPutStaticResolvedClinitedObjectVolatile.<clinit>()");
    }
}

class DoPutStaticResolvedObjectVolatile {
    public static volatile Object staticField = null;
    static {
        System.out.println("\tDoPutStaticResolvedObjectVolatile.<clinit>()");
    }
}
class DoPutStaticUnresolvedObjectVolatile {
    public static volatile Object staticField = null;
    static {
        System.out.println("\tDoPutStaticUnresolvedObjectVolatile.<clinit>()");
    }
}

class DoPutStatic
{
    static final String[] compileItems = {
	// Non-Volatile Set:
        "DoPutStatic.doResolvedClinitedBoolean(Z)V",
        "DoPutStatic.doResolvedBoolean(Z)V",
        "DoPutStatic.doUnresolvedBoolean(Z)V",

        "DoPutStatic.doResolvedClinitedByte(B)V",
        "DoPutStatic.doResolvedByte(B)V",
        "DoPutStatic.doUnresolvedByte(B)V",

        "DoPutStatic.doResolvedClinitedChar(C)V",
        "DoPutStatic.doResolvedChar(C)V",
        "DoPutStatic.doUnresolvedChar(C)V",

        "DoPutStatic.doResolvedClinitedShort(S)V",
        "DoPutStatic.doResolvedShort(S)V",
        "DoPutStatic.doUnresolvedShort(S)V",

        "DoPutStatic.doResolvedClinitedInt(I)V",
        "DoPutStatic.doResolvedInt(I)V",
        "DoPutStatic.doUnresolvedInt(I)V",

        "DoPutStatic.doResolvedClinitedFloat(F)V",
        "DoPutStatic.doResolvedFloat(F)V",
        "DoPutStatic.doUnresolvedFloat(F)V",

        "DoPutStatic.doResolvedClinitedLong(J)V",
        "DoPutStatic.doResolvedLong(J)V",
        "DoPutStatic.doUnresolvedLong(J)V",

        "DoPutStatic.doResolvedClinitedDouble(D)V",
        "DoPutStatic.doResolvedDouble(D)V",
        "DoPutStatic.doUnresolvedDouble(D)V",

        "DoPutStatic.doResolvedClinitedObject(Ljava/lang/Object;)V",
        "DoPutStatic.doResolvedObject(Ljava/lang/Object;)V",
        "DoPutStatic.doUnresolvedObject(Ljava/lang/Object;)V",

	// Volatile Set:
        "DoPutStatic.doResolvedClinitedBooleanVolatile(Z)V",
        "DoPutStatic.doResolvedBooleanVolatile(Z)V",
        "DoPutStatic.doUnresolvedBooleanVolatile(Z)V",

        "DoPutStatic.doResolvedClinitedByteVolatile(B)V",
        "DoPutStatic.doResolvedByteVolatile(B)V",
        "DoPutStatic.doUnresolvedByteVolatile(B)V",

        "DoPutStatic.doResolvedClinitedCharVolatile(C)V",
        "DoPutStatic.doResolvedCharVolatile(C)V",
        "DoPutStatic.doUnresolvedCharVolatile(C)V",

        "DoPutStatic.doResolvedClinitedShortVolatile(S)V",
        "DoPutStatic.doResolvedShortVolatile(S)V",
        "DoPutStatic.doUnresolvedShortVolatile(S)V",

        "DoPutStatic.doResolvedClinitedIntVolatile(I)V",
        "DoPutStatic.doResolvedIntVolatile(I)V",
        "DoPutStatic.doUnresolvedIntVolatile(I)V",

        "DoPutStatic.doResolvedClinitedFloatVolatile(F)V",
        "DoPutStatic.doResolvedFloatVolatile(F)V",
        "DoPutStatic.doUnresolvedFloatVolatile(F)V",

        "DoPutStatic.doResolvedClinitedLongVolatile(J)V",
        "DoPutStatic.doResolvedLongVolatile(J)V",
        "DoPutStatic.doUnresolvedLongVolatile(J)V",

        "DoPutStatic.doResolvedClinitedDoubleVolatile(D)V",
        "DoPutStatic.doResolvedDoubleVolatile(D)V",
        "DoPutStatic.doUnresolvedDoubleVolatile(D)V",

        "DoPutStatic.doResolvedClinitedObjectVolatile(Ljava/lang/Object;)V",
        "DoPutStatic.doResolvedObjectVolatile(Ljava/lang/Object;)V",
        "DoPutStatic.doUnresolvedObjectVolatile(Ljava/lang/Object;)V",
    };

    public static void doSetup() {
        // Setup initial conditions:
        Object o = new Object();

	// Non-Volatile Set:
        {
            // Resolve and clinit it:
            boolean i = DoPutStaticResolvedClinitedBoolean.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoPutStaticResolvedBoolean;
        }
        {
            // Resolve and clinit it:
            byte i = DoPutStaticResolvedClinitedByte.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoPutStaticResolvedByte;
        }
        {
            // Resolve and clinit it:
            char i = DoPutStaticResolvedClinitedChar.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoPutStaticResolvedChar;
        }
        {
            // Resolve and clinit it:
            short i = DoPutStaticResolvedClinitedShort.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoPutStaticResolvedShort;
        }
        {
            // Resolve and clinit it:
            int i = DoPutStaticResolvedClinitedInt.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoPutStaticResolvedInt;
        }
        {
            // Resolve and clinit it:
            float i = DoPutStaticResolvedClinitedFloat.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoPutStaticResolvedFloat;
        }
        {
            // Resolve and clinit it:
            long i = DoPutStaticResolvedClinitedLong.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoPutStaticResolvedLong;
        }
        {
            // Resolve and clinit it:
            double i = DoPutStaticResolvedClinitedDouble.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoPutStaticResolvedDouble;
        }
        {
            // Resolve and clinit it:
            Object o1 = DoPutStaticResolvedClinitedObject.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoPutStaticResolvedObject;
        }

	// Volatile Set:
        {
            // Resolve and clinit it:
            boolean i = DoPutStaticResolvedClinitedBooleanVolatile.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoPutStaticResolvedBooleanVolatile;
        }
        {
            // Resolve and clinit it:
            byte i = DoPutStaticResolvedClinitedByteVolatile.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoPutStaticResolvedByteVolatile;
        }
        {
            // Resolve and clinit it:
            char i = DoPutStaticResolvedClinitedCharVolatile.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoPutStaticResolvedCharVolatile;
        }
        {
            // Resolve and clinit it:
            short i = DoPutStaticResolvedClinitedShortVolatile.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoPutStaticResolvedShortVolatile;
        }
        {
            // Resolve and clinit it:
            int i = DoPutStaticResolvedClinitedIntVolatile.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoPutStaticResolvedIntVolatile;
        }
        {
            // Resolve and clinit it:
            float i = DoPutStaticResolvedClinitedFloatVolatile.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoPutStaticResolvedFloatVolatile;
        }
        {
            // Resolve and clinit it:
            long i = DoPutStaticResolvedClinitedLongVolatile.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoPutStaticResolvedLongVolatile;
        }
        {
            // Resolve and clinit it:
            double i = DoPutStaticResolvedClinitedDoubleVolatile.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoPutStaticResolvedDoubleVolatile;
        }
        {
            // Resolve and clinit it:
            Object o1 = DoPutStaticResolvedClinitedObjectVolatile.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoPutStaticResolvedObjectVolatile;
        }

        // Do compilation:
        CompilerTest.main(compileItems);
    }

    public static void doTest() {

	// Non-Volatile Set:

        // 1. Resolved & Clinited putstatic:
        {
            doResolvedClinitedBoolean(true);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedClinited(boolean)",
                DoPutStaticResolvedClinitedBoolean.staticField, true);
        }
        // 2. Resolved putstatic:
        {
            doResolvedBoolean(true);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolved(boolean)",
                DoPutStaticResolvedBoolean.staticField, true);
        }
        // 3. Unresolved putstatic:
        {
            doUnresolvedBoolean(true);
            DoResolveAndClinit.reportPassIf(
                "PutStaticUnresolved(boolean)",
                DoPutStaticUnresolvedBoolean.staticField, true);
        }

        // 1. Resolved & Clinited putstatic:
        {
            doResolvedClinitedByte((byte)5);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedClinited(byte)",
                DoPutStaticResolvedClinitedByte.staticField, (byte)5);
        }
        // 2. Resolved putstatic:
        {
            doResolvedByte((byte)50);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolved(byte)",
                DoPutStaticResolvedByte.staticField, (byte)50);
        }
        // 3. Unresolved putstatic:
        {
            doUnresolvedByte((byte)53);
            DoResolveAndClinit.reportPassIf(
                "PutStaticUnresolved(byte)",
                DoPutStaticUnresolvedByte.staticField, (byte)53);
        }

        // 1. Resolved & Clinited putstatic:
        {
            doResolvedClinitedChar('A');
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedClinited(char)",
                DoPutStaticResolvedClinitedChar.staticField, 'A');
        }
        // 2. Resolved putstatic:
        {
            doResolvedChar('B');
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolved(char)",
                DoPutStaticResolvedChar.staticField, 'B');
        }
        // 3. Unresolved putstatic:
        {
            doUnresolvedChar('C');
            DoResolveAndClinit.reportPassIf(
                "PutStaticUnresolved(char)",
                DoPutStaticUnresolvedChar.staticField, 'C');
        }

        // 1. Resolved & Clinited putstatic:
        {
            doResolvedClinitedShort((short)6);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedClinited(short)",
                DoPutStaticResolvedClinitedShort.staticField, (short)6);
        }
        // 2. Resolved putstatic:
        {
            doResolvedShort((short)60);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolved(short)",
                DoPutStaticResolvedShort.staticField, (short)60);
        }
        // 3. Unresolved putstatic:
        {
            doUnresolvedShort((short)600);
            DoResolveAndClinit.reportPassIf(
                "PutStaticUnresolved(short)",
                DoPutStaticUnresolvedShort.staticField, (short)600);
        }

        // 1. Resolved & Clinited putstatic:
        {
            doResolvedClinitedInt(7);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedClinited(int)",
                DoPutStaticResolvedClinitedInt.staticField, 7);
        }
        // 2. Resolved putstatic:
        {
            doResolvedInt(70);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolved(int)",
                DoPutStaticResolvedInt.staticField, 70);
        }
        // 3. Unresolved putstatic:
        {
            doUnresolvedInt(700);
            DoResolveAndClinit.reportPassIf(
                "PutStaticUnresolved(int)",
                DoPutStaticUnresolvedInt.staticField, 700);
        }

        // 1. Resolved & Clinited putstatic:
        {
            doResolvedClinitedFloat(8.0f);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedClinited(float)",
                DoPutStaticResolvedClinitedFloat.staticField, 8.0f);
        }
        // 2. Resolved putstatic:
        {
            doResolvedFloat(80.0f);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolved(float)",
                DoPutStaticResolvedFloat.staticField, 80.0f);
        }
        // 3. Unresolved putstatic:
        {
            doUnresolvedFloat(800.0f);
            DoResolveAndClinit.reportPassIf(
                "PutStaticUnresolved(float)",
                DoPutStaticUnresolvedFloat.staticField, 800.0f);
        }

        // 1. Resolved & Clinited putstatic:
        {
            doResolvedClinitedLong(9l);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedClinited(long)",
                DoPutStaticResolvedClinitedLong.staticField, 9l);
        }
        // 2. Resolved putstatic:
        {
            doResolvedLong(90l);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolved(long)",
                DoPutStaticResolvedLong.staticField, 90l);
        }
        // 3. Unresolved putstatic:
        {
            doUnresolvedLong(900l);
            DoResolveAndClinit.reportPassIf(
                "PutStaticUnresolved(long)",
                DoPutStaticUnresolvedLong.staticField, 900l);
        }

        // 1. Resolved & Clinited putstatic:
        {
            doResolvedClinitedDouble(10.0d);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedClinited(double)",
                DoPutStaticResolvedClinitedDouble.staticField, 10.0d);
        }
        // 2. Resolved putstatic:
        {
            doResolvedDouble(100.0d);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolved(double)",
                DoPutStaticResolvedDouble.staticField, 100.0d);
        }
        // 3. Unresolved putstatic:
        {
            doUnresolvedDouble(1000.0d);
            DoResolveAndClinit.reportPassIf(
                "PutStaticUnresolved(double)",
                DoPutStaticUnresolvedDouble.staticField, 1000.0d);
        }

        // 1. Resolved & Clinited putstatic:
        {
            Object o1 = new Object();
            doResolvedClinitedObject(o1);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedClinited(object)",
                DoPutStaticResolvedClinitedObject.staticField, o1);
        }
        // 2. Resolved putstatic:
        {
            Object o2 = new Object();
            doResolvedObject(o2);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolved(object)",
                DoPutStaticResolvedObject.staticField, o2);
        }
        // 3. Unresolved putstatic:
        {
            Object o3 = new Object();
            doUnresolvedObject(o3);
            DoResolveAndClinit.reportPassIf(
                "PutStaticUnresolved(object)",
                DoPutStaticUnresolvedObject.staticField, o3);
        }

	//////////////////////////////////////////////////////////////////////
	// Volatile field access:
	//

        // 1. Resolved & Clinited putstatic:
        {
            doResolvedClinitedBooleanVolatile(true);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedClinitedVolatile(boolean)",
                DoPutStaticResolvedClinitedBooleanVolatile.staticField, true);
        }
        // 2. Resolved putstatic:
        {
            doResolvedBooleanVolatile(true);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedVolatile(boolean)",
                DoPutStaticResolvedBooleanVolatile.staticField, true);
        }
        // 3. Unresolved putstatic:
        {
            doUnresolvedBooleanVolatile(true);
            DoResolveAndClinit.reportPassIf(
                "PutStaticUnresolvedVolatile(boolean)",
                DoPutStaticUnresolvedBooleanVolatile.staticField, true);
        }

        // 1. Resolved & Clinited putstatic:
        {
            doResolvedClinitedByteVolatile((byte)5);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedClinitedVolatile(byte)",
                DoPutStaticResolvedClinitedByteVolatile.staticField, (byte)5);
        }
        // 2. Resolved putstatic:
        {
            doResolvedByteVolatile((byte)50);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedVolatile(byte)",
                DoPutStaticResolvedByteVolatile.staticField, (byte)50);
        }
        // 3. Unresolved putstatic:
        {
            doUnresolvedByteVolatile((byte)53);
            DoResolveAndClinit.reportPassIf(
                "PutStaticUnresolvedVolatile(byte)",
                DoPutStaticUnresolvedByteVolatile.staticField, (byte)53);
        }

        // 1. Resolved & Clinited putstatic:
        {
            doResolvedClinitedCharVolatile('A');
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedClinitedVolatile(char)",
                DoPutStaticResolvedClinitedCharVolatile.staticField, 'A');
        }
        // 2. Resolved putstatic:
        {
            doResolvedCharVolatile('B');
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedVolatile(char)",
                DoPutStaticResolvedCharVolatile.staticField, 'B');
        }
        // 3. Unresolved putstatic:
        {
            doUnresolvedCharVolatile('C');
            DoResolveAndClinit.reportPassIf(
                "PutStaticUnresolvedVolatile(char)",
                DoPutStaticUnresolvedCharVolatile.staticField, 'C');
        }

        // 1. Resolved & Clinited putstatic:
        {
            doResolvedClinitedShortVolatile((short)6);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedClinitedVolatile(short)",
                DoPutStaticResolvedClinitedShortVolatile.staticField, (short)6);
        }
        // 2. Resolved putstatic:
        {
            doResolvedShortVolatile((short)60);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedVolatile(short)",
                DoPutStaticResolvedShortVolatile.staticField, (short)60);
        }
        // 3. Unresolved putstatic:
        {
            doUnresolvedShortVolatile((short)600);
            DoResolveAndClinit.reportPassIf(
                "PutStaticUnresolvedVolatile(short)",
                DoPutStaticUnresolvedShortVolatile.staticField, (short)600);
        }

        // 1. Resolved & Clinited putstatic:
        {
            doResolvedClinitedIntVolatile(7);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedClinitedVolatile(int)",
                DoPutStaticResolvedClinitedIntVolatile.staticField, 7);
        }
        // 2. Resolved putstatic:
        {
            doResolvedIntVolatile(70);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedVolatile(int)",
                DoPutStaticResolvedIntVolatile.staticField, 70);
        }
        // 3. Unresolved putstatic:
        {
            doUnresolvedIntVolatile(700);
            DoResolveAndClinit.reportPassIf(
                "PutStaticUnresolvedVolatile(int)",
                DoPutStaticUnresolvedIntVolatile.staticField, 700);
        }

        // 1. Resolved & Clinited putstatic:
        {
            doResolvedClinitedFloatVolatile(8.0f);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedClinitedVolatile(float)",
                DoPutStaticResolvedClinitedFloatVolatile.staticField, 8.0f);
        }
        // 2. Resolved putstatic:
        {
            doResolvedFloatVolatile(80.0f);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedVolatile(float)",
                DoPutStaticResolvedFloatVolatile.staticField, 80.0f);
        }
        // 3. Unresolved putstatic:
        {
            doUnresolvedFloatVolatile(800.0f);
            DoResolveAndClinit.reportPassIf(
                "PutStaticUnresolvedVolatile(float)",
                DoPutStaticUnresolvedFloatVolatile.staticField, 800.0f);
        }

        // 1. Resolved & Clinited putstatic:
        {
            doResolvedClinitedLongVolatile(9l);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedClinitedVolatile(long)",
                DoPutStaticResolvedClinitedLongVolatile.staticField, 9l);
        }
        // 2. Resolved putstatic:
        {
            doResolvedLongVolatile(90l);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedVolatile(long)",
                DoPutStaticResolvedLongVolatile.staticField, 90l);
        }
        // 3. Unresolved putstatic:
        {
            doUnresolvedLongVolatile(900l);
            DoResolveAndClinit.reportPassIf(
                "PutStaticUnresolvedVolatile(long)",
                DoPutStaticUnresolvedLongVolatile.staticField, 900l);
        }

        // 1. Resolved & Clinited putstatic:
        {
            doResolvedClinitedDoubleVolatile(10.0d);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedClinitedVolatile(double)",
                DoPutStaticResolvedClinitedDoubleVolatile.staticField, 10.0d);
        }
        // 2. Resolved putstatic:
        {
            doResolvedDoubleVolatile(100.0d);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedVolatile(double)",
                DoPutStaticResolvedDoubleVolatile.staticField, 100.0d);
        }
        // 3. Unresolved putstatic:
        {
            doUnresolvedDoubleVolatile(1000.0d);
            DoResolveAndClinit.reportPassIf(
                "PutStaticUnresolvedVolatile(double)",
                DoPutStaticUnresolvedDoubleVolatile.staticField, 1000.0d);
        }

        // 1. Resolved & Clinited putstatic:
        {
            Object o1 = new Object();
            doResolvedClinitedObjectVolatile(o1);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedClinitedVolatile(object)",
                DoPutStaticResolvedClinitedObjectVolatile.staticField, o1);
        }
        // 2. Resolved putstatic:
        {
            Object o2 = new Object();
            doResolvedObjectVolatile(o2);
            DoResolveAndClinit.reportPassIf(
                "PutStaticResolvedVolatile(object)",
                DoPutStaticResolvedObjectVolatile.staticField, o2);
        }
        // 3. Unresolved putstatic:
        {
            Object o3 = new Object();
            doUnresolvedObjectVolatile(o3);
            DoResolveAndClinit.reportPassIf(
                "PutStaticUnresolvedVolatile(object)",
                DoPutStaticUnresolvedObjectVolatile.staticField, o3);
        }

    }

    static void doResolvedClinitedBoolean(boolean value) {
        DoPutStaticResolvedClinitedBoolean.staticField = value;
    }
    static void doResolvedBoolean(boolean value) {
        DoPutStaticResolvedBoolean.staticField = value;
    }
    static void doUnresolvedBoolean(boolean value) {
        DoPutStaticUnresolvedBoolean.staticField = value;
    }
    static void doResolvedClinitedByte(byte value) {
        DoPutStaticResolvedClinitedByte.staticField = value;
    }
    static void doResolvedByte(byte value) {
        DoPutStaticResolvedByte.staticField = value;
    }
    static void doUnresolvedByte(byte value) {
        DoPutStaticUnresolvedByte.staticField = value;
    }
    static void doResolvedClinitedChar(char value) {
        DoPutStaticResolvedClinitedChar.staticField = value;
    }
    static void doResolvedChar(char value) {
        DoPutStaticResolvedChar.staticField = value;
    }
    static void doUnresolvedChar(char value) {
        DoPutStaticUnresolvedChar.staticField = value;
    }
    static void doResolvedClinitedShort(short value) {
        DoPutStaticResolvedClinitedShort.staticField = value;
    }
    static void doResolvedShort(short value) {
        DoPutStaticResolvedShort.staticField = value;
    }
    static void doUnresolvedShort(short value) {
        DoPutStaticUnresolvedShort.staticField = value;
    }
    static void doResolvedClinitedInt(int value) {
        DoPutStaticResolvedClinitedInt.staticField = value;
    }
    static void doResolvedInt(int value) {
        DoPutStaticResolvedInt.staticField = value;
    }
    static void doUnresolvedInt(int value) {
        DoPutStaticUnresolvedInt.staticField = value;
    }
    static void doResolvedClinitedFloat(float value) {
        DoPutStaticResolvedClinitedFloat.staticField = value;
    }
    static void doResolvedFloat(float value) {
        DoPutStaticResolvedFloat.staticField = value;
    }
    static void doUnresolvedFloat(float value) {
        DoPutStaticUnresolvedFloat.staticField = value;
    }
    static void doResolvedClinitedLong(long value) {
        DoPutStaticResolvedClinitedLong.staticField = value;
    }
    static void doResolvedLong(long value) {
        DoPutStaticResolvedLong.staticField = value;
    }
    static void doUnresolvedLong(long value) {
        DoPutStaticUnresolvedLong.staticField = value;
    }
    static void doResolvedClinitedDouble(double value) {
        DoPutStaticResolvedClinitedDouble.staticField = value;
    }
    static void doResolvedDouble(double value) {
        DoPutStaticResolvedDouble.staticField = value;
    }
    static void doUnresolvedDouble(double value) {
        DoPutStaticUnresolvedDouble.staticField = value;
    }
    static void doResolvedClinitedObject(Object value) {
        DoPutStaticResolvedClinitedObject.staticField = value;
    }
    static void doResolvedObject(Object value) {
        DoPutStaticResolvedObject.staticField = value;
    }
    static void doUnresolvedObject(Object value) {
        DoPutStaticUnresolvedObject.staticField = value;
    }

    //////////////////////////////////////////////////////////////////////////
    // Volatile field access:
    //

    static void doResolvedClinitedBooleanVolatile(boolean value) {
        DoPutStaticResolvedClinitedBooleanVolatile.staticField = value;
    }
    static void doResolvedBooleanVolatile(boolean value) {
        DoPutStaticResolvedBooleanVolatile.staticField = value;
    }
    static void doUnresolvedBooleanVolatile(boolean value) {
        DoPutStaticUnresolvedBooleanVolatile.staticField = value;
    }
    static void doResolvedClinitedByteVolatile(byte value) {
        DoPutStaticResolvedClinitedByteVolatile.staticField = value;
    }
    static void doResolvedByteVolatile(byte value) {
        DoPutStaticResolvedByteVolatile.staticField = value;
    }
    static void doUnresolvedByteVolatile(byte value) {
        DoPutStaticUnresolvedByteVolatile.staticField = value;
    }
    static void doResolvedClinitedCharVolatile(char value) {
        DoPutStaticResolvedClinitedCharVolatile.staticField = value;
    }
    static void doResolvedCharVolatile(char value) {
        DoPutStaticResolvedCharVolatile.staticField = value;
    }
    static void doUnresolvedCharVolatile(char value) {
        DoPutStaticUnresolvedCharVolatile.staticField = value;
    }
    static void doResolvedClinitedShortVolatile(short value) {
        DoPutStaticResolvedClinitedShortVolatile.staticField = value;
    }
    static void doResolvedShortVolatile(short value) {
        DoPutStaticResolvedShortVolatile.staticField = value;
    }
    static void doUnresolvedShortVolatile(short value) {
        DoPutStaticUnresolvedShortVolatile.staticField = value;
    }
    static void doResolvedClinitedIntVolatile(int value) {
        DoPutStaticResolvedClinitedIntVolatile.staticField = value;
    }
    static void doResolvedIntVolatile(int value) {
        DoPutStaticResolvedIntVolatile.staticField = value;
    }
    static void doUnresolvedIntVolatile(int value) {
        DoPutStaticUnresolvedIntVolatile.staticField = value;
    }
    static void doResolvedClinitedFloatVolatile(float value) {
        DoPutStaticResolvedClinitedFloatVolatile.staticField = value;
    }
    static void doResolvedFloatVolatile(float value) {
        DoPutStaticResolvedFloatVolatile.staticField = value;
    }
    static void doUnresolvedFloatVolatile(float value) {
        DoPutStaticUnresolvedFloatVolatile.staticField = value;
    }
    static void doResolvedClinitedLongVolatile(long value) {
        DoPutStaticResolvedClinitedLongVolatile.staticField = value;
    }
    static void doResolvedLongVolatile(long value) {
        DoPutStaticResolvedLongVolatile.staticField = value;
    }
    static void doUnresolvedLongVolatile(long value) {
        DoPutStaticUnresolvedLongVolatile.staticField = value;
    }
    static void doResolvedClinitedDoubleVolatile(double value) {
        DoPutStaticResolvedClinitedDoubleVolatile.staticField = value;
    }
    static void doResolvedDoubleVolatile(double value) {
        DoPutStaticResolvedDoubleVolatile.staticField = value;
    }
    static void doUnresolvedDoubleVolatile(double value) {
        DoPutStaticUnresolvedDoubleVolatile.staticField = value;
    }
    static void doResolvedClinitedObjectVolatile(Object value) {
        DoPutStaticResolvedClinitedObjectVolatile.staticField = value;
    }
    static void doResolvedObjectVolatile(Object value) {
        DoPutStaticResolvedObjectVolatile.staticField = value;
    }
    static void doUnresolvedObjectVolatile(Object value) {
        DoPutStaticUnresolvedObjectVolatile.staticField = value;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Testing opcode getstatic:
// Note: Resolves to a field address.

/* Boolean: */
class DoGetStaticResolvedClinitedBoolean {
    public static boolean staticField = false;
    static {
        System.out.println("\tDoGetStaticResolvedClinitedBoolean.<clinit>()");
    }
}

class DoGetStaticResolvedBoolean {
    public static boolean staticField = false;
    static {
        System.out.println("\tDoGetStaticResolvedBoolean.<clinit>()");
    }
}
class DoGetStaticUnresolvedBoolean {
    public static boolean staticField = false;
    static {
        System.out.println("\tDoGetStaticUnresolvedBoolean.<clinit>()");
    }
}

/* Byte: */
class DoGetStaticResolvedClinitedByte {
    public static byte staticField = 0;
    static {
        System.out.println("\tDoGetStaticResolvedClinitedByte.<clinit>()");
    }
}

class DoGetStaticResolvedByte {
    public static byte staticField = 0;
    static {
        System.out.println("\tDoGetStaticResolvedByte.<clinit>()");
    }
}
class DoGetStaticUnresolvedByte {
    public static byte staticField = 0;
    static {
        System.out.println("\tDoGetStaticUnresolvedByte.<clinit>()");
    }
}

/* Char: */
class DoGetStaticResolvedClinitedChar {
    public static char staticField = '\0';
    static {
        System.out.println("\tDoGetStaticResolvedClinitedChar.<clinit>()");
    }
}

class DoGetStaticResolvedChar {
    public static char staticField = '\0';
    static {
        System.out.println("\tDoGetStaticResolvedChar.<clinit>()");
    }
}
class DoGetStaticUnresolvedChar {
    public static char staticField = '\0';
    static {
        System.out.println("\tDoGetStaticUnresolvedChar.<clinit>()");
    }
}

/* Short: */
class DoGetStaticResolvedClinitedShort {
    public static short staticField = 0;
    static {
        System.out.println("\tDoGetStaticResolvedClinitedShort.<clinit>()");
    }
}

class DoGetStaticResolvedShort {
    public static short staticField = 0;
    static {
        System.out.println("\tDoGetStaticResolvedShort.<clinit>()");
    }
}
class DoGetStaticUnresolvedShort {
    public static short staticField = 0;
    static {
        System.out.println("\tDoGetStaticUnresolvedShort.<clinit>()");
    }
}

/* Int: */
class DoGetStaticResolvedClinitedInt {
    public static int staticField = 0;
    static {
        System.out.println("\tDoGetStaticResolvedClinitedInt.<clinit>()");
    }
}

class DoGetStaticResolvedInt {
    public static int staticField = 0;
    static {
        System.out.println("\tDoGetStaticResolvedInt.<clinit>()");
    }
}
class DoGetStaticUnresolvedInt {
    public static int staticField = 0;
    static {
        System.out.println("\tDoGetStaticUnresolvedInt.<clinit>()");
    }
}

/* Float: */
class DoGetStaticResolvedClinitedFloat {
    public static float staticField = 0.0f;
    static {
        System.out.println("\tDoGetStaticResolvedClinitedFloat.<clinit>()");
    }
}

class DoGetStaticResolvedFloat {
    public static float staticField = 0.0f;
    static {
        System.out.println("\tDoGetStaticResolvedFloat.<clinit>()");
    }
}
class DoGetStaticUnresolvedFloat {
    public static float staticField = 0.0f;
    static {
        System.out.println("\tDoGetStaticUnresolvedFloat.<clinit>()");
    }
}

/* Long: */
class DoGetStaticResolvedClinitedLong {
    public static long staticField = 0l;
    static {
        System.out.println("\tDoGetStaticResolvedClinitedLong.<clinit>()");
    }
}

class DoGetStaticResolvedLong {
    public static long staticField = 0l;
    static {
        System.out.println("\tDoGetStaticResolvedLong.<clinit>()");
    }
}
class DoGetStaticUnresolvedLong {
    public static long staticField = 0l;
    static {
        System.out.println("\tDoGetStaticUnresolvedLong.<clinit>()");
    }
}

/* Double: */
class DoGetStaticResolvedClinitedDouble {
    public static double staticField = 0.0d;
    static {
        System.out.println("\tDoGetStaticResolvedClinitedDouble.<clinit>()");
    }
}

class DoGetStaticResolvedDouble {
    public static double staticField = 0.0d;
    static {
        System.out.println("\tDoGetStaticResolvedDouble.<clinit>()");
    }
}
class DoGetStaticUnresolvedDouble {
    public static double staticField = 0.0d;
    static {
        System.out.println("\tDoGetStaticUnresolvedDouble.<clinit>()");
    }
}

/* Object: */
class DoGetStaticResolvedClinitedObject {
    public static Object staticField = null;
    static {
        System.out.println("\tDoGetStaticResolvedClinitedObject.<clinit>()");
    }
}

class DoGetStaticResolvedObject {
    public static Object staticField = null;
    static {
        System.out.println("\tDoGetStaticResolvedObject.<clinit>()");
    }
}
class DoGetStaticUnresolvedObject {
    public static Object staticField = null;
    static {
        System.out.println("\tDoGetStaticUnresolvedObject.<clinit>()");
    }
}

/////////////////////////////////////////////////////////////////////////////
// Volatile GetStatic:
//

/* VolatileBoolean: */
class DoGetStaticResolvedClinitedBooleanVolatile {
    public static volatile boolean staticField = false;
    static {
        System.out.println("\tDoGetStaticResolvedClinitedBooleanVolatile.<clinit>()");
    }
}

class DoGetStaticResolvedBooleanVolatile {
    public static volatile boolean staticField = false;
    static {
        System.out.println("\tDoGetStaticResolvedBooleanVolatile.<clinit>()");
    }
}
class DoGetStaticUnresolvedBooleanVolatile {
    public static volatile boolean staticField = false;
    static {
        System.out.println("\tDoGetStaticUnresolvedBooleanVolatile.<clinit>()");
    }
}

/* Volatile Byte: */
class DoGetStaticResolvedClinitedByteVolatile {
    public static volatile byte staticField = 0;
    static {
        System.out.println("\tDoGetStaticResolvedClinitedByteVolatile.<clinit>()");
    }
}

class DoGetStaticResolvedByteVolatile {
    public static volatile byte staticField = 0;
    static {
        System.out.println("\tDoGetStaticResolvedByteVolatile.<clinit>()");
    }
}
class DoGetStaticUnresolvedByteVolatile {
    public static volatile byte staticField = 0;
    static {
        System.out.println("\tDoGetStaticUnresolvedByteVolatile.<clinit>()");
    }
}

/* Volatile Char: */
class DoGetStaticResolvedClinitedCharVolatile {
    public static volatile char staticField = '\0';
    static {
        System.out.println("\tDoGetStaticResolvedClinitedCharVolatile.<clinit>()");
    }
}

class DoGetStaticResolvedCharVolatile {
    public static volatile char staticField = '\0';
    static {
        System.out.println("\tDoGetStaticResolvedCharVolatile.<clinit>()");
    }
}
class DoGetStaticUnresolvedCharVolatile {
    public static volatile char staticField = '\0';
    static {
        System.out.println("\tDoGetStaticUnresolvedCharVolatile.<clinit>()");
    }
}

/* Volatile Short: */
class DoGetStaticResolvedClinitedShortVolatile {
    public static volatile short staticField = 0;
    static {
        System.out.println("\tDoGetStaticResolvedClinitedShortVolatile.<clinit>()");
    }
}

class DoGetStaticResolvedShortVolatile {
    public static volatile short staticField = 0;
    static {
        System.out.println("\tDoGetStaticResolvedShortVolatile.<clinit>()");
    }
}
class DoGetStaticUnresolvedShortVolatile {
    public static volatile short staticField = 0;
    static {
        System.out.println("\tDoGetStaticUnresolvedShortVolatile.<clinit>()");
    }
}

/* Volatile Int: */
class DoGetStaticResolvedClinitedIntVolatile {
    public static volatile int staticField = 0;
    static {
        System.out.println("\tDoGetStaticResolvedClinitedIntVolatile.<clinit>()");
    }
}

class DoGetStaticResolvedIntVolatile {
    public static volatile int staticField = 0;
    static {
        System.out.println("\tDoGetStaticResolvedIntVolatile.<clinit>()");
    }
}
class DoGetStaticUnresolvedIntVolatile {
    public static volatile int staticField = 0;
    static {
        System.out.println("\tDoGetStaticUnresolvedIntVolatile.<clinit>()");
    }
}

/* Volatile Float: */
class DoGetStaticResolvedClinitedFloatVolatile {
    public static volatile float staticField = 0.0f;
    static {
        System.out.println("\tDoGetStaticResolvedClinitedFloatVolatile.<clinit>()");
    }
}

class DoGetStaticResolvedFloatVolatile {
    public static volatile float staticField = 0.0f;
    static {
        System.out.println("\tDoGetStaticResolvedFloatVolatile.<clinit>()");
    }
}
class DoGetStaticUnresolvedFloatVolatile {
    public static volatile float staticField = 0.0f;
    static {
        System.out.println("\tDoGetStaticUnresolvedFloatVolatile.<clinit>()");
    }
}

/* Volatile Long: */
class DoGetStaticResolvedClinitedLongVolatile {
    public static volatile long staticField = 0l;
    static {
        System.out.println("\tDoGetStaticResolvedClinitedLongVolatile.<clinit>()");
    }
}

class DoGetStaticResolvedLongVolatile {
    public static volatile long staticField = 0l;
    static {
        System.out.println("\tDoGetStaticResolvedLongVolatile.<clinit>()");
    }
}
class DoGetStaticUnresolvedLongVolatile {
    public static volatile long staticField = 0l;
    static {
        System.out.println("\tDoGetStaticUnresolvedLongVolatile.<clinit>()");
    }
}

/* Volatile Double: */
class DoGetStaticResolvedClinitedDoubleVolatile {
    public static volatile double staticField = 0.0d;
    static {
        System.out.println("\tDoGetStaticResolvedClinitedDoubleVolatile.<clinit>()");
    }
}

class DoGetStaticResolvedDoubleVolatile {
    public static volatile double staticField = 0.0d;
    static {
        System.out.println("\tDoGetStaticResolvedDoubleVolatile.<clinit>()");
    }
}
class DoGetStaticUnresolvedDoubleVolatile {
    public static volatile double staticField = 0.0d;
    static {
        System.out.println("\tDoGetStaticUnresolvedDoubleVolatile.<clinit>()");
    }
}

/* Volatile Object: */
class DoGetStaticResolvedClinitedObjectVolatile {
    public static volatile Object staticField = null;
    static {
        System.out.println("\tDoGetStaticResolvedClinitedObjectVolatile.<clinit>()");
    }
}

class DoGetStaticResolvedObjectVolatile {
    public static volatile Object staticField = null;
    static {
        System.out.println("\tDoGetStaticResolvedObjectVolatile.<clinit>()");
    }
}
class DoGetStaticUnresolvedObjectVolatile {
    public static volatile Object staticField = null;
    static {
        System.out.println("\tDoGetStaticUnresolvedObjectVolatile.<clinit>()");
    }
}


class DoGetStatic
{
    static final String[] compileItems = {
        "DoGetStatic.doResolvedClinitedBoolean()Z",
        "DoGetStatic.doResolvedBoolean()Z",
        "DoGetStatic.doUnresolvedBoolean()Z",

        "DoGetStatic.doResolvedClinitedByte()B",
        "DoGetStatic.doResolvedByte()B",
        "DoGetStatic.doUnresolvedByte()B",

        "DoGetStatic.doResolvedClinitedChar()C",
        "DoGetStatic.doResolvedChar()C",
        "DoGetStatic.doUnresolvedChar()C",

        "DoGetStatic.doResolvedClinitedShort()S",
        "DoGetStatic.doResolvedShort()S",
        "DoGetStatic.doUnresolvedShort()S",

        "DoGetStatic.doResolvedClinitedInt()I",
        "DoGetStatic.doResolvedInt()I",
        "DoGetStatic.doUnresolvedInt()I",

        "DoGetStatic.doResolvedClinitedFloat()F",
        "DoGetStatic.doResolvedFloat()F",
        "DoGetStatic.doUnresolvedFloat()F",

        "DoGetStatic.doResolvedClinitedLong()J",
        "DoGetStatic.doResolvedLong()J",
        "DoGetStatic.doUnresolvedLong()J",

        "DoGetStatic.doResolvedClinitedDouble()D",
        "DoGetStatic.doResolvedDouble()D",
        "DoGetStatic.doUnresolvedDouble()D",

        "DoGetStatic.doResolvedClinitedObject()Ljava/lang/Object;",
        "DoGetStatic.doResolvedObject()Ljava/lang/Object;",
        "DoGetStatic.doUnresolvedObject()Ljava/lang/Object;",

	// Volatile set:
        "DoGetStatic.doResolvedClinitedBooleanVolatile()Z",
        "DoGetStatic.doResolvedBooleanVolatile()Z",
        "DoGetStatic.doUnresolvedBooleanVolatile()Z",

        "DoGetStatic.doResolvedClinitedByteVolatile()B",
        "DoGetStatic.doResolvedByteVolatile()B",
        "DoGetStatic.doUnresolvedByteVolatile()B",

        "DoGetStatic.doResolvedClinitedCharVolatile()C",
        "DoGetStatic.doResolvedCharVolatile()C",
        "DoGetStatic.doUnresolvedCharVolatile()C",

        "DoGetStatic.doResolvedClinitedShortVolatile()S",
        "DoGetStatic.doResolvedShortVolatile()S",
        "DoGetStatic.doUnresolvedShortVolatile()S",

        "DoGetStatic.doResolvedClinitedIntVolatile()I",
        "DoGetStatic.doResolvedIntVolatile()I",
        "DoGetStatic.doUnresolvedIntVolatile()I",

        "DoGetStatic.doResolvedClinitedFloatVolatile()F",
        "DoGetStatic.doResolvedFloatVolatile()F",
        "DoGetStatic.doUnresolvedFloatVolatile()F",

        "DoGetStatic.doResolvedClinitedLongVolatile()J",
        "DoGetStatic.doResolvedLongVolatile()J",
        "DoGetStatic.doUnresolvedLongVolatile()J",

        "DoGetStatic.doResolvedClinitedDoubleVolatile()D",
        "DoGetStatic.doResolvedDoubleVolatile()D",
        "DoGetStatic.doUnresolvedDoubleVolatile()D",

        "DoGetStatic.doResolvedClinitedObjectVolatile()Ljava/lang/Object;",
        "DoGetStatic.doResolvedObjectVolatile()Ljava/lang/Object;",
        "DoGetStatic.doUnresolvedObjectVolatile()Ljava/lang/Object;",
    };

    public static void doSetup() {
        // Setup initial conditions:
        Object o = new Object();
        {
            // Resolve and clinit it:
            boolean i = DoGetStaticResolvedClinitedBoolean.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoGetStaticResolvedBoolean;
        }
        {
            // Resolve and clinit it:
            byte i = DoGetStaticResolvedClinitedByte.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoGetStaticResolvedByte;
        }
        {
            // Resolve and clinit it:
            char i = DoGetStaticResolvedClinitedChar.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoGetStaticResolvedChar;
        }
        {
            // Resolve and clinit it:
            short i = DoGetStaticResolvedClinitedShort.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoGetStaticResolvedShort;
        }
        {
            // Resolve and clinit it:
            int i = DoGetStaticResolvedClinitedInt.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoGetStaticResolvedInt;
        }
        {
            // Resolve and clinit it:
            float i = DoGetStaticResolvedClinitedFloat.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoGetStaticResolvedFloat;
        }
        {
            // Resolve and clinit it:
            long i = DoGetStaticResolvedClinitedLong.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoGetStaticResolvedLong;
        }
        {
            // Resolve and clinit it:
            double i = DoGetStaticResolvedClinitedDouble.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoGetStaticResolvedDouble;
        }
        {
            // Resolve and clinit it:
            Object o1 = DoGetStaticResolvedClinitedObject.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoGetStaticResolvedObject;
        }

        // Setup initial conditions for Volatile set:
        {
            // Resolve and clinit it:
            boolean i = DoGetStaticResolvedClinitedBooleanVolatile.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoGetStaticResolvedBooleanVolatile;
        }
        {
            // Resolve and clinit it:
            byte i = DoGetStaticResolvedClinitedByteVolatile.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoGetStaticResolvedByteVolatile;
        }
        {
            // Resolve and clinit it:
            char i = DoGetStaticResolvedClinitedCharVolatile.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoGetStaticResolvedCharVolatile;
        }
        {
            // Resolve and clinit it:
            short i = DoGetStaticResolvedClinitedShortVolatile.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoGetStaticResolvedShortVolatile;
        }
        {
            // Resolve and clinit it:
            int i = DoGetStaticResolvedClinitedIntVolatile.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoGetStaticResolvedIntVolatile;
        }
        {
            // Resolve and clinit it:
            float i = DoGetStaticResolvedClinitedFloatVolatile.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoGetStaticResolvedFloatVolatile;
        }
        {
            // Resolve and clinit it:
            long i = DoGetStaticResolvedClinitedLongVolatile.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoGetStaticResolvedLongVolatile;
        }
        {
            // Resolve and clinit it:
            double i = DoGetStaticResolvedClinitedDoubleVolatile.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoGetStaticResolvedDoubleVolatile;
        }
        {
            // Resolve and clinit it:
            Object o1 = DoGetStaticResolvedClinitedObjectVolatile.staticField;
            // Resolve but don't clinit it:
            boolean b = o instanceof DoGetStaticResolvedObjectVolatile;
        }

        // Do compilation:
        CompilerTest.main(compileItems);
    }

    public static void doTest() {

        // 1. Resolved & Clinited getstatic:
        {
            DoGetStaticResolvedClinitedBoolean.staticField = true;
            boolean value = doResolvedClinitedBoolean();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedClinited(boolean)", value, true);
        }
        // 2. Resolved getstatic:
        {
            DoGetStaticResolvedBoolean.staticField = true;
            boolean value = doResolvedBoolean();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolved(boolean)", value, true);
        }
        // 3. Unresolved getstatic:
        {
            DoGetStaticUnresolvedBoolean.staticField = true;
            boolean value = doUnresolvedBoolean();
            DoResolveAndClinit.reportPassIf(
                "GetStaticUnresolved(boolean)", value, true);
        }

        // 1. Resolved & Clinited getstatic:
        {
            DoGetStaticResolvedClinitedByte.staticField = (byte)5;
            byte value = doResolvedClinitedByte();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedClinited(byte)", value, (byte)5);
        }
        // 2. Resolved getstatic:
        {
            DoGetStaticResolvedByte.staticField = (byte)50;
            byte value = doResolvedByte();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolved(byte)", value, (byte)50);
        }
        // 3. Unresolved getstatic:
        {
            DoGetStaticUnresolvedByte.staticField = (byte)53;
            byte value = doUnresolvedByte();
            DoResolveAndClinit.reportPassIf(
                "GetStaticUnresolved(byte)", value, (byte)53);
        }

        // 1. Resolved & Clinited getstatic:
        {
            DoGetStaticResolvedClinitedChar.staticField = 'A';
            char value = doResolvedClinitedChar();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedClinited(char)", value, 'A');
        }
        // 2. Resolved getstatic:
        {
            DoGetStaticResolvedChar.staticField = 'B';
            char value = doResolvedChar();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolved(char)", value, 'B');
        }
        // 3. Unresolved getstatic:
        {
            DoGetStaticUnresolvedChar.staticField = 'C';
            char value = doUnresolvedChar();
            DoResolveAndClinit.reportPassIf(
                "GetStaticUnresolved(char)", value, 'C');
        }

        // 1. Resolved & Clinited getstatic:
        {
            DoGetStaticResolvedClinitedShort.staticField = (short)6;
            short value = doResolvedClinitedShort();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedClinited(short)", value, (short)6);
        }
        // 2. Resolved getstatic:
        {
            DoGetStaticResolvedShort.staticField = (short)60;
            short value = doResolvedShort();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolved(short)", value, (short)60);
        }
        // 3. Unresolved getstatic:
        {
            DoGetStaticUnresolvedShort.staticField = (short)600;
            short value = doUnresolvedShort();
            DoResolveAndClinit.reportPassIf(
                "GetStaticUnresolved(short)", value, (short)600);
        }

        // 1. Resolved & Clinited getstatic:
        {
            DoGetStaticResolvedClinitedInt.staticField = 7;
            int value = doResolvedClinitedInt();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedClinited(int)", value, 7);
        }
        // 2. Resolved getstatic:
        {
            DoGetStaticResolvedInt.staticField = 70;
            int value = doResolvedInt();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolved(int)", value, 70);
        }
        // 3. Unresolved getstatic:
        {
            DoGetStaticUnresolvedInt.staticField = 700;
            int value = doUnresolvedInt();
            DoResolveAndClinit.reportPassIf(
                "GetStaticUnresolved(int)", value, 700);
        }

        // 1. Resolved & Clinited getstatic:
        {
            DoGetStaticResolvedClinitedFloat.staticField = 8.0f;
            float value = doResolvedClinitedFloat();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedClinited(float)", value, 8.0f);
        }
        // 2. Resolved getstatic:
        {
            DoGetStaticResolvedFloat.staticField = 80.0f;
            float value = doResolvedFloat();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolved(float)", value, 80.0f);
        }
        // 3. Unresolved getstatic:
        {
            DoGetStaticUnresolvedFloat.staticField = 800.0f;
            float value = doUnresolvedFloat();
            DoResolveAndClinit.reportPassIf(
                "GetStaticUnresolved(float)", value, 800.0f);
        }

        // 1. Resolved & Clinited getstatic:
        {
            DoGetStaticResolvedClinitedLong.staticField = 9l;
            long value = doResolvedClinitedLong();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedClinited(long)", value, 9l);
        }
        // 2. Resolved getstatic:
        {
            DoGetStaticResolvedLong.staticField = 90l;
            long value = doResolvedLong();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolved(long)", value, 90l);
        }
        // 3. Unresolved getstatic:
        {
            DoGetStaticUnresolvedLong.staticField = 900l;
            long value = doUnresolvedLong();
            DoResolveAndClinit.reportPassIf(
                "GetStaticUnresolved(long)", value, 900l);
        }

        // 1. Resolved & Clinited getstatic:
        {
            DoGetStaticResolvedClinitedDouble.staticField = 10.0d;
            double value = doResolvedClinitedDouble();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedClinited(double)", value, 10.0d);
        }
        // 2. Resolved getstatic:
        {
            DoGetStaticResolvedDouble.staticField = 100.0d;
            double value = doResolvedDouble();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolved(double)", value, 100.0d);
        }
        // 3. Unresolved getstatic:
        {
            DoGetStaticUnresolvedDouble.staticField = 1000.0d;
            double value = doUnresolvedDouble();
            DoResolveAndClinit.reportPassIf(
                "GetStaticUnresolved(double)", value, 1000.0d);
        }

        // 1. Resolved & Clinited getstatic:
        {
            Object o1 = new Object();
            DoGetStaticResolvedClinitedObject.staticField = o1;
            Object value = doResolvedClinitedObject();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedClinited(object)", value, o1);
        }
        // 2. Resolved getstatic:
        {
            Object o2 = new Object();
            DoGetStaticResolvedObject.staticField = o2;
            Object value = doResolvedObject();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolved(object)", value, o2);
        }
        // 3. Unresolved getstatic:
        {
            Object o3 = new Object();
            DoGetStaticUnresolvedObject.staticField = o3;
            Object value = doUnresolvedObject();
            DoResolveAndClinit.reportPassIf(
                "GetStaticUnresolved(object)", value, o3);
        }

	///////////////////////////////////////////////////////////////
	// Volatile Set:

        // 1. Resolved & Clinited getstatic:
        {
            DoGetStaticResolvedClinitedBooleanVolatile.staticField = true;
            boolean value = doResolvedClinitedBooleanVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedClinitedVolatile(boolean)", value, true);
        }
        // 2. Resolved getstatic:
        {
            DoGetStaticResolvedBooleanVolatile.staticField = true;
            boolean value = doResolvedBooleanVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedVolatile(boolean)", value, true);
        }
        // 3. Unresolved getstatic:
        {
            DoGetStaticUnresolvedBooleanVolatile.staticField = true;
            boolean value = doUnresolvedBooleanVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticUnresolvedVolatile(boolean)", value, true);
        }

        // 1. Resolved & Clinited getstatic:
        {
            DoGetStaticResolvedClinitedByteVolatile.staticField = (byte)5;
            byte value = doResolvedClinitedByteVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedClinitedVolatile(byte)", value, (byte)5);
        }
        // 2. Resolved getstatic:
        {
            DoGetStaticResolvedByteVolatile.staticField = (byte)50;
            byte value = doResolvedByteVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedVolatile(byte)", value, (byte)50);
        }
        // 3. Unresolved getstatic:
        {
            DoGetStaticUnresolvedByteVolatile.staticField = (byte)53;
            byte value = doUnresolvedByteVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticUnresolvedVolatile(byte)", value, (byte)53);
        }

        // 1. Resolved & Clinited getstatic:
        {
            DoGetStaticResolvedClinitedCharVolatile.staticField = 'A';
            char value = doResolvedClinitedCharVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedClinitedVolatile(char)", value, 'A');
        }
        // 2. Resolved getstatic:
        {
            DoGetStaticResolvedCharVolatile.staticField = 'B';
            char value = doResolvedCharVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedVolatile(char)", value, 'B');
        }
        // 3. Unresolved getstatic:
        {
            DoGetStaticUnresolvedCharVolatile.staticField = 'C';
            char value = doUnresolvedCharVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticUnresolvedVolatile(char)", value, 'C');
        }

        // 1. Resolved & Clinited getstatic:
        {
            DoGetStaticResolvedClinitedShortVolatile.staticField = (short)6;
            short value = doResolvedClinitedShortVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedClinitedVolatile(short)", value, (short)6);
        }
        // 2. Resolved getstatic:
        {
            DoGetStaticResolvedShortVolatile.staticField = (short)60;
            short value = doResolvedShortVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedVolatile(short)", value, (short)60);
        }
        // 3. Unresolved getstatic:
        {
            DoGetStaticUnresolvedShortVolatile.staticField = (short)600;
            short value = doUnresolvedShortVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticUnresolvedVolatile(short)", value, (short)600);
        }

        // 1. Resolved & Clinited getstatic:
        {
            DoGetStaticResolvedClinitedIntVolatile.staticField = 7;
            int value = doResolvedClinitedIntVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedClinitedVolatile(int)", value, 7);
        }
        // 2. Resolved getstatic:
        {
            DoGetStaticResolvedIntVolatile.staticField = 70;
            int value = doResolvedIntVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedVolatile(int)", value, 70);
        }
        // 3. Unresolved getstatic:
        {
            DoGetStaticUnresolvedIntVolatile.staticField = 700;
            int value = doUnresolvedIntVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticUnresolvedVolatile(int)", value, 700);
        }

        // 1. Resolved & Clinited getstatic:
        {
            DoGetStaticResolvedClinitedFloatVolatile.staticField = 8.0f;
            float value = doResolvedClinitedFloatVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedClinitedVolatile(float)", value, 8.0f);
        }
        // 2. Resolved getstatic:
        {
            DoGetStaticResolvedFloatVolatile.staticField = 80.0f;
            float value = doResolvedFloatVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedVolatile(float)", value, 80.0f);
        }
        // 3. Unresolved getstatic:
        {
            DoGetStaticUnresolvedFloatVolatile.staticField = 800.0f;
            float value = doUnresolvedFloatVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticUnresolvedVolatile(float)", value, 800.0f);
        }

        // 1. Resolved & Clinited getstatic:
        {
            DoGetStaticResolvedClinitedLongVolatile.staticField = 9l;
            long value = doResolvedClinitedLongVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedClinitedVolatile(long)", value, 9l);
        }
        // 2. Resolved getstatic:
        {
            DoGetStaticResolvedLongVolatile.staticField = 90l;
            long value = doResolvedLongVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedVolatile(long)", value, 90l);
        }
        // 3. Unresolved getstatic:
        {
            DoGetStaticUnresolvedLongVolatile.staticField = 900l;
            long value = doUnresolvedLongVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticUnresolvedVolatile(long)", value, 900l);
        }

        // 1. Resolved & Clinited getstatic:
        {
            DoGetStaticResolvedClinitedDoubleVolatile.staticField = 10.0d;
            double value = doResolvedClinitedDoubleVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedClinitedVolatile(double)", value, 10.0d);
        }
        // 2. Resolved getstatic:
        {
            DoGetStaticResolvedDoubleVolatile.staticField = 100.0d;
            double value = doResolvedDoubleVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedVolatile(double)", value, 100.0d);
        }
        // 3. Unresolved getstatic:
        {
            DoGetStaticUnresolvedDoubleVolatile.staticField = 1000.0d;
            double value = doUnresolvedDoubleVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticUnresolvedVolatile(double)", value, 1000.0d);
        }

        // 1. Resolved & Clinited getstatic:
        {
            Object o1 = new Object();
            DoGetStaticResolvedClinitedObjectVolatile.staticField = o1;
            Object value = doResolvedClinitedObjectVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedClinitedVolatile(object)", value, o1);
        }
        // 2. Resolved getstatic:
        {
            Object o2 = new Object();
            DoGetStaticResolvedObjectVolatile.staticField = o2;
            Object value = doResolvedObjectVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticResolvedVolatile(object)", value, o2);
        }
        // 3. Unresolved getstatic:
        {
            Object o3 = new Object();
            DoGetStaticUnresolvedObjectVolatile.staticField = o3;
            Object value = doUnresolvedObjectVolatile();
            DoResolveAndClinit.reportPassIf(
                "GetStaticUnresolvedVolatile(object)", value, o3);
        }

    }

    static boolean doResolvedClinitedBoolean() {
        return DoGetStaticResolvedClinitedBoolean.staticField;
    }
    static boolean doResolvedBoolean() {
        return DoGetStaticResolvedBoolean.staticField;
    }
    static boolean doUnresolvedBoolean() {
        return DoGetStaticUnresolvedBoolean.staticField;
    }
    static byte doResolvedClinitedByte() {
        return DoGetStaticResolvedClinitedByte.staticField;
    }
    static byte doResolvedByte() {
        return DoGetStaticResolvedByte.staticField;
    }
    static byte doUnresolvedByte() {
        return DoGetStaticUnresolvedByte.staticField;
    }
    static char doResolvedClinitedChar() {
        return DoGetStaticResolvedClinitedChar.staticField;
    }
    static char doResolvedChar() {
        return DoGetStaticResolvedChar.staticField;
    }
    static char doUnresolvedChar() {
        return DoGetStaticUnresolvedChar.staticField;
    }
    static short doResolvedClinitedShort() {
        return DoGetStaticResolvedClinitedShort.staticField;
    }
    static short doResolvedShort() {
        return DoGetStaticResolvedShort.staticField;
    }
    static short doUnresolvedShort() {
        return DoGetStaticUnresolvedShort.staticField;
    }
    static int doResolvedClinitedInt() {
        return DoGetStaticResolvedClinitedInt.staticField;
    }
    static int doResolvedInt() {
        return DoGetStaticResolvedInt.staticField;
    }
    static int doUnresolvedInt() {
        return DoGetStaticUnresolvedInt.staticField;
    }
    static float doResolvedClinitedFloat() {
        return DoGetStaticResolvedClinitedFloat.staticField;
    }
    static float doResolvedFloat() {
        return DoGetStaticResolvedFloat.staticField;
    }
    static float doUnresolvedFloat() {
        return DoGetStaticUnresolvedFloat.staticField;
    }
    static long doResolvedClinitedLong() {
        return DoGetStaticResolvedClinitedLong.staticField;
    }
    static long doResolvedLong() {
        return DoGetStaticResolvedLong.staticField;
    }
    static long doUnresolvedLong() {
        return DoGetStaticUnresolvedLong.staticField;
    }
    static double doResolvedClinitedDouble() {
        return DoGetStaticResolvedClinitedDouble.staticField;
    }
    static double doResolvedDouble() {
        return DoGetStaticResolvedDouble.staticField;
    }
    static double doUnresolvedDouble() {
        return DoGetStaticUnresolvedDouble.staticField;
    }
    static Object doResolvedClinitedObject() {
        return DoGetStaticResolvedClinitedObject.staticField;
    }
    static Object doResolvedObject() {
        return DoGetStaticResolvedObject.staticField;
    }
    static Object doUnresolvedObject() {
        return DoGetStaticUnresolvedObject.staticField;
    }

    ////////////////////////////////////////////////////////////////////
    // Volatile Set:

    static boolean doResolvedClinitedBooleanVolatile() {
        return DoGetStaticResolvedClinitedBooleanVolatile.staticField;
    }
    static boolean doResolvedBooleanVolatile() {
        return DoGetStaticResolvedBooleanVolatile.staticField;
    }
    static boolean doUnresolvedBooleanVolatile() {
        return DoGetStaticUnresolvedBooleanVolatile.staticField;
    }
    static byte doResolvedClinitedByteVolatile() {
        return DoGetStaticResolvedClinitedByteVolatile.staticField;
    }
    static byte doResolvedByteVolatile() {
        return DoGetStaticResolvedByteVolatile.staticField;
    }
    static byte doUnresolvedByteVolatile() {
        return DoGetStaticUnresolvedByteVolatile.staticField;
    }
    static char doResolvedClinitedCharVolatile() {
        return DoGetStaticResolvedClinitedCharVolatile.staticField;
    }
    static char doResolvedCharVolatile() {
        return DoGetStaticResolvedCharVolatile.staticField;
    }
    static char doUnresolvedCharVolatile() {
        return DoGetStaticUnresolvedCharVolatile.staticField;
    }
    static short doResolvedClinitedShortVolatile() {
        return DoGetStaticResolvedClinitedShortVolatile.staticField;
    }
    static short doResolvedShortVolatile() {
        return DoGetStaticResolvedShortVolatile.staticField;
    }
    static short doUnresolvedShortVolatile() {
        return DoGetStaticUnresolvedShortVolatile.staticField;
    }
    static int doResolvedClinitedIntVolatile() {
        return DoGetStaticResolvedClinitedIntVolatile.staticField;
    }
    static int doResolvedIntVolatile() {
        return DoGetStaticResolvedIntVolatile.staticField;
    }
    static int doUnresolvedIntVolatile() {
        return DoGetStaticUnresolvedIntVolatile.staticField;
    }
    static float doResolvedClinitedFloatVolatile() {
        return DoGetStaticResolvedClinitedFloatVolatile.staticField;
    }
    static float doResolvedFloatVolatile() {
        return DoGetStaticResolvedFloatVolatile.staticField;
    }
    static float doUnresolvedFloatVolatile() {
        return DoGetStaticUnresolvedFloatVolatile.staticField;
    }
    static long doResolvedClinitedLongVolatile() {
        return DoGetStaticResolvedClinitedLongVolatile.staticField;
    }
    static long doResolvedLongVolatile() {
        return DoGetStaticResolvedLongVolatile.staticField;
    }
    static long doUnresolvedLongVolatile() {
        return DoGetStaticUnresolvedLongVolatile.staticField;
    }
    static double doResolvedClinitedDoubleVolatile() {
        return DoGetStaticResolvedClinitedDoubleVolatile.staticField;
    }
    static double doResolvedDoubleVolatile() {
        return DoGetStaticResolvedDoubleVolatile.staticField;
    }
    static double doUnresolvedDoubleVolatile() {
        return DoGetStaticUnresolvedDoubleVolatile.staticField;
    }
    static Object doResolvedClinitedObjectVolatile() {
        return DoGetStaticResolvedClinitedObjectVolatile.staticField;
    }
    static Object doResolvedObjectVolatile() {
        return DoGetStaticResolvedObjectVolatile.staticField;
    }
    static Object doUnresolvedObjectVolatile() {
        return DoGetStaticUnresolvedObjectVolatile.staticField;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Testing opcode putfield:
// Note: Resolves to a field address.

/* Boolean: */
class DoPutFieldResolvedBoolean {
    public boolean field = false;
}
class DoPutFieldUnresolvedBoolean {
    public boolean field = false;
}

/* Byte: */
class DoPutFieldResolvedByte {
    public byte field = 0;
}
class DoPutFieldUnresolvedByte {
    public byte field = 0;
}

/* Char: */
class DoPutFieldResolvedChar {
    public char field = '\0';
}
class DoPutFieldUnresolvedChar {
    public char field = '\0';
}

/* Short: */
class DoPutFieldResolvedShort {
    public short field = 0;
}
class DoPutFieldUnresolvedShort {
    public short field = 0;
}

/* Int: */
class DoPutFieldResolvedInt {
    public int field = 0;
}
class DoPutFieldUnresolvedInt {
    public int field = 0;
}

/* Float: */
class DoPutFieldResolvedFloat {
    public float field = 0.0f;
}
class DoPutFieldUnresolvedFloat {
    public float field = 0.0f;
}

/* Long: */
class DoPutFieldResolvedLong {
    public long field = 0l;
}
class DoPutFieldUnresolvedLong {
    public long field = 0l;
}

/* Double: */
class DoPutFieldResolvedDouble {
    public double field = 0.0d;
}
class DoPutFieldUnresolvedDouble {
    public double field = 0.0d;
}

/* Object: */
class DoPutFieldResolvedObject {
    public Object field = null;
}
class DoPutFieldUnresolvedObject {
    public Object field = null;
}

// Volatile Set:

/* Volatile Boolean: */
class DoPutFieldResolvedBooleanVolatile {
    public volatile boolean field = false;
}
class DoPutFieldUnresolvedBooleanVolatile {
    public volatile boolean field = false;
}

/* Volatile Byte: */
class DoPutFieldResolvedByteVolatile {
    public volatile byte field = 0;
}
class DoPutFieldUnresolvedByteVolatile {
    public volatile byte field = 0;
}

/* Volatile Char: */
class DoPutFieldResolvedCharVolatile {
    public volatile char field = '\0';
}
class DoPutFieldUnresolvedCharVolatile {
    public volatile char field = '\0';
}

/* Volatile Short: */
class DoPutFieldResolvedShortVolatile {
    public volatile short field = 0;
}
class DoPutFieldUnresolvedShortVolatile {
    public volatile short field = 0;
}

/* Volatile Int: */
class DoPutFieldResolvedIntVolatile {
    public volatile int field = 0;
}
class DoPutFieldUnresolvedIntVolatile {
    public volatile int field = 0;
}

/* Volatile Float: */
class DoPutFieldResolvedFloatVolatile {
    public volatile float field = 0.0f;
}
class DoPutFieldUnresolvedFloatVolatile {
    public volatile float field = 0.0f;
}

/* Volatile Long: */
class DoPutFieldResolvedLongVolatile {
    public volatile long field = 0l;
}
class DoPutFieldUnresolvedLongVolatile {
    public volatile long field = 0l;
}

/* Volatile Double: */
class DoPutFieldResolvedDoubleVolatile {
    public volatile double field = 0.0d;
}
class DoPutFieldUnresolvedDoubleVolatile {
    public volatile double field = 0.0d;
}

/* Volatile Object: */
class DoPutFieldResolvedObjectVolatile {
    public volatile Object field = null;
}
class DoPutFieldUnresolvedObjectVolatile {
    public volatile Object field = null;
}


class DoPutField
{
    static final String[] compileItems = {
        "DoPutField.doResolvedBoolean(LDoPutFieldResolvedBoolean;Z)V",
        "DoPutField.doUnresolvedBoolean(LDoPutFieldUnresolvedBoolean;Z)V",

        "DoPutField.doResolvedByte(LDoPutFieldResolvedByte;B)V",
        "DoPutField.doUnresolvedByte(LDoPutFieldUnresolvedByte;B)V",

        "DoPutField.doResolvedChar(LDoPutFieldResolvedChar;C)V",
        "DoPutField.doUnresolvedChar(LDoPutFieldUnresolvedChar;C)V",

        "DoPutField.doResolvedShort(LDoPutFieldResolvedShort;S)V",
        "DoPutField.doUnresolvedShort(LDoPutFieldUnresolvedShort;S)V",

        "DoPutField.doResolvedInt(LDoPutFieldResolvedInt;I)V",
        "DoPutField.doUnresolvedInt(LDoPutFieldUnresolvedInt;I)V",

        "DoPutField.doResolvedFloat(LDoPutFieldResolvedFloat;F)V",
        "DoPutField.doUnresolvedFloat(LDoPutFieldUnresolvedFloat;F)V",

        "DoPutField.doResolvedLong(LDoPutFieldResolvedLong;J)V",
        "DoPutField.doUnresolvedLong(LDoPutFieldUnresolvedLong;J)V",

        "DoPutField.doResolvedDouble(LDoPutFieldResolvedDouble;D)V",
        "DoPutField.doUnresolvedDouble(LDoPutFieldUnresolvedDouble;D)V",

        "DoPutField.doResolvedObject(LDoPutFieldResolvedObject;Ljava/lang/Object;)V",
        "DoPutField.doUnresolvedObject(LDoPutFieldUnresolvedObject;Ljava/lang/Object;)V",

	// Volatile Set:
        "DoPutField.doResolvedBooleanVolatile(LDoPutFieldResolvedBooleanVolatile;Z)V",
        "DoPutField.doUnresolvedBooleanVolatile(LDoPutFieldUnresolvedBooleanVolatile;Z)V",

        "DoPutField.doResolvedByteVolatile(LDoPutFieldResolvedByteVolatile;B)V",
        "DoPutField.doUnresolvedByteVolatile(LDoPutFieldUnresolvedByteVolatile;B)V",

        "DoPutField.doResolvedCharVolatile(LDoPutFieldResolvedCharVolatile;C)V",
        "DoPutField.doUnresolvedCharVolatile(LDoPutFieldUnresolvedCharVolatile;C)V",

        "DoPutField.doResolvedShortVolatile(LDoPutFieldResolvedShortVolatile;S)V",
        "DoPutField.doUnresolvedShortVolatile(LDoPutFieldUnresolvedShortVolatile;S)V",

        "DoPutField.doResolvedIntVolatile(LDoPutFieldResolvedIntVolatile;I)V",
        "DoPutField.doUnresolvedIntVolatile(LDoPutFieldUnresolvedIntVolatile;I)V",

        "DoPutField.doResolvedFloatVolatile(LDoPutFieldResolvedFloatVolatile;F)V",
        "DoPutField.doUnresolvedFloatVolatile(LDoPutFieldUnresolvedFloatVolatile;F)V",

        "DoPutField.doResolvedLongVolatile(LDoPutFieldResolvedLongVolatile;J)V",
        "DoPutField.doUnresolvedLongVolatile(LDoPutFieldUnresolvedLongVolatile;J)V",

        "DoPutField.doResolvedDoubleVolatile(LDoPutFieldResolvedDoubleVolatile;D)V",
        "DoPutField.doUnresolvedDoubleVolatile(LDoPutFieldUnresolvedDoubleVolatile;D)V",

        "DoPutField.doResolvedObjectVolatile(LDoPutFieldResolvedObjectVolatile;Ljava/lang/Object;)V",
        "DoPutField.doUnresolvedObjectVolatile(LDoPutFieldUnresolvedObjectVolatile;Ljava/lang/Object;)V",
    };

    public static void doSetup() {
        // Setup initial conditions:
        Object o = new Object();
        {
            // Resolve it:
            boolean i = new DoPutFieldResolvedBoolean().field;
        }
        {
            // Resolve it:
            byte i = new DoPutFieldResolvedByte().field;
        }
        {
            // Resolve it:
            char i = new DoPutFieldResolvedChar().field;
        }
        {
            // Resolve it:
            short i = new DoPutFieldResolvedShort().field;
        }
        {
            // Resolve it:
            int i = new DoPutFieldResolvedInt().field;
        }
        {
            // Resolve it:
            float i = new DoPutFieldResolvedFloat().field;
        }
        {
            // Resolve it:
            long i = new DoPutFieldResolvedLong().field;
        }
        {
            // Resolve it:
            double i = new DoPutFieldResolvedDouble().field;
        }
        {
            // Resolve it:
            Object o1 = new DoPutFieldResolvedObject().field;
        }

	// Volatile Set:
        {
            // Resolve it:
            boolean i = new DoPutFieldResolvedBooleanVolatile().field;
        }
        {
            // Resolve it:
            byte i = new DoPutFieldResolvedByteVolatile().field;
        }
        {
            // Resolve it:
            char i = new DoPutFieldResolvedCharVolatile().field;
        }
        {
            // Resolve it:
            short i = new DoPutFieldResolvedShortVolatile().field;
        }
        {
            // Resolve it:
            int i = new DoPutFieldResolvedIntVolatile().field;
        }
        {
            // Resolve it:
            float i = new DoPutFieldResolvedFloatVolatile().field;
        }
        {
            // Resolve it:
            long i = new DoPutFieldResolvedLongVolatile().field;
        }
        {
            // Resolve it:
            double i = new DoPutFieldResolvedDoubleVolatile().field;
        }
        {
            // Resolve it:
            Object o1 = new DoPutFieldResolvedObjectVolatile().field;
        }

        // Do compilation:
        CompilerTest.main(compileItems);
    }

    public static void doTest() {

	////////////////////////////////////////////////////////////////
	// Non-Volatile Set:

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedBoolean pf = new DoPutFieldResolvedBoolean();
            doResolvedBoolean(pf, true);
            DoResolveAndClinit.reportPassIf(
                "PutFieldResolved(boolean)", pf.field, true);
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedBoolean pf = new DoPutFieldUnresolvedBoolean();
            doUnresolvedBoolean(pf, true);
            DoResolveAndClinit.reportPassIf(
                "PutFieldUnresolved(boolean)", pf.field, true);
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedByte pf = new DoPutFieldResolvedByte();
            doResolvedByte(pf, (byte)50);
            DoResolveAndClinit.reportPassIf(
                "PutFieldResolved(byte)", pf.field, (byte)50);
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedByte pf = new DoPutFieldUnresolvedByte();
            doUnresolvedByte(pf, (byte)53);
            DoResolveAndClinit.reportPassIf(
                "PutFieldUnresolved(byte)", pf.field, (byte)53);
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedChar pf = new DoPutFieldResolvedChar();
            doResolvedChar(pf, 'B');
            DoResolveAndClinit.reportPassIf(
                "PutFieldResolved(char)", pf.field, 'B');
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedChar pf = new DoPutFieldUnresolvedChar();
            doUnresolvedChar(pf, 'C');
            DoResolveAndClinit.reportPassIf(
                "PutFieldUnresolved(char)", pf.field, 'C');
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedShort pf = new DoPutFieldResolvedShort();
            doResolvedShort(pf, (short)60);
            DoResolveAndClinit.reportPassIf(
                "PutFieldResolved(short)", pf.field, (short)60);
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedShort pf = new DoPutFieldUnresolvedShort();
            doUnresolvedShort(pf, (short)600);
            DoResolveAndClinit.reportPassIf(
                "PutFieldUnresolved(short)", pf.field, (short)600);
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedInt pf = new DoPutFieldResolvedInt();
            doResolvedInt(pf, 70);
            DoResolveAndClinit.reportPassIf(
                "PutFieldResolved(int)", pf.field, 70);
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedInt pf = new DoPutFieldUnresolvedInt();
            doUnresolvedInt(pf, 700);
            DoResolveAndClinit.reportPassIf(
                "PutFieldUnresolved(int)", pf.field, 700);
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedFloat pf = new DoPutFieldResolvedFloat();
            doResolvedFloat(pf, 80.0f);
            DoResolveAndClinit.reportPassIf(
                "PutFieldResolved(float)", pf.field, 80.0f);
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedFloat pf = new DoPutFieldUnresolvedFloat();
            doUnresolvedFloat(pf, 800.0f);
            DoResolveAndClinit.reportPassIf(
                "PutFieldUnresolved(float)", pf.field, 800.0f);
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedLong pf = new DoPutFieldResolvedLong();
            doResolvedLong(pf, 90l);
            DoResolveAndClinit.reportPassIf(
		"PutFieldResolved(long)", pf.field, 90l);
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedLong pf = new DoPutFieldUnresolvedLong();
            doUnresolvedLong(pf, 900l);
            DoResolveAndClinit.reportPassIf(
	        "PutFieldUnresolved(long)", pf.field, 900l);
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedDouble pf = new DoPutFieldResolvedDouble();
            doResolvedDouble(pf, 100.0d);
            DoResolveAndClinit.reportPassIf(
	        "PutFieldResolved(double)", pf.field, 100.0d);
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedDouble pf = new DoPutFieldUnresolvedDouble();
            doUnresolvedDouble(pf, 1000.0d);
            DoResolveAndClinit.reportPassIf(
		"PutFieldUnresolved(double)", pf.field, 1000.0d);
        }

        // 1. Resolved putfield:
        {
            Object o2 = new Object();
            DoPutFieldResolvedObject pf = new DoPutFieldResolvedObject();
            doResolvedObject(pf, o2);
            DoResolveAndClinit.reportPassIf(
                "PutFieldResolved(object)", pf.field, o2);
        }
        // 2. Unresolved putfield:
        {
            Object o3 = new Object();
            DoPutFieldUnresolvedObject pf = new DoPutFieldUnresolvedObject();
            doUnresolvedObject(pf, o3);
            DoResolveAndClinit.reportPassIf(
                "PutFieldUnresolved(object)", pf.field, o3);
        }

	////////////////////////////////////////////////////////////////
	// Volatile Set:

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedBooleanVolatile pf = new DoPutFieldResolvedBooleanVolatile();
            doResolvedBooleanVolatile(pf, true);
            DoResolveAndClinit.reportPassIf(
                "PutFieldResolvedVolatile(boolean)", pf.field, true);
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedBooleanVolatile pf = new DoPutFieldUnresolvedBooleanVolatile();
            doUnresolvedBooleanVolatile(pf, true);
            DoResolveAndClinit.reportPassIf(
                "PutFieldUnresolvedVolatile(boolean)", pf.field, true);
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedByteVolatile pf = new DoPutFieldResolvedByteVolatile();
            doResolvedByteVolatile(pf, (byte)50);
            DoResolveAndClinit.reportPassIf(
                "PutFieldResolvedVolatile(byte)", pf.field, (byte)50);
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedByteVolatile pf = new DoPutFieldUnresolvedByteVolatile();
            doUnresolvedByteVolatile(pf, (byte)53);
            DoResolveAndClinit.reportPassIf(
                "PutFieldUnresolvedVolatile(byte)", pf.field, (byte)53);
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedCharVolatile pf = new DoPutFieldResolvedCharVolatile();
            doResolvedCharVolatile(pf, 'B');
            DoResolveAndClinit.reportPassIf(
                "PutFieldResolvedVolatile(char)", pf.field, 'B');
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedCharVolatile pf = new DoPutFieldUnresolvedCharVolatile();
            doUnresolvedCharVolatile(pf, 'C');
            DoResolveAndClinit.reportPassIf(
                "PutFieldUnresolvedVolatile(char)", pf.field, 'C');
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedShortVolatile pf = new DoPutFieldResolvedShortVolatile();
            doResolvedShortVolatile(pf, (short)60);
            DoResolveAndClinit.reportPassIf(
                "PutFieldResolvedVolatile(short)", pf.field, (short)60);
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedShortVolatile pf = new DoPutFieldUnresolvedShortVolatile();
            doUnresolvedShortVolatile(pf, (short)600);
            DoResolveAndClinit.reportPassIf(
                "PutFieldUnresolvedVolatile(short)", pf.field, (short)600);
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedIntVolatile pf = new DoPutFieldResolvedIntVolatile();
            doResolvedIntVolatile(pf, 70);
            DoResolveAndClinit.reportPassIf(
                "PutFieldResolvedVolatile(int)", pf.field, 70);
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedIntVolatile pf = new DoPutFieldUnresolvedIntVolatile();
            doUnresolvedIntVolatile(pf, 700);
            DoResolveAndClinit.reportPassIf(
                "PutFieldUnresolvedVolatile(int)", pf.field, 700);
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedFloatVolatile pf = new DoPutFieldResolvedFloatVolatile();
            doResolvedFloatVolatile(pf, 80.0f);
            DoResolveAndClinit.reportPassIf(
                "PutFieldResolvedVolatile(float)", pf.field, 80.0f);
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedFloatVolatile pf = new DoPutFieldUnresolvedFloatVolatile();
            doUnresolvedFloatVolatile(pf, 800.0f);
            DoResolveAndClinit.reportPassIf(
                "PutFieldUnresolvedVolatile(float)", pf.field, 800.0f);
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedLongVolatile pf = new DoPutFieldResolvedLongVolatile();
            doResolvedLongVolatile(pf, 90l);
            DoResolveAndClinit.reportPassIf(
                "PutFieldResolvedVolatile(long)", pf.field, 90l);
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedLongVolatile pf = new DoPutFieldUnresolvedLongVolatile();
            doUnresolvedLongVolatile(pf, 900l);
            DoResolveAndClinit.reportPassIf(
                "PutFieldUnresolvedVolatile(long)", pf.field, 900l);
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedDoubleVolatile pf = new DoPutFieldResolvedDoubleVolatile();
            doResolvedDoubleVolatile(pf, 100.0d);
            DoResolveAndClinit.reportPassIf(
                "PutFieldResolvedVolatile(double)", pf.field, 100.0d);
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedDoubleVolatile pf = new DoPutFieldUnresolvedDoubleVolatile();
            doUnresolvedDoubleVolatile(pf, 1000.0d);
            DoResolveAndClinit.reportPassIf(
                "PutFieldUnresolvedVolatile(double)", pf.field, 1000.0d);
        }

        // 1. Resolved putfield:
        {
            Object o2 = new Object();
            DoPutFieldResolvedObjectVolatile pf = new DoPutFieldResolvedObjectVolatile();
            doResolvedObjectVolatile(pf, o2);
            DoResolveAndClinit.reportPassIf(
                "PutFieldResolvedVolatile(object)", pf.field, o2);
        }
        // 2. Unresolved putfield:
        {
            Object o3 = new Object();
            DoPutFieldUnresolvedObjectVolatile pf = new DoPutFieldUnresolvedObjectVolatile();
            doUnresolvedObjectVolatile(pf, o3);
            DoResolveAndClinit.reportPassIf(
                "PutFieldUnresolvedVolatile(object)", pf.field, o3);
        }

	////////////////////////////////////////////////////////////////
	// Non-Volatile Set w/ NULL Check:

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedBoolean pf = null;
	    try {
		doResolvedBoolean(pf, true);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "PutFieldResolvedNullCheck(boolean)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedBoolean pf = null;
	    try {
		doUnresolvedBoolean(pf, true);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
		    "PutFieldUnresolvedNullCheck(boolean)",
		    (t instanceof NullPointerException));
	    }
	}

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedByte pf = null;
	    try {
		doResolvedByte(pf, (byte)50);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "PutFieldResolvedNullCheck(byte)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedByte pf = null;
	    try {
		doUnresolvedByte(pf, (byte)53);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "PutFieldUnresolvedNullCheck(byte)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedChar pf = null;
	    try {
		doResolvedChar(pf, 'B');
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldResolvedNullCheck(char)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedChar pf = null;
	    try {
		doUnresolvedChar(pf, 'C');
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldUnresolvedNullCheck(char)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedShort pf = null;
	    try {
		doResolvedShort(pf, (short)60);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldResolvedNullCheck(short)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedShort pf = null;
	    try {
		doUnresolvedShort(pf, (short)600);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldUnresolvedNullCheck(short)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedInt pf = null;
	    try {
		doResolvedInt(pf, 70);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldResolvedNullCheck(int)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedInt pf = null;
	    try {
		doUnresolvedInt(pf, 700);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldUnresolvedNullCheck(int)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedFloat pf = null;
	    try {
		doResolvedFloat(pf, 80.0f);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldResolvedNullCheck(float)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedFloat pf = null;
	    try {
		doUnresolvedFloat(pf, 800.0f);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldUnresolvedNullCheck(float)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedLong pf = null;
	    try {
		doResolvedLong(pf, 90l);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
	            "PutFieldResolvedNullCheck(long)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedLong pf = null;
	    try {
		doUnresolvedLong(pf, 900l);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
	            "PutFieldUnresolvedNullCheck(long)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedDouble pf = null;
	    try {
		doResolvedDouble(pf, 100.0d);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
	            "PutFieldResolvedNullCheck(double)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedDouble pf = null;
	    try {
		doUnresolvedDouble(pf, 1000.0d);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
		    "PutFieldUnresolvedNullCheck(double)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved putfield:
        {
            Object o2 = new Object();
            DoPutFieldResolvedObject pf = null;
	    try {
		doResolvedObject(pf, o2);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldResolvedNullCheck(object)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved putfield:
        {
            Object o3 = new Object();
            DoPutFieldUnresolvedObject pf = null;
	    try {
		doUnresolvedObject(pf, o3);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldUnresolvedNullCheck(object)",
		    (t instanceof NullPointerException));
	    }
        }

	////////////////////////////////////////////////////////////////
	// Volatile Set w/ NULL Check:

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedBooleanVolatile pf = null;
	    try {
		doResolvedBooleanVolatile(pf, true);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldResolvedVolatileNullCheck(boolean)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedBooleanVolatile pf = null;
	    try {
		doUnresolvedBooleanVolatile(pf, true);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldUnresolvedVolatileNullCheck(boolean)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedByteVolatile pf = null;
	    try {
		doResolvedByteVolatile(pf, (byte)50);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldResolvedVolatileNullCheck(byte)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedByteVolatile pf = null;
	    try {
		doUnresolvedByteVolatile(pf, (byte)53);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldUnresolvedVolatileNullCheck(byte)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedCharVolatile pf = null;
	    try {
		doResolvedCharVolatile(pf, 'B');
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldResolvedVolatileNullCheck(char)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedCharVolatile pf = null;
	    try {
		doUnresolvedCharVolatile(pf, 'C');
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldUnresolvedVolatileNullCheck(char)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedShortVolatile pf = null;
	    try {
		doResolvedShortVolatile(pf, (short)60);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldResolvedVolatileNullCheck(short)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedShortVolatile pf = null;
	    try {
		doUnresolvedShortVolatile(pf, (short)600);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldUnresolvedVolatileNullCheck(short)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedIntVolatile pf = null;
	    try {
		doResolvedIntVolatile(pf, 70);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldResolvedVolatileNullCheck(int)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedIntVolatile pf = null;
	    try {
		doUnresolvedIntVolatile(pf, 700);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldUnresolvedVolatileNullCheck(int)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedFloatVolatile pf = null;
	    try {
		doResolvedFloatVolatile(pf, 80.0f);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                     "PutFieldResolvedVolatileNullCheck(float)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedFloatVolatile pf = null;
	    try {
		doUnresolvedFloatVolatile(pf, 800.0f);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldUnresolvedVolatileNullCheck(float)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedLongVolatile pf = null;
	    try {
		doResolvedLongVolatile(pf, 90l);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldResolvedVolatileNullCheck(long)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedLongVolatile pf = null;
	    try {
		doUnresolvedLongVolatile(pf, 900l);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldUnresolvedVolatileNullCheck(long)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved putfield:
        {
            DoPutFieldResolvedDoubleVolatile pf = null;
	    try {
		doResolvedDoubleVolatile(pf, 100.0d);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldResolvedVolatileNullCheck(double)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved putfield:
        {
            DoPutFieldUnresolvedDoubleVolatile pf = null;
	    try {
		doUnresolvedDoubleVolatile(pf, 1000.0d);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldUnresolvedVolatileNullCheck(double)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved putfield:
        {
            Object o2 = new Object();
            DoPutFieldResolvedObjectVolatile pf = null;
	    try {
		doResolvedObjectVolatile(pf, o2);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldResolvedVolatileNullCheck(object)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved putfield:
        {
            Object o3 = new Object();
            DoPutFieldUnresolvedObjectVolatile pf = null;
	    try {
		doUnresolvedObjectVolatile(pf, o3);
	    } catch (Throwable t) {
                DoResolveAndClinit.reportPassIf(
                    "PutFieldUnresolvedVolatileNullCheck(object)",
		    (t instanceof NullPointerException));
	    }
        }

    }

    static void
    doResolvedBoolean(DoPutFieldResolvedBoolean pf, boolean value) {
        pf.field = value;
    }
    static void
    doUnresolvedBoolean(DoPutFieldUnresolvedBoolean pf, boolean value) {
        pf.field = value;
    }

    static void
    doResolvedByte(DoPutFieldResolvedByte pf, byte value) {
        pf.field = value;
    }
    static void
    doUnresolvedByte(DoPutFieldUnresolvedByte pf, byte value) {
        pf.field = value;
    }

    static void
    doResolvedChar(DoPutFieldResolvedChar pf, char value) {
        pf.field = value;
    }
    static void
    doUnresolvedChar(DoPutFieldUnresolvedChar pf, char value) {
        pf.field = value;
    }

    static void
    doResolvedShort(DoPutFieldResolvedShort pf, short value) {
        pf.field = value;
    }
    static void
    doUnresolvedShort(DoPutFieldUnresolvedShort pf, short value) {
        pf.field = value;
    }

    static void
    doResolvedInt(DoPutFieldResolvedInt pf, int value) {
        pf.field = value;
    }
    static void
    doUnresolvedInt(DoPutFieldUnresolvedInt pf, int value) {
        pf.field = value;
    }

    static void
    doResolvedFloat(DoPutFieldResolvedFloat pf, float value) {
        pf.field = value;
    }
    static void
    doUnresolvedFloat(DoPutFieldUnresolvedFloat pf, float value) {
        pf.field = value;
    }

    static void
    doResolvedLong(DoPutFieldResolvedLong pf, long value) {
        pf.field = value;
    }
    static void
    doUnresolvedLong(DoPutFieldUnresolvedLong pf, long value) {
        pf.field = value;
    }

    static void
    doResolvedDouble(DoPutFieldResolvedDouble pf, double value) {
        pf.field = value;
    }
    static void
    doUnresolvedDouble(DoPutFieldUnresolvedDouble pf, double value) {
        pf.field = value;
    }

    static void
    doResolvedObject(DoPutFieldResolvedObject pf, Object value) {
        pf.field = value;
    }
    static void
    doUnresolvedObject(DoPutFieldUnresolvedObject pf, Object value) {
        pf.field = value;
    }

    // Volatile Set:
    static void
    doResolvedBooleanVolatile(DoPutFieldResolvedBooleanVolatile pf, boolean value) {
        pf.field = value;
    }
    static void
    doUnresolvedBooleanVolatile(DoPutFieldUnresolvedBooleanVolatile pf, boolean value) {
        pf.field = value;
    }

    static void
    doResolvedByteVolatile(DoPutFieldResolvedByteVolatile pf, byte value) {
        pf.field = value;
    }
    static void
    doUnresolvedByteVolatile(DoPutFieldUnresolvedByteVolatile pf, byte value) {
        pf.field = value;
    }

    static void
    doResolvedCharVolatile(DoPutFieldResolvedCharVolatile pf, char value) {
        pf.field = value;
    }
    static void
    doUnresolvedCharVolatile(DoPutFieldUnresolvedCharVolatile pf, char value) {
        pf.field = value;
    }

    static void
    doResolvedShortVolatile(DoPutFieldResolvedShortVolatile pf, short value) {
        pf.field = value;
    }
    static void
    doUnresolvedShortVolatile(DoPutFieldUnresolvedShortVolatile pf, short value) {
        pf.field = value;
    }

    static void
    doResolvedIntVolatile(DoPutFieldResolvedIntVolatile pf, int value) {
        pf.field = value;
    }
    static void
    doUnresolvedIntVolatile(DoPutFieldUnresolvedIntVolatile pf, int value) {
        pf.field = value;
    }

    static void
    doResolvedFloatVolatile(DoPutFieldResolvedFloatVolatile pf, float value) {
        pf.field = value;
    }
    static void
    doUnresolvedFloatVolatile(DoPutFieldUnresolvedFloatVolatile pf, float value) {
        pf.field = value;
    }

    static void
    doResolvedLongVolatile(DoPutFieldResolvedLongVolatile pf, long value) {
        pf.field = value;
    }
    static void
    doUnresolvedLongVolatile(DoPutFieldUnresolvedLongVolatile pf, long value) {
        pf.field = value;
    }

    static void
    doResolvedDoubleVolatile(DoPutFieldResolvedDoubleVolatile pf, double value) {
        pf.field = value;
    }
    static void
    doUnresolvedDoubleVolatile(DoPutFieldUnresolvedDoubleVolatile pf, double value) {
        pf.field = value;
    }

    static void
    doResolvedObjectVolatile(DoPutFieldResolvedObjectVolatile pf, Object value) {
        pf.field = value;
    }
    static void
    doUnresolvedObjectVolatile(DoPutFieldUnresolvedObjectVolatile pf, Object value) {
        pf.field = value;
    }
}

//////////////////////////////////////////////////////////////////////////////
// Testing opcode getfield:
// Note: Resolves to a field address.

/* Boolean: */
class DoGetFieldResolvedBoolean {
    public boolean field = false;
}
class DoGetFieldUnresolvedBoolean {
    public boolean field = false;
}

/* Byte: */
class DoGetFieldResolvedByte {
    public byte field = 0;
}
class DoGetFieldUnresolvedByte {
    public byte field = 0;
}

/* Char: */
class DoGetFieldResolvedChar {
    public char field = '\0';
}
class DoGetFieldUnresolvedChar {
    public char field = '\0';
}

/* Short: */
class DoGetFieldResolvedShort {
    public short field = 0;
}
class DoGetFieldUnresolvedShort {
    public short field = 0;
}

/* Int: */
class DoGetFieldResolvedInt {
    public int field = 0;
}
class DoGetFieldUnresolvedInt {
    public int field = 0;
}

/* Float: */
class DoGetFieldResolvedFloat {
    public float field = 0.0f;
}
class DoGetFieldUnresolvedFloat {
    public float field = 0.0f;
}

/* Long: */
class DoGetFieldResolvedLong {
    public long field = 0l;
}
class DoGetFieldUnresolvedLong {
    public long field = 0l;
}

/* Double: */
class DoGetFieldResolvedDouble {
    public double field = 0.0d;
}
class DoGetFieldUnresolvedDouble {
    public double field = 0.0d;
}

/* Object: */
class DoGetFieldResolvedObject {
    public Object field = null;
}
class DoGetFieldUnresolvedObject {
    public Object field = null;
}

// Volatile Set:

/* Volatile Boolean: */
class DoGetFieldResolvedBooleanVolatile {
    public volatile boolean field = false;
}
class DoGetFieldUnresolvedBooleanVolatile {
    public volatile boolean field = false;
}

/* Volatile Byte: */
class DoGetFieldResolvedByteVolatile {
    public volatile byte field = 0;
}
class DoGetFieldUnresolvedByteVolatile {
    public volatile byte field = 0;
}

/* Volatile Char: */
class DoGetFieldResolvedCharVolatile {
    public volatile char field = '\0';
}
class DoGetFieldUnresolvedCharVolatile {
    public volatile char field = '\0';
}

/* Volatile Short: */
class DoGetFieldResolvedShortVolatile {
    public volatile short field = 0;
}
class DoGetFieldUnresolvedShortVolatile {
    public volatile short field = 0;
}

/* Volatile Int: */
class DoGetFieldResolvedIntVolatile {
    public volatile int field = 0;
}
class DoGetFieldUnresolvedIntVolatile {
    public volatile int field = 0;
}

/* Volatile Float: */
class DoGetFieldResolvedFloatVolatile {
    public volatile float field = 0.0f;
}
class DoGetFieldUnresolvedFloatVolatile {
    public volatile float field = 0.0f;
}

/* Volatile Long: */
class DoGetFieldResolvedLongVolatile {
    public volatile long field = 0l;
}
class DoGetFieldUnresolvedLongVolatile {
    public volatile long field = 0l;
}

/* Volatile Double: */
class DoGetFieldResolvedDoubleVolatile {
    public volatile double field = 0.0d;
}
class DoGetFieldUnresolvedDoubleVolatile {
    public volatile double field = 0.0d;
}

/* Volatile Object: */
class DoGetFieldResolvedObjectVolatile {
    public volatile Object field = null;
}
class DoGetFieldUnresolvedObjectVolatile {
    public volatile Object field = null;
}


class DoGetField
{
    static final String[] compileItems = {
        "DoGetField.doResolvedBoolean(LDoGetFieldResolvedBoolean;)Z",
        "DoGetField.doUnresolvedBoolean(LDoGetFieldUnresolvedBoolean;)Z",

        "DoGetField.doResolvedByte(LDoGetFieldResolvedByte;)B",
        "DoGetField.doUnresolvedByte(LDoGetFieldUnresolvedByte;)B",

        "DoGetField.doResolvedChar(LDoGetFieldResolvedChar;)C",
        "DoGetField.doUnresolvedChar(LDoGetFieldUnresolvedChar;)C",

        "DoGetField.doResolvedShort(LDoGetFieldResolvedShort;)S",
        "DoGetField.doUnresolvedShort(LDoGetFieldUnresolvedShort;)S",

        "DoGetField.doResolvedInt(LDoGetFieldResolvedInt;)I",
        "DoGetField.doUnresolvedInt(LDoGetFieldUnresolvedInt;)I",

        "DoGetField.doResolvedFloat(LDoGetFieldResolvedFloat;)F",
        "DoGetField.doUnresolvedFloat(LDoGetFieldUnresolvedFloat;)F",

        "DoGetField.doResolvedLong(LDoGetFieldResolvedLong;)J",
        "DoGetField.doUnresolvedLong(LDoGetFieldUnresolvedLong;)J",

        "DoGetField.doResolvedDouble(LDoGetFieldResolvedDouble;)D",
        "DoGetField.doUnresolvedDouble(LDoGetFieldUnresolvedDouble;)D",

        "DoGetField.doResolvedObject(LDoGetFieldResolvedObject;)Ljava/lang/Object;",
        "DoGetField.doUnresolvedObject(LDoGetFieldUnresolvedObject;)Ljava/lang/Object;",

	// Volatile Set:
        "DoGetField.doResolvedBooleanVolatile(LDoGetFieldResolvedBooleanVolatile;)Z",
        "DoGetField.doUnresolvedBooleanVolatile(LDoGetFieldUnresolvedBooleanVolatile;)Z",

        "DoGetField.doResolvedByteVolatile(LDoGetFieldResolvedByteVolatile;)B",
        "DoGetField.doUnresolvedByteVolatile(LDoGetFieldUnresolvedByteVolatile;)B",

        "DoGetField.doResolvedCharVolatile(LDoGetFieldResolvedCharVolatile;)C",
        "DoGetField.doUnresolvedCharVolatile(LDoGetFieldUnresolvedCharVolatile;)C",

        "DoGetField.doResolvedShortVolatile(LDoGetFieldResolvedShortVolatile;)S",
        "DoGetField.doUnresolvedShortVolatile(LDoGetFieldUnresolvedShortVolatile;)S",

        "DoGetField.doResolvedIntVolatile(LDoGetFieldResolvedIntVolatile;)I",
        "DoGetField.doUnresolvedIntVolatile(LDoGetFieldUnresolvedIntVolatile;)I",

        "DoGetField.doResolvedFloatVolatile(LDoGetFieldResolvedFloatVolatile;)F",
        "DoGetField.doUnresolvedFloatVolatile(LDoGetFieldUnresolvedFloatVolatile;)F",

        "DoGetField.doResolvedLongVolatile(LDoGetFieldResolvedLongVolatile;)J",
        "DoGetField.doUnresolvedLongVolatile(LDoGetFieldUnresolvedLongVolatile;)J",

        "DoGetField.doResolvedDoubleVolatile(LDoGetFieldResolvedDoubleVolatile;)D",
        "DoGetField.doUnresolvedDoubleVolatile(LDoGetFieldUnresolvedDoubleVolatile;)D",

        "DoGetField.doResolvedObjectVolatile(LDoGetFieldResolvedObjectVolatile;)Ljava/lang/Object;",
        "DoGetField.doUnresolvedObjectVolatile(LDoGetFieldUnresolvedObjectVolatile;)Ljava/lang/Object;",
    };

    public static void doSetup() {
        // Setup initial conditions:
        Object o = new Object();
        {
            // Resolve it:
            boolean i = new DoGetFieldResolvedBoolean().field;
        }
        {
            // Resolve it:
            byte i = new DoGetFieldResolvedByte().field;
        }
        {
            // Resolve it:
            char i = new DoGetFieldResolvedChar().field;
        }
        {
            // Resolve it:
            short i = new DoGetFieldResolvedShort().field;
        }
        {
            // Resolve it:
            int i = new DoGetFieldResolvedInt().field;
        }
        {
            // Resolve it:
            float i = new DoGetFieldResolvedFloat().field;
        }
        {
            // Resolve it:
            long i = new DoGetFieldResolvedLong().field;
        }
        {
            // Resolve it:
            double i = new DoGetFieldResolvedDouble().field;
        }
        {
            // Resolve it:
            Object o1 = new DoGetFieldResolvedObject().field;
        }

	// Volatile Set:
        {
            // Resolve it:
            boolean i = new DoGetFieldResolvedBooleanVolatile().field;
        }
        {
            // Resolve it:
            byte i = new DoGetFieldResolvedByteVolatile().field;
        }
        {
            // Resolve it:
            char i = new DoGetFieldResolvedCharVolatile().field;
        }
        {
            // Resolve it:
            short i = new DoGetFieldResolvedShortVolatile().field;
        }
        {
            // Resolve it:
            int i = new DoGetFieldResolvedIntVolatile().field;
        }
        {
            // Resolve it:
            float i = new DoGetFieldResolvedFloatVolatile().field;
        }
        {
            // Resolve it:
            long i = new DoGetFieldResolvedLongVolatile().field;
        }
        {
            // Resolve it:
            double i = new DoGetFieldResolvedDoubleVolatile().field;
        }
        {
            // Resolve it:
            Object o1 = new DoGetFieldResolvedObjectVolatile().field;
        }


        // Do compilation:
        CompilerTest.main(compileItems);
    }

    public static void doTest() {

	//////////////////////////////////////////////////////////////////////
	// Non-Volatile Set:
	//

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedBoolean gf = new DoGetFieldResolvedBoolean();
            gf.field = true;
            boolean value = doResolvedBoolean(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldResolved(boolean)", value, true);
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedBoolean gf = new DoGetFieldUnresolvedBoolean();
            gf.field = true;
            boolean value = doUnresolvedBoolean(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldUnresolved(boolean)", value, true);
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedByte gf = new DoGetFieldResolvedByte();
            gf.field = (byte)50;
            byte value = doResolvedByte(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldResolved(byte)", value, (byte)50);
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedByte gf = new DoGetFieldUnresolvedByte();
            gf.field = (byte)53;
            byte value = doUnresolvedByte(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldUnresolved(byte)", value, (byte)53);
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedChar gf = new DoGetFieldResolvedChar();
            gf.field = 'B';
            char value = doResolvedChar(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldResolved(char)", value, 'B');
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedChar gf = new DoGetFieldUnresolvedChar();
            gf.field = 'C';
            char value = doUnresolvedChar(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldUnresolved(char)", value, 'C');
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedShort gf = new DoGetFieldResolvedShort();
            gf.field = (short)60;
            short value = doResolvedShort(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldResolved(short)", value, (short)60);
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedShort gf = new DoGetFieldUnresolvedShort();
            gf.field = (short)600;
            short value = doUnresolvedShort(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldUnresolved(short)", value, (short)600);
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedInt gf = new DoGetFieldResolvedInt();
            gf.field = 70;
            int value = doResolvedInt(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldResolved(int)", value, 70);
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedInt gf = new DoGetFieldUnresolvedInt();
            gf.field = 700;
            int value = doUnresolvedInt(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldUnresolved(int)", value, 700);
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedFloat gf = new DoGetFieldResolvedFloat();
            gf.field = 80.0f;
            float value = doResolvedFloat(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldResolved(float)", value, 80.0f);
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedFloat gf = new DoGetFieldUnresolvedFloat();
            gf.field = 800.0f;
            float value = doUnresolvedFloat(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldUnresolved(float)", value, 800.0f);
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedLong gf = new DoGetFieldResolvedLong();
            gf.field = 90l;
            long value = doResolvedLong(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldResolved(long)", value, 90l);
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedLong gf = new DoGetFieldUnresolvedLong();
            gf.field = 900l;
            long value = doUnresolvedLong(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldUnresolved(long)", value, 900l);
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedDouble gf = new DoGetFieldResolvedDouble();
            gf.field = 100.0d;
            double value = doResolvedDouble(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldResolved(double)", value, 100.0d);
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedDouble gf = new DoGetFieldUnresolvedDouble();
            gf.field = 1000.0d;
            double value = doUnresolvedDouble(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldUnresolved(double)", value, 1000.0d);
        }

        // 1. Resolved getfield:
        {
            Object o2 = new Object();
            DoGetFieldResolvedObject gf = new DoGetFieldResolvedObject();
            gf.field = o2;
            Object value = doResolvedObject(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldResolved(object)", value, o2);
        }
        // 2. Unresolved getfield:
        {
            Object o3 = new Object();
            DoGetFieldUnresolvedObject gf = new DoGetFieldUnresolvedObject();
            gf.field = o3;
            Object value = doUnresolvedObject(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldUnresolved(object)", value, o3);
        }

	//////////////////////////////////////////////////////////////////////
	// Volatile Set:
	//

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedBooleanVolatile gf = new DoGetFieldResolvedBooleanVolatile();
            gf.field = true;
            boolean value = doResolvedBooleanVolatile(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldResolvedVolatile(boolean)", value, true);
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedBooleanVolatile gf = new DoGetFieldUnresolvedBooleanVolatile();
            gf.field = true;
            boolean value = doUnresolvedBooleanVolatile(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldUnresolvedVolatile(boolean)", value, true);
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedByteVolatile gf = new DoGetFieldResolvedByteVolatile();
            gf.field = (byte)50;
            byte value = doResolvedByteVolatile(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldResolvedVolatile(byte)", value, (byte)50);
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedByteVolatile gf = new DoGetFieldUnresolvedByteVolatile();
            gf.field = (byte)53;
            byte value = doUnresolvedByteVolatile(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldUnresolvedVolatile(byte)", value, (byte)53);
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedCharVolatile gf = new DoGetFieldResolvedCharVolatile();
            gf.field = 'B';
            char value = doResolvedCharVolatile(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldResolvedVolatile(char)", value, 'B');
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedCharVolatile gf = new DoGetFieldUnresolvedCharVolatile();
            gf.field = 'C';
            char value = doUnresolvedCharVolatile(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldUnresolvedVolatile(char)", value, 'C');
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedShortVolatile gf = new DoGetFieldResolvedShortVolatile();
            gf.field = (short)60;
            short value = doResolvedShortVolatile(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldResolvedVolatile(short)", value, (short)60);
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedShortVolatile gf = new DoGetFieldUnresolvedShortVolatile();
            gf.field = (short)600;
            short value = doUnresolvedShortVolatile(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldUnresolvedVolatile(short)", value, (short)600);
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedIntVolatile gf = new DoGetFieldResolvedIntVolatile();
            gf.field = 70;
            int value = doResolvedIntVolatile(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldResolvedVolatile(int)", value, 70);
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedIntVolatile gf = new DoGetFieldUnresolvedIntVolatile();
            gf.field = 700;
            int value = doUnresolvedIntVolatile(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldUnresolvedVolatile(int)", value, 700);
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedFloatVolatile gf = new DoGetFieldResolvedFloatVolatile();
            gf.field = 80.0f;
            float value = doResolvedFloatVolatile(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldResolvedVolatile(float)", value, 80.0f);
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedFloatVolatile gf = new DoGetFieldUnresolvedFloatVolatile();
            gf.field = 800.0f;
            float value = doUnresolvedFloatVolatile(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldUnresolvedVolatile(float)", value, 800.0f);
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedLongVolatile gf = new DoGetFieldResolvedLongVolatile();
            gf.field = 90l;
            long value = doResolvedLongVolatile(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldResolvedVolatile(long)", value, 90l);
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedLongVolatile gf = new DoGetFieldUnresolvedLongVolatile();
            gf.field = 900l;
            long value = doUnresolvedLongVolatile(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldUnresolvedVolatile(long)", value, 900l);
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedDoubleVolatile gf = new DoGetFieldResolvedDoubleVolatile();
            gf.field = 100.0d;
            double value = doResolvedDoubleVolatile(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldResolvedVolatile(double)", value, 100.0d);
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedDoubleVolatile gf = new DoGetFieldUnresolvedDoubleVolatile();
            gf.field = 1000.0d;
            double value = doUnresolvedDoubleVolatile(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldUnresolvedVolatile(double)", value, 1000.0d);
        }

        // 1. Resolved getfield:
        {
            Object o2 = new Object();
            DoGetFieldResolvedObjectVolatile gf = new DoGetFieldResolvedObjectVolatile();
            gf.field = o2;
            Object value = doResolvedObjectVolatile(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldResolvedVolatile(object)", value, o2);
        }
        // 2. Unresolved getfield:
        {
            Object o3 = new Object();
            DoGetFieldUnresolvedObjectVolatile gf = new DoGetFieldUnresolvedObjectVolatile();
            gf.field = o3;
            Object value = doUnresolvedObjectVolatile(gf);
            DoResolveAndClinit.reportPassIf(
                "GetFieldUnresolvedVolatile(object)", value, o3);
        }

	//////////////////////////////////////////////////////////////////////
	// Non-Volatile Set w/ Null Check:
	//

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedBoolean gf = null;
	    try {
		boolean value = doResolvedBoolean(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldResolved(boolean)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedBoolean gf = null;
	    try {
		boolean value = doUnresolvedBoolean(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldUnresolved(boolean)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedByte gf = null;
	    try {
		byte value = doResolvedByte(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldResolved(byte)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedByte gf = null;
	    try {
		byte value = doUnresolvedByte(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldUnresolved(byte)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedChar gf = null;
	    try {
		char value = doResolvedChar(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldResolved(char)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedChar gf = null;
	    try {
		char value = doUnresolvedChar(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldUnresolved(char)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedShort gf = null;
	    try {
		short value = doResolvedShort(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldResolved(short)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedShort gf = null;
	    try {
		short value = doUnresolvedShort(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldUnresolved(short)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedInt gf = null;
	    try {
		int value = doResolvedInt(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldResolved(int)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedInt gf = null;
	    try {
		int value = doUnresolvedInt(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldUnresolved(int)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedFloat gf = null;
	    try {
		float value = doResolvedFloat(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldResolved(float)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedFloat gf = null;
	    try {
		float value = doUnresolvedFloat(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldUnresolved(float)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedLong gf = null;
	    try {
		long value = doResolvedLong(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldResolved(long)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedLong gf = null;
	    try {
		long value = doUnresolvedLong(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldUnresolved(long)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedDouble gf = null;
	    try {
		double value = doResolvedDouble(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldResolved(double)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedDouble gf = null;
	    try {
		double value = doUnresolvedDouble(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldUnresolved(double)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedObject gf = null;
	    try {
		Object value = doResolvedObject(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldResolved(object)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedObject gf = null;
	    try {
		Object value = doUnresolvedObject(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldUnresolved(object)",
		    (t instanceof NullPointerException));
	    }
        }

	//////////////////////////////////////////////////////////////////////
	// Volatile Set w/ Null Check:
	//

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedBooleanVolatile gf = null;
	    try {
		boolean value = doResolvedBooleanVolatile(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldResolvedVolatile(boolean)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedBooleanVolatile gf = null;
	    try {
		boolean value = doUnresolvedBooleanVolatile(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldUnresolvedVolatile(boolean)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedByteVolatile gf = null;
	    try {
		byte value = doResolvedByteVolatile(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldResolvedVolatile(byte)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedByteVolatile gf = null;
	    try {
		byte value = doUnresolvedByteVolatile(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldUnresolvedVolatile(byte)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedCharVolatile gf = null;
	    try {
		char value = doResolvedCharVolatile(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldResolvedVolatile(char)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedCharVolatile gf = null;
	    try {
		char value = doUnresolvedCharVolatile(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldUnresolvedVolatile(char)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedShortVolatile gf = null;
	    try {
		short value = doResolvedShortVolatile(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldResolvedVolatile(short)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedShortVolatile gf = null;
	    try {
		short value = doUnresolvedShortVolatile(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldUnresolvedVolatile(short)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedIntVolatile gf = null;
	    try {
		int value = doResolvedIntVolatile(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldResolvedVolatile(int)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedIntVolatile gf = null;
	    try {
		int value = doUnresolvedIntVolatile(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldUnresolvedVolatile(int)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedFloatVolatile gf = null;
	    try {
		float value = doResolvedFloatVolatile(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldResolvedVolatile(float)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedFloatVolatile gf = null;
	    try {
		float value = doUnresolvedFloatVolatile(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldUnresolvedVolatile(float)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedLongVolatile gf = null;
	    try {
		long value = doResolvedLongVolatile(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldResolvedVolatile(long)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedLongVolatile gf = null;
	    try {
		long value = doUnresolvedLongVolatile(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldUnresolvedVolatile(long)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedDoubleVolatile gf = null;
	    try {
		double value = doResolvedDoubleVolatile(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldResolvedVolatile(double)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedDoubleVolatile gf = null;
	    try {
		double value = doUnresolvedDoubleVolatile(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldUnresolvedVolatile(double)",
		    (t instanceof NullPointerException));
	    }
        }

        // 1. Resolved getfield:
        {
            DoGetFieldResolvedObjectVolatile gf = null;
	    try {
		Object value = doResolvedObjectVolatile(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldResolvedVolatile(object)",
		    (t instanceof NullPointerException));
	    }
        }
        // 2. Unresolved getfield:
        {
            DoGetFieldUnresolvedObjectVolatile gf = null;
	    try {
		Object value = doUnresolvedObjectVolatile(gf);
	    } catch (Throwable t) {
		DoResolveAndClinit.reportPassIf(
                    "GetFieldUnresolvedVolatile(object)",
		    (t instanceof NullPointerException));
	    }
        }

    }

    static boolean doResolvedBoolean(DoGetFieldResolvedBoolean gf) {
        return gf.field;
    }
    static boolean doUnresolvedBoolean(DoGetFieldUnresolvedBoolean gf) {
        return gf.field;
    }

    static byte doResolvedByte(DoGetFieldResolvedByte gf) {
        return gf.field;
    }
    static byte doUnresolvedByte(DoGetFieldUnresolvedByte gf) {
        return gf.field;
    }

    static char doResolvedChar(DoGetFieldResolvedChar gf) {
        return gf.field;
    }
    static char doUnresolvedChar(DoGetFieldUnresolvedChar gf) {
        return gf.field;
    }

    static short doResolvedShort(DoGetFieldResolvedShort gf) {
        return gf.field;
    }
    static short doUnresolvedShort(DoGetFieldUnresolvedShort gf) {
        return gf.field;
    }

    static int doResolvedInt(DoGetFieldResolvedInt gf) {
        return gf.field;
    }
    static int doUnresolvedInt(DoGetFieldUnresolvedInt gf) {
        return gf.field;
    }

    static float doResolvedFloat(DoGetFieldResolvedFloat gf) {
        return gf.field;
    }
    static float doUnresolvedFloat(DoGetFieldUnresolvedFloat gf) {
        return gf.field;
    }

    static long doResolvedLong(DoGetFieldResolvedLong gf) {
        return gf.field;
    }
    static long doUnresolvedLong(DoGetFieldUnresolvedLong gf) {
        return gf.field;
    }

    static double doResolvedDouble(DoGetFieldResolvedDouble gf) {
        return gf.field;
    }
    static double doUnresolvedDouble(DoGetFieldUnresolvedDouble gf) {
        return gf.field;
    }

    static Object doResolvedObject(DoGetFieldResolvedObject gf) {
        return gf.field;
    }
    static Object doUnresolvedObject(DoGetFieldUnresolvedObject gf) {
        return gf.field;
    }

    // Volatile Set:
    static boolean doResolvedBooleanVolatile(DoGetFieldResolvedBooleanVolatile gf) {
        return gf.field;
    }
    static boolean doUnresolvedBooleanVolatile(DoGetFieldUnresolvedBooleanVolatile gf) {
        return gf.field;
    }

    static byte doResolvedByteVolatile(DoGetFieldResolvedByteVolatile gf) {
        return gf.field;
    }
    static byte doUnresolvedByteVolatile(DoGetFieldUnresolvedByteVolatile gf) {
        return gf.field;
    }

    static char doResolvedCharVolatile(DoGetFieldResolvedCharVolatile gf) {
        return gf.field;
    }
    static char doUnresolvedCharVolatile(DoGetFieldUnresolvedCharVolatile gf) {
        return gf.field;
    }

    static short doResolvedShortVolatile(DoGetFieldResolvedShortVolatile gf) {
        return gf.field;
    }
    static short doUnresolvedShortVolatile(DoGetFieldUnresolvedShortVolatile gf) {
        return gf.field;
    }

    static int doResolvedIntVolatile(DoGetFieldResolvedIntVolatile gf) {
        return gf.field;
    }
    static int doUnresolvedIntVolatile(DoGetFieldUnresolvedIntVolatile gf) {
        return gf.field;
    }

    static float doResolvedFloatVolatile(DoGetFieldResolvedFloatVolatile gf) {
        return gf.field;
    }
    static float doUnresolvedFloatVolatile(DoGetFieldUnresolvedFloatVolatile gf) {
        return gf.field;
    }

    static long doResolvedLongVolatile(DoGetFieldResolvedLongVolatile gf) {
        return gf.field;
    }
    static long doUnresolvedLongVolatile(DoGetFieldUnresolvedLongVolatile gf) {
        return gf.field;
    }

    static double doResolvedDoubleVolatile(DoGetFieldResolvedDoubleVolatile gf) {
        return gf.field;
    }
    static double doUnresolvedDoubleVolatile(DoGetFieldUnresolvedDoubleVolatile gf) {
        return gf.field;
    }

    static Object doResolvedObjectVolatile(DoGetFieldResolvedObjectVolatile gf) {
        return gf.field;
    }
    static Object doUnresolvedObjectVolatile(DoGetFieldUnresolvedObjectVolatile gf) {
        return gf.field;
    }
}


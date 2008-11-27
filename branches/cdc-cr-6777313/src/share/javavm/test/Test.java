/*
 * @(#)Test.java	1.61 06/10/10
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
 * Test.java 00/04/25
 * Please keep TestExpectedResult in sync.
 */

import sun.misc.CVM;
import sun.misc.GC;

public class Test {
    static int sfoo = 3;
    static final int sbar = 6;
    static int ssumbar;
    static int sprodbar;
    static int srecursebar;

    int nTrailers;
    Test next;

    static int nFailure;
    static int nSuccess;
    static int refUninitIndicator;
    static int clinitIndicator;

    public static boolean assert0( boolean val, String expr ){
	if ( val ){
	    nSuccess += 1;
	    return true;
	} else {
	    nFailure += 1;
	    System.out.print("*TEST FAILURE: ");
	    System.out.println( expr );
	    return false;
	}
    }

    /* flags for refUninitIndicator */
    static final int finishedTry = 1;
    static final int caughtStackOverflow = 2;
    static final int didFinally     = 4;
    static final int caughtThrowable = 8;

    /* flags for clinitIndicator */
    static final int clinitA	 = 1;
    static final int clinitB	 = 2;
    static final int clinitC	 = 3;
    static final int clinitD	 = 4;
    static final int clinitE	 = 5;
    static final int clinitF	 = 6;
    static final int clinitG	 = 7;
    static final int clinitShift = 3;

    /** This class takes as arguments the names of System properties
	to get and print out, thereby testing the command line parsing
	in CVM.initialize() as well as the passing of arguments into
	main(). */

    public static void main(String argv[]) {
	nFailure = 0;
	nSuccess = 0;
	refUninitIndicator = 0;
	clinitIndicator = 0;
	System.out.println("*Number of command line arguments: " + argv.length);
	for (int i = 0; i < argv.length; i++) {
	    System.out.println("*"+ i + ": System.getProperty(\"" +
			       argv[i] + "\") = " +
			       System.getProperty(argv[i]));
	}

	String a = "foobar";
	a = a + "bar";
	assert0( a.intern() == "foobarbar", "Trivial string intern");

	System.out.print("Starting test1\n");
	int oldvalue = CVM.setDebugFlags(CVM.DEBUGFLAG_TRACE_METHOD |
					 CVM.DEBUGFLAG_TRACE_GCALLOC);
	test1(false);
	CVM.restoreDebugFlags(CVM.DEBUGFLAG_TRACE_METHOD |
			      CVM.DEBUGFLAG_TRACE_GCALLOC, oldvalue);
	/* Now test out the java.lang.System version of the method tracing */
	System.out.print("Starting test1 again\n");
	Runtime.getRuntime().traceMethodCalls(true);
	test1(true);
	Runtime.getRuntime().traceMethodCalls(false);

	test3();
	internTest();
	test5();
	test6();
	testArrayCopy();
	testStaticInitalizers();
	assert0( clinitIndicator ==
	    ( (((((((clinitD<<clinitShift)+clinitC)<<clinitShift)+clinitE)<<clinitShift)+clinitF)<<clinitShift)+clinitG ),
	    "<clinit> test unexpected result" );

	testArrayClassAccess();
	testCloning();
	testFloatBits();
	testDeepArrayConstruction();

	/*
	 * This had better run flawlessly -- we implement code rewriting now
	 */
	refUninitConflictTest();
	assert0( refUninitIndicator == (didFinally|caughtThrowable),
	    "refUninitConflictTest unexpected path "+refUninitIndicator );
	
	/*
	 * Here is a method with a ref-val conflict which we don't yet
	 * know how to handle.
	 *
	 */
	refValConflictTest(5);

	testSunMiscGC();

 	testManyFieldsAndMethods();

	if ( nFailure == 0 ){
	    System.out.print("\n*CONGRATULATIONS: ");
	} else {
	    System.out.print("\n*TOO BAD: ");
	}
	System.out.print("test Test completed with ");
	System.out.print( nSuccess );
	System.out.print( " tests passed and " );
	System.out.print( nFailure );
	System.out.println( " failures" );
	System.out.println("*Output lines starting with a * should be checked for correctness");
	System.out.println("*They can be compared to src/share/javavm/test/TestExpectedResult" );
    }


    /*
     * Test the sun.misc.GC class, and check it by issuing a latency
     * request on GC. We should be seeing full GC's happening.
     */
    static void testSunMiscGC() {
	System.err.println("Requesting GC with a latency request of 2 seconds");
	System.err.println("(Turning GC tracing on)");
	int oldvalue = 0;
	GC.LatencyRequest latencyRequest = GC.requestLatency((long)2000);
	System.err.println("Sleeping 5 seconds, and waiting for GC's");
	try {
	    oldvalue = CVM.setDebugFlags(CVM.DEBUGFLAG_TRACE_GCSTARTSTOP);
	    Thread.sleep((long)5000);
	} catch (InterruptedException e) {
	}
	CVM.restoreDebugFlags(CVM.DEBUGFLAG_TRACE_GCSTARTSTOP, oldvalue);
	System.err.println("Woke up! Cancelling latency request");
	latencyRequest.cancel();
    }
	
    /*
     * This method involves a Ref-uninit conflict. Use it to test
     * handling of conflicts.
     */
    static void refUninitConflictTest() {
	try {
	    Object[] o1 = new Object[5];
	    Object[] o2 = new Object[7];
	    int flag;
	    try {
		int i = 4;
		o1[8] = null;
		System.arraycopy(o2, 2, o1, 3, 8);
		refUninitIndicator = finishedTry;
	    } catch (StackOverflowError e) {
		//System.out.println("caught!");
		flag = 1;
		e.printStackTrace();
		refUninitIndicator |= caughtStackOverflow;
	    } finally {
		flag = 2;
		Object o = new Object();
		refUninitIndicator |= didFinally;
		System.out.println("finally!");
	    }
	} catch (Throwable e) {
	    // ignore.
	    refUninitIndicator |= caughtThrowable;
	}
    }

    static void refValConflictTest(int param) {
	if (param == 0) {
	    int val = 5; /* Integer use of a local */
	}
	/* The reference use of the local above is in the 'any'
	   exception handler. The exception object is stored in the same
	   local variable assigned to val above. */
	try {
	    Object o = new int[4];
	} finally {
	    /* A GC point within a jsr block. Has ref-val conflict. */
	    Object o2 = new int[4]; 
	}
    }

    /*
     * The following is a refVal conflict in a <clinit> method.
     * Since theses are separately allocated, they can be 
     * separately deallocated, too.
     */

    private static Object oWhatever = null;

    static {
	if (oWhatever == null) {
	    int val = 5; /* Integer use of a local */
	}
	/* The reference use of the local above is in the 'any'
	   exception handler. The exception object is stored in the same
	   local variable assigned to val above. */
	try {
	    Object o = null;
	    o = new int[4];
	} finally {
	    /* A GC point within a jsr block. Has ref-val conflict. */
	    oWhatever = new int[4]; 
	}
    }

    static final int expectedPlusResult = 7;
    static final int expectedMultResult = 10;
    static final int expectedRecurseResult = 15;
    static final int expectedStaticPlusResult = 9;
    static final int expectedStaticMultResult = 18;
    static final int expectedStaticRecurseResult = 18*19/2;
    static final int expectedLinkResult = 3;

    // If userRuntime = true, use the java.lang.Runtime version of the opcode
    // and method tracing. Otherwise, use the sun.misc.CVM version.
    static void test1(boolean useRuntime) {
	int foo = 5;
	int bar = 2;
	int sumbar;
	int prodbar;
	int recursebar;
	int linkval = 0;

	sumbar = foo + bar;

	prodbar = mult(foo,bar);

	recursebar = recurse(5);

	ssumbar = sfoo + sbar;

	int oldvalue = 0;
	if (useRuntime) {
	    Runtime.getRuntime().traceInstructions(true);
	} else {
	    oldvalue = CVM.setDebugFlags(CVM.DEBUGFLAG_TRACE_OPCODE);
	}
	sprodbar = mult(sfoo,sbar);
	if (useRuntime) {
	    Runtime.getRuntime().traceInstructions(false);
	} else {
	    CVM.restoreDebugFlags(CVM.DEBUGFLAG_TRACE_OPCODE, oldvalue);
	}

	srecursebar = recurse(sprodbar);

	Test t = new Test();
	linkval = t.link( new Test() );
	linkval += t.link( new Test() );

	int x = 5;
	x += -1;
	x += 1;

	oldvalue = CVM.clearDebugFlags(-1);
	assert0( sumbar == expectedPlusResult, "test1: sumbar = foo+bar" );
	assert0( prodbar == expectedMultResult, "test1: prodbar = mult(foo,bar)" );
	assert0( recursebar == expectedRecurseResult, "test1: recursebar = recurse(5)" );
	assert0( ssumbar == expectedStaticPlusResult, "test1: ssumbar = sfoo+sbar" );
	assert0( sprodbar == expectedStaticMultResult, "test1: sprodbar = mult(sfoo,sbar)" );
	assert0( srecursebar == expectedStaticRecurseResult, "test1: srecursebar = recurse(sprodbar)" );
	assert0( linkval == expectedLinkResult, "test1: number of t.link calls" );
	assert0( x == 5, "test1: 5 +/- 1");
	CVM.restoreDebugFlags(-1, oldvalue);
    }

    /*
     * A test of basic and multi-dimensional
     * array types: read, write and allocation.
     */
    static void test3() {
	char[]    carr = new char[3];    char c;
	float[]   farr = new float[3];   float f;
	double[]  darr = new double[3];  double d;
	int[]     iarr = new int[3];     int i;
	long[]    larr = new long[3];    long l;
	boolean[] zarr = new boolean[3]; boolean z;
	short[]   sarr = new short[3];   short s;
	byte[]    barr = new byte[3];    byte b;
	Object[]  oarr = new Object[3];  Object o;
	
	char[][][] c3arr = new char[2][5][4];

	for (int j = 0; j < 2; j++) {
	    for (int k = 0; k < 5; k++) {
		for (int m = 0; m < 4; m++) {
		    assert0(c3arr[j][k][m] == (char)0,
			"test3: Non-zero element in multiarray!!!");
		}
	    }
	}
	
	Object[][] o2arr = new Object[4][];

	oarr[0] = carr;
	oarr[1] = barr;
	oarr[2] = iarr;

	o2arr[0] = oarr;
	o2arr[1] = oarr;
	o2arr[2] = oarr;
	o2arr[3] = oarr;
	
	char[][] c2arr = new char[2][1];
	c2arr[0] = carr;
	c2arr[1] = carr;

	c3arr[0] = new char[10][10];
	c3arr[1] = new char[10][10];

	for (int k = 0; k < 4; k++) {
	    for (int j = 0; j < 2; j++) {
		o2arr[k][j] = carr;
	    }
	}

	for(int k = 0; k < 3; k++) {
	    carr[k] = 'a';
	    c = carr[k];
	}
	for(int k = 0; k < 3; k++) {
	    farr[k] = k;
	    f = farr[k];
	}
	for(int k = 0; k < 3; k++) {
	    darr[k] = k;
	    d = darr[k];
	}
	for(int k = 0; k < 3; k++) {
	    iarr[k] = k;
	    i = iarr[k];
	}
	for(int k = 0; k < 3; k++) {
	    larr[k] = k;
	    l = larr[k];
	}
	for(int k = 0; k < 3; k++) {
	    zarr[k] = (k == 2);
	    z = zarr[k];
	}
	for(int k = 0; k < 3; k++) {
	    sarr[k] = (short)k;
	    s = sarr[k];
	}
	for(int k = 0; k < 3; k++) {
	    barr[k] = (byte)k;
	    b = barr[k];
	}
    }

    /*
     * Simple string testing. Construction, interning.
     */


    static void internSelf( char v[] ){
	String vString = new String( v );
	String newV    = vString.intern();
	// true will not intern to itself, as there already is one.
	// other random strings will intern to themselves.
	// but they should all compare equal to their interned selves.
	assert0( vString.equals("true") != (vString == newV),
	    "internTest: #1 of \"" + vString + "\""  );
	assert0( vString.equals(newV),
	    "internTest: #2 of \"" + vString + "\"" );
    }

    static void internTest(){
	char tarray[] = new char[4];
	tarray[0] = 't';
	tarray[1] = 'r';
	tarray[2] = 'u';
	tarray[3] = 'e';

	internSelf( tarray );

	tarray[3] = 'X';
	internSelf( tarray );
	assert0("UniqueStringXYZ" == "UniqueStringXYZ",
	    "internTest: #3");
	assert0(InternStringHelper1.getUniqueString() == 
	    InternStringHelper2.getUniqueString(),
	    "internTest: #4");
    }

    static void callA( simpleA a ){
	assert0( a.aMethod() == C.Avalue, "test5: a.aMethod()");
    }
    static void callB( simpleB b ){
	assert0( b.bMethod() == C.Bvalue, "test5: b.bMethod()");
    }
    static void test5(){
	C cObject = new C();
	callA( cObject );
	callB( cObject );
    }
    
    static int mult(int a, int b) {
	int z=0;
	try {
	    z =  a * b;
	} catch ( Exception e ){
	    z = -1;
	}
	return z;
    }

    static int recurse(int count) {
	int oldvalue = CVM.clearDebugFlags(-1);
	System.out.print("...recurse\n");
	CVM.restoreDebugFlags(-1, oldvalue);
	if (count > 0)
	    return count + recurse(count - 1);
	return 0;
    }

    /*
     * Put t on the end of the linked list.
     * Increment nTrailers for each element
     * already on the list.
     */
    int link( Test t ){
	int oldvalue = CVM.clearDebugFlags(-1);
	System.out.print("...link\n");
	CVM.restoreDebugFlags(-1, oldvalue);
	nTrailers += 1;
	if ( next == null ){
	    next = t;
	    return 1;
	} else {
	    return 1+next.link( t );
	}
    }

    static int testStaticInitalizers() {
	//return StaticB.foo2;
	try {
	    return StaticD.foo1 + StaticE.x;
	} catch (Throwable t) {
	    System.out.println("*<clinit> threw " + t);
	}
	try {
	    return StaticE.x;
	} catch (Throwable t) {
	    System.out.println("*<clinit> threw " + t);
	    return 0;
	}
    }
    
    static int arrayGetLengthCatchException(Object[] arr) {
	try {
	    return arr.length;
	} catch (NullPointerException e) {
	    System.out.println("test6: Caught inner "+e);
	    return -1;
	}
    }

    static int arrayGetLengthDontCatchException(Object[] arr) {
	return arr.length;
    }

    //
    // Testing stackmaps while throwing exception
    //
    static void test6() {
	boolean didCatchOuter = false;
	assert0( arrayGetLengthCatchException(null) == -1,
	    "test 6: arrayGetLengthCatchException(null)" );
	try {
	    arrayGetLengthDontCatchException(null);
	} catch (NullPointerException e) {
	    System.out.println("test6: Caught outer "+e);
	    didCatchOuter = true;
	}
	assert0( didCatchOuter, "test6: outer catch");
    }

    static void testArrayCopy() {
	char[]    carr1 = new char[3], carr2 = new char[4];
	float[]   farr1 = new float[3], farr2 = new float[4];
	double[]  darr1 = new double[3], darr2 = new double[4];
	int[]     iarr1 = new int[3], iarr2 = new int[4];
	long[]    larr1 = new long[3], larr2 = new long[4];
	Object[]  oarr1 = new Object[3], oarr2 = new Object[4];

	System.out.println("Testing Array Copy");

	/*
	 * A copy of a 16-bit size. Char in this case
	 */
	carr1[0] = 'a'; carr1[1] = 'b'; carr1[2] = 'c';
	System.arraycopy(carr1, 0, carr2, 1, carr1.length);
	assert0 ((carr2[0] == '\0') && (carr2[1] == 'a') &&
	    (carr2[2] == 'b') && (carr2[3] == 'c'), "testArrayCopy: Char copy");

	/*
	 * A copy of a 32-bit floating point
	 */
	farr1[0] = (float)1.0; farr1[1] = (float)2.0; farr1[2] = (float)3.0;
	System.arraycopy(farr1, 0, farr2, 1, farr1.length);
	assert0((farr2[0] == 0) && (farr2[1] == 1.0) &&
	    (farr2[2] == 2.0) && (farr2[3] == 3.0), "testArrayCopy: Float copy");

	/*
	 * A copy of a 32-bit integer
	 */
	iarr1[0] = 1; iarr1[1] = 2; iarr1[2] = 3;
	System.arraycopy(iarr1, 0, iarr2, 1, iarr1.length);
	assert0( (iarr2[0] == 0) && (iarr2[1] == 1) &&
	    (iarr2[2] == 2) && (iarr2[3] == 3), "testArrayCopy: Int copy");

	/*
	 * A copy of a double precision floating point
	 */
	darr1[0] = 1.0; darr1[1] = 2.0; darr1[2] = 3.0;
	System.arraycopy(darr1, 0, darr2, 1, darr1.length);
	assert0( (darr2[0] == 0.0) && (darr2[1] == 1.0) &&
	    (darr2[2] == 2.0) && (darr2[3] == 3.0), "testArrayCopy: Double copy");
	/*
	 * A copy of a long
	 */
	larr1[0] = 1L; larr1[1] = 2L; larr1[2] = 3L;
	System.arraycopy(larr1, 0, larr2, 1, larr1.length);
	assert0( (larr2[0] == 0L) && (larr2[1] == 1L) &&
	    (larr2[2] == 2L) && (larr2[3] == 3L), "testArrayCopy: Long copy");

	/* 
	 * Reference copy
	 */
	oarr1[0] = new Integer(1); oarr1[1] = new Integer(2); 
	oarr1[2] = new Integer(3);
	System.arraycopy(oarr1, 0, oarr2, 1, oarr1.length);
	assert0( (oarr2[0] == null) && (oarr2[1] == oarr1[0]) &&
	    (oarr2[2] == oarr1[1]) && (oarr2[3] == oarr1[2]), "testArrayCopy: Object copy");
    }
    
    static void testArrayClassAccess() {
	Test[] tt1 = new Test[4];
	Test[][] tt2 = new Test[1][2];
	C[] cc1 = new C[4];
	C[][] cc2 = new C[1][2];
	cc1[1] = null;
	tt2[0][0] = null;
	Class c1 = tt1.getClass();
	Class c2 =  tt2.getClass();
	
	Class c3 = cc1.getClass();
	Class c4 =  cc2.getClass();

	System.out.println("c1 = "+c1);
	System.out.println("c2 = "+c2);
	System.out.println("c3 = "+c3);
	System.out.println("c4 = "+c4);

	System.out.println("c1.modifiers = "+c1.getModifiers());
	System.out.println("c2.modifiers = "+c2.getModifiers());
	System.out.println("c3.modifiers = "+c3.getModifiers());
	System.out.println("c4.modifiers = "+c4.getModifiers());
	assert0( (c1.getModifiers() & java.lang.reflect.Modifier.PUBLIC) ==
	    (Test.class.getModifiers() & java.lang.reflect.Modifier.PUBLIC),
	    "testArrayClassAccess: c1 is NOT as public as Test!");
	assert0( (c2.getModifiers() & java.lang.reflect.Modifier.PUBLIC) ==
	    (Test.class.getModifiers() & java.lang.reflect.Modifier.PUBLIC),
	    "testArrayClassAccess: c2 is NOT as public as Test!");
	assert0( (c3.getModifiers() & java.lang.reflect.Modifier.PUBLIC) ==
	    (C.class.getModifiers() & java.lang.reflect.Modifier.PUBLIC),
	    "testArrayClassAccess: c3 is NOT as public as class C!");
	assert0( (c4.getModifiers() & java.lang.reflect.Modifier.PUBLIC) ==
	    (C.class.getModifiers() & java.lang.reflect.Modifier.PUBLIC),
	    "testArrayClassAccess: c4 is NOT as public as class C!");
    }

    static void testCloning() {
	try {
	    /* should succeed on cloneable objects */
	    CloneableObject co = new CloneableObject();
	    Object o = co.clone();
	    assert0( co.equals(o),
		"testCloning: Cloned object not the same as original");
	} catch (Throwable e) {
	    System.out.println("Failed clone test: Cloneable cloning failed!");
	    nFailure += 1;
	}
	Object y = null;
	Object[] x = new Object[3];
	try {
	    y = x.clone(); /* should succeed on arrays */
	} catch (Throwable e) {
	    System.out.println("Failed clone test: array cloning failed!");
	    nFailure += 1;
	}
	boolean cloneExceptionThrown = false;
	try {
	    /* should fail on non-cloneable objects */
	    NonCloneableObject nco = new NonCloneableObject();
	    Object o = nco.clone();
	} catch (CloneNotSupportedException e) {
	    cloneExceptionThrown = true;
	} catch (Throwable e) {
	    cloneExceptionThrown = false;
	} finally {
	    assert0( cloneExceptionThrown,
		"testCloning: CloneNotSupportedException not thrown" );
	}
    }

    final static int expectedFloatMinIntBits = 1;
    final static int expectedFloatMaxIntBits = 0x7f7fffff;

    static void testFloatBits() {
	System.out.println("*FloatMIN ="+Float.MIN_VALUE);	
	System.out.println("*FloatMAX ="+Float.MAX_VALUE);

	System.out.println("FloatMIN (the int bits) ="+
			   Float.floatToIntBits(Float.MIN_VALUE));
	System.out.println("FloatMAX (the int bits) ="+
			   Float.floatToIntBits(Float.MAX_VALUE));
	assert0( Float.floatToIntBits(Float.MIN_VALUE) == expectedFloatMinIntBits,
	    "testFloatBits: Float.MIN_VALUE as int bits");
	assert0( Float.floatToIntBits(Float.MAX_VALUE) == expectedFloatMaxIntBits,
	    "testFloatBits: Float.MAX_VALUE as int bits");

	assert0( Float.intBitsToFloat(Float.floatToIntBits(Float.MIN_VALUE))
		== Float.MIN_VALUE, "FloatMIN (the two-way transformation)" );
	assert0( Float.intBitsToFloat(Float.floatToIntBits(Float.MAX_VALUE))
		== Float.MAX_VALUE, "FloatMAX (the two-way transformation)" );
    }

    static final int expectedNDimensions = 255;

    static void testDeepArrayConstruction(){
	Object res = null;
	int nDimensions = 0;
	try {
	    int [] dim = {1};
	    int depth = 287;
	    Class basetype = (new Test()).getClass();
	    for ( int i = 0 ; i < depth; i ++ ){
		res = java.lang.reflect.Array.newInstance( basetype, dim );
		basetype = res.getClass();
		nDimensions += 1;
	    }
	} catch ( Exception e ){
	    e.printStackTrace();
	}
	if ( res != null ){
	    System.out.print("Constructed an object of type ");
	    System.out.println( res.getClass().getName());
	}
	assert0( nDimensions == expectedNDimensions, "testDeepArrayConstruction: constructed array depth of "+nDimensions );

	/*
	 * As an ancillary test, instantiate an array of a basetype class,
	 * which is in a package, and that package is not preloaded.
	 * Later on (when this whole program is done) that array type will be
	 * unloaded, and packagename typeid refcounting will get exercised.
	 * See bugid 4333203
	 */
	Object bug_o = new cvmtest.TypeidRefcountHelper[1][][][][][][][][];
    }

    static void testManyFieldsAndMethods() {
	ManyFieldsAndMethods many = new ManyFieldsAndMethods();
	Class c = many.getClass();
	int i;
	try {
	    for (i = 0; i < 4*256; i++) {
		if (i % 25 == 0) {
		    System.out.print(".");
		}
		if (i == 100) i+= 100;
		if (i == 300) i+= 200;
		if (i == 600) i+= 400;
		java.lang.reflect.Method m = c.getMethod("method" + i, null);
		Integer result = (Integer)m.invoke(many, null);
		if ( !assert0( result.intValue() == i,
		    "testManyFieldsAndMethods: method"+i ) ){
		    return;
		}
	    }
	} catch (Throwable e) {
	    System.out.println("\ntestManyFieldsAndMethods() failed: ");
	    e.printStackTrace();
	    nFailure += 1;
	}
	many.field678 = -1;
	assert0( many.method499() + many.method678() == 498,
	    "testManyFieldsAndMethods: many.method499() + many.method678() == 498");
    }
}

class CloneableObject implements Cloneable {
    int a;
    float b;
    Object o;
    double d;
    boolean z;
    long l;
    short s;

    CloneableObject() {
	a = 5;
	b = (float)6.0;
	o = new Object();
	d = 7.0;
	z = true;
	l = 8L;
	s = 9;
    }

    protected Object clone() {
	try {
	    return super.clone();
	} catch (CloneNotSupportedException e) {
	    return null;
	}
    }

    public boolean equals(Object o) {
	try {
	    CloneableObject co = (CloneableObject)o;
	    return ((co.a == this.a) &&
		    (co.b == this.b) &&
		    (co.o.equals(this.o)) &&
		    (co.d == this.d) &&
		    (co.z == this.z) &&
		    (co.l == this.l) &&
		    (co.s == this.s));
	} catch (Throwable e) {
	    System.out.println("Casting to CloneableObject failed: "+o);
	    return false;
	}
    }

    public String toString() {
	return 
	    "a = "+a+", "+
	    "b = "+b+", "+
	    "o = "+o+", "+
	    "d = "+d+", "+
	    "z = "+z+", "+
	    "l = "+l+", "+
	    "s = "+s;
    }
}

class NonCloneableObject {
    int justData;

    protected Object clone() throws CloneNotSupportedException {
	return super.clone();
    }
}


interface simpleA {
    public int aMethod();
}

interface simpleB {
    public int bMethod();
}

class C implements simpleA, simpleB {
    final static int Avalue = 1;
    final static int Bvalue = 2;
    final static int Cvalue = 3;

    public int aMethod(){
	return Avalue;
    }
    public int bMethod(){
	return Bvalue;
    }

    // public abstract int cMethod();
}

class subC extends C {
    final static int subCvalue = 3;

    public int cMethod(){ 
	return subCvalue;
    }
}


class StaticA {
    static {
	Test.clinitIndicator <<= Test.clinitShift;
	Test.clinitIndicator |= Test.clinitA;
    }
    static int foo1 = 3;
    static int foo2 = StaticB.foo1 + StaticB.foo2;  /* 4 + 3 */
}

/*
 * <clinit> tests.
 */

class StaticB extends StaticA {
    static {
	Test.clinitIndicator <<= Test.clinitShift;
	Test.clinitIndicator |= Test.clinitB;
    }
    static int foo1 = 4;
    static int foo2 = StaticA.foo1;   /* 3 */
    static int foo3 = StaticA.foo2;   /* 0 */
}


interface StaticI1 {
    static int foo1 = 88;
}

interface StaticI2 {
    static int foo1 = StaticC.init();
}

class TestE extends Throwable {
    TestE(String msg) {
	super(msg);
    }
};

class StaticC {
    static {
	Test.clinitIndicator <<= Test.clinitShift;
	Test.clinitIndicator |= Test.clinitC;
	StaticD.foo1 = 2;
	try {
	    StaticD.foo2 = StaticD.init2(6);
	} catch (TestE e) {
	    System.out.print("*TestE exception thrown because \"" +
			 e.getMessage() + "\"\n");
	    StaticD.foo2 = 5;
	}
    }
    static int init() {return StaticD.foo1;}
}

class StaticD implements StaticI1, StaticI2 {
    static {
	Test.clinitIndicator <<= Test.clinitShift;
	Test.clinitIndicator |= Test.clinitD;
    }
    static int foo1 = StaticI1.foo1 + StaticC.init();   /* 7 + 2 */
    static int foo2;
    static int init2(int val) throws TestE {
	if (val == 6)
	    throw new TestE("I'm feeling Testy");
	return 5;
    }
}

class StaticE {
    static {
	Test.clinitIndicator <<= Test.clinitShift;
	Test.clinitIndicator |= Test.clinitE;
    }
    static int x;
    static {
	x = StaticF.x;
    }
}

class StaticF {
    static {
	Test.clinitIndicator <<= Test.clinitShift;
	Test.clinitIndicator |= Test.clinitF;
    }
    static int x;
    static {
	x = StaticG.x;
    }
}

class StaticG {
    static {
	Test.clinitIndicator <<= Test.clinitShift;
	Test.clinitIndicator |= Test.clinitG;
    }
    static int x;
    static {
	x = 1;
	if (x > 0) {
	    throw new RuntimeException("StaticG Error");
	}
    }
}

class InternStringHelper1 {
    static String getUniqueString() {
	return "UniqueStringXYZ";
    }
}

class InternStringHelper2 {
    static String getUniqueString() {
	return "UniqueStringXYZ";
    }
}

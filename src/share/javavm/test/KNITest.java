/*
 * @(#)KNITest.java	1.7 06/10/17
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
 */

public class KNITest {
    int i;
    static double d;
    public static void error(String errStr) {
	System.out.println("Failed: " + errStr);
    }

    static {
	try {
	    System.loadLibrary("KNITest");
	} catch (Throwable e) {
	    System.out.println("Fail to load KNITest dll: " +
			       e.getMessage());
	}
    }

    public static void main(String[] args) {
	KNITest kniTest = new KNITest();
	if (kniTest.returnThis() != kniTest) {
	    error("returnThis()");
	}
	if (returnClass() != KNITest.class) {
	    error("returnClass()");
	}
	if (testIntArgs(20,100000,55) != 20*100000+55) {
	    error("testIntArg()");
	}
	if (testFloatArgs(20.03f,  100000.011f,  55.123f) !=
	                  20.03f * 100000.011f + 55.123f) {
	    error("testFloatArgs()");
	}
	if (testLongArgs(20,100000,55) != 20*100000+55) {
	    error("testLongArgs()");
	}
	if (testDoubleArgs(20.03d,  100000.011d,  55.123d) !=
	                   20.03d * 100000.011d + 55.123d) {
	    error("testDoubleArgs()");
	}
	if (testIntLongArgs(20,100000,55) != 20*100000+55) {
	    error("testIntLongArgs()");
	}
	if (testFloatDoubleArgs(20.03d,  100000.011f,  55.123d) !=
	                        20.03d * 100000.011f + 55.123d) {
	    error("testFloatDoubleArgs()");
	}
	if (kniTest.testVirtualArg(55) != 2*55) {
	    error("testVirtualArg()");
	}
	kniTest.i = 30;
	kniTest.testGetSetIntField(60);
	if (kniTest.i != 30*60) {
	    error("testGetSetIntField()");
	}
	d = 30.145;
	testGetSetStaticDoubleField(60.55);
	if (d != 30.145*60.55) {
	    error("testGetSetStaticDoubleField()");
	}
	if (testFindClass() != java.lang.String.class) {
	    error("testFindClass()");
	}
	if (getSuperClass(OutOfMemoryError.class) != VirtualMachineError.class) {
	    error("testGetSuperClass()");
	}
	if (!isAssignableFrom(OutOfMemoryError.class, Error.class)) {
	    error("isAssignableFrom() #1");
	}
	if (isAssignableFrom(OutOfMemoryError.class, KNITest.class)) {
	    error("isAssignableFrom() #2");
	}
	try {
	    throwException();
	} catch (java.io.IOException e1) {
	    // e1.printStackTrace();
	} catch (Throwable e2) {
	    //e2.printStackTrace();
	    error("printStackTrace");
	}
	if (!isInstanceOf(new OutOfMemoryError(), Error.class)) {
	    error("isInstanceOf() #1");
	}
	if (isInstanceOf(new OutOfMemoryError(), KNITest.class)) {
	    error("isInstanceOf() #2");
	}
	if (!isSameObject(kniTest, kniTest)) {
	    error("isSameObject() #1");
	}
	if (isSameObject(kniTest, new KNITest())) {
	    error("isSameObject() #2");
	}
	String str = "helloworld";
	if (getStringLength(str) != str.length()) {
	    error("getStringLength()");
	}
	char buf[] = new char[3];
	;
	if (!getStringRegion(str, 2, 3).equals(new String(str.toCharArray(),2,3))) {
	    error("getStringRegion()");
	}
	if (!newStringUTF().equals("hello")) {
	    error("newStringUTF()");
	}
        if (newString() != null) {
            error("newString()");
        }
	
	String[] strs = (String[]) newStringArray(10);
	if (strs.length != 10) {
            error("newStringArray()");
	}

	KNITest[] arr = (KNITest[]) newObjectArray(KNITest.class, 10);
	if (arr.length != 10) {
            error("newObjectArray()");
	}

	System.out.println("Done!");
    }

    public native KNITest returnThis();
    public native static Class returnClass();

    public native static int testIntArgs(int x, int y, int z);
    public native static float testFloatArgs(float x, float y, float z);
    public native static long testLongArgs(long x, long y, long z);
    public native static double testDoubleArgs(double x, double y, double z);

    public native static long testIntLongArgs(long x, int y, long z);
    public native static double testFloatDoubleArgs(double x, float y, double z);

    public native int testVirtualArg(int x);
    public native void testGetSetIntField(int x);
    public native static void testGetSetStaticDoubleField(double x);

    public native static Class testFindClass();
    public native static Class getSuperClass(Class clazz);
    public native static boolean isAssignableFrom(Class srcClass,
						Class destClass);

    public native static void throwException() throws java.io.IOException;

    public native static boolean isInstanceOf(Object srcObj, Class srcClass);
    public native static boolean isSameObject(Object obj1, Object obj2);

    public native static int getStringLength(String str);
    public native static String getStringRegion(String str, int offset,
						int len);
    public native static String newStringUTF();
    public native static String newString();

    public native static Object newStringArray(int len);
    public native static Object newObjectArray(Class clazz, int len);
}

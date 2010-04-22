/*
 * @(#)ReflectionTest.java	1.18 06/10/10
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
import java.lang.reflect.*;
import cvmtest.ReflectionTestOtherPackageHelper;

/** Fairly comprehensive test of Method.invoke() and
  java.lang.Field.{set,get} functionality. Tests most, if not all,
  widening conversions and their exception-throwing cases. Do not
  rename this class without modifying the using code below.

  @author Kenneth Russell */

public class ReflectionTest {

    // Do not make any changes to these inner class hierarchies,
    // including class names and how many inner classes are declared,
    // without taking care to modify the using code below!

    public static class A {
    }

    public static class B extends A {
    }

    public ReflectionTest() {
    }

    public ReflectionTest(int i) {
	intField = i;
    }

    public ReflectionTest(double d, B b) {
	doubleField = d;
	bField = b;
    }

    public ReflectionTest(B b, double d) {
	doubleField = d;
	bField = b;
    }

    public static class D {
	public static class E {
	}
    }
    public static class F {
    }
    
    public static abstract class AbstractClass {
	int intField;

	public AbstractClass(int i) {
	    intField = i;
	}
	public abstract void abstractMethod();
    }

    public boolean booleanField = false;
    public byte byteField = 0;
    public char charField = 'a';
    public short shortField = 0;
    public int intField = 0;
    public float floatField = 0;
    public double doubleField = 0;
    public long longField = 0;
    public B bField = null;
    public boolean voidMethodCalled = false;

    public void setBooleanField(boolean newVal) {
	System.out.println("setBooleanField(" + newVal + ")");
	booleanField = newVal;
    }

    public void setByteField(byte newVal) {
	System.out.println("setByteField(" + newVal + ")");
	byteField = newVal;
    }

    public void setCharField(char newVal) {
	System.out.println("setCharField(" + newVal + ")");
	charField = newVal;
    }

    public void setShortField(short newVal) {
	System.out.println("setShortField(" + newVal + ")");
	shortField = newVal;
    }

    public void setIntField(int newVal) {
	System.out.println("setIntField(" + newVal + ")");
	intField = newVal;
    }

    public void setFloatField(float newVal) {
	System.out.println("setFloatField(" + newVal + ")");
	floatField = newVal;
    }

    public void setDoubleField(double newVal) {
	System.out.println("setDoubleField(" + newVal + ")");
	doubleField = newVal;
    }

    public void setLongField(long newVal) {
	System.out.println("setLongField(" + newVal + ")");
	longField = newVal;
    }

    public boolean getBooleanField() {
	System.out.println("getBooleanField() returning " + booleanField);
	return booleanField;
    }

    public byte getByteField() {
	System.out.println("getByteField() returning " + byteField);
	return byteField;
    }

    public char getCharField() {
	System.out.println("getCharField() returning " + charField);
	return charField;
    }

    public short getShortField() {
	System.out.println("getShortField() returning " + shortField);
	return shortField;
    }

    public int getIntField() {
	System.out.println("getIntField() returning " + intField);
	return intField;
    }

    public float getFloatField() {
	System.out.println("getFloatField() returning " + floatField);
	return floatField;
    }

    public double getDoubleField() {
	System.out.println("getDoubleField() returning " + doubleField);
	return doubleField;
    }

    public long getLongField() {
	System.out.println("getLongField() returning " + longField);
	return longField;
    }

    public void voidMethod() {
	voidMethodCalled = true;
    }

    public static void check(String message, boolean condition) {
	System.out.print(message + ": ");
	if (condition)
	    System.out.println("OK");
	else
	    System.out.println("**FAIL**");
    }

    public static boolean find(Class[] classes, String name) {
	for (int i = 0; i < classes.length; i++) {
	    if (classes[i].getName().equals(name))
		return true;
	}
	return false;
    }

    public static void main(String[] args) {
	ReflectionTest test = new ReflectionTest();
	Class myClass = test.getClass();
	Method setMethod = null;
	Method getMethod = null;
	boolean booleanVal;
	byte byteVal;
	char charVal;
	short shortVal;
	int intVal;
	float floatVal;
	double doubleVal;
	long longVal;
	Object retVal;
	boolean success;
	boolean allTestsPassed = true;
	boolean warnAtEnd = false;
	int numWarnings = 0;
	boolean runMethodInvokeTests = true;
	boolean runAccessTests = true;
	boolean runFieldTests = true;

	Class[] setArgTypes = new Class[1];
	setArgTypes[0] = myClass;
	Object[] setArgs = new Object[1];
	Object[] getArgs = new Object[0];
	
	if (runMethodInvokeTests) {
	    //
	    // Normal (non-exception) method tests
	    //
	    
	    // System.out.println("1");
	    // Method.invoke() tests
	    // set/getBooleanField
	    setArgTypes[0] = Boolean.TYPE;
	    success = false;
	    try {
		//System.out.println("2");
		setMethod = myClass.getMethod("setBooleanField", setArgTypes);
		//System.out.println("3");
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    // System.out.println("4");
	    check("getMethod(\"setBooleanField\")", success);
	    allTestsPassed &= success;
	    // System.out.println("5");
	    try {
		getMethod = myClass.getMethod("getBooleanField", null);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getMethod(\"getBooleanField\")", success);
	    allTestsPassed &= success;
	    booleanVal = true;
	    setArgs[0] = new Boolean(booleanVal);
	    success = false;
	    try {
		retVal = setMethod.invoke(test, setArgs);
		success = (retVal == null);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("invoke of setBoolean", success);
	    allTestsPassed &= success;
	    success = false;
	    try {
		retVal = getMethod.invoke(test, getArgs);
		// System.out.println("retval was a " +
		// retVal.getClass().getName());
		success = (((Boolean) retVal).booleanValue() == booleanVal);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("invoke of getBoolean", success);
	    allTestsPassed &= success;

	    // set/getByteField
	    setArgTypes[0] = Byte.TYPE;
	    success = false;
	    try {
		setMethod = myClass.getMethod("setByteField", setArgTypes);
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getMethod(\"setByteField\")", success);
	    allTestsPassed &= success;
	    success = false;
	    try {
		getMethod = myClass.getMethod("getByteField", null);
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getMethod(\"getByteField\")", success);
	    allTestsPassed &= success;
	    byteVal = 1;
	    setArgs[0] = new Byte(byteVal);
	    success = false;
	    try {
		retVal = setMethod.invoke(test, setArgs);
		success = (retVal == null);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("invoke of setByte", success);
	    allTestsPassed &= success;
	    success = false;
	    try {
		retVal = getMethod.invoke(test, getArgs);
		success = (((Byte) retVal).byteValue() == byteVal);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("invoke of getByte", success);
	    allTestsPassed &= success;

	    // set/getCharField
	    setArgTypes[0] = Character.TYPE;
	    success = false;
	    try {
		setMethod = myClass.getMethod("setCharField", setArgTypes);
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getMethod(\"setCharField\")", success);
	    allTestsPassed &= success;
	    success = false;
	    try {
		getMethod = myClass.getMethod("getCharField", null);
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getMethod(\"getCharField\")", success);
	    allTestsPassed &= success;
	    charVal = 'b';
	    setArgs[0] = new Character(charVal);
	    success = false;
	    try {
		retVal = setMethod.invoke(test, setArgs);
		success = (retVal == null);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("invoke of setChar", success);
	    allTestsPassed &= success;
	    success = false;
	    try {
		retVal = getMethod.invoke(test, getArgs);
		success = (((Character) retVal).charValue() == charVal);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("invoke of getChar", success);
	    allTestsPassed &= success;

	    // set/getShortField
	    setArgTypes[0] = Short.TYPE;
	    success = false;
	    try {
		setMethod = myClass.getMethod("setShortField", setArgTypes);
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getMethod(\"setShortField\")", success);
	    allTestsPassed &= success;
	    success = false;
	    try {
		getMethod = myClass.getMethod("getShortField", null);
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getMethod(\"getShortField\")", success);
	    allTestsPassed &= success;
	    shortVal = 7;
	    setArgs[0] = new Short(shortVal);
	    success = false;
	    try {
		retVal = setMethod.invoke(test, setArgs);
		success = (retVal == null);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("invoke of setShort", success);
	    allTestsPassed &= success;
	    success = false;
	    try {
		retVal = getMethod.invoke(test, getArgs);
		success = (((Short) retVal).shortValue() == shortVal);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("invoke of getShort", success);
	    allTestsPassed &= success;

	    // set/getIntField
	    setArgTypes[0] = Integer.TYPE;
	    success = false;
	    try {
		setMethod = myClass.getMethod("setIntField", setArgTypes);
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getMethod(\"setIntField\")", success);
	    allTestsPassed &= success;
	    success = false;
	    try {
		getMethod = myClass.getMethod("getIntField", null);
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getMethod(\"getIntField\")", success);
	    allTestsPassed &= success;
	    intVal = 1;
	    setArgs[0] = new Integer(intVal);
	    success = false;
	    try {
		retVal = setMethod.invoke(test, setArgs);
		success = (retVal == null);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("invoke of setInt", success);
	    allTestsPassed &= success;
	    success = false;
	    try {
		retVal = getMethod.invoke(test, getArgs);
		success = (((Integer) retVal).intValue() == intVal);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("invoke of getInt", success);
	    allTestsPassed &= success;

	    // set/getFloatField
	    setArgTypes[0] = Float.TYPE;
	    success = false;
	    try {
		setMethod = myClass.getMethod("setFloatField", setArgTypes);
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getMethod(\"setFloatField\")", success);
	    allTestsPassed &= success;
	    success = false;
	    try {
		getMethod = myClass.getMethod("getFloatField", null);
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getMethod(\"getFloatField\")", success);
	    allTestsPassed &= success;
	    floatVal = 2.0f;
	    setArgs[0] = new Float(floatVal);
	    success = false;
	    try {
		retVal = setMethod.invoke(test, setArgs);
		success = (retVal == null);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("invoke of setFloat", success);
	    allTestsPassed &= success;
	    success = false;
	    try {
		retVal = getMethod.invoke(test, getArgs);
		success = (((Float) retVal).floatValue() == floatVal);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("invoke of getFloat", success);
	    allTestsPassed &= success;

	    // set/getDoubleField
	    setArgTypes[0] = Double.TYPE;
	    success = false;
	    try {
		setMethod = myClass.getMethod("setDoubleField", setArgTypes);
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getMethod(\"setDoubleField\")", success);
	    allTestsPassed &= success;
	    success = false;
	    try {
		getMethod = myClass.getMethod("getDoubleField", null);
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getMethod(\"getDoubleField\")", success);
	    allTestsPassed &= success;
	    doubleVal = 3.0;
	    setArgs[0] = new Double(doubleVal);
	    success = false;
	    try {
		retVal = setMethod.invoke(test, setArgs);
		success = (retVal == null);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("invoke of setDouble", success);
	    allTestsPassed &= success;
	    success = false;
	    try {
		retVal = getMethod.invoke(test, getArgs);
		success = (((Double) retVal).doubleValue() == doubleVal);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("invoke of getDouble", success);
	    allTestsPassed &= success;

	    // set/getLongField
	    setArgTypes[0] = Long.TYPE;
	    success = false;
	    try {
		setMethod = myClass.getMethod("setLongField", setArgTypes);
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getMethod(\"setLongField\")", success);
	    allTestsPassed &= success;
	    success = false;
	    try {
		getMethod = myClass.getMethod("getLongField", null);
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getMethod(\"getLongField\")", success);
	    allTestsPassed &= success;
	    longVal = 1;
	    setArgs[0] = new Long(longVal);
	    success = false;
	    try {
		retVal = setMethod.invoke(test, setArgs);
		success = (retVal == null);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("invoke of setLong", success);
	    allTestsPassed &= success;
	    success = false;
	    try {
		retVal = getMethod.invoke(test, getArgs);
		success = (((Long) retVal).longValue() == longVal);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("invoke of getLong", success);
	    allTestsPassed &= success;

	    // voidMethod
	    setArgTypes = null;
	    success = false;
	    test.voidMethodCalled = false;
	    try {
		setMethod = myClass.getMethod("voidMethod", setArgTypes);
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getMethod(\"voidMethod\") (1)", success);
	    allTestsPassed &= success;
	    setArgs = null;
	    try {
		retVal = setMethod.invoke(test, setArgs);
		success = ((retVal == null) && (test.voidMethodCalled = true));
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("invoke of voidMethod (1)", success);
	    allTestsPassed &= success;

	    // voidMethod
	    setArgTypes = new Class[0];
	    success = false;
	    test.voidMethodCalled = false;
	    try {
		setMethod = myClass.getMethod("voidMethod", setArgTypes);
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getMethod(\"voidMethod\") (2)", success);
	    allTestsPassed &= success;
	    setArgs = new Object[0];
	    try {
		retVal = setMethod.invoke(test, setArgs);
		success = ((retVal == null) && (test.voidMethodCalled = true));
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("invoke of voidMethod (2)", success);
	    allTestsPassed &= success;
	    setArgTypes = new Class[1];
	    setArgs = new Object[1];

	    //
	    // Abnormal (exception-throwing) method tests
	    //

	    // null object argument to invocation
	    try {
		setArgTypes[0] = Integer.TYPE;
		setMethod = myClass.getMethod("setIntField", setArgTypes);
	    }
	    catch (Exception e) {
		e.printStackTrace();
	    }
	    success = false;
	    test.intField = 0;
	    setArgs[0] = new Integer(1);
	    try {
		setMethod.invoke(null, setArgs);
	    }
	    catch (NullPointerException e) {
		success = true;
	    }
	    catch (Exception e) {
	    }
	    check("invoke with null object argument", success);
	    allTestsPassed &= success;

	    // illegal conversion of argument
	    setArgs[0] = new Double(1.0);
	    success = false;
	    try {
		setMethod.invoke(test, setArgs);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
	    }
	    check("invoke with illegal argument conversion", success);
	    allTestsPassed &= success;
	
	    // null argument object
	    setArgs[0] = null;
	    success = false;
	    try {
		setMethod.invoke(test, setArgs);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (NullPointerException e) {
		System.out.println(
		    "*** WARNING: JDK 1.2 throws a NullPointerException here"
		);
		System.out.println(
		    "where the spec indicates an IllegalArgumentException "
		);
		System.out.println(
		    "should be thrown. Saying the test succeeded anyway.");
		success = true;
		warnAtEnd = true;
		numWarnings++;
	    }
	    catch (Exception e) {
		e.printStackTrace();
	    }
	    check("invoke with null argument object", success);
	    allTestsPassed &= success;

	    // wrong number of arguments
	    setArgTypes = null;
	    success = false;
	    test.voidMethodCalled = false;
	    try {
		setMethod = myClass.getMethod("voidMethod", setArgTypes);
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getMethod(\"voidMethod\") (3)", success);
	    allTestsPassed &= success;
	    setArgs = new Object[1];
	    setArgs[0] = null;
	    try {
		retVal = setMethod.invoke(test, setArgs);
		success = false;
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("invoke of voidMethod with wrong number of arguments",
		  success);
	    allTestsPassed &= success;
	    setArgTypes = new Class[1];
	}

	if (runAccessTests) {
	    //
	    // Access Tests
	    //
		
	    ReflectionTestOtherPackageHelper oph =
		new ReflectionTestOtherPackageHelper();
	    try {
		myClass = ReflectionTestOtherPackageHelper.class;
		getMethod = myClass.getDeclaredMethod("protectedVoidMethod",
						      null);
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getMethod(\"cvmtest.ReflectionTestOtherPackageHelper." +
		  "protectedVoidMethod\")", success);
	    allTestsPassed &= success;
	    try {
		getMethod.invoke(oph, null);
		success = false;
	    }
	    catch (IllegalAccessException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("protectedVoidMethod invoke exception case", success);
	    allTestsPassed &= success;

	    ReflectionTestSamePackageHelper sph =
		new ReflectionTestSamePackageHelper();
	    try {
		myClass = ReflectionTestSamePackageHelper.class;
		getMethod = myClass.getDeclaredMethod("protectedVoidMethod",
						      null);
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getMethod(\"ReflectionTestSamePackageHelper." +
		  "protectedVoidMethod\")", success);
	    allTestsPassed &= success;
	    try {
		getMethod.invoke(sph, null);
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("invoke of ReflectionTestSamePackageHelper." + 
		  "protectedVoidMethod", success);
	    allTestsPassed &= success;
	}

	if (runFieldTests) {

	    //
	    // Field tests
	    //

	    Field field = null;
	    myClass = ReflectionTest.class;

	    //
	    // Field.get() of all types
	    //

	    ///////////////////
	    // boolean field //
	    ///////////////////
	    test.booleanField = false;
	    try {
		field = myClass.getField("booleanField");
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getField(\"booleanField\")", success);
	    allTestsPassed &= success;
	    try {
		Boolean obj = (Boolean) field.get(test);
		success = (obj.booleanValue() == false);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive boolean field.get()", success);
	    allTestsPassed &= success;
	    booleanVal = true;
	    test.booleanField = booleanVal;
	    try {
		success = (field.getBoolean(test) == booleanVal);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive boolean field.getBoolean()", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		byte foo = field.getByte(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive boolean field.getByte() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		char foo = field.getChar(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive boolean field.getChar() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		short foo = field.getShort(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive boolean field.getShort() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		int foo = field.getInt(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive boolean field.getInt() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		long foo = field.getLong(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive boolean field.getLong() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		float foo = field.getFloat(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive boolean field.getFloat() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		double foo = field.getDouble(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive boolean field.getDouble() exception case", success);
	    allTestsPassed &= success;

	    ////////////////
	    // byte field //
	    ////////////////
	    test.byteField = 5;
	    try {
		field = myClass.getField("byteField");
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getField(\"byteField\")", success);
	    allTestsPassed &= success;
	    try {
		Byte obj = (Byte) field.get(test);
		success = (obj.byteValue() == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive byte field.get()", success);
	    allTestsPassed &= success;
	    test.byteField = 7;
	    try {
		success = (field.getByte(test) == 7);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive byte field.getByte()", success);
	    allTestsPassed &= success;
	    try {
		success = (field.getShort(test) == 7);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive byte field.getShort()", success);
	    allTestsPassed &= success;
	    try {
		success = (field.getInt(test) == 7);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive byte field.getInt()", success);
	    allTestsPassed &= success;
	    try {
		success = (field.getLong(test) == 7);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive byte field.getLong()", success);
	    allTestsPassed &= success;
	    try {
		success = (field.getFloat(test) == 7.0f);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive byte field.getFloat()", success);
	    allTestsPassed &= success;
	    try {
		success = (field.getDouble(test) == 7.0);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive byte field.getDouble()", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		boolean foo = field.getBoolean(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive byte field.getBoolean() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		char foo = field.getChar(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive byte field.getChar() exception case", success);
	    allTestsPassed &= success;

	    ////////////////
	    // char field //
	    ////////////////
	    test.charField = 'c';
	    try {
		field = myClass.getField("charField");
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getField(\"charField\")", success);
	    allTestsPassed &= success;
	    try {
		Character obj = (Character) field.get(test);
		success = (obj.charValue() == 'c');
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive char field.get()", success);
	    allTestsPassed &= success;
	    test.charField = 'd';
	    try {
		success = (field.getChar(test) == 'd');
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive char field.getChar()", success);
	    allTestsPassed &= success;
	    try {
		success = (field.getInt(test) == (int) 'd');
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive char field.getInt()", success);
	    allTestsPassed &= success;
	    try {
		success = (field.getLong(test) == (long) 'd');
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive char field.getLong()", success);
	    allTestsPassed &= success;
	    try {
		success = (field.getFloat(test) == (float) 'd');
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive char field.getFloat()", success);
	    allTestsPassed &= success;
	    try {
		success = (field.getDouble(test) == (double) 'd');
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive char field.getDouble()", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		boolean foo = field.getBoolean(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive char field.getBoolean() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		byte foo = field.getByte(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive char field.getByte() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		short foo = field.getShort(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive char field.getShort() exception case", success);
	    allTestsPassed &= success;
	
	    /////////////////
	    // short field //
	    /////////////////
	    test.shortField = 6;
	    try {
		field = myClass.getField("shortField");
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getField(\"shortField\")", success);
	    allTestsPassed &= success;
	    try {
		Short obj = (Short) field.get(test);
		success = (obj.shortValue() == 6);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive short field.get()", success);
	    allTestsPassed &= success;
	    test.shortField = 5;
	    try {
		success = (field.getShort(test) == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive short field.getShort()", success);
	    allTestsPassed &= success;
	    try {
		success = (field.getInt(test) == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive short field.getInt()", success);
	    allTestsPassed &= success;
	    try {
		success = (field.getLong(test) == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive short field.getLong()", success);
	    allTestsPassed &= success;
	    try {
		success = (field.getFloat(test) == 5.0f);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive short field.getFloat()", success);
	    allTestsPassed &= success;
	    try {
		success = (field.getDouble(test) == 5.0);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive short field.getDouble()", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		boolean foo = field.getBoolean(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive short field.getBoolean() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		byte foo = field.getByte(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive short field.getByte() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		char foo = field.getChar(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive short field.getChar() exception case", success);
	    allTestsPassed &= success;

	    ///////////////
	    // int field //
	    ///////////////
	    test.intField = 7;
	    try {
		field = myClass.getField("intField");
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getField(\"intField\")", success);
	    allTestsPassed &= success;
	    try {
		Integer obj = (Integer) field.get(test);
		success = (obj.intValue() == 7);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive int field.get()", success);
	    allTestsPassed &= success;
	    test.intField = 8;
	    try {
		success = (field.getInt(test) == 8);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive int field.getInt()", success);
	    allTestsPassed &= success;
	    try {
		success = (field.getLong(test) == 8);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive int field.getLong()", success);
	    allTestsPassed &= success;
	    try {
		success = (field.getFloat(test) == 8.0f);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive int field.getFloat()", success);
	    allTestsPassed &= success;
	    try {
		success = (field.getDouble(test) == 8.0);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive int field.getDouble()", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		boolean foo = field.getBoolean(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive int field.getBoolean() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		byte foo = field.getByte(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive int field.getByte() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		char foo = field.getChar(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive int field.getChar() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		short foo = field.getShort(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive int field.getShort() exception case", success);
	    allTestsPassed &= success;

	    /////////////////
	    // float field //
	    /////////////////
	    test.floatField = 4.0f;
	    try {
		field = myClass.getField("floatField");
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getField(\"floatField\")", success);
	    allTestsPassed &= success;
	    try {
		Float obj = (Float) field.get(test);
		success = (obj.floatValue() == 4.0f);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive float field.get()", success);
	    allTestsPassed &= success;
	    test.floatField = 5.0f;
	    try {
		success = (field.getFloat(test) == 5.0f);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive float field.getFloat()", success);
	    allTestsPassed &= success;
	    try {
		success = (field.getDouble(test) == 5.0);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive float field.getDouble()", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		boolean foo = field.getBoolean(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive float field.getBoolean() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		byte foo = field.getByte(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive float field.getByte() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		char foo = field.getChar(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive float field.getChar() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		short foo = field.getShort(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive float field.getShort() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		int foo = field.getInt(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive float field.getInt() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		long foo = field.getLong(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive float field.getLong() exception case", success);
	    allTestsPassed &= success;

	    //////////////////
	    // double field //
	    //////////////////
	    test.doubleField = 2.0;
	    try {
		field = myClass.getField("doubleField");
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getField(\"doubleField\")", success);
	    allTestsPassed &= success;
	    try {
		Double obj = (Double) field.get(test);
		success = (obj.doubleValue() == 2.0);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive double field.get()", success);
	    allTestsPassed &= success;
	    test.doubleField = 3.0;
	    try {
		success = (field.getDouble(test) == 3.0);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive double field.getDouble()", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		boolean foo = field.getBoolean(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive double field.getBoolean() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		byte foo = field.getByte(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive double field.getByte() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		char foo = field.getChar(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive double field.getChar() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		short foo = field.getShort(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive double field.getShort() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		int foo = field.getInt(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive double field.getInt() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		long foo = field.getLong(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive double field.getLong() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		float foo = field.getFloat(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive double field.getFloat() exception case", success);
	    allTestsPassed &= success;

	    ////////////////
	    // long field //
	    ////////////////
	    test.longField = 17;
	    try {
		field = myClass.getField("longField");
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getField(\"longField\")", success);
	    allTestsPassed &= success;
	    try {
		Long obj = (Long) field.get(test);
		success = (obj.longValue() == 17);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive long field.get()", success);
	    allTestsPassed &= success;
	    test.longField = 15;
	    try {
		success = (field.getLong(test) == 15);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive long field.getLong()", success);
	    allTestsPassed &= success;
	    try {
		success = (field.getFloat(test) == 15.0f);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive long field.getFloat()", success);
	    allTestsPassed &= success;
	    try {
		success = (field.getDouble(test) == 15.0);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive long field.getDouble()", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		boolean foo = field.getBoolean(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive long field.getBoolean() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		byte foo = field.getByte(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive long field.getByte() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		char foo = field.getChar(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive long field.getChar() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		short foo = field.getShort(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive long field.getShort() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		int foo = field.getInt(test);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive long field.getInt() exception case", success);
	    allTestsPassed &= success;

	    //
	    // Field.set()
	    //

	    //////////////////
	    // double field //
	    //////////////////
	    try {
		field = myClass.getField("doubleField");
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getField(\"doubleField\")", success);
	    allTestsPassed &= success;
	    try {
		field.set(test, new Double(14.0));
		success = (test.doubleField == 14.0);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive double field.set()", success);
	    allTestsPassed &= success;
	
	    /////////////////
	    // short field //
	    /////////////////
	    try {
		field = myClass.getField("shortField");
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getField(\"shortField\")", success);
	    allTestsPassed &= success;
	    shortVal = 10;
	    try {
		field.set(test, new Short(shortVal));
		success = (test.shortField == shortVal);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive short field.set()", success);
	    allTestsPassed &= success;

	    //////////////////////
	    // Object (B) field //
	    //////////////////////
	    B bVal = new B();
	    try {
		field = myClass.getField("bField");
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getField(\"bField\")", success);
	    allTestsPassed &= success;
	    try {
		field.set(test, bVal);
		success = (test.bField == bVal);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("Object (B) field.set()", success);
	    allTestsPassed &= success;
	    try {
		field.set(test, null);
		success = (test.bField == null);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("Object (B) field.set(null)", success);
	    allTestsPassed &= success;
	
	    ////////////////////////////////////
	    // Wrong type for primitive field //
	    ////////////////////////////////////
	    try {
		field = myClass.getField("shortField");
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getField(\"shortField\")", success);
	    allTestsPassed &= success;
	    try {
		field.set(test, bVal);
		success = false;
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive short field set() wrong type exception case",
		  success);
	    allTestsPassed &= success;

	    ///////////////////////////////////////
	    // Null argument for primitive field //
	    ///////////////////////////////////////
	    try {
		field.set(test, null);
		success = false;
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (NullPointerException e) {
		System.out.println(
		    "*** WARNING: JDK 1.2 throws a NullPointerException here"
		);
		System.out.println(
		    "where the spec indicates an IllegalArgumentException "
		);
		System.out.println(
		    "should be thrown. Saying the test succeeded anyway.");
		success = true;
		warnAtEnd = true;
		numWarnings++;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive short field set() null argument exception case",
		  success);
	    allTestsPassed &= success;
	
	    /////////////////////////////////
	    // Wrong type for object field //
	    /////////////////////////////////
	    A aVal = new A();
	    try {
		field = myClass.getField("bField");
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getField(\"bField\")", success);
	    allTestsPassed &= success;
	    try {
		field.set(test, aVal);
		success = false;
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("Object (B) field.set(A) illegal argument exception case",
		  success);
	    allTestsPassed &= success;

	    //
	    // Field.set() of all types
	    //

	    ///////////////////
	    // boolean field //
	    ///////////////////
	    try {
		field = myClass.getField("booleanField");
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getField(\"booleanField\")", success);
	    allTestsPassed &= success;
	    test.booleanField = false;
	    try {
		field.set(test, new Boolean(true));
		success = (test.booleanField == true);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive boolean field.set()", success);
	    allTestsPassed &= success;
	    test.booleanField = false;
	    try {
		field.setBoolean(test, true);
		success = (test.booleanField == true);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive boolean field.setBoolean()", success);
	    allTestsPassed &= success;
	    test.booleanField = false;
	    try {
		success = false;
		field.setByte(test, (byte) 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive boolean field.setByte() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setChar(test, 'a');
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive boolean field.setChar() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setShort(test, (short) 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive boolean field.setShort() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setInt(test, 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive boolean field.setInt() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setLong(test, 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive boolean field.setLong() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setFloat(test, 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive boolean field.setFloat() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setDouble(test, 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive boolean field.setDouble() exception case", success);
	    allTestsPassed &= success;

	    ////////////////
	    // byte field //
	    ////////////////
	    try {
		field = myClass.getField("byteField");
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getField(\"byteField\")", success);
	    allTestsPassed &= success;
	    test.byteField = 0;
	    try {
		field.set(test, new Byte((byte) 5));
		success = (test.byteField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive byte field.set()", success);
	    allTestsPassed &= success;
	    test.byteField = 0;
	    try {
		field.setByte(test, (byte) 5);
		success = (test.byteField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive byte field.setByte()", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setBoolean(test, false);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive byte field.setBoolean() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setChar(test, 'a');
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive byte field.setChar() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setShort(test, (short) 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive byte field.setShort() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setInt(test, 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive byte field.setInt() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setLong(test, 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive byte field.setLong() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setFloat(test, 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive byte field.setFloat() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setDouble(test, 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive byte field.setDouble() exception case", success);
	    allTestsPassed &= success;

	    ////////////////
	    // char field //
	    ////////////////
	    try {
		field = myClass.getField("charField");
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getField(\"charField\")", success);
	    allTestsPassed &= success;
	    test.charField = 'a';
	    try {
		field.set(test, new Character('b'));
		success = (test.charField == 'b');
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive char field.set()", success);
	    allTestsPassed &= success;
	    test.charField = 'a';
	    try {
		field.setChar(test, 'b');
		success = (test.charField == 'b');
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive char field.setChar()", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setBoolean(test, false);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive char field.setBoolean() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setByte(test, (byte) 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive char field.setByte() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setShort(test, (short) 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive char field.setShort() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setInt(test, 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive char field.setInt() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setLong(test, 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive char field.setLong() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setFloat(test, 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive char field.setFloat() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setDouble(test, 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive char field.setDouble() exception case", success);
	    allTestsPassed &= success;

	    /////////////////
	    // short field //
	    /////////////////
	    try {
		field = myClass.getField("shortField");
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getField(\"shortField\")", success);
	    allTestsPassed &= success;
	    test.shortField = 0;
	    try {
		field.set(test, new Short((short) 5));
		success = (test.shortField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive short field.set()", success);
	    allTestsPassed &= success;
	    test.shortField = 0;
	    try {
		field.setShort(test, (short) 5);
		success = (test.shortField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive short field.setShort()", success);
	    allTestsPassed &= success;
	    test.shortField = 0;
	    try {
		field.setByte(test, (byte) 5);
		success = (test.shortField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive short field.setByte()", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setBoolean(test, false);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive short field.setBoolean() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setChar(test, 'a');
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive short field.setChar() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setInt(test, 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive short field.setInt() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setLong(test, 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive short field.setLong() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setFloat(test, 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive short field.setFloat() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setDouble(test, 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive short field.setDouble() exception case", success);
	    allTestsPassed &= success;

	    ///////////////
	    // int field //
	    ///////////////
	    try {
		field = myClass.getField("intField");
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getField(\"intField\")", success);
	    allTestsPassed &= success;
	    test.intField = 0;
	    try {
		field.set(test, new Integer(5));
		success = (test.intField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive int field.set()", success);
	    allTestsPassed &= success;
	    test.intField = 0;
	    try {
		field.setInt(test, 5);
		success = (test.intField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive int field.setInt()", success);
	    allTestsPassed &= success;
	    test.intField = 0;
	    try {
		field.setByte(test, (byte) 5);
		success = (test.intField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive int field.setByte()", success);
	    allTestsPassed &= success;
	    test.intField = 0;
	    try {
		field.setChar(test, (char) 5);
		success = (test.intField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive int field.setChar()", success);
	    allTestsPassed &= success;
	    try {
		field.setShort(test, (short) 5);
		success = (test.intField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive int field.setShort()", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setBoolean(test, false);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive int field.setBoolean() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setLong(test, 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive int field.setLong() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setFloat(test, 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive int field.setFloat() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setDouble(test, 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive int field.setDouble() exception case", success);
	    allTestsPassed &= success;

	    ////////////////
	    // long field //
	    ////////////////
	    try {
		field = myClass.getField("longField");
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getField(\"longField\")", success);
	    allTestsPassed &= success;
	    test.longField = 0;
	    try {
		field.set(test, new Long(5));
		success = (test.longField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive long field.set()", success);
	    allTestsPassed &= success;
	    test.longField = 0;
	    try {
		field.setLong(test, 5);
		success = (test.longField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive long field.setLong()", success);
	    allTestsPassed &= success;
	    test.longField = 0;
	    try {
		field.setByte(test, (byte) 5);
		success = (test.longField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive long field.setByte()", success);
	    allTestsPassed &= success;
	    test.longField = 0;
	    try {
		field.setChar(test, (char) 5);
		success = (test.longField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive long field.setChar()", success);
	    allTestsPassed &= success;
	    try {
		field.setShort(test, (short) 5);
		success = (test.longField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive long field.setShort()", success);
	    allTestsPassed &= success;
	    try {
		field.setInt(test, 5);
		success = (test.longField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive long field.setInt()", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setBoolean(test, false);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive long field.setBoolean() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setFloat(test, 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive long field.setFloat() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setDouble(test, 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive long field.setDouble() exception case", success);
	    allTestsPassed &= success;

	    /////////////////
	    // float field //
	    /////////////////
	    try {
		field = myClass.getField("floatField");
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getField(\"floatField\")", success);
	    allTestsPassed &= success;
	    test.floatField = 0;
	    try {
		field.set(test, new Float(5));
		success = (test.floatField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive float field.set()", success);
	    allTestsPassed &= success;
	    test.floatField = 0;
	    try {
		field.setFloat(test, 5);
		success = (test.floatField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive float field.setFloat()", success);
	    allTestsPassed &= success;
	    test.floatField = 0;
	    try {
		field.setByte(test, (byte) 5);
		success = (test.floatField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive float field.setByte()", success);
	    allTestsPassed &= success;
	    test.floatField = 0;
	    try {
		field.setChar(test, (char) 5);
		success = (test.floatField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive float field.setChar()", success);
	    allTestsPassed &= success;
	    try {
		field.setShort(test, (short) 5);
		success = (test.floatField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive float field.setShort()", success);
	    allTestsPassed &= success;
	    try {
		field.setInt(test, 5);
		success = (test.floatField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive float field.setInt()", success);
	    allTestsPassed &= success;
	    try {
		field.setLong(test, (long) 5);
		success = (test.floatField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive float field.setLong()", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setBoolean(test, false);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive float field.setBoolean() exception case", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setDouble(test, 0);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive float field.setDouble() exception case", success);
	    allTestsPassed &= success;

	    //////////////////
	    // double field //
	    //////////////////
	    try {
		field = myClass.getField("doubleField");
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("getField(\"doubleField\")", success);
	    allTestsPassed &= success;
	    test.doubleField = 0;
	    try {
		field.set(test, new Double(5));
		success = (test.doubleField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive double field.set()", success);
	    allTestsPassed &= success;
	    test.doubleField = 0;
	    try {
		field.setDouble(test, 5);
		success = (test.doubleField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive double field.setDouble()", success);
	    allTestsPassed &= success;
	    test.doubleField = 0;
	    try {
		field.setByte(test, (byte) 5);
		success = (test.doubleField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive double field.setByte()", success);
	    allTestsPassed &= success;
	    test.doubleField = 0;
	    try {
		field.setChar(test, (char) 5);
		success = (test.doubleField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive double field.setChar()", success);
	    allTestsPassed &= success;
	    try {
		field.setShort(test, (short) 5);
		success = (test.doubleField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive double field.setShort()", success);
	    allTestsPassed &= success;
	    try {
		field.setInt(test, 5);
		success = (test.doubleField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive double field.setInt()", success);
	    allTestsPassed &= success;
	    try {
		field.setFloat(test, (float) 5);
		success = (test.doubleField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive double field.setFloat()", success);
	    allTestsPassed &= success;
	    try {
		field.setLong(test, (long) 5);
		success = (test.doubleField == 5);
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive double field.setLong()", success);
	    allTestsPassed &= success;
	    try {
		success = false;
		field.setBoolean(test, false);
	    }
	    catch (IllegalArgumentException e) {
		success = true;
	    }
	    catch (Exception e) {
		e.printStackTrace();
		success = false;
	    }
	    check("primitive double field.setBoolean() exception case", success);
	    allTestsPassed &= success;
	}

	//
	// Constructor.newInstance tests
	//

	/////////////////////////////////
	// single-argument constructor //
	/////////////////////////////////
	Constructor constructor = null;
	ReflectionTest constructorTest = null;
	Object[] constructorArgs = new Object[1];
	try {
	    Class[] paramTypes = {Integer.TYPE};
	    constructor = 
		ReflectionTest.class.getConstructor(paramTypes);
	    success = (constructor != null);
	}
	catch (Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("getConstructor(ReflectionTest(int))", success);
	allTestsPassed &= success;
	try {
	    constructorArgs[0] = new Integer(5);
	    constructorTest =
		(ReflectionTest) constructor.newInstance(constructorArgs);
	    success = (constructorTest.intField == 5);
	}
	catch (Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("Constructor.newInstance() (Reflection(int))", success);
	allTestsPassed &= success;

	try {
	    constructorTest =
		(ReflectionTest) constructor.newInstance(null);
	    success = false;
	}
	catch (IllegalArgumentException e) {
	    success = true;
	}
	catch (Exception e) {
	    success = false;
	}
	check("Constructor.newInstance() null argument exception case",
	      success);
	allTestsPassed &= success;

	try {
	    constructorArgs[0] = new Double(5);
	    constructorTest =
		(ReflectionTest) constructor.newInstance(constructorArgs);
	    success = false;
	}
	catch (IllegalArgumentException e) {
	    success = true;
	}
	catch (Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("Constructor.newInstance() narrowing conversion exception case",
	      success);
	allTestsPassed &= success;

	/////////////////////////////////
	// double-argument constructor //
	/////////////////////////////////
	constructorArgs = new Object[2];
	try {
	    Class[] paramTypes = {Integer.TYPE, B.class};
	    constructor = 
		ReflectionTest.class.getConstructor(paramTypes);
	    success = false;
	}
	catch (NoSuchMethodException e) {
	    success = true;
	}
	catch (Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("getConstructor(ReflectionTest(double, B)) exception case",
	      success);
	allTestsPassed &= success;

	try {
	    Class[] paramTypes = {Double.TYPE, B.class};
	    constructor = 
		ReflectionTest.class.getConstructor(paramTypes);
	    success = (constructor != null);
	}
	catch (Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("getConstructor(ReflectionTest(double, B))", success);
	allTestsPassed &= success;
	
	try {
	    constructorArgs[0] = new Double(2.0);
	    B b = new B();
	    constructorArgs[1] = b;
	    constructorTest =
		(ReflectionTest) constructor.newInstance(constructorArgs);
	    success = ((constructorTest.doubleField == 2.0) &&
		       (constructorTest.bField == b));
	}
	catch (Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("Constructor.newInstance(ReflectionTest(double, B))", success);
	allTestsPassed &= success;
	
	// widening conversion
	try {
	    constructorArgs[0] = new Integer(3);
	    B b = new B();
	    constructorArgs[1] = b;
	    constructorTest =
		(ReflectionTest) constructor.newInstance(constructorArgs);
	    success = ((constructorTest.doubleField == 3.0) &&
		       (constructorTest.bField == b));
	}
	catch (Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("Constructor.newInstance(ReflectionTest(int, B))", success);
	allTestsPassed &= success;
	
	// null argument
	try {
	    constructorArgs[0] = new Double(4.0);
	    constructorArgs[1] = null;
	    constructorTest =
		(ReflectionTest) constructor.newInstance(constructorArgs);
	    success = ((constructorTest.doubleField == 4.0) &&
		       (constructorTest.bField == null));
	}
	catch (Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("Constructor.newInstance(ReflectionTest(double, null))",
	      success);
	allTestsPassed &= success;
	
	// no conversion exception case
	try {
	    constructorArgs[0] = new Double(2.0);
	    A a = new A();
	    constructorArgs[1] = a;
	    constructorTest =
		(ReflectionTest) constructor.newInstance(constructorArgs);
	    success = false;
	}
	catch (IllegalArgumentException e) {
	    success = true;
	}
	catch (Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("Constructor.newInstance(ReflectionTest(double, A)) " +
	      "exception case", success);
	allTestsPassed &= success;
	
	try {
	    Class[] paramTypes = {B.class, Double.TYPE};
	    constructor = 
		ReflectionTest.class.getConstructor(paramTypes);
	    success = (constructor != null);
	}
	catch (Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("getConstructor(ReflectionTest(B, double))", success);
	allTestsPassed &= success;
	
	try {
	    B b = new B();
	    constructorArgs[0] = b;
	    constructorArgs[1] = new Double(2.0);
	    constructorTest =
		(ReflectionTest) constructor.newInstance(constructorArgs);
	    success = ((constructorTest.doubleField == 2.0) &&
		       (constructorTest.bField == b));
	}
	catch (Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("Constructor.newInstance(ReflectionTest(B, double))", success);
	allTestsPassed &= success;
	
	// single-argument InstantiationException case
	constructorArgs = new Object[1];
	try {
	    Class[] paramTypes = {Integer.TYPE};
	    constructor = 
		ReflectionTest.AbstractClass.class.getConstructor(paramTypes);
	    success = (constructor != null);
	}
	catch (Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("getConstructor(ReflectionTest.AbstractClass(int))", success);
	allTestsPassed &= success;
	
	try {
	    constructorArgs[0] = new Integer(3);
	    constructorTest =
		(ReflectionTest) constructor.newInstance(constructorArgs);
	    success = false;
	}
	catch (InstantiationException e) {
	    success = true;
	}
	catch (Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("Constructor.newInstance( " +
	      "ReflectionTest.AbstractClass(int, B)) exception case", success);
	allTestsPassed &= success;
	
	// %comment: mam053

	//
	// Array tests
	//

	int[][] intArray2D;
	try {
	    success = true;
	    int[] dims = {2, 5};
	    Object res =
		java.lang.reflect.Array.newInstance(Integer.TYPE, dims);
	    System.out.println("Array.newInstance() result was of type " +
			       res.getClass().getName());
	    intArray2D = (int[][]) res;
	    for (int i = 0; i < 2; i++) {
		if (intArray2D[i].length != 5)
		    success = false;
	    }
	}
	catch (Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("integer array Array.newInstance() of 2-D int array", success);
	allTestsPassed &= success;

	try {
	    int[] dims = {2, 5};
	    intArray2D =
		(int[][]) java.lang.reflect.Array.newInstance(null, dims);
	    success = false;
	}
	catch (NullPointerException e) {
	    success = true;
	}
	catch (Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("integer array Array.newInstance() null type exception case",
	      success);
	allTestsPassed &= success;
	
	try {
	    int[] dims = {2, 5};
	    intArray2D = 
		(int[][]) java.lang.reflect.Array.newInstance(Integer.TYPE,
							      null);
	    success = false;
	}
	catch (NullPointerException e) {
	    success = true;
	}
	catch (Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("integer array Array.newInstance() " +
	      "null dimensions exception case",
	      success);
	allTestsPassed &= success;
	
	try {
	    intArray2D = 
		(int[][]) java.lang.reflect.Array.newInstance(Integer.TYPE,
							      new int[0]);
	    success = false;
	}
	catch (IllegalArgumentException e) {
	    success = true;
	}
	catch (Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("integer array Array.newInstance() " +
	      "zero-dimension exception case", success);
	allTestsPassed &= success;
	
	try {
	    success = true;
	    int[] dims = {2, 0};
	    intArray2D = 
		(int[][]) java.lang.reflect.Array.newInstance(Integer.TYPE,
							      dims);
	    for (int i = 0; i < 2; i++) {
		if (intArray2D[i].length != 0)
		    success = false;
	    }
	}
	catch (Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("integer array Array.newInstance() truncated dimensions",
	      success);
	allTestsPassed &= success;

	/////////////////////////////////
	// integer array set/get tests //
	/////////////////////////////////
	success = false;
	try {
	    int[] intArray = {3, 2, 5};
	    java.lang.reflect.Array.set(intArray, 0, new Integer(1));
	    if (intArray[0] == 1)
		success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	}
	check("integer array Array.set()", success);
	allTestsPassed &= success;

	success = false;
	try {
	    int[] intArray = {3, 2, 5};
	    Integer i = (Integer) java.lang.reflect.Array.get(intArray, 0);
	    if (i.intValue() == 3)
		success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	}
	check("integer array Array.get()", success);
	allTestsPassed &= success;

	success = false;
	try {
	    int[] intArray = {3, 2, 5};
	    java.lang.reflect.Array.setInt(intArray, 0, 1);
	    if (intArray[0] == 1)
		success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	}
	check("integer array Array.setInt()", success);
	allTestsPassed &= success;

	success = false;
	try {
	    int[] intArray = {3, 2, 5};
	    int i = java.lang.reflect.Array.getInt(intArray, 0);
	    if (i == 3)
		success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	}
	check("integer array Array.getInt()", success);
	allTestsPassed &= success;

	success = false;
	try {
	    int[] intArray = {3, 2, 5};
	    java.lang.reflect.Array.setShort(intArray, 0, (short) 1);
	    if (intArray[0] == 1)
		success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	}
	check("integer array Array.setShort() widening case", success);
	allTestsPassed &= success;

	success = false;
	try {
	    int[] intArray = {3, 2, 5};
	    short i = java.lang.reflect.Array.getShort(intArray, 0);
	    success = false;
	}
	catch(IllegalArgumentException e) {
	    success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	}
	check("integer array Array.getShort() exception case", success);
	allTestsPassed &= success;

	try {
	    int[] intArray = {3, 2, 5};
	    java.lang.reflect.Array.setDouble(intArray, 0, 1.0);
	    success = false;
	}
	catch (IllegalArgumentException e) {
	    success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("integer array Array.setDouble() exception case", success);
	allTestsPassed &= success;

	try {
	    int[] intArray = {3, 2, 5};
	    java.lang.reflect.Array.set(intArray, 4, new Integer(1));
	    success = false;
	}
	catch (ArrayIndexOutOfBoundsException e) {
	    success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("integer array Array.set() out-of-bounds case", success);
	allTestsPassed &= success;

	try {
	    int[] intArray = {3, 2, 5};
	    java.lang.reflect.Array.set(intArray, -1, new Integer(1));
	    success = false;
	}
	catch (ArrayIndexOutOfBoundsException e) {
	    success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("integer array Array.set() negative index case", success);
	allTestsPassed &= success;

	try {
	    int[] intArray = {3, 2, 5};
	    java.lang.reflect.Array.setInt(intArray, 4, 1);
	    success = false;
	}
	catch (ArrayIndexOutOfBoundsException e) {
	    success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("integer array Array.setInt() out-of-bounds case", success);
	allTestsPassed &= success;

	try {
	    int[] intArray = {3, 2, 5};
	    java.lang.reflect.Array.setInt(intArray, -1, 1);
	    success = false;
	}
	catch (ArrayIndexOutOfBoundsException e) {
	    success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("integer array Array.setInt() negative index case", success);
	allTestsPassed &= success;

	try {
	    int[] intArray = {3, 2, 5};
	    Integer i = (Integer) java.lang.reflect.Array.get(intArray, 4);
	    success = false;
	}
	catch (ArrayIndexOutOfBoundsException e) {
	    success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("integer array Array.get() out-of-bounds case", success);
	allTestsPassed &= success;

	try {
	    int[] intArray = {3, 2, 5};
	    Integer i = (Integer) java.lang.reflect.Array.get(intArray, -1);
	    success = false;
	}
	catch (ArrayIndexOutOfBoundsException e) {
	    success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("integer array Array.get() negative index case", success);
	allTestsPassed &= success;

	try {
	    int[] intArray = {3, 2, 5};
	    int i = java.lang.reflect.Array.getInt(intArray, 4);
	    success = false;
	}
	catch (ArrayIndexOutOfBoundsException e) {
	    success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("integer array Array.getInt() out-of-bounds case", success);
	allTestsPassed &= success;

	try {
	    int[] intArray = {3, 2, 5};
	    int i = java.lang.reflect.Array.getInt(intArray, -1);
	    success = false;
	}
	catch (ArrayIndexOutOfBoundsException e) {
	    success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("integer array Array.getInt() negative index case", success);
	allTestsPassed &= success;

	////////////////////////////////
	// double array set/get tests //
	////////////////////////////////
	success = false;
	try {
	    double[] doubleArray = {3.0, 2.0, 5.0};
	    java.lang.reflect.Array.set(doubleArray, 0, new Double(1.0));
	    if (doubleArray[0] == 1.0)
		success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	}
	check("double array Array.set()", success);
	allTestsPassed &= success;

	success = false;
	try {
	    double[] doubleArray = {3.0, 2.0, 5.0};
	    Double d = (Double) java.lang.reflect.Array.get(doubleArray, 0);
	    if (d.doubleValue() == 3.0)
		success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	}
	check("double array Array.get()", success);
	allTestsPassed &= success;

	success = false;
	try {
	    double[] doubleArray = {3.0, 2.0, 5.0};
	    java.lang.reflect.Array.setDouble(doubleArray, 0, 1.0);
	    if (doubleArray[0] == 1.0)
		success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	}
	check("double array Array.setDouble()", success);
	allTestsPassed &= success;

	success = false;
	try {
	    double[] doubleArray = {3.0, 2.0, 5.0};
	    double d = java.lang.reflect.Array.getDouble(doubleArray, 0);
	    if (d == 3.0)
		success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	}
	check("double array Array.getDouble()", success);
	allTestsPassed &= success;

	success = false;
	try {
	    double[] doubleArray = {3.0, 2.0, 5.0};
	    java.lang.reflect.Array.setInt(doubleArray, 0, 1);
	    if (doubleArray[0] == 1.0)
		success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	}
	check("double array Array.setInt() widening case", success);
	allTestsPassed &= success;

	success = false;
	try {
	    double[] doubleArray = {3.0, 2.0, 5.0};
	    int i = java.lang.reflect.Array.getInt(doubleArray, 0);
	    success = false;
	}
	catch(IllegalArgumentException e) {
	    success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	}
	check("double array Array.getInt() exception case", success);
	allTestsPassed &= success;

	try {
	    double[] doubleArray = {3.0, 2.0, 5.0};
	    java.lang.reflect.Array.set(doubleArray, 4, new Double(1));
	    success = false;
	}
	catch (ArrayIndexOutOfBoundsException e) {
	    success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("double array Array.set() out-of-bounds case", success);
	allTestsPassed &= success;

	try {
	    double[] doubleArray = {3.0, 2.0, 5.0};
	    java.lang.reflect.Array.set(doubleArray, -1, new Double(1));
	    success = false;
	}
	catch (ArrayIndexOutOfBoundsException e) {
	    success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("double array Array.set() negative index case", success);
	allTestsPassed &= success;

	try {
	    double[] doubleArray = {3.0, 2.0, 5.0};
	    java.lang.reflect.Array.setDouble(doubleArray, 4, 1.0);
	    success = false;
	}
	catch (ArrayIndexOutOfBoundsException e) {
	    success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("double array Array.setDouble() out-of-bounds case", success);
	allTestsPassed &= success;

	try {
	    double[] doubleArray = {3.0, 2.0, 5.0};
	    java.lang.reflect.Array.setDouble(doubleArray, -1, 1.0);
	    success = false;
	}
	catch (ArrayIndexOutOfBoundsException e) {
	    success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("double array Array.setDouble() negative index case", success);
	allTestsPassed &= success;

	try {
	    double[] doubleArray = {3.0, 2.0, 5.0};
	    Double d = (Double) java.lang.reflect.Array.get(doubleArray, 4);
	    success = false;
	}
	catch (ArrayIndexOutOfBoundsException e) {
	    success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("double array Array.get() out-of-bounds case", success);
	allTestsPassed &= success;

	try {
	    double[] doubleArray = {3.0, 2.0, 5.0};
	    Double d = (Double) java.lang.reflect.Array.get(doubleArray, -1);
	    success = false;
	}
	catch (ArrayIndexOutOfBoundsException e) {
	    success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("double array Array.get() negative index case", success);
	allTestsPassed &= success;

	try {
	    double[] doubleArray = {3.0, 2.0, 5.0};
	    double d = java.lang.reflect.Array.getDouble(doubleArray, 4);
	    success = false;
	}
	catch (ArrayIndexOutOfBoundsException e) {
	    success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("double array Array.getDouble() out-of-bounds case", success);
	allTestsPassed &= success;

	try {
	    double[] doubleArray = {3.0, 2.0, 5.0};
	    double d = java.lang.reflect.Array.getDouble(doubleArray, -1);
	    success = false;
	}
	catch (ArrayIndexOutOfBoundsException e) {
	    success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	    success = false;
	}
	check("double array Array.getDouble() negative index case", success);
	allTestsPassed &= success;

	////////////////////////////////
	// object array set/get tests //
	////////////////////////////////
	success = false;
	Object[] objectArray = new Object[1];
	Object obj = new Object();
	objectArray[0] = obj;
	try {
	    java.lang.reflect.Array.set(objectArray, 0, null);
	    if (objectArray[0] == null)
		success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	}
	check("object array Array.set(null)", success);
	allTestsPassed &= success;

	try {
	    Object obj2 = java.lang.reflect.Array.get(objectArray, 0);
	    if (obj2 == null)
		success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	}
	check("object array Array.get() null result", success);
	allTestsPassed &= success;

	try {
	    java.lang.reflect.Array.set(objectArray, 0, obj);
	    if (objectArray[0] == obj)
		success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	}
	check("object array Array.set(Object)", success);
	allTestsPassed &= success;

	try {
	    Object obj2 = java.lang.reflect.Array.get(objectArray, 0);
	    if (obj2 == obj)
		success = true;
	}
	catch(Exception e) {
	    e.printStackTrace();
	}
	check("object array Array.get() non-null result", success);
	allTestsPassed &= success;

	//
	// Class.getDeclaredClasses and Class.getDeclaringClass tests
	//

	Class clazz = ReflectionTest.class;
	Class[] inners = clazz.getDeclaredClasses();
	success = true;
	if (inners.length != 5)
	    success = false;
	if ((!find(inners, "ReflectionTest$A")) ||
	    (!find(inners, "ReflectionTest$B")) ||
	    (!find(inners, "ReflectionTest$D")) ||
	    (!find(inners, "ReflectionTest$F")) ||
	    (!find(inners, "ReflectionTest$AbstractClass")))
	    success = false;
	check("ReflectionTest.getDeclaredClasses()", success);
	allTestsPassed &= success;
	success = true;
	for (int i = 0; i < inners.length; i++) {
	    if (!inners[i].getDeclaringClass().getName().
		equals("ReflectionTest")) {
		success = false;
		break;
	    }
	}
	check("getDeclaringClass() on inner classes of ReflectionTest",
	      success);
	allTestsPassed &= success;

	clazz = ReflectionTest.D.class;
	inners = clazz.getDeclaredClasses();
	success = true;
	if (inners.length != 1)
	    success = false;
	if (!find(inners, "ReflectionTest$D$E"))
	    success = false;
	check("ReflectionTest$D.getDeclaredClasses()", success);
	allTestsPassed &= success;
	success = true;
	if (inners.length == 0 || !inners[0].getDeclaringClass().getName().
	    equals("ReflectionTest$D"))
	    success = false;
	check("getDeclaringClass() on inner classes of ReflectionTest$D",
	      success);
	allTestsPassed &= success;

	//
	// Class.getName() and Class.getComponentType()
	//

	// %comment: mam055
	clazz = Integer.TYPE;
	if (Integer.TYPE.getName().equals("int"))
	    success = true;
	else
	    success = false;
	check("Integer.TYPE.getName()", success);
	allTestsPassed &= success;

	int[] foo = new int[1];
	clazz = foo.getClass();
	if (clazz.getName().equals("[I"))
	    success = true;
	else
	    success = false;
	check("int[].class.getName()", success);
	allTestsPassed &= success;

	clazz = clazz.getComponentType();
	success = (clazz == Integer.TYPE);
	check("int[].class.getComponentType == Integer.TYPE", success);
	allTestsPassed &= success;

	clazz = clazz.getComponentType();
	if (clazz == null)
	    success = true;
	else
	    success = false;
	check("null return value from getComponentType(Integer.TYPE)",
	      success);
	allTestsPassed &= success;

	// This should stay at the end
	if (allTestsPassed) {
	    System.out.println("All tests passed.");
	} else {
	    System.out.println("** ERROR: SOME TESTS FAILED. **");
	}
	if (warnAtEnd) {
	    System.out.println("** WARNING: " + numWarnings +
			       " warnings were printed.");
	}
    }
}

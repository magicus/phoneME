/*
 * @(#)InterfaceTest.java	1.8 06/10/10
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

import java.io.PrintStream;

public class InterfaceTest {
    public static void main(String[] args) {
	Class[] classes;
	int i;

	if (new conv05101().run(args, System.out) != 0) {
	    System.out.println("InterfaceTest: conv05101 failed");
	} else {
	    System.out.println("InterfaceTest: conv05101 passed");
	}

	C2C1I2 c2 = new C2C1I2();
	if (c2.i1() != 1 || c2.i2() != -2) {
	    System.out.println("InterfaceTest: test #1 failed");
	} else {
	    System.out.println("InterfaceTest: test #1 passed");
	}

	C3I4 c3 = new C3I4();
	if (c3.i1() != 1 || c3.i2() != -2) {
	    System.out.println("InterfaceTest: test #2 failed");
	} else {
	    System.out.println("InterfaceTest: test #2 passed");
	}

	classes = C3I4.class.getInterfaces();
	if (classes.length != 2 || 
	    classes[0] != I4.class || classes[1] != I2.class) {
	    System.out.println("InterfaceTest: test #3 failed");
	} else {
	    System.out.println("InterfaceTest: test #3 passed");
	}

	classes = I4.class.getInterfaces();
	if (classes.length != 3 || classes[0] != I1.class ||
	    classes[1] != I2.class || classes[2] != I3.class) {
	    System.out.println("InterfaceTest: test #4 failed");
	} else {
	    System.out.println("InterfaceTest: test #4 passed");
	}
    }
}


interface I1 {
    int i1();
}


interface I2 {
    int i2();
}

interface I3 extends I2 {
}

interface I4 extends I1, I2, I3 {
}

abstract class C1I2 implements I2 {
    abstract public int i1();
    public int i2() { return 2;};
}

class C2C1I2 extends C1I2 implements I1 {
    public int i1() { return 1;}
    public int i2() { return -2;}
}

class C3I4 implements I4, I2 {
    public int i1() { return 1;}
    public int i2() { return -2;}
}



interface SomeInterface {

	void incFields();

}


interface ImmediateSubinterface extends SomeInterface {

	void decFields();

}


interface LastSubinterface extends ImmediateSubinterface {

	void doubleFields();

}


class SomeClass implements LastSubinterface {

	int i;
	float f;

	public void incFields() {
		i++;
		f++;
	}
 
	public void decFields() {
		i--;
		f--;
	}
 
	public void doubleFields() {
		i *= 2;
		f *= 2;
	}
 
	SomeClass(int i, float f) {
		this.i = i;
		this.f = f;
	}

}


class conv05101 { 
	static int errorStatus = 0/*STATUS_PASSED*/;

	static void errorAlert(PrintStream out, int errorLevel) {
		out.println("conv05101: failure #" + errorLevel);
		errorStatus = 2/*STATUS_FAILED*/;
	}

	public static int run(String args[], PrintStream out) { 
		SomeInterface u;
		ImmediateSubinterface v;
		LastSubinterface w;
		SomeClass x;

		x = new SomeClass(-2, 2.7195f);
		v = (ImmediateSubinterface) x;
		w = (LastSubinterface) new SomeClass(6, 0.123f);

		u = v;
		v.decFields();
		if (! (u instanceof SomeClass))
			errorAlert(out, 2);
		else if (u != x)
			errorAlert(out, 3);
		else if (((SomeClass) u).i != -3)
			errorAlert(out, 4);
		else if (((SomeClass) u).f != 2.7195f - 1)
			errorAlert(out, 5);

		u = w;
		u.incFields();
		w.doubleFields();
		if (! (u instanceof SomeClass))
			errorAlert(out, 6);
		else if (u != w)
			errorAlert(out, 7);
		else if (((SomeClass) u).i != 14)
			errorAlert(out, 8);
		else if (((SomeClass) u).f != (0.123f + 1) * 2)
			errorAlert(out, 9);

		return errorStatus;
	}

}

/*
 * @(#)DebuggerTest.java	1.10 06/10/10
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
// Test designed to allow various specific pieces of functionality of
// debugger to be tested.

public class DebuggerTest {
    // Instance field
    public int intField;

    public boolean throwException;

    // Static field
    public static int staticIntField;

    public void printFields() {
	// Test field access watchpoints
	int i = intField;
	int j = staticIntField;
	System.out.println("intField: " + i +
			   ", staticIntField: " + j);
    }

    public void run() {
	// Can examine locals here
	System.out.println("DebuggerTest entered.");

	int a;
	float b;

	// Try single-stepping through the next few bytecodes
	b = 3.0f;
	a = 2;
	float c = a + b;
	System.out.println("c = " + c);

	// Test breakpoints and clearing of them
	// Also test field watchpoints
	intField = 4;
	staticIntField = 5;
	printFields();
	intField = 1;
	staticIntField = 6;
	printFields();

	// Test exception throwing and catching
	try {
	    intField = 3;
	    if (throwException) {
		throw new RuntimeException("RuntimeException to be caught");
	    }
	    staticIntField = 4;
	}
	catch (RuntimeException e) {
	    System.out.println("RuntimeException caught: " + e);
	    e.printStackTrace();
	}

	System.out.println("DebuggerTest exiting.");

	// Functionality to test:
	// threads [threadgroup] (works)
	// thread <thread id> (works)
	// suspend [thread id(s)] (seems to work)
	// resume [thread id(s)] (seems to work)
	// where [thread id] | all (works)
	// wherei [thread id] | all (works)
	// up [n frames] (works)
	// down [n frames] (works)
	// kill <thread> <expr>
	// interrupt <thread>
	// print <expr>
	// dump <expr>
	// eval <expr>
	// set <lvalue> = <expr>
	// locals
	// classes
	// class <class id>
	// methods <class id>
	// fields <class id>
	// threadgroups
	// threadgroup <name>
	// stop in <class id>.<method>[(argument_type,...)]
	// stop at <class id>:<line>
	// clear <class id>.<method>[(argument_type,...)]
	// clear <class id>:<line>
	// clear
	// catch <class id>
	// ignore <class id>
	// watch [access|all] <class id>.<field name>
	// unwatch [access|all] <class id>.<field name>
	// trace methods [thread]
	// untrace methods [thread]
	// step
	// step up
	// stepi
	// next
	// cont
	// list [line number|method]
	// use (or sourcepath) [source file path]
	// exclude [class id ... | "none"]
	// classpath
	// monitor <command>  (Poor choice of terminology...not related
	//		       in any way to a Java monitor, just a command
	//		       which gets executed every time the debugger
	//		       steps?)
	// monitor
	// unmonitor <monitor#>
	// lock <expr>
	// threadlocks [thread id]
	// disablegc <expr>
	// enablegc <expr>




	// All jdb functionality:
	// run [class [args]]
	// threads [threadgroup]
	// thread <thread id>
	// suspend [thread id(s)]
	// resume [thread id(s)]
	// where [thread id] | all
	// wherei [thread id] | all
	// up [n frames]
	// down [n frames]
	// kill <thread> <expr>
	// interrupt <thread>
	// print <expr>
	// dump <expr>
	// eval <expr>
	// set <lvalue> = <expr>
	// locals
	// classes
	// class <class id>
	// methods <class id>
	// fields <class id>
	// threadgroups
	// threadgroup <name>
	// stop in <class id>.<method>[(argument_type,...)]
	// stop at <class id>:<line>
	// clear <class id>.<method>[(argument_type,...)]
	// clear <class id>:<line>
	// clear
	// catch <class id>
	// ignore <class id>
	// watch [access|all] <class id>.<field name>
	// unwatch [access|all] <class id>.<field name>
	// trace methods [thread]
	// untrace methods [thread]
	// step
	// step up
	// stepi
	// next
	// cont
	// list [line number|method]
	// use (or sourcepath) [source file path]
	// exclude [class id ... | "none"]
	// classpath
	// monitor <command>
	// monitor
	// unmonitor <monitor#>
	// read <filename>
	// lock <expr>
	// threadlocks [thread id]
	// disablegc <expr>
	// enablegc <expr>
    }



    public static void main(String[] args) {
	// Type "break DebuggerTest.main" before running to get in here
	DebuggerTest dt = new DebuggerTest();
	dt.throwException = true;
	dt.run();
    }
}

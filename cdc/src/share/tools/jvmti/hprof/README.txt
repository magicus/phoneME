#
# Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.  
# DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER  
#   
# This program is free software; you can redistribute it and/or  
# modify it under the terms of the GNU General Public License version  
# 2 only, as published by the Free Software Foundation.   
#   
# This program is distributed in the hope that it will be useful, but  
# WITHOUT ANY WARRANTY; without even the implied warranty of  
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU  
# General Public License version 2 for more details (a copy is  
# included at /legal/license.txt).   
#   
# You should have received a copy of the GNU General Public License  
# version 2 along with this work; if not, write to the Free Software  
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  
# 02110-1301 USA   
#   
# Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa  
# Clara, CA 95054 or visit www.sun.com if you need additional  
# information or have any questions. 
#

README
------

Design and Implementation:

    * The Tracker Class (Tracker.java & hprof_tracker.c)
        It was added to the sun.tools.hprof.Tracker in JDK 5.0 FCS, then
	moved to a package that didn't cause classload errors due to
	the security manager not liking the sun.* package name.
	5091195 detected that this class needs to be in com.sun.demo.jvmti.hprof.
        The BCI code will call these static methods, which will in turn
        (if engaged) call matching native methods in the hprof library,
	with the additional current Thread argument (Thread.currentThread()).
	Doing the currentThread call on the Java side was necessary due
	to the difficulty of getting the current thread while inside one
	of these Tracker native methods.  This class lives in rt.jar.

    * Byte Code Instrumentation (BCI)
        Using the ClassFileLoadHook feature and a C language
        implementation of a byte code injection transformer, the following
        bytecodes get injections:
	    - On entry to the java.lang.Object <init> method, 
	      a invokestatic call to
		Tracker.ObjectInit(this);
	      is injected. 
	    - On any newarray type opcode, immediately following it, 
	      the array object is duplicated on the stack and an
	      invokestatic call to
		Tracker.NewArray(obj);
	      is injected. 
	    - On entry to all methods, a invokestatic call to 
		Tracker.CallSite(cnum,mnum);
	      is injected. The hprof agent can map the two integers
	      (cnum,mnum) to a method in a class. This is the BCI based
	      "method entry" event.
	    - On return from any method (any return opcode),
	      a invokestatic call to
		Tracker.ReturnSite(cnum,mnum);
	      is injected.  
        All classes found via ClassFileLoadHook are injected with the
        exception of some system class methods "<init>" and "finalize" 
        whose length is 1 and system class methods with name "<clinit>",
	and also java.lang.Thread.currentThread() which is used in the
	class Tracker (preventing nasty recursion issue).
        System classes are currently defined as any class seen by the
	ClassFileLoadHook prior to VM_INIT. This does mean that
	objects created in the system classes inside <clinit> might not
	get tracked initially.
	See the java_crw_demo source and documentation for more info.
	The injections are based on what the hprof options
	are requesting, e.g. if heap=sites or heap=all is requested, the
	newarray and Object.<init> method injections happen.
	If cpu=times is requested, all methods get their entries and
	returns tracked. Options like cpu=samples or monitor=y
	do not require BCI.

    * BCI Allocation Tags (hprof_tag.c)
        The current jlong tag being used on allocated objects
	is an ObjectIndex, or an index into the object table inside
	the hprof code. Depending on whether heap=sites or heap=dump 
	was asked for, these ObjectIndex's might represent unique
	objects, or unique allocation sites for types of objects.
	The heap=dump option requires considerable more space
	due to the one jobject per ObjectIndex mapping.

    * BCI Performance
        The cpu=times seems to have the most negative affect on
	performance, this could be improved by not having the 
	Tracker class methods call native code directly, but accumulate
	the data in a file or memory somehow and letting it buffer down
	to the agent. The cpu=samples is probably a better way to
	measure cpu usage, varying the interval as needed.
	The heap=dump seems to use memory like crazy, but that's 
	partially the way it has always been. 

    * Sources in the JDK workspace
        The sources and Makefiles live in:
                src/share/classes/com/sun/demo/jvmti/hprof/*
                src/share/demo/jvmti/hprof/*
                src/share/demo/jvmti/java_crw_demo/*
                src/solaris/demo/jvmti/hprof/*
                src/windows/demo/jvmti/hprof/*
                make/java/java_hprof_demo/*
                make/java/java_crw_demo/*
   
--------

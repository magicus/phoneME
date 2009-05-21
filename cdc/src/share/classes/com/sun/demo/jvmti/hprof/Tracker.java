/*
 * @(#)Tracker.java	1.2 05/11/17
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

package com.sun.demo.jvmti.hprof;

/* This class and it's methods are used by hprof when injecting bytecodes
 *   into class file images.
 *   See the directory src/share/demo/jvmti/hprof and the file README.txt
 *   for more details.
 */

public class Tracker {
 
    /* Master switch that activates calls to native functions. */
    
    private static int engaged = 0; 
  
    /* To track memory allocated, we need to catch object init's and arrays. */
    
    /* At the beginning of java.jang.Object.<init>(), a call to
     *   Tracker.ObjectInit() is injected.
     */

    private static native void nativeObjectInit(Object thr, Object obj);
    
    public static void ObjectInit(Object obj)
    {
	if ( engaged != 0 ) {
	    nativeObjectInit(Thread.currentThread(), obj);
	}
    }
    
    /* Immediately following any of the newarray bytecodes, a call to
     *   Tracker.NewArray() is injected.
     */

    private static native void nativeNewArray(Object thr, Object obj);
   
    public static void NewArray(Object obj)
    {
	if ( engaged != 0 ) {
	    nativeNewArray(Thread.currentThread(), obj);
	}
    }
   
    /* For cpu time spent in methods, we need to inject for every method. */

    /* At the very beginning of every method, a call to
     *   Tracker.CallSite() is injected.
     */

    private static native void nativeCallSite(Object thr, int cnum, int mnum);
    
    public static void CallSite(int cnum, int mnum)
    {
	if ( engaged != 0 ) {
	    nativeCallSite(Thread.currentThread(), cnum, mnum);
	}
    }
    
    /* Before any of the return bytecodes, a call to
     *   Tracker.ReturnSite() is injected.
     */

    private static native void nativeReturnSite(Object thr, int cnum, int mnum);
    
    public static void ReturnSite(int cnum, int mnum)
    {
	if ( engaged != 0 ) {
	    nativeReturnSite(Thread.currentThread(), cnum, mnum);
	}
    }
    
}


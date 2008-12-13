/*
 *   
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

package com.sun.cldchi.jvm;

/**
 * Java API to support run-time method trapping from Java code.
 * It is used internally by J2ME software stack implementations
 * and should be placed in a hidden package.
 * MethodTrap class allows to change execution entry of Java methods.
 */
public final class MethodTrap {

    /** Invoke callback function */
    public static final int ACTION_CALLBACK     =  0;
    /** Causes JVM to stop */
    public static final int ACTION_EXIT         = -1;
    /** Causes current isolate to stop */
    public static final int ACTION_STOP_ISOLATE = -2;
    /** Causes native breakpoint to happen */
    public static final int ACTION_BREAKPOINT   = -3;

    /**
     * Trap specified Java method for JVM to take special action before
     * the method is invoked.
     * @param methodName  - fully-qualified name of the method to trap,
     *                      looks like package.ClassName.methodName
     * @param callCount   - take an action only when the method is called
     *                      callCount times
     * @param action      - the code of the action for JVM to take
     *                      on the method invocation, can be
     *                      one of ACTION_* 
     * @param targetTask  - id of a task that should be stopped on the method call,
     *                      method with the same from other tasks won't be trapped.
     *                      0 means any task will match
     *                 
     * @return trapHandle - unique identifier of the trap that can be passed
     *                      to subsequent repeaseTrap() call
     */
    public static native int setTrap(String methodName, int callCount,
                                     int action, int targetTask);

    /**
     * Replace the execution entry of the specified method with
     * the execution entry of another method with the similar signature.
     * @param methodName  - fully-qualified name of the method to trap,
     *                      looks like package.ClassName.methodName
     * @param handlerName - fully-qualified name of the method to be invoked
     *                      instead of the trapped method. The handler must
     *                      have the same parameter types as the original
     *                      method.
     */
    public static native int setJavaTrap(String methodName, String handlerName);

    /**
     * Release Java method that was previously trapped by setTrap() call.
     * @param trapHandle  - the identifier of the trap that was returned
     *                      by previous call to setTrap() or setJavaTrap()
     */
    public static native void releaseTrap(int trapHandle);

}

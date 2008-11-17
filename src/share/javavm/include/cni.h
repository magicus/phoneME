/*
 * @(#)cni.h	1.7 06/10/10
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

#ifndef _INCLUDED_CNI_H
#define _INCLUDED_CNI_H

#include "javavm/include/defs.h"
#include "javavm/include/porting/jni.h"

/*****************************************************************
 * Support for CNI native methods. 07/13/99 
 *
 * Using the CNI native method interface rather than JNI provides a few
 * advantages. In general they are much faster and allow you to
 * eliminate C recursion back into the interpreter loop (like you get in
 * JDK when you call Method.invoke() or when static initializers are run).
 * Specifically, they have the following advantages:
 *
 * 1. No stack frame has to be pushed or popped for the method call.
 * 2. No argument wrapping/marshalling has to be done. Arguments are
 *    accessed directly from the java stack.
 * 3. No glue code has to be used to invoke the native method. It is
 *    called directly from the interpreter loop.
 * 4. CNI methods execute gcUnsafe, so they save the overhead of having
 *    to become gcSafe first.
 * 5. CNI methods can be used to invoke a method without the
 *    need for any C recursion back into the interpreter.
 *
 * Of course, all these advantages come at a price. Mostly the price you
 * pay is that you need to be very careful about gcSafety, so closely
 * follow the rules mentioned below.
 *
 * When an CNI method is entered, frame->topOfStack points above the
 * arguments so they will be scanned. There is no need to touch
 * frame->topOfStack, since the interpreter will handle updating it
 * after the CNI method returns.
 *
 ******************************************************************/

/* Result codes for CNI native methods. */

typedef enum {
    /* The following 3 CNIResultCodes are used for simple CNI
     * methods that act just like a regular methods that process the 
     * arguments and return a result. The CNIResultCode indicates the
     * size of the result. The result should be stored at *arguments
     * (see CNINativeMethod prototype below). 
     *
     * WARNING: Do not become gcSafe after storing the result, because
     * the GC will end up scanning the result with the wrong stackmap.
     */
    CNI_VOID = 0,     /* no result stored on stack. */
    CNI_SINGLE = 1,   /* a single word result was stored on the stack. */
    CNI_DOUBLE = 2,   /* a double word result was stored on the stack. */

    /* CNI_NEW_TRANSITION_FRAME is used to indicate that the CNI method
     * pushed a transition frame on the stack. This is used by CNI methods
     * that want the interpreter to call another method. The method could
     * have been called directly by the CNI method, but this introduces
     * C recursion. This facility is used by Method.invoke(), which calls
     * an CNI method to push a transition frame. The transition frame will
     * invoke the method passed to Method.invoke().
     *
     * Arguments are pushed on the transition frame just like they are
     * for any other transition frame, such as when jni uses a transition
     * frame to invoke a method.
     *
     * WARNING: The mb in the transition frame for non static methods
     * is assummed to be fully resolved. In other words, an interface
     * or superclass mb must be preresolved to the mb in the object's
     * methodtable, unless you really want the superclass method to be
     * invoked.
     */
    CNI_NEW_TRANSITION_FRAME = -1, /* method pushed a transition frame. */

    /* CNI_NEW_JAVA_FRAME is used when an CNI method pushes a JaveFrame
     * and it wants the interpreter to continue execution at the pc indicated
     * in the JavaFrame. Currently this isn't used because dealing with
     * arguments can get ugly, so CNI_NEW_TRANSITION_FRAME usually gets
     * used instead. 
     */
#if 0 /* currently no one uses this feature */    
    CNI_NEW_JAVA_FRAME = -2,       /* method pushed a java frame. */
#endif

    /* CNI_NEW_MB is used when an CNI method wants the interpreter to start
     * execution in a specific mb. The mb to start executing in is stored
     * in the p_mb argument (see CNINativeMethod prototype below), which is
     * a pointer to the mb local variable in the interpreter loop. 
     *
     * CNI_NEW_MB is useful when you want to call to a method that you 
     * aren't allowed to call directly from Java. <clinit> methods are an 
     * example of this. Class.runStaticInitializers is written in Java and
     * wants to directly call <clinit> methods, but the compiler won't 
     * allow this. Instead it calls the CVM.executeClinit CNI method,
     * which simply returns the mb of the <clinit> method so the interpeter 
     * can call it.
     *
     * WARNING: There are limitations on the type of mb that can be returned
     * with CNI_NEW_MB. You can't return an mb that will cause the stackmap
     * used by gc to be wrong. This means the arguments must meet the
     * following criteria:
     * 1. The CNI method must have at least as many arguments as the mb
     *    that it returns.
     * 2. Arguments that are refs for the CNI method must be refs for the
     *    returned mb and vice-versa.
     * 3. The results for the CNI method and the returned mb must be 
     *    compatible.
     */
    CNI_NEW_MB = -3,               /* method returned a new mb to invoke. */
    
    /* CNI_EXCEPTION is used to indicate that an exception was thrown
     * by the CNI method. This would normally be done by calling
     * CVMsignalError().
     */
    CNI_EXCEPTION = -4             /* an exception was thrown. */
} CNIResultCode;

/* All CNI native methods have the CNINativeMethod prototype. "arguments"
 * points to the beginning of the arguments on the java stack and is also
 * the location where any result should be stored. p_mb is a pointer
 * to the CVMMethodBlock* local variable in the interpreter loop. You
 * can use it to cause the interpreter to start execution in a new
 * method (see the CNI_NEW_MB description above.)
 */

typedef CNIResultCode CNINativeMethod(CVMExecEnv* ee,
				      CVMStackVal32* arguments,
				      CVMMethodBlock** p_mb);

#define CNIEXPORT JNIEXPORT

#endif /* _INCLUDED_CNI_H */

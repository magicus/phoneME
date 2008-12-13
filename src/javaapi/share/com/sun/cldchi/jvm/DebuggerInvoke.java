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

public class DebuggerInvoke {


    private static final int T_BOOLEAN   =  4;
    private static final int T_CHAR      =  5;
    private static final int T_FLOAT     =  6;
    private static final int T_DOUBLE    =  7;
    private static final int T_BYTE      =  8;
    private static final int T_SHORT     =  9;
    private static final int T_INT       = 10;
    private static final int T_LONG      = 11;
    private static final int T_OBJECT    = 12;
    private static final int T_ARRAY     = 13;
    private static final int T_VOID      = 14;

    /**
     * This method is used to synchronously invoke a method on 
     * behalf of a debugger and then return the method return value back
     * to the debugger.  The reason for this sync method is the
     * non-synchronous nature of EntryActivations in the VM.  Picture this,
     * some event happens in the VM, like a breakpoint, which gets sent to
     * the debugger.  The debugger decides to invoke a Method.  The return
     * packet to the debugger for that invoke can't be sent until the Method
     * returns and we send the return value (or exception object!) back to 
     * the debugger.  So, all threads are suspended in the VM, we have
     * a Method in hand and the debugger has picked a thread to execute this
     * Method.  We create an entry activation for that Method with all the
     * args passed in by debugger.  Then we create another entry activattion
     * for this method.  This allows us to synchronously call the Method
     * requested, catch any exceptions and then call back down into the VM
     * to send the reply packet.  The return code in the VM will strip off
     * these method calls from the thread stack so the thread will be back
     * in the state it was before we started this whole invoke thing.  We 
     * use two different return call methods based on what type of caller
     * was on the stack when we started the whole invoke process.  This is
     * because there are two entry points into the VM; shared_call_vm and
     * shared_call_vm_oop. (Don't care about shared_call_vm_exception). We
     * need to make sure that the original call into the VM returns properly
     * including any return values (object or otherwise) stored on the stack
     * or in the EntryFrame or ThreadDesc.
     *
     * @param entry really an EntryActivation object from the VM
     * 
     * @param transport a Transport object from the VM
     *
     * @param id debugger request id used to generate outgoing reply packet
     *
     * @param options invoke options from debugger
     *
     * @param return_type return type of Method being invoked
     *
     * @param is_obj_return if original caller of VM was shared_call_vm_oop
     *
     */

    private static void debuggerInvoke(Object entry, Object transport, int id,
                                       int options, int return_type,
                                       int is_obj_return)
    {
      try {
        Object ret_obj = null;
        switch (return_type) {
        case T_VOID:
            invokeV(entry);
            break;
        case  T_OBJECT: {
            Object[] ret = new Object[1];
            Object o = invokeL(entry);
            ret[0] = o;
            ret_obj = ret;
            break;
        }

        case T_BOOLEAN:
        case T_BYTE:
        case T_SHORT:
        case T_CHAR:
        case T_INT: {
            int[] ret = new int[1];
            switch (return_type) {
            case T_BOOLEAN:
                boolean b = invokeZ(entry);
                ret[0] = b ? 1 : 0;
                break;
            case T_BYTE:
                ret[0] = (int)invokeB(entry);
                break;
            case T_SHORT:
                ret[0] = (int)invokeS(entry);
                break;
            case T_CHAR:
                ret[0] = (int)invokeC(entry);
                break;
            case T_INT:
                ret[0] = invokeI(entry);
                break;
            }
            ret_obj = ret;
            break;
        }
        case T_LONG: {
            long[] ret = new long[1];
            ret[0] = invokeJ(entry);
            ret_obj = ret;
            break;
        }
        case T_DOUBLE: {
            double[] ret = new double[1];
            ret[0] = invokeD(entry);
            ret_obj = ret;
            break;
        }
        case T_FLOAT: {
            float[] ret = new float[1];
            ret[0] = invokeF(entry);
            ret_obj = ret;
            break;
        }
        }
        if (is_obj_return == 1) {
            debuggerInvokeReturnObj(ret_obj, null, transport, id, options,
                                    return_type);
        } else {
            debuggerInvokeReturn(ret_obj, null, transport, id, options,
                                 return_type);
        }
      } catch (Throwable t) {
          // we send the exception object to the debugger via the native call
          if (is_obj_return == 1) {
              debuggerInvokeReturnObj(null, t, transport, id, options,
                                      return_type);
          } else {
              debuggerInvokeReturn(null, t, transport, id, options,
                                   return_type);
          }
      }
    }

    /**
     * This method is used by the java debugger code to signal the
     * synchronous end of a method invoke
     */

    private native static long debuggerInvokeReturn(Object ret,
                                                   Object exception,
                                                   Object transport,
                                                   int id,
                                                   int options,
                                                   int return_type);

    /**
     * You might think "Oh, I could combine this two methods into one
     * and pass an arg to distinguish them".  Don't!  This one returns
     * Object so that it enters the VM via shared_call_vm_oop.  When
     * we strip the frames off the stack in ObjectReferenceImpl::invoke_return
     * we need to make sure we return the original return value that was
     * stored in the EntryFrame back to the original caller.
     */
    private native static Object debuggerInvokeReturnObj(Object ret,
                                                   Object exception,
                                                   Object transport,
                                                   int id,
                                                   int options,
                                                   int return_type);

    private static native void    invokeV(Object entry);

    private static native boolean invokeZ(Object entry);
    private static native char    invokeC(Object entry);
    private static native float   invokeF(Object entry);
    private static native double  invokeD(Object entry);
    private static native byte    invokeB(Object entry);
    private static native short   invokeS(Object entry);
    private static native int     invokeI(Object entry);
    private static native long    invokeJ(Object entry);
    private static native Object  invokeL(Object entry);


}

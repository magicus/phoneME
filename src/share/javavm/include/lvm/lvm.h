/*
 * @(#)lvm.h	1.8 06/10/10
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
 * Because of mutual dependencies, this needs to be included directly 
 * from javavm/include/interpreter.h only.
 */
#ifndef _INCLUDED_INTERPRETER_H
#error Include javavm/include/interpreter.h instead of this header file
#endif

#ifndef _INCLUDED_LOGICALVM_H
#define _INCLUDED_LOGICALVM_H

/*
  CVMLVMContext is the abstract representation of an LVM isolate at the
  native layer.  There is one instance of a CVMLVMContext per isolate.  Each
  CVMLVMContext is associated with a sun.misc.Isolate Java Object instance.

  CVMLVMContext also provides storage for the replicated statics that is
  owned by this isolate.

  All CVMLVMContext are linked together as elements in a list that
  hangs off of the CVMLVMGlobals.  We can iterate over all isolates via this
  list.
*/

typedef struct CVMLVMContext CVMLVMContext;
struct CVMLVMContext
{
    /* Links for the global list of contexts: */
    struct CVMLVMContext** prevPtr;
    struct CVMLVMContext*  next;

    /* LVM particulars: */
    CVMInt32         id;
    CVMObjectICell*  lvmICell; /* sun.misc.LVM for this context. */

    /* If this LVM is in process of halting */
    CVMBool  halting;
#ifdef CVM_CLASSLOADING
    /* Per-LVM replication of system class loader */
    CVMClassLoaderICell* systemClassLoader;
#endif

    /* Per-LVM static field table. This field needs to be at the end of this
       struct: */
    CVMJavaVal32    statics[1];
};

#define CVMLVMstatics2Context(s) \
    ((CVMLVMContext*)((char*)(s) - CVMoffsetof(CVMLVMContext, statics[0])))

#define CVMLVMeeGetContext(ee)  ((ee)->lvmEE.context)

#define CVMLVMeeSystemClassLoader(ee) \
    (CVMLVMeeGetContext(ee)->systemClassLoader)

/*
 * Logical VM specific globally defined variables 
 * (embedded in CVMGlobalState structure).
 */
typedef struct CVMLVMGlobals CVMLVMGlobals;
struct CVMLVMGlobals
{
    CVMUint32       numberOfContexts;
    CVMLVMContext  *contexts;
    CVMMutex        lvmLock; /* Syncs operations on the LVM list. */

    /* Number of classes that are subject to the replication by 
     * Logical VM */
    CVMUint32       numClasses;
    /* Number of classes that are subject to the replication by 
     * Logical VM and that have <clinit> (i.e. the number of necessary
     * slots for CVMExecEnv.clinitEEX replication per Logical VM). */
    CVMUint32       numStatics;
    /* Initial values of the static fields that Logical VM replicates. */
    CVMJavaVal32*   staticDataMaster;
    /* Holds a global reference to the main (primordial) Logical VM
     * throughout the lifecycle of the JVM. */
    CVMObjectICell* mainLVM;
    /* ee->lvmEE.statics value of the main Logical VM.
     * The reason why we need to store this in addition to mainLVM is
     * described in comments in CVMLVMinitExecEnv(). */
    CVMJavaVal32*   mainStatics;
};

/*
 * Per-EE Logical VM related info (embedded in CVMExecEnv structure).
 */
typedef struct CVMLVMExecEnv CVMLVMExecEnv;
struct CVMLVMExecEnv
{
    CVMLVMContext *context;

    /* Pointer to the LVM object to which this EE belongs */
    CVMObjectICell*  lvmICell;
    /* Pointer to the per-LVM static field table */
    CVMJavaVal32*    statics;
};

#define CVMLVMeeStatics(ee) ((ee)->lvmEE.statics)

/*
 * Structure for passing Logical VM related info from parent thread to child.
 */
typedef struct {
    /* This points to a global root which contains a Logical VM object.
     * CVMinitExecEnv() call in start_func() will result in storing the
     * object in the new thread's CVMLVMExecEnv.  Deallocation of the 
     * global root happends in CVMLVMeeInitAuxDestroy() which is invoked 
     * from start_func(). */
    CVMObjectICell* lvmICell;
    CVMJavaVal32*   statics;
} CVMLVMEEInitAux;

extern CVMBool CVMLVMglobalsInitPhase1(CVMLVMGlobals* lvm);
extern CVMBool CVMLVMglobalsInitPhase2(CVMExecEnv* ee, CVMLVMGlobals* lvm);
extern void    CVMLVMglobalsDestroy(CVMExecEnv* ee, CVMLVMGlobals* lvm);
extern CVMBool CVMLVMfinishUpBootstrapping(CVMExecEnv* ee);

extern CVMBool CVMLVMeeInitAuxInit(CVMExecEnv* ee, CVMLVMEEInitAux* aux);
extern void    CVMLVMeeInitAuxDestroy(CVMExecEnv* ee, CVMLVMEEInitAux* aux);

extern CVMBool CVMLVMinitExecEnv(CVMExecEnv* ee, CVMLVMEEInitAux* aux);
extern void    CVMLVMdestroyExecEnv(CVMExecEnv* ee);
extern void    CVMLVMwalkStatics(CVMRefCallbackFunc callback, 
				 void* callbackData);

extern CVMBool CVMLVMjobjectInit(JNIEnv* env, jobject lvmObj,
				 jobject initThread);
extern void    CVMLVMjobjectDestroy(JNIEnv* env, jobject lvmObj);
extern void    CVMLVMcontextSwitch(JNIEnv* env, jobject targetLvmObj);

extern void    CVMLVMcontextHalt(CVMExecEnv* ee);
extern CVMBool CVMLVMskipExceptionHandling(CVMExecEnv* ee,
					   CVMFrameIterator* iter);
extern CVMBool CVMLVMinMainLVM(CVMExecEnv* ee);

#ifdef CVM_DEBUG
extern void    CVMLVMnativeInfoShowAll(CVMExecEnv* ee);

extern void    CVMLVMtraceAttachThread(CVMExecEnv* ee);
extern void    CVMLVMtraceDetachThread(CVMExecEnv* ee);
extern void    CVMLVMtraceNewThread(CVMExecEnv* ee);
extern void    CVMLVMtraceTerminateThread(CVMExecEnv* ee);
#endif

#endif /* _INCLUDED_LOGICALVM_H */

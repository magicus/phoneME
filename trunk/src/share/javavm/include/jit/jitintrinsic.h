/*
 * @(#)jitintrinsic.h	1.13 06/10/10
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

#ifndef _INCLUDED_JITINTRINSIC_H
#define _INCLUDED_JITINTRINSIC_H

#include "javavm/include/classes.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/jit_common.h"
#include "javavm/include/jit/jit_defs.h"
#include "javavm/include/porting/jit/jit.h"

#ifdef CVMJIT_INTRINSICS

/*
    NOTE: How to fill out the CVMJITIntrinsicConfig entries below?
    =============================================================
    The CVMJITIntrinsicConfig is a constant struct that can contain
    configuration information about how a specific intrinsic method is handled
    by the compiler.  The fields are as follows:

    struct CVMJITIntrinsicConfig {
        const char *classname;
        const char *methodName;
        const char *methodSignature;
        CVMUint16 properties;
        CVMUint16 irnodeFlags;
        void *emitterOrCCMRuntimeHelper;
    };

    NOTE: Only classes loaded by the bootclassloader (i.e. loaded from the
          boot classpath) can have intrinsic methods.  If you wish to add
          and intrinsic method, you'll have to make sure that its class is
          in the boot classpath.

    className is the class name of the method's class.
    methodName is the method name.
    methodSignature is the method signature.

    NOTE: The className, methodName, and methodSignature must be in utf8
          format as required by the JNI specification.

    properties is a bitwise combination of intrinsic properties flag values
        which are OR'ed together.  Legal values are:

            CVMJITINTRINSIC_IS_NOT_STATIC   or
            CVMJITINTRINSIC_IS_STATIC
        |
            CVMJITINTRINSIC_OPERATOR_ARGS       or
            CVMJITINTRINSIC_C_ARGS              or
            CVMJITINTRINSIC_JAVA_ARGS
        |
            CVMJITINTRINSIC_SPILLS_NOT_NEEDED   or
            CVMJITINTRINSIC_NEED_MINOR_SPILL    or
            CVMJITINTRINSIC_NEED_MAJOR_SPILL
        |
            CVMJITINTRINSIC_STACKMAP_NOT_NEEDED or
            CVMJITINTRINSIC_NEED_STACKMAP       or
            CVMJITINTRINSIC_NEED_EXTENDED_STACKMAP
        |
            CVMJITINTRINSIC_NO_CP_DUMP or
            CVMJITINTRINSIC_CP_DUMP_OK
        |
            CVMJITINTRINSIC_ADD_CCEE_ARG
        |
            CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS
        |
            CVMJITINTRINSIC_FLUSH_JAVA_STACK_FRAME

        CVMJITINTRINSIC_IS_NOT_STATIC, CVMJITINTRINSIC_IS_STATIC:
            Indicates if the intrinsic method is static or not.

        CVMJITINTRINSIC_OPERATOR_ARGS, CVMJITINTRINSIC_C_ARGS,
        CVMJITINTRINSIC_JAVA_ARGS:
            Indicated the calling conventions to be used.  See
            emitterOrCCMRuntimeHelper below for details.

        CVMJITINTRINSIC_SPILLS_NOT_NEEDED, CVMJITINTRINSIC_NEED_MINOR_SPILL,
        CVMJITINTRINSIC_NEED_MAJOR_SPILL:
            Indicates if spills need to happen and what type of spills.  Minor
            spills are necessary if the helper will use volatile registers.
            Major spills are necessary if the helper can become GC safe.

        CVMJITINTRINSIC_STACKMAP_NOT_NEEDED, CVMJITINTRINSIC_NEED_STACKMAP,
        CVMJITINTRINSIC_NEED_EXTENDED_STACKMAP:
            Indicates if a stackmap capture need to be done for the call to
            the CCM helper.

        CVMJITINTRINSIC_NO_CP_DUMP, CVMJITINTRINSIC_CP_DUMP_OK:
            Indicates if a constant pool dump is allowed after the call to
            the CCM helper.  CVMJITINTRINSIC_CP_DUMP_OK does not mean that a
            dump will actually occur.  It only gives permission to allow a
            dump if necessary.

        CVMJITINTRINSIC_ADD_CCEE_ARG:
            Indicates that the first argument must be the ccee.  All other
            arguments are bumped to the right to make room for the ccee as
            the first argument.

        CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS:
            Indicates to the compiler front end that cached references/fields
            need to be killed because this method may invalidate those
            cached references/field values.

            If the intrinsic method may write into some instance or static
            fields or array elements, then this property need to be set.  This
            property also needs to be set if the intrinsics method can become
            GC safe.

            NOTE: If the method has the CVMJITINTRINSIC_NEED_MAJOR_SPILL
                  and/or CVMJITINTRINSIC_NEED_STACKMAP properties set, then
                  CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS needs to be set.
                  Otherwise, CVM will fail with an assertion if assertions are
                  enabled.  This is because these properties imply that GC can
                  run and therefore change the values of references and
                  invalidating the cache.

        CVMJITINTRINSIC_FLUSH_JAVA_STACK_FRAME:
            Indicates to the compiler back end that the Java stack pointer and
            return PC will have to be flushed to the current Java frame, and
            the Java frame pointer will have to be flushed to the Java stack.
            These need to be done before the intrinsic method is called.

            If the intrinsic method can throw exceptions or can become GC
            safe, then this property needs to be set.  This is because
            exception handling and GC processes require a consistent Java
            stack.  Flushing the Java stack frame makes the Java stack
            consistent.  Note that the intrinsic method is still responsible
            for fixing up compiled frames using the CVMCCMruntimeLazyFixups()
            macro or equivalent if it is actually going to throw an exception
            or become GC safe.

            NOTE: If the method has CVMJITIRNODE_THROWS_EXCEPTIONS set in its
                  irnodeFlags, then CVMJITINTRINSIC_FLUSH_JAVA_STACK_FRAME
                  needs to be set.  Otherwise, CVM will fail with an assertion
                  if assertions are enabled.

                  This assertion is also true for when
                  CVMJITINTRINSIC_NEED_MAJOR_SPILL and/or
                  CVMJITINTRINSIC_NEED_STACKMAP are set.

    irnodeFlags can be:
        CVMJITIRNODE_NULL_FLAGS or
        CVMJITIRNODE_THROWS_EXCEPTIONS or
        CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT

        CVMJITIRNODE_NULL_FLAGS is used when the intrinsic method call has no
            side effects and hence can be relocated relative to adjacent
            byte code instructions.  An example of this would be:
                java.lang.StrictMath.sin(D)D
            which returns the same result independent of where in the
            caller method it is called from.  It also doesn't throw any
            exceptions and there for does not have to bounded to any "try"
            ranges for exception handling.  The only requirement is that
            its input remains the same, and that is enforced by the IR
            generator.
        CVMJITIRNODE_THROWS_EXCEPTIONS is used when the intrinsic method call
            can throw an exception.  This means that the call site will not be
            relocated to ensure that it remains within the "try" range
            specified for exception handling.
        CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT is used when the intrinsic
            method call is position sensitive and should not be relocated.
            An example of this would be:
                java.lang.System.currentTimeMillis()J
            Relocating this call can cause the results of the caller to be
            incorrect.  The caller is calling this method at a certain
            location which usually demarks the boundary of a region of code
            around which the caller wishes to measure a time differential.
            Moving the location of the call to currentTimeMillis() would
            cause the boundary to effectively move thereby changing the
            nature of the measurement made by the caller and possibly
            invalidating it.  This is just one example of why an intrinsic
            method needs to be set as CVMJITIRNODE_HAS_UNDEFINED_SIDE_EFFECT.

    emitterOrCCMRuntimeHelper is used to point to an emitter or CCM runtime
        helper or glue that will implement the intrinsic method.

        The prototype of the CCM runtime helper will depend on the calling
        convention set above.  The following calling conventions are only used
        for CCM runtime helpers as the intrinsic method:

        CVMJITINTRINSIC_C_ARGS: The call will use C calling conventions.
            The prototype of the helper must match the prototype of the
            intrinsic method.  The implicit this pointer of instance methods
            will be made explicit as the first argument of the C helper.
            Static methods will not have the this pointer argument.
        CVMJITINTRINSIC_JAVA_ARGS: The call will use Java calling conventions
            as far as argument passing and return value retrieval are
            concerned i.e. the arguments are pushed on the Java stack and the
            return value can be popped off the Java stack.  However, a Java
            stack frame will not be pushed or popped.  Hence, this is not a
            pure substitute for a Java call.  The prototype of the helper is
            void(*)(void).  If any other arguments need to be passed, the
            helper will need to be glue code which sets up the additional
            arguments.
       CVMJITINTRINSIC_REG_ARGS: The call will use a target specific
            convention that passes all arguments in registers.  The selection
	    and assignment of registers for each arg is defined by platform
	    specific code for the target i.e. the scheme may (and is likely
	    to) vary from platform to platform.  However, given a specific
	    platform, the register assignment scheme is fixed for the
	    platform.  Hence, if 2 intrinsic methods uses this convention on
	    a given target, both will expected to apply the same register
	    assignment scheme for their args.

        To use an emitter, this field must point to a
        CVMJITIntrinsicEmitterVtbl struct as follows:

        struct CVMJITIntrinsicEmitterVtbl {
            CVMJITRegsRequiredType (*getRequired)
	                              (CVMJITCompilationContext *con,
                                       CVMJITIRNode *intrinsicNode,
                                       CVMJITRegsRequiredType argsRequired);
            CVMRMregset (*getArgTarget)(CVMJITCompilationContext *con,
                                        int typeTag, CVMUint16 argNumber,
                                        CVMUint16 argWordIndex);
            void (*emitOperator)(CVMJITCompilationContext *con,
                                 CVMJITIRNode *intrinsicNode);
        };

        CVMJITINTRINSIC_OPERATOR_ARGS: This calling convention will indicate
            to the code generator to use the emitter callbacks.

            emitter->getRequired() will be called to get the register required
            set for the operator.

            emitter->getArgTarget() will be called to get the register
            targetting info for each argument for the operator.

            emitter->emitOperator() will be called to emit the instructions
            needed to implement the intrinsic method.  The emitOperator()
            function is handle all activity for the codegen rule action
            including popping resources for the argument, emitting needed
            instructions, and pushing resources for the result.
*/

/* CVMJITIntrinsic properties & misc constants: */
enum {
    CVMJITINTRINSIC_UNDEFINED                   = 0,

    /* Calling convention: determines how args and return vals are passed: */
    CVMJITINTRINSIC_OPERATOR_ARGS               = (1U << 0),
    CVMJITINTRINSIC_C_ARGS                      = (1U << 1),
    CVMJITINTRINSIC_JAVA_ARGS                   = (1U << 2),
    CVMJITINTRINSIC_REG_ARGS			= (1U << 3),

    /* Indicates if method is static or not: */
    CVMJITINTRINSIC_IS_NOT_STATIC               = 0,
    CVMJITINTRINSIC_IS_STATIC                   = (1U << 4),

    /* Spill requirements: determines what type of spill is needed: */
    CVMJITINTRINSIC_SPILLS_NOT_NEEDED           = 0,
    CVMJITINTRINSIC_NEED_MINOR_SPILL            = (1U << 5),
    CVMJITINTRINSIC_NEED_MAJOR_SPILL            = (1U << 6),

    /* Stackmap requirements: determines what type of stackmap is needed: */
    CVMJITINTRINSIC_STACKMAP_NOT_NEEDED         = 0,
    CVMJITINTRINSIC_NEED_STACKMAP               = (1U << 7),
    CVMJITINTRINSIC_NEED_EXTENDED_STACKMAP      = (1U << 8),

    /* Constant pool dump allowance: determines if a constant pool dumps is
       allowed: */
    CVMJITINTRINSIC_NO_CP_DUMP                  = 0,
    CVMJITINTRINSIC_CP_DUMP_OK                  = (1U << 9),

    /* Miscellaneous options: */
    CVMJITINTRINSIC_ADD_CCEE_ARG                = (1U << 10),
    CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS    = (1U << 11),
    CVMJITINTRINSIC_FLUSH_JAVA_STACK_FRAME      = (1U << 12)
};

typedef struct CVMJITIntrinsicEmitterVtbl CVMJITIntrinsicEmitterVtbl;
struct CVMJITIntrinsicEmitterVtbl
{
    CVMJITRegsRequiredType (*getRequired)
	 (CVMJITCompilationContext *con,
	  CVMJITIRNode *intrinsicNode,
	  CVMJITRegsRequiredType argsRequired);
    CVMRMregset (*getArgTarget)
	 (CVMJITCompilationContext *con, int typeTag,
	  CVMUint16 argNumber, CVMUint16 argWordIndex);
    void (*emitOperator)
	 (CVMJITCompilationContext *con,
	  CVMJITIRNode *intrinsicNode);
};

typedef struct CVMJITIntrinsicConfig CVMJITIntrinsicConfig;
struct CVMJITIntrinsicConfig
{
    const char *className;
    const char *methodName;
    const char *methodSignature;
    CVMUint16 properties;
    CVMUint16 irnodeFlags;
    void *emitterOrCCMRuntimeHelper;
};

/* NOTE: The typedef is defined in javavm/include/defs.h */
struct CVMJITIntrinsicConfigList
{
    const CVMJITIntrinsicConfigList * const parentList;
    const CVMJITIntrinsicConfig * const configs;
    CVMUint16 numberOfConfigs;
#ifdef CVM_DEBUG
    const char *name;
#endif
};

#ifdef CVM_DEBUG
#define CVMJIT_INTRINSIC_LIST_NAME(listname__) #listname__
#else
#define CVMJIT_INTRINSIC_LIST_NAME(listname__)
#endif

#define CVMJIT_INTRINSIC_CONFIG_BEGIN(listname__) \
static const CVMJITIntrinsicConfig listname__##Configs__[] = {

#define CVMJIT_INTRINSIC_CONFIG_END(listname__, parentList__) \
};                                                            \
const CVMJITIntrinsicConfigList listname__ = {                \
    /* Parent List       */ parentList__,                     \
    /* Configs           */ listname__##Configs__,            \
    /* Number of Configs */ sizeof(listname__##Configs__) /   \
                            sizeof(listname__##Configs__[0]), \
    /* Name of List      */ CVMJIT_INTRINSIC_LIST_NAME(listname__) \
};

extern const CVMJITIntrinsicConfigList CVMJITdefaultIntrinsicsList;

/* NOTE: typedef'ed in jit_defs.h */
struct CVMJITIntrinsic
{
    const CVMJITIntrinsicConfig *config;
    CVMMethodBlock *mb;
    CVMClassTypeID classTypeID;
    CVMMethodTypeID methodTypeID;
    CVMUint8 intrinsicID;
    CVMUint8 numberOfArgs;
    CVMUint8 numberOfArgsWords;
#ifdef CVM_DEBUG
    const CVMJITIntrinsicConfigList *configList;
#endif
};

/*==========================================================================*/

/* Purpose: Initialized the JIT intrinsics manager. */
extern CVMBool
CVMJITintrinsicInit(CVMExecEnv *ee, CVMJITGlobalState *jgs);

/* Purpose: Destroys the JIT intrinsics manager. */
extern void
CVMJITintrinsicDestroy(CVMJITGlobalState *jgs);

/* Purpose: Gets the intrinsic record for the specified method if available.*/
extern CVMJITIntrinsic *
CVMJITintrinsicGetIntrinsicRecord(CVMExecEnv *ee, CVMMethodBlock *mb);

/* Purpose: Scans the newly loaded class for intrinsic methods.  If intrinsic
            methods are found in the specified class, then those methods will
            be added to the intrinsics list. */
extern void
CVMJITintrinsicScanForIntrinsicMethods(CVMExecEnv *ee, CVMClassBlock *cb);

#endif /* CVMJIT_INTRINSICS */

#endif /* _INCLUDED_JITINTRINSIC_H */

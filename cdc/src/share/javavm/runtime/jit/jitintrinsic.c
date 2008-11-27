/*
 * @(#)jitintrinsic.c	1.13 06/10/10
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

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/globals.h"
#include "javavm/include/utils.h"
#include "javavm/include/preloader.h"

#include "javavm/include/jit/jit.h"
#include "javavm/include/porting/jit/jit.h"
#include "javavm/include/jit/jitintrinsic.h"
#include "javavm/include/jit/jitirnode.h"

#ifdef CVMJIT_INTRINSICS

/* NOTE: #define CVM_DEBUG_INTRINSICS to turn on debugging of JIT intrinsics
   processing. */
/* #define CVM_DEBUG_INTRINSICS */

#define CVMJIT_INTRINSIC_ERROR  ((CVMUint16)-1)

/* Purpose: Fills in the intrinsics list with info from the config list.
            If an entry already exists for a method, the new entry will be
            ignored. */
static CVMUint16
fillInIntrinsicsFromConfigs(CVMExecEnv *ee,
                            const CVMJITIntrinsicConfigList *clist,
                            CVMJITIntrinsic *ilist,
                            CVMUint16 numberOfIntrinsics)
{
    CVMUint16 origNumberOfIntrinsics = numberOfIntrinsics;
    CVMUint16 i;
    for (i = 0; i < clist->numberOfConfigs; i++) {
        const CVMJITIntrinsicConfig *cf = &clist->configs[i];
        CVMClassTypeID classTypeID;
        CVMMethodTypeID methodTypeID;
        CVMClassBlock *cb;
        CVMMethodBlock *mb;
        CVMUint16 j;
        CVMBool isStatic;

        isStatic = ((cf->properties & CVMJITINTRINSIC_IS_STATIC) != 0);

        /* Get the classTypeID for the intrinsic: */
        classTypeID = CVMtypeidNewClassID(ee, cf->className,
                                          strlen(cf->className));
        if (classTypeID == CVM_TYPEID_ERROR) {
            goto errorCase;
        }

        /* Get the methodTypeID for the intrinsic: */
        methodTypeID = CVMtypeidNewMethodIDFromNameAndSig(ee,
                           cf->methodName, cf->methodSignature);
        if (methodTypeID == CVM_TYPEID_ERROR) {
            goto errorCase;
        }

        for (j = 0; j < origNumberOfIntrinsics; j++) {
            if ((ilist[j].classTypeID == classTypeID) &&
                (ilist[j].methodTypeID == methodTypeID)) {
                break;
            }
        }
        /* NOT Found!  Add it to the list: */
        if (j == origNumberOfIntrinsics) {
            CVMJITIntrinsic *intrinsic;

            /* We only allow intrinsic methods for classes which are loaded
               with the bootclassloader.  Hence, we do the class lookup with
               NULL as the classloader: */
            cb = CVMclassLookupClassWithoutLoading(ee, classTypeID, NULL);
            if (cb != NULL) {
                mb = CVMclassGetMethodBlock(cb, methodTypeID, isStatic);
                CVMassert(mb != NULL);
            } else {
                mb = NULL;
            }

            intrinsic = &ilist[numberOfIntrinsics];
            intrinsic->config = cf;
            intrinsic->mb = mb;
            intrinsic->classTypeID = classTypeID;
            intrinsic->methodTypeID = methodTypeID;
            intrinsic->intrinsicID = ++numberOfIntrinsics;
            intrinsic->numberOfArgs = CVMtypeidGetArgsCount(methodTypeID);
            intrinsic->numberOfArgsWords = CVMtypeidGetArgsSize(methodTypeID);
            if ((intrinsic->config->properties & CVMJITINTRINSIC_IS_STATIC)
                == 0) {
                intrinsic->numberOfArgs++;
                intrinsic->numberOfArgsWords++;
            }
#ifdef CVM_DEBUG_ASSERTS
            if (mb != NULL) {
                CVMassert(CVMmbArgsSize(mb) == intrinsic->numberOfArgsWords);
            }
#endif
            if ((intrinsic->config->properties & CVMJITINTRINSIC_ADD_CCEE_ARG)
                != 0) {
                intrinsic->numberOfArgsWords++;
            }
#ifdef CVM_DEBUG
            intrinsic->configList = clist;
#endif
        }
    }
    return numberOfIntrinsics;

errorCase:
#ifdef CVM_DEBUG
    CVMconsolePrintf("Failed to allocate a typeID when initializing JIT "
                     "intrinsics\n");
#endif
    return CVMJIT_INTRINSIC_ERROR;
}

/* Purpose: Comparator used for sorting the sortedJavaIntrinsics list and the
            sortedNativeIntrinsics list. */
static int
compareIntrinsic(const void *v1, const void *v2)
{
    CVMJITIntrinsic *ip1;
    CVMJITIntrinsic *ip2;

    CVMassert(v1 != NULL);
    CVMassert(v2 != NULL);
    ip1 = *(CVMJITIntrinsic **)v1;
    ip2 = *(CVMJITIntrinsic **)v2;
    CVMassert(ip1->mb != NULL);
    CVMassert(ip2->mb != NULL);
    return ((CVMInt32)ip1->mb - (CVMInt32)ip2->mb);
}

/* Purpose: Comparator used for sorting the sortedUnknownIntrinsics list. */
static int
compareUnknownIntrinsic(const void *v1, const void *v2)
{
    CVMJITIntrinsic *ip1;
    CVMJITIntrinsic *ip2;

    CVMassert(v1 != NULL);
    CVMassert(v2 != NULL);
    ip1 = *(CVMJITIntrinsic **)v1;
    ip2 = *(CVMJITIntrinsic **)v2;
    if (ip1->classTypeID == ip2->classTypeID) {
        return ((CVMInt32)ip1->methodTypeID - (CVMInt32)ip2->methodTypeID);
    }
    return ((CVMInt32)ip1->classTypeID - (CVMInt32)ip2->classTypeID);
}

#if defined(CVM_DEBUG_ASSERTS) || defined(CVM_DEBUG_INTRINSICS)

#ifdef CVM_DEBUG
#define SETUP_ERROR() { \
    if (!headerDumped) {                                  \
        CVMconsolePrintf("INTRINSIC %s.%s%s from %s:\n",  \
            config->className, config->methodName,        \
            config->methodSignature, irec->configList->name); \
        headerDumped =  CVM_TRUE;                         \
    }                                                     \
    errors++;                                             \
}
#else
#define SETUP_ERROR() { \
    if (!headerDumped) {                                  \
        CVMconsolePrintf("INTRINSIC %s.%s%s:\n",          \
            config->className, config->methodName,        \
            config->methodSignature);                     \
        headerDumped =  CVM_TRUE;                         \
    }                                                     \
    errors++;                                             \
}
#endif

static int verifyIntrinsic(CVMJITIntrinsic *irec, int errors)
{
    const CVMJITIntrinsicConfig *config = irec->config;
    CVMBool headerDumped = CVM_FALSE;
    CVMUint16 properties = config->properties;
    CVMUint16 irnodeFlags = config->irnodeFlags;
    CVMUint16 cconv, spill, stackmap, cpdump, cceearg, killcache, flushstack;

    /* Verify properties: */
    cconv = properties & (CVMJITINTRINSIC_OPERATOR_ARGS |
#ifdef CVMJIT_INTRINSICS_HAVE_PLATFORM_SPECIFIC_REG_ARGS
			  CVMJITINTRINSIC_REG_ARGS |
#endif
                          CVMJITINTRINSIC_C_ARGS | CVMJITINTRINSIC_JAVA_ARGS);
    if (cconv == 0) {
        SETUP_ERROR();
        CVMconsolePrintf("Error %d: Calling convention must be "
            "CVMJITINTRINSIC_OPERATOR_ARGS, CVMJITINTRINSIC_C_ARGS, or "
                         "CVMJITINTRINSIC_JAVA_ARGS\n", errors);
    } else if ((cconv != CVMJITINTRINSIC_OPERATOR_ARGS) &&
#ifdef CVMJIT_INTRINSICS_HAVE_PLATFORM_SPECIFIC_REG_ARGS
               (cconv != CVMJITINTRINSIC_REG_ARGS) &&
#endif
               (cconv != CVMJITINTRINSIC_C_ARGS) &&
               (cconv != CVMJITINTRINSIC_JAVA_ARGS)) {
        SETUP_ERROR();
        CVMconsolePrintf("Error %d: Too many calling conventions specified: ",
                         errors);
        if ((cconv & CVMJITINTRINSIC_OPERATOR_ARGS) != 0) {
            CVMconsolePrintf("   CVMJITINTRINSIC_OPERATOR_ARGS\n");
        }
#ifdef CVMJIT_INTRINSICS_HAVE_PLATFORM_SPECIFIC_REG_ARGS
        if ((cconv & CVMJITINTRINSIC_REG_ARGS) != 0) {
            CVMconsolePrintf("   CVMJITINTRINSIC_REG_ARGS\n");
        }
#endif
        if ((cconv & CVMJITINTRINSIC_C_ARGS) != 0) {
            CVMconsolePrintf("   CVMJITINTRINSIC_C_ARGS\n");
        }
        if ((cconv & CVMJITINTRINSIC_JAVA_ARGS) != 0) {
            CVMconsolePrintf("   CVMJITINTRINSIC_JAVA_ARGS\n");
        }
        CVMconsolePrintf("    Only one may be specified\n");
    }

    spill = properties & (CVMJITINTRINSIC_NEED_MINOR_SPILL |
                          CVMJITINTRINSIC_NEED_MAJOR_SPILL);
    if (cconv == CVMJITINTRINSIC_OPERATOR_ARGS) {
        if (spill != 0) {
            SETUP_ERROR();
            CVMconsolePrintf("Error %d: CVMJITINTRINSIC_NEED_MINOR_SPILL and "
                "CVMJITINTRINSIC_NEED_MAJOR_SPILL not supported for "
                "CVMJITINTRINSIC_OPERATOR_ARGS calling convention\n");
        }
    } else 
        if (spill == (CVMJITINTRINSIC_NEED_MINOR_SPILL |
                      CVMJITINTRINSIC_NEED_MAJOR_SPILL)) {
        SETUP_ERROR();
        CVMconsolePrintf("Error %d: Spill requirement is ambiguous: both "
            "CVMJITINTRINSIC_NEED_MINOR_SPILL and "
            "CVMJITINTRINSIC_NEED_MAJOR_SPILL are specified\n",
            errors);
    }

    stackmap = properties & (CVMJITINTRINSIC_NEED_STACKMAP |
                             CVMJITINTRINSIC_NEED_EXTENDED_STACKMAP);
    if (cconv == CVMJITINTRINSIC_OPERATOR_ARGS) {
        if (stackmap != 0) {
            SETUP_ERROR();
            CVMconsolePrintf("Error %d: CVMJITINTRINSIC_NEED_STACKMAP and "
                "CVMJITINTRINSIC_NEED_EXTENDED_STACKMAP not supported for "
                "CVMJITINTRINSIC_OPERATOR_ARGS calling convention\n", errors);
        }
    } else 
        if (stackmap == (CVMJITINTRINSIC_NEED_STACKMAP |
                         CVMJITINTRINSIC_NEED_EXTENDED_STACKMAP)) {
        SETUP_ERROR();
        CVMconsolePrintf("Error %d: Stackmap requirement is ambiguous: both "
            "CVMJITINTRINSIC_NEED_STACKMAP and "
            "CVMJITINTRINSIC_NEED_EXTENDED_STACKMAP are specified\n",
            errors);
    }

    cpdump = properties & CVMJITINTRINSIC_CP_DUMP_OK;
    if (cconv == CVMJITINTRINSIC_OPERATOR_ARGS) {
        if (cpdump != 0) {
            SETUP_ERROR();
            CVMconsolePrintf("Error %d: CVMJITINTRINSIC_CP_DUMP_OK "
                "not supported for CVMJITINTRINSIC_OPERATOR_ARGS "
                "calling convention\n", errors);
        }
    }

    cceearg = properties & CVMJITINTRINSIC_ADD_CCEE_ARG;
    if (cconv == CVMJITINTRINSIC_OPERATOR_ARGS) {
        if (cceearg != 0) {
            SETUP_ERROR();
            CVMconsolePrintf("Error %d: CVMJITINTRINSIC_ADD_CCEE_ARG "
                "not supported for CVMJITINTRINSIC_OPERATOR_ARGS "
                "calling convention\n", errors);
        }
    }

    killcache = properties & CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS;
    if (killcache != CVMJITINTRINSIC_NEED_TO_KILL_CACHED_REFS) {
        if ((spill == CVMJITINTRINSIC_NEED_MAJOR_SPILL) ||
            (stackmap == CVMJITINTRINSIC_NEED_STACKMAP)) {
            SETUP_ERROR();
            CVMconsolePrintf("Error %d: CVMJITINTRINSIC_NEED_TO_KILL_CACHED"
                "_REFS is required if CVMJITINTRINSIC_NEED_MAJOR_SPILL "
                "and/or CVMJITINTRINSIC_NEED_STACKMAP is specified\n",
                errors);
        }
    }

    flushstack = properties & CVMJITINTRINSIC_FLUSH_JAVA_STACK_FRAME;
    if (flushstack != CVMJITINTRINSIC_FLUSH_JAVA_STACK_FRAME) {
        if (irnodeFlags & CVMJITIRNODE_THROWS_EXCEPTIONS) {
            SETUP_ERROR();
            CVMconsolePrintf("Error %d: CVMJITINTRINSIC_FLUSH_JAVA_STACK_FRAME"
                " is required if CVMJITIRNODE_THROWS_EXCEPTIONS is "
                "specified\n", errors);
        }
        if ((spill == CVMJITINTRINSIC_NEED_MAJOR_SPILL) ||
            (stackmap == CVMJITINTRINSIC_NEED_STACKMAP)) {
            SETUP_ERROR();
            CVMconsolePrintf("Error %d: CVMJITINTRINSIC_FLUSH_JAVA_STACK_FRAME"
                " is required if CVMJITINTRINSIC_NEED_MAJOR_SPILL and/or "
                "CVMJITINTRINSIC_NEED_STACKMAP is specified\n", errors);
        }
    }

    /* Verify emitterOrCCMRuntimeHelper: */
    if (config->emitterOrCCMRuntimeHelper == NULL) {
        SETUP_ERROR();
        CVMconsolePrintf("Error %d: The emitterOrCCMRuntimeHelper field is "
                         "NULL\n", errors);
    } else if (cconv == CVMJITINTRINSIC_OPERATOR_ARGS) {
        const CVMJITIntrinsicEmitterVtbl *vtbl =
            (const CVMJITIntrinsicEmitterVtbl *)
                config->emitterOrCCMRuntimeHelper;
        if (vtbl->getRequired == NULL) {
            SETUP_ERROR();
            CVMconsolePrintf("Error %d: The getRequired function of the "
                             "emitter is NULL\n", errors);
        }
        if (vtbl->getArgTarget == NULL) {
            SETUP_ERROR();
            CVMconsolePrintf("Error %d: The getArgTarget function of the "
                             "emitter is NULL\n", errors);
        }
        if (vtbl->emitOperator == NULL) {
            SETUP_ERROR();
            CVMconsolePrintf("Error %d: The emitOperator function of the "
                             "emitter is NULL\n", errors);
        }
    }

    /* Check args: */
#ifndef CVMCPU_ALLOW_C_ARGS_BEYOND_MAX_ARG_REGS
    if (irec->numberOfArgsWords > CVMCPU_MAX_ARG_REGS) {
        SETUP_ERROR();
        errors--; /* Undo the error count.  A warning is not an error. */
        CVMconsolePrintf("Warning: Number of words needed for the method "
            "arguments exceed the maximum number of argument registers "
            "CVMCPU_MAX_ARG_REGS (i.e. %d)\n", CVMCPU_MAX_ARG_REGS);
    }
#endif

    return errors;
}

void
CVMJITintrinsicVerifyIntrinsicConfigs(CVMJITIntrinsic *ilist,
                                      CVMUint16 numberOfIntrinsics)
{
    int errors = 0;
    const int MAX_INTRINSICS = 255;
    int i;

    /* We currently only allow up to 255 intrinsics because of the limitations
       of data fields in binary nodes for IARG node.  We need to encode the
       intrinsic ID into one of the data fields and only have room for 8 bits.
       Since 0 is reserved, only a max of 255 intrinsics are allowed: */
    if (numberOfIntrinsics > MAX_INTRINSICS) {
        errors++;
        CVMconsolePrintf("Error %d: Too many intrinsics.  Only a maximum of "
            "%d intrinsics is allowed\n", errors, MAX_INTRINSICS);
    }

    /* Iterate over all installed intrinsics and verify them: */
    for (i = 0; i < numberOfIntrinsics; i++) {
        errors = verifyIntrinsic(&ilist[i], errors);
    }

    /* Report total errors if applicable: */
    if (errors != 0) {
        CVMconsolePrintf("A total of %d errors and warnings found in "
            "intrinsics config table\n", errors);
        CVMassert(CVM_FALSE /* Have errors in intrinsics list. */);
    }
}
#endif

/* Purpose: Initialized the JIT intrinsics manager. */
CVMBool
CVMJITintrinsicInit(CVMExecEnv *ee, CVMJITGlobalState *jgs)
{
    const CVMJITIntrinsicConfigList *clist;
    CVMJITIntrinsic *ilist;
    CVMUint16 numberOfIntrinsics = 0;
    CVMUint16 numberOfJavaIntrinsics = 0;
    CVMUint16 numberOfNativeIntrinsics = 0;
    CVMUint16 numberOfUnknownIntrinsics = 0;
    CVMUint16 i;

    /* Compute the total number of intrinsics (may include duplicates due to
       overriding by platform specific intrinsics): */
    clist = &CVMJITintrinsicsList;
    while (clist != NULL) {
        numberOfIntrinsics += clist->numberOfConfigs;
        clist = clist->parentList;
    }

    /* Allocate the intrinsics list and the sorted lists as well based on a
       worst case estimate: */
    ilist = malloc((sizeof(CVMJITIntrinsic) + sizeof(CVMJITIntrinsic *)) *
                   numberOfIntrinsics);
    if (ilist == NULL) {
#if defined(CVM_DEBUG) || defined(CVM_DEBUG_INTRINSICS)
        CVMconsolePrintf("Failed to malloc() when initializing JIT "
                         "intrinsics\n");
#endif
        return CVM_FALSE;
    }

    /* Fill in the intrinsics list: 
       NOTE: We start with the child and move backwards through to the parent.
             This way, we give the child the opportunity to define an
             intrinsic before its parent thereby giving the child the right
             to override the parent's implementation.  In this case, the
             child is the platform specific configs, and the parent could be
             the default/shared configs.
       NOTE: We can have multiple levels of overriding.  For example, the
             OS can override the default/shared configs, and the OS-CPU port
             could override the OS configs.  This is done by #define'ing
             CVMJITintrinsicsList to the most target specific config list.
    */
    numberOfIntrinsics = 0;
    clist = &CVMJITintrinsicsList;
    while (clist != NULL) {
        /* NOTE: fillInIntrinsicsFromConfigs() only fills in entries that are
                 not already in the list and returns the new updated count.
        */
        numberOfIntrinsics =
            fillInIntrinsicsFromConfigs(ee, clist, ilist, numberOfIntrinsics);
        if (numberOfIntrinsics == CVMJIT_INTRINSIC_ERROR) {
            /* Encountered an error.  Error message has already been printed.
               Just clean up and bail: */
            free (ilist);
            return CVM_FALSE;
        }
        clist = clist->parentList;
    }

#if defined(CVM_DEBUG_ASSERTS) || defined(CVM_DEBUG_INTRINSICS)
    CVMJITintrinsicVerifyIntrinsicConfigs(ilist, numberOfIntrinsics);
#endif

    CVM_COMPILEFLAGS_LOCK(ee);

    jgs->intrinsics = ilist;
    jgs->numberOfIntrinsics = numberOfIntrinsics;

    /* Compute the number of Java and Native intrinsics: */
    for (i = 0; i < numberOfIntrinsics; i++) {
        CVMMethodBlock *mb = ilist[i].mb;
        if (mb == NULL) {
            numberOfUnknownIntrinsics++;
        } else if (CVMmbIs(mb, NATIVE)) {
            numberOfNativeIntrinsics++;
        } else {
            /* If it isn't native, then it better be a Java bytecode method
               because we don't support any other kind of intrinsic methods.
               It doesn't make sense to have intrinsic abstract and miranda
               methods: */
            CVMassert(CVMmbIsJava(mb));

            /* Intrinsic methods are not inlinable (because we want to use the
               intrinsic version instead of the inlined version) and, of
               course, should be marked as intrinsic: */
            CVMmbCompileFlags(mb) |=
		(CVMJIT_NOT_INLINABLE | CVMJIT_IS_INTRINSIC);
            numberOfJavaIntrinsics++;
        }
    }
    CVMassert((numberOfJavaIntrinsics + numberOfNativeIntrinsics +
               numberOfUnknownIntrinsics) == numberOfIntrinsics);

    /* Create the sorted Java intrinsics list: */
    jgs->sortedJavaIntrinsics =
        (CVMJITIntrinsic **)&ilist[numberOfIntrinsics];
    jgs->numberOfJavaIntrinsics = numberOfJavaIntrinsics;

    /* Create the sorted Unknown intrinsics list: */
    jgs->sortedUnknownIntrinsics =
        jgs->sortedJavaIntrinsics + numberOfJavaIntrinsics;
    jgs->numberOfUnknownIntrinsics = numberOfUnknownIntrinsics;

    /* Create the sorted Native intrinsics list: */
    jgs->sortedNativeIntrinsics =
        jgs->sortedUnknownIntrinsics + numberOfUnknownIntrinsics;
    jgs->numberOfNativeIntrinsics = numberOfNativeIntrinsics;

    /* Fill in the Java and native intrinsics list: */
    numberOfUnknownIntrinsics = 0;
    numberOfNativeIntrinsics = 0;
    numberOfJavaIntrinsics = 0;
    for (i = 0; i < numberOfIntrinsics; i++) {
        CVMMethodBlock *mb = ilist[i].mb;
        if (mb == NULL) {
            jgs->sortedUnknownIntrinsics[numberOfUnknownIntrinsics++] =
                &ilist[i];
        } else if (CVMmbIs(mb, NATIVE)) {
            jgs->sortedNativeIntrinsics[numberOfNativeIntrinsics++] =
                &ilist[i];
        } else {
            CVMassert(CVMmbIsJava(mb));
            jgs->sortedJavaIntrinsics[numberOfJavaIntrinsics++] = &ilist[i];
        }
    }
    CVMassert(numberOfJavaIntrinsics == jgs->numberOfJavaIntrinsics);
    CVMassert(numberOfNativeIntrinsics == jgs->numberOfNativeIntrinsics);
    CVMassert(numberOfUnknownIntrinsics == jgs->numberOfUnknownIntrinsics);

    /* Sort the Java intrinsics list and eliminate duplicates: */
    qsort(jgs->sortedJavaIntrinsics, numberOfJavaIntrinsics,
          sizeof(CVMJITIntrinsic *), compareIntrinsic);

    /* Sort the Native intrinsics list and eliminate duplicates: */
    qsort(jgs->sortedNativeIntrinsics, numberOfNativeIntrinsics,
          sizeof(CVMJITIntrinsic *), compareIntrinsic);

    /* Sort the Native intrinsics list and eliminate duplicates: */
    qsort(jgs->sortedUnknownIntrinsics, numberOfUnknownIntrinsics,
          sizeof(CVMJITIntrinsic *), compareUnknownIntrinsic);

    /* Test code to dump the initial sorted lists: */
#ifdef CVM_DEBUG_INTRINSICS
    {
        int i;
        CVMconsolePrintf("Intrinsics:\n");
        for (i = 0; i < numberOfJavaIntrinsics; i++) {
	    CVMJITIntrinsic *irec = jgs->sortedJavaIntrinsics[i];
            CVMMethodBlock *mb = irec->mb;
#ifdef CVM_DEBUG
            CVMconsolePrintf(" java %d: irecID %d : 0x%08x %C.%M from %s\n",
                i, irec->intrinsicID, mb, CVMmbClassBlock(mb), mb,
                irec->configList->name);
#else
            CVMconsolePrintf(" java %d: irecID %d : 0x%08x %C.%M\n",
                i, irec->intrinsicID, mb, CVMmbClassBlock(mb), mb);
#endif
        }
        for (i = 0; i < numberOfNativeIntrinsics; i++) {
            CVMJITIntrinsic *irec = jgs->sortedNativeIntrinsics[i];
            CVMMethodBlock *mb = irec->mb;
#ifdef CVM_DEBUG
            CVMconsolePrintf(" native %d: irecID %d : 0x%08x %C.%M from %s\n",
                i, irec->intrinsicID, mb, CVMmbClassBlock(mb), mb,
                irec->configList->name);
#else
            CVMconsolePrintf(" native %d: irecID %d : 0x%08x %C.%M\n",
                i, irec->intrinsicID, mb, CVMmbClassBlock(mb), mb);
#endif
        }
        for (i = 0; i < numberOfUnknownIntrinsics; i++) {
            CVMJITIntrinsic *irec = jgs->sortedUnknownIntrinsics[i];
#ifdef CVM_DEBUG
            CVMconsolePrintf(" unknown %d: irecID %d : %!C.%!M from %s\n", i,
                irec->intrinsicID, irec->classTypeID, irec->methodTypeID,
                irec->configList->name);
#else
            CVMconsolePrintf(" unknown %d: irecID %d : %!C.%!M\n", i,
                irec->intrinsicID, irec->classTypeID, irec->methodTypeID);
#endif
        }
    }
#endif /* CVM_DEBUG_INTRINSICS */

    CVM_COMPILEFLAGS_UNLOCK(ee);

    return CVM_TRUE;
}

/* Purpose: Destroys the JIT intrinsics manager. */
void
CVMJITintrinsicDestroy(CVMJITGlobalState *jgs)
{
    free(jgs->intrinsics);
#ifdef CVM_DEBUG_ASSERTS
    jgs->intrinsics = NULL;
    jgs->numberOfIntrinsics = 0;
    jgs->sortedJavaIntrinsics = NULL;
    jgs->numberOfJavaIntrinsics = 0;
    jgs->sortedNativeIntrinsics = NULL;
    jgs->numberOfNativeIntrinsics = 0;
    jgs->sortedUnknownIntrinsics = NULL;
    jgs->numberOfUnknownIntrinsics = 0;
#endif
}

/* Purpose: Gets the intrinsic record for the specified method if available.*/
CVMJITIntrinsic *
CVMJITintrinsicGetIntrinsicRecord(CVMExecEnv *ee, CVMMethodBlock *mb)
{
    CVMJITIntrinsic *irec = NULL;
    CVMJITIntrinsic **ilist;
    CVMUint16 numberOfIntrinsics;
    CVMInt32 start, end, current;
    CVMBool needLock = (CVMglobals.jit.numberOfUnknownIntrinsics != 0);

    /* NOTE: If the numberOfUnknownIntrinsics is 0, then there is no need to
       acquire the lock because the intrinsics list won't be changing anymore.
       Otherwise, we have to prevent the list from changing while we're
       searching the list. */

    /* Decide which list to use: */
    if (CVMmbIs(mb, NATIVE)) {
        if (needLock) {
            CVM_COMPILEFLAGS_LOCK(ee);
        }
        ilist = CVMglobals.jit.sortedNativeIntrinsics;
        numberOfIntrinsics = CVMglobals.jit.numberOfNativeIntrinsics;

    } else if (CVMmbIsJava(mb)) {
        if ((CVMmbCompileFlags(mb) & CVMJIT_IS_INTRINSIC) == 0) {
            /* Java intrinsics must have been marked with CVMJIT_IS_INTRINSIC.
               Otherwise, it is not an intrinsic: */
            return NULL;
        }

        if (needLock) {
            CVM_COMPILEFLAGS_LOCK(ee);
        }
        ilist = CVMglobals.jit.sortedJavaIntrinsics;
        numberOfIntrinsics = CVMglobals.jit.numberOfJavaIntrinsics;

    } else {
        return NULL;
    }

    /* Do a binary search to see if we can find a matching entry in the
       list: */
    start = 0;
    end = numberOfIntrinsics - 1;
    while (start <= end) {
        CVMJITIntrinsic *ip;
        current = start + ((end - start) >> 1); /* start + (end-start)/2. */
        ip = ilist[current];
        if ((CVMInt32)mb == (CVMInt32)ip->mb) {
            irec = ip; /* Found it. */
            break;
        } else if ((CVMInt32)mb < (CVMInt32)ip->mb) {
            end = current - 1;   /* Search lower half region. */
        } else {
            start = current + 1; /* Search upper half region. */
        }
    }
    if (needLock) {
        CVM_COMPILEFLAGS_UNLOCK(ee);
    }
    return irec;
}

/* Purpose: Scans the newly loaded class for intrinsic methods.  If intrinsic
            methods are found in the specified class, then those methods will
            be added to the intrinsics list. */
void
CVMJITintrinsicScanForIntrinsicMethods(CVMExecEnv *ee, CVMClassBlock *cb)
{
    CVMJITGlobalState *jgs;
    CVMClassTypeID classTypeID;
    CVMInt32 start, end, current, first, last, i, numberOfIntrinsics;
    CVMInt32 numberOfUnknownIntrinsics;
    CVMJITIntrinsic **ilist;
    CVMJITIntrinsic *irec;
    CVMBool touchedJavaList = CVM_FALSE;
    CVMBool touchedNativeList = CVM_FALSE;

    /* If this class is not loaded by the bootclassloader, then we know that
       it doesn't have an intrinsics because we only support intrinsics for
       methods from classes loaded from the bootclasspath. */
    if (CVMcbClassLoader(cb) != NULL) {
        return;
    }

    jgs = &CVMglobals.jit;

    /* If we have no more unknown intrinsics, then we're done: */
    if (jgs->numberOfUnknownIntrinsics == 0) {
        return;
    }

    /* Acquire the lock to keep other threads from changing the list, or
       searching it while we're changing it: */
    CVM_COMPILEFLAGS_LOCK(ee);

    numberOfUnknownIntrinsics = jgs->numberOfUnknownIntrinsics;
    classTypeID = CVMcbClassName(cb);
    /* Do a binary search to see if the class is in the unknown list: */
    irec = NULL;
    start = 0;
    end = numberOfUnknownIntrinsics - 1;
    current = 0;
    ilist = jgs->sortedUnknownIntrinsics;
    while (start <= end) {
        CVMJITIntrinsic *ip;
        current = start + ((end - start) >> 1); /* start + (end-start)/2. */
        ip = ilist[current];
        if ((CVMInt32)classTypeID == (CVMInt32)ip->classTypeID) {
            irec = ip; /* Found it. */
            break;
        } else if ((CVMInt32)classTypeID < (CVMInt32)ip->classTypeID) {
            end = current - 1;   /* Search lower half region. */
        } else {
            start = current + 1; /* Search upper half region. */
        }
    }

    /* If this class is not an intrinsic class, then we're done: */
    if (irec == NULL) {
        CVM_COMPILEFLAGS_UNLOCK(ee);
        return;
    }

    /* Find the block of intrinsics that correspond to this class: */
    end = numberOfUnknownIntrinsics - 1;
    first = current;
    last = current;

    /* Find the first intrinsic method for this class: */
    current--;
    while (current >= 0) {
        CVMJITIntrinsic *ip;
        ip = ilist[current];
        if (classTypeID == ip->classTypeID) {
            first = current;
        }
        current--;
    }

    /* Find the last intrinsic method for this class: */
    current = last + 1;
    while (current <= end) {
        CVMJITIntrinsic *ip;
        ip = ilist[current];
        if (classTypeID == ip->classTypeID) {
            last = current;
        }
        current++;
    }

    /* Iterate over all methods and check if they ae intrinsic: */
    numberOfIntrinsics = last - start + 1;
    for (i = 0; (i < CVMcbMethodCount(cb)) && (numberOfIntrinsics != 0); i++){
        CVMMethodBlock *mb = CVMcbMethodSlot(cb, i);
        CVMMethodTypeID methodTypeID = CVMmbNameAndTypeID(mb);

        /* Check if method is intrinsic: */
        irec = NULL;
        start = first;
        end = last;
        while (start <= end) {
            CVMJITIntrinsic *ip;
            current = start + ((end - start) >> 1);
            ip = ilist[current];
            if ((CVMInt32)methodTypeID == (CVMInt32)ip->methodTypeID) {
                irec = ip; /* Found it. */
                break;
            } else if ((CVMInt32)methodTypeID < (CVMInt32)ip->methodTypeID) {
                end = current - 1;   /* Search lower half region. */
            } else {
                start = current + 1; /* Search upper half region. */
            }
        }

        /* If this method is an intrinsic, then set the mb: */
        if (irec != NULL) {
            irec->mb = mb;
            CVMassert(CVMmbArgsSize(mb) == irec->numberOfArgsWords);
            numberOfIntrinsics--;
        }
    }

    ilist = jgs->sortedUnknownIntrinsics;
    numberOfIntrinsics = last - start + 1;
    for (i = start; numberOfIntrinsics > 0; i++) {
        CVMJITIntrinsic *ip = ilist[i];
        CVMMethodBlock *mb = ip->mb;
        if (mb != NULL) {
            if (CVMmbIs(mb, NATIVE)) {
                /* Move from the unknown list to the head of the native list:*/
                ilist[i] = jgs->sortedNativeIntrinsics[-1];
                jgs->sortedNativeIntrinsics[-1] = ip;
                jgs->sortedNativeIntrinsics--;
                jgs->numberOfNativeIntrinsics++;

                /* Do the current entry again because the current entry now
                   contains one that has not been scanned yet: */
                i--;
                touchedNativeList = CVM_TRUE;

            } else {
                /* If it isn't native, then it better be a Java bytecode method
                   because we don't support any other kind of intrinsic methods.
                   It doesn't make sense to have intrinsic abstract and miranda
                   methods: */
                CVMassert(CVMmbIsJava(mb));

                /* Intrinsic methods are not inlinable (because we want to use the
                   intrinsic version instead of the inlined version) and, of
                   course, should be marked as intrinsic: */
                CVMmbCompileFlags(mb) |=
                    (CVMJIT_NOT_INLINABLE | CVMJIT_IS_INTRINSIC);

                /* Move from the unknown list to the tail of the java list: */
                ilist[i] = jgs->sortedUnknownIntrinsics[0];
                jgs->sortedJavaIntrinsics[jgs->numberOfJavaIntrinsics] = ip;
                jgs->numberOfJavaIntrinsics++;
                jgs->sortedUnknownIntrinsics++;
                touchedJavaList = CVM_TRUE;
            }
            numberOfUnknownIntrinsics--;
            numberOfIntrinsics--;
        }
    }

    /* Sort the Java intrinsics list and eliminate duplicates: */
    if (touchedJavaList) {
        qsort(jgs->sortedJavaIntrinsics, jgs->numberOfJavaIntrinsics,
              sizeof(CVMJITIntrinsic *), compareIntrinsic);
    }

    /* Sort the Native intrinsics list and eliminate duplicates: */
    if (touchedJavaList) {
        qsort(jgs->sortedNativeIntrinsics, jgs->numberOfNativeIntrinsics,
              sizeof(CVMJITIntrinsic *), compareIntrinsic);
    }

    /* Sort the Unknown intrinsics list and eliminate duplicates: */
    if (numberOfUnknownIntrinsics > 1) {
        qsort(jgs->sortedUnknownIntrinsics, numberOfUnknownIntrinsics,
              sizeof(CVMJITIntrinsic *), compareUnknownIntrinsic);
    }

    /* We update jgs->numberOfUnknownIntrinsics last because this value can
       be used by another thread to determine if the locks need to be
       acquired or not before searching the intrinsics list.  Hence, we
       must be completely done sorting the intrinsics lists before we
       uodate the numberOfUnknownIntrinsics in CVMglobals.jit: */
    jgs->numberOfUnknownIntrinsics = numberOfUnknownIntrinsics;
    CVM_COMPILEFLAGS_UNLOCK(ee);
}

#endif /* CVMJIT_INTRINSICS */

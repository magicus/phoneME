/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "VirtualMachineImpl.h"
#include "commonRef.h"
#include "inStream.h"
#include "outStream.h"
#include "util.h"
#include "eventHandler.h"
#include "eventHelper.h"
#include "threadControl.h"
#include "JDWP.h"
#include "version.h"
#include "SDE.h"

static char *versionName = "Java Debug Wire Protocol";
static int majorVersion = 1;  /* JDWP major version */
static int minorVersion = 4;  /* JDWP minor version */

/* retrieved on init - defaults in-case of failure to init */
static char *classpath_property = "";
static char *bootclasspath_property = "";
static char path_separator_property = ';';
static char *user_dir_property = "";
static char *vm_info_property = "";
    
static jboolean 
version(PacketInputStream *in, PacketOutputStream *out)
{
    char buf[500];
    char *vmName;
    char *vmVersion;
    
    if (vmDead) {
        outStream_setError(out, JDWP_Error_VM_DEAD);                    
        return JNI_TRUE;
    }

    vmVersion = version_vmVersion();
    if (vmVersion == NULL) {
        vmVersion = "<unknown>";
    }
    vmName = version_vmName();
    if (vmName == NULL) {
        vmName = "<unknown>";
    }

    /*
     * Write the descriptive version information
     */
    sprintf(buf, "%s version %d.%d\nJVM Debug Interface version %d.%d\n"
                 "JVM version %s (%s, %s)",
                  versionName, majorVersion, minorVersion, 
                  jvmdiMajorVersion(), jvmdiMinorVersion(),
                  vmVersion, vmName, vm_info_property);
    outStream_writeString(out, buf);

    /*
     * Write the JDWP version numbers
     */
    outStream_writeInt(out, majorVersion);
    outStream_writeInt(out, minorVersion);

    /*
     * Write the VM version and name
     */
    outStream_writeString(out, vmVersion);
    outStream_writeString(out, vmName);

    return JNI_TRUE;
}

static jboolean 
classesForSignature(PacketInputStream *in, PacketOutputStream *out) 
{
    JNIEnv *env;
    jint classCount;
    jint i;
    jclass *theClasses;
    char *signature;
    char *candidate;

    if (vmDead) {
        outStream_setError(out, JDWP_Error_VM_DEAD);                    
        return JNI_TRUE;
    }

    env = getEnv();
    signature = inStream_readString(in);
    if (signature == NULL) {
        outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
        return JNI_TRUE;
    }
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    theClasses = allLoadedClasses(&classCount);
    if (theClasses == NULL) {
        outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
    } else {
        /* Count classes in theClasses which match signature */
        int matchCount = 0;
        /* Count classes written to the JDWP connection */
        int writtenCount = 0;

        for (i=0; i<classCount; i++) {
            jclass clazz = theClasses[i];
            jint status = classStatus(clazz);

            /* Filter out unprepared classes (arrays may or
             * may not be marked as prepared) */
            if (((status & JVMDI_CLASS_STATUS_PREPARED) == 0)
                      && !isArrayClass(clazz)) {
                continue;
            }
            candidate = classSignature(clazz);
            if (candidate == NULL) {
                freeGlobalRefs(theClasses, classCount);
                jdwpFree(signature);
                outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
                return JNI_TRUE;
            }
            if (strcmp(candidate, signature) == 0) {
                /* Float interesting classes (those that
                 * are matching and are prepared) to the
                 * beginning of the array. 
                 * Do a swap so that uninteresting classes
                 * (at the end of the array) can properly
                 * have their global references deleted.
                 */
                theClasses[i] = theClasses[matchCount];
                theClasses[matchCount++] = clazz;
            }
            jdwpFree(candidate);
        }
        /* At this point matching prepared classes occupy
         * indicies 0 thru matchCount-1 of theClasses.
         * Indicies matchCount thru classCount-1 of 
         * theClasses will not be written but remain there
         * so they can be deleted by deleteRefArray() below.
         */

        outStream_writeInt(out, matchCount);
        for (; writtenCount < matchCount; writtenCount++) {
            jclass clazz = theClasses[writtenCount];
            jint status = classStatus(clazz);
            jbyte tag = referenceTypeTag(clazz);
            outStream_writeByte(out, tag);
            WRITE_GLOBAL_REF(env, out, clazz);
            outStream_writeInt(out, status);

            /* No point in continuing if there's an error */
            if (outStream_error(out)) {
                break;
            }
        }
        /* All written classes have their global ref
         * freed in the WRITE_GLOBAL_REF; The rest,
         * there because of failure termination and/or
         * because they do not match or are not prepared,
         * must be freed.
         */
        freeGlobalRefsPartial(theClasses, writtenCount, classCount);
    }
    jdwpFree(signature);
    return JNI_TRUE;
}

static jboolean 
allClasses(PacketInputStream *in, PacketOutputStream *out)
{
    JNIEnv *env;
    jint classCount;
    jint i;
    jclass *theClasses;

    if (vmDead) {
        outStream_setError(out, JDWP_Error_VM_DEAD);                    
        return JNI_TRUE;
    }

    env = getEnv();
    theClasses = allLoadedClasses(&classCount);
    if (theClasses == NULL) {
        outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
    } else {
        /* Count classes in theClasses which are prepared */
        int prepCount = 0;
        /* Count classes written to the JDWP connection */
        int writtenCount = 0;  

        for (i=0; i<classCount; i++) {
            jclass clazz = theClasses[i];
            jint status = classStatus(clazz);

            /* Filter out unprepared classes (arrays may or
             * may not be marked as prepared) */
            if (((status & JVMDI_CLASS_STATUS_PREPARED) != 0)
                      || isArrayClass(clazz)) {
                /* Float interesting classes (those that
                 * are prepared) to the beginning of the array. 
                 * Do a swap so that uninteresting classes
                 * (at the end of the array) can properly
                 * have their global references deleted.
                 */
                theClasses[i] = theClasses[prepCount];
                theClasses[prepCount++] = clazz;
            }
        }
        /* At this point prepared classes occupy
         * indicies 0 thru prepCount-1 of theClasses.
         * Indicies prepCount thru classCount-1 of 
         * theClasses will not be written but remain there
         * so they can be deleted by deleteRefArray() below.
         */

        outStream_writeInt(out, prepCount);
        for (; writtenCount < prepCount; writtenCount++) {
            char *signature;
            jclass clazz = theClasses[writtenCount];
            jint status = classStatus(clazz);
            jbyte tag = referenceTypeTag(clazz);

            signature = classSignature(clazz);
            if (signature == NULL) {
                outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
                break;
            }

            outStream_writeByte(out, tag);
            WRITE_GLOBAL_REF(env, out, clazz);
            outStream_writeString(out, signature);
            outStream_writeInt(out, status);
            jdwpFree(signature);

            /* No point in continuing if there's an error */
            if (outStream_error(out)) {
                break;
            }
        }
        /* All written classes have their global ref
         * freed in the WRITE_GLOBAL_REF; The rest,
         * there because of failure termination and/or
         * because they are not prepared, must be freed.
         */
        freeGlobalRefsPartial(theClasses, writtenCount, classCount);
    }
    return JNI_TRUE;
}

static jboolean 
redefineClasses(PacketInputStream *in, PacketOutputStream *out) 
{
    JNIEnv *env;
    JVMDI_class_definition *classDefs;
    JVMDI_class_definition *currClassDef;
    jboolean ok = JNI_TRUE;
    int classCount;
    int i;

    if (vmDead) {
        /* quietly ignore */                
        return JNI_TRUE;
    }

    env = getEnv();
    classCount = inStream_readInt(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }
    classDefs = jdwpAlloc(classCount * 
                          sizeof(JVMDI_class_definition));
    if (classDefs == NULL) {
        outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
        return JNI_TRUE;
    }
   
    for (i = 0, currClassDef = classDefs; 
                   i < classCount; 
                   ++i, ++currClassDef) {
        int byteCount;
        jbyte *bytes;
        jclass clazz = inStream_readClassRef(in);

        if (inStream_error(in)) {
            ok = JNI_FALSE;
            break;
        }
        byteCount = inStream_readInt(in);
        if (inStream_error(in)) {
            ok = JNI_FALSE;
            break;
        }
        bytes = jdwpAlloc(byteCount);
        if (bytes == NULL) {
            outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
            ok = JNI_FALSE;
            break;
        }        
        inStream_readBytes(in, byteCount, bytes);
        if (inStream_error(in)) {
            ok = JNI_FALSE;
            break;
        }

        currClassDef->clazz = clazz;
        currClassDef->class_byte_count = byteCount;
        currClassDef->class_bytes = bytes;
    }
    
    if (ok == JNI_TRUE) {
        jint error;

        error = jvmdi->RedefineClasses(classCount, classDefs);
        if (error != JVMDI_ERROR_NONE) {
            outStream_setError(out, error);
            ok = JNI_FALSE;
        } else {
            /* zap our BP info */
            for (i = 0, currClassDef = classDefs; i < classCount; 
                                               ++i, ++currClassDef) {
                eventHandler_freeClassBreakpoints(currClassDef->clazz);
            }
        }
    }

    /* free up allocated memory */
    for (--currClassDef; currClassDef >= classDefs; 
         --currClassDef) {
        jdwpFree(currClassDef->class_bytes);
    }
    jdwpFree(classDefs);

    return JNI_TRUE;
}

static jboolean
setDefaultStratum(PacketInputStream *in, PacketOutputStream *out)
{
    char *stratumId;

    if (vmDead) {
        /* quietly ignore */                
        return JNI_TRUE;
    }

    stratumId = inStream_readString(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    } else if (strcmp(stratumId, "") == 0) {
        stratumId = NULL;
    }
    setGlobalStratumId(stratumId);

    return JNI_TRUE;
}

static jboolean 
getAllThreads(PacketInputStream *in, PacketOutputStream *out)
{
    JNIEnv *env;
    jint threadCount;
    jint i;
    jthread *theThreads;

    if (vmDead) {
        outStream_setError(out, JDWP_Error_VM_DEAD);                    
        return JNI_TRUE;
    }

    env = getEnv();
    theThreads = allThreads(&threadCount);
    if (theThreads == NULL) {
        outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
    } else {
        /* Squish out all of the debugger-spawned threads */
        threadCount = filterDebugThreads(theThreads, threadCount);
        
        outStream_writeInt(out, threadCount);
        for (i = 0; i <threadCount; i++) {
            WRITE_GLOBAL_REF(env, out, theThreads[i]);
        }

        jdwpFree(theThreads);
    } 
    return JNI_TRUE;
}

static jboolean 
topLevelThreadGroups(PacketInputStream *in, PacketOutputStream *out)
{
    jint groupCount;
    jthreadGroup *groups;

    if (vmDead) {
        outStream_setError(out, JDWP_Error_VM_DEAD);                    
        return JNI_TRUE;
    }

    groups = topThreadGroups(&groupCount);
    if (groups == NULL) {
        outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
    } else {
        JNIEnv *env = getEnv();
        int i;

        outStream_writeInt(out, groupCount);
        for (i = 0; i < groupCount; i++) {
            WRITE_GLOBAL_REF(env, out, groups[i]);
        }

        jdwpFree(groups);
    }
    return JNI_TRUE;
}

static jboolean 
dispose(PacketInputStream *in, PacketOutputStream *out)
{
    return JNI_TRUE;
}

static jboolean 
idSizes(PacketInputStream *in, PacketOutputStream *out)
{
    outStream_writeInt(out, sizeof(jfieldID));    /* fields */
    outStream_writeInt(out, sizeof(jmethodID));   /* methods */
    outStream_writeInt(out, sizeof(jlong));       /* objects */
    outStream_writeInt(out, sizeof(jlong));       /* referent types */
    outStream_writeInt(out, sizeof(jframeID));    /* frames */
    return JNI_TRUE;
}

static jboolean 
suspend(PacketInputStream *in, PacketOutputStream *out)
{
    jint error = vmDead? JDWP_Error_VM_DEAD : threadControl_suspendAll();

    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
    }
    return JNI_TRUE;
}

static jboolean 
resume(PacketInputStream *in, PacketOutputStream *out)
{
    jint error = vmDead? JDWP_Error_VM_DEAD : threadControl_resumeAll();
    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
    }
    return JNI_TRUE;
}

static jboolean 
doExit(PacketInputStream *in, PacketOutputStream *out)
{
    jint exitCode = inStream_readInt(in);
    
    if (vmDead) {
        /* quietly ignore */                
        return JNI_FALSE;
    }

    /* We send the reply from here because we are about to exit. */
    if (inStream_error(in)) {
        outStream_setError(out, inStream_error(in));
    } 
    outStream_sendReply(out);

    /*
     * %comment gordonh005
     */
    exit(exitCode);

    /* Shouldn't get here */
    JDI_ASSERT(JNI_FALSE);

    /* Shut up the compiler */
    return JNI_FALSE;

}

static jboolean 
createString(PacketInputStream *in, PacketOutputStream *out)
{
    JNIEnv *env;
    jstring string;
    char *cstring;

    if (vmDead) {
        outStream_setError(out, JDWP_Error_VM_DEAD);                    
        return JNI_TRUE;
    }

    env = getEnv();
    cstring = inStream_readString(in);
    if (cstring == NULL) {
        outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
        return JNI_TRUE;
    }
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    WITH_LOCAL_REFS(env, 1);

    string = (*env)->NewStringUTF(env, cstring);
    if ((*env)->ExceptionOccurred(env)) {
        outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
    } else {
        WRITE_LOCAL_REF(env, out, string);
    } 
    END_WITH_LOCAL_REFS(env);

    jdwpFree(cstring);
    return JNI_TRUE;
}

static jboolean 
capabilities(PacketInputStream *in, PacketOutputStream *out)
{
    JVMDI_capabilities caps;
    jint error = vmDead? JDWP_Error_VM_DEAD : jvmdi->GetCapabilities(&caps);

    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
        return JNI_TRUE;
    }

    outStream_writeBoolean(out, (jboolean)caps.can_watch_field_modification);
    outStream_writeBoolean(out, (jboolean)caps.can_watch_field_access);
    outStream_writeBoolean(out, (jboolean)caps.can_get_bytecodes);
    outStream_writeBoolean(out, (jboolean)caps.can_get_synthetic_attribute);
    outStream_writeBoolean(out, (jboolean)caps.can_get_owned_monitor_info);
    outStream_writeBoolean(out, (jboolean)caps.can_get_current_contended_monitor);
    outStream_writeBoolean(out, (jboolean)caps.can_get_monitor_info);
    return JNI_TRUE;
}

static jboolean 
capabilitiesNew(PacketInputStream *in, PacketOutputStream *out)
{
    JVMDI_capabilities caps;
    jint error = vmDead? JDWP_Error_VM_DEAD : jvmdi->GetCapabilities(&caps);

    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
        return JNI_TRUE;
    }

    outStream_writeBoolean(out, (jboolean)caps.can_watch_field_modification);
    outStream_writeBoolean(out, (jboolean)caps.can_watch_field_access);
    outStream_writeBoolean(out, (jboolean)caps.can_get_bytecodes);
    outStream_writeBoolean(out, (jboolean)caps.can_get_synthetic_attribute);
    outStream_writeBoolean(out, (jboolean)caps.can_get_owned_monitor_info);
    outStream_writeBoolean(out, (jboolean)caps.can_get_current_contended_monitor);
    outStream_writeBoolean(out, (jboolean)caps.can_get_monitor_info);

    /* new since JDWP version 1.4 */
    outStream_writeBoolean(out, (jboolean)caps.can_redefine_classes);
    outStream_writeBoolean(out, (jboolean)caps.can_add_method);
    outStream_writeBoolean(out, (jboolean)caps.can_unrestrictedly_redefine_classes);
    /* 11: canPopFrames */
    outStream_writeBoolean(out, (jboolean)caps.can_pop_frame);
    /* 12: canUseInstanceFilters */
    outStream_writeBoolean(out, (jboolean)JNI_TRUE);
    /* 13: canGetSourceDebugExtension */
    outStream_writeBoolean(out, canGetSourceDebugExtension());
    /* 14: canRequestVMDeathEvent */
    outStream_writeBoolean(out, (jboolean)JNI_TRUE);
    /* 15: canSetDefaultStratum */
    outStream_writeBoolean(out, (jboolean)JNI_TRUE);

    /* remaining reserved */
    outStream_writeBoolean(out, (jboolean)JNI_FALSE); /* 16 */
    outStream_writeBoolean(out, (jboolean)JNI_FALSE); /* 17 */
    outStream_writeBoolean(out, (jboolean)JNI_FALSE); /* 18 */
    outStream_writeBoolean(out, (jboolean)JNI_FALSE); /* 19 */
    outStream_writeBoolean(out, (jboolean)JNI_FALSE); /* 20 */
    outStream_writeBoolean(out, (jboolean)JNI_FALSE); /* 21 */
    outStream_writeBoolean(out, (jboolean)JNI_FALSE); /* 22 */
    outStream_writeBoolean(out, (jboolean)JNI_FALSE); /* 23 */
    outStream_writeBoolean(out, (jboolean)JNI_FALSE); /* 24 */
    outStream_writeBoolean(out, (jboolean)JNI_FALSE); /* 25 */
    outStream_writeBoolean(out, (jboolean)JNI_FALSE); /* 26 */
    outStream_writeBoolean(out, (jboolean)JNI_FALSE); /* 27 */
    outStream_writeBoolean(out, (jboolean)JNI_FALSE); /* 28 */
    outStream_writeBoolean(out, (jboolean)JNI_FALSE); /* 29 */
    outStream_writeBoolean(out, (jboolean)JNI_FALSE); /* 30 */
    outStream_writeBoolean(out, (jboolean)JNI_FALSE); /* 31 */
    outStream_writeBoolean(out, (jboolean)JNI_FALSE); /* 32 */
    return JNI_TRUE;
}

#define NULL_CHECK(e) if (((e) == 0) || (*env)->ExceptionOccurred(env)) return

/*
 * Set classpath, bootclasspath and path separator properties.
 * Called when back-end is initialized.
 */
void
VirtualMachine_initialize(void)
{
    char *value;

    value = getPropertyCString("java.class.path");
    if (value != NULL) {
        classpath_property = value;
    }

    value = getPropertyCString("sun.boot.class.path");
    if (value != NULL) {
        bootclasspath_property = value;
    }

    value = getPropertyCString("path.separator");
    if (value != NULL) {
        path_separator_property = value[0];
        jdwpFree(value);
    }

    value = getPropertyCString("user.dir");
    if (value != NULL) {
        user_dir_property = value;
    }

    value = getPropertyCString("java.vm.info");
    if (value != NULL) {
        vm_info_property = value;
    }
}

void
VirtualMachine_reset(void)
{
}

static int 
countPaths(char *string) {
    int cnt = 1; /* always have one */
    char *pos = string;

    while ((pos = strchr(pos, path_separator_property)) != NULL) {
        ++cnt;
        ++pos;
    }
    return cnt;
}

static void 
writePaths(PacketOutputStream *out, char *string) {
    char *pos = string;
    char *newPos;

    outStream_writeInt(out, countPaths(string));

    while ((newPos = strchr(pos, path_separator_property)) != NULL) {
        outStream_writeByteArray(out, newPos-pos, (jbyte *)pos);
        pos = newPos + 1;
    }
    outStream_writeByteArray(out, strlen(pos), (jbyte *)pos);
}



static jboolean 
classPaths(PacketInputStream *in, PacketOutputStream *out)
{
    outStream_writeString(out, user_dir_property);
    writePaths(out, classpath_property);
    writePaths(out, bootclasspath_property);
    return JNI_TRUE;
}

static jboolean 
disposeObjects(PacketInputStream *in, PacketOutputStream *out)
{
    int i;
    int refCount;
    jlong id;
    int requestCount;

    if (vmDead) {
        /* quietly ignore */                
        return JNI_TRUE;
    }

    requestCount = inStream_readInt(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    for (i = 0; i < requestCount; i++) {
        id = inStream_readObjectID(in);
        refCount = inStream_readInt(in);
        if (inStream_error(in)) {
            return JNI_TRUE;
        }
        commonRef_releaseMultiple(id, refCount);
    }

    return JNI_TRUE;
}

static jboolean 
holdEvents(PacketInputStream *in, PacketOutputStream *out)
{
    eventHelper_holdEvents();
    return JNI_TRUE;
}

static jboolean 
releaseEvents(PacketInputStream *in, PacketOutputStream *out)
{
    eventHelper_releaseEvents();
    return JNI_TRUE;
}

void *VirtualMachine_Cmds[] = { (void *)19
    ,(void *)version
    ,(void *)classesForSignature
    ,(void *)allClasses
    ,(void *)getAllThreads
    ,(void *)topLevelThreadGroups
    ,(void *)dispose
    ,(void *)idSizes
    ,(void *)suspend
    ,(void *)resume
    ,(void *)doExit
    ,(void *)createString
    ,(void *)capabilities
    ,(void *)classPaths
    ,(void *)disposeObjects
    ,(void *)holdEvents
    ,(void *)releaseEvents
    ,(void *)capabilitiesNew
    ,(void *)redefineClasses
    ,(void *)setDefaultStratum
};


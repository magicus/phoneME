/*
 * @(#)MethodImpl.c	1.32 06/10/10
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
#include <stdlib.h>
#include <string.h>

#include "MethodImpl.h"
#include "util.h"
#include "inStream.h"
#include "outStream.h"
#include "JDWP.h"

static jboolean 
lineTable(PacketInputStream *in, PacketOutputStream *out)
{
    jint error;
    jint count;
    JVMDI_line_number_entry *table;
    jmethodID method;
    jlocation firstCodeIndex;
    jlocation lastCodeIndex;
    jboolean isNative;

    jclass clazz = inStream_readClassRef(in);
    method = inStream_readMethodID(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    /*
     * JVMDI behavior for the calls below is unspecified for native 
     * methods, so we must check explicitly.
     */
    error = jvmdi->IsMethodNative(clazz, method, &isNative);
    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
        return JNI_TRUE;
    }

    if (isNative) {
        outStream_setError(out, JDWP_ERROR(NATIVE_METHOD));
        return JNI_TRUE;
    }

    error = jvmdi->GetMethodLocation(clazz, method, &firstCodeIndex,
                                     &lastCodeIndex);
    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
        return JNI_TRUE;
    }
    outStream_writeLocation(out, firstCodeIndex);
    outStream_writeLocation(out, lastCodeIndex);

    error = jvmdi->GetLineNumberTable(clazz, method, &count, &table);
    if (error == JVMDI_ERROR_ABSENT_INFORMATION) {
        /* 
         * Indicate no line info with an empty table. The code indices
         * are still useful, so we don't want to return an error
         */
        outStream_writeInt(out, 0);
    } else if (error == JVMDI_ERROR_NONE) {
        jint i;
        outStream_writeInt(out, count);
        for (i = 0; (i < count) && !outStream_error(out); i++) {
            outStream_writeLocation(out, table[i].start_location);
            outStream_writeInt(out, table[i].line_number);
        }
        jdwpFree(table);
    } else {
        outStream_setError(out, error);
    }
    return JNI_TRUE;
}

static jboolean 
variableTable(PacketInputStream *in, PacketOutputStream *out)
{
    jint error;
    jint count;
    JVMDI_local_variable_entry *table;
    jmethodID method;
    jint argsSize;
    jboolean isNative;

    jclass clazz = inStream_readClassRef(in);
    method = inStream_readMethodID(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    /*
     * JVMDI behavior for the calls below is unspecified for native 
     * methods, so we must check explicitly.
     */
    error = jvmdi->IsMethodNative(clazz, method, &isNative);
    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
        return JNI_TRUE;
    }

    if (isNative) {
        outStream_setError(out, JDWP_ERROR(NATIVE_METHOD));
        return JNI_TRUE;
    }

    error = jvmdi->GetArgumentsSize(clazz, method, &argsSize);
    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
        return JNI_TRUE;
    }

    error = jvmdi->GetLocalVariableTable(clazz, method, &count, &table);
    if (error == JVMDI_ERROR_NONE) {
        jint i;
        outStream_writeInt(out, argsSize);
        outStream_writeInt(out, count);
        for (i = 0; (i < count) && !outStream_error(out); i++) {
            JVMDI_local_variable_entry *entry = &table[i];
            outStream_writeLocation(out, entry->start_location);
            outStream_writeString(out, entry->name);
            outStream_writeString(out, entry->signature);
            outStream_writeInt(out, entry->length);
            outStream_writeInt(out, entry->slot);

            jdwpFree(entry->name);
            jdwpFree(entry->signature);
        }

        jdwpFree(table);
    } else {
        outStream_setError(out, error);
    }
    return JNI_TRUE;
}

static jboolean 
bytecodes(PacketInputStream *in, PacketOutputStream *out)
{
    jint error;
    jbyte *bytecodes;
    jint bytecodeCount;
    jclass clazz = inStream_readClassRef(in);
    jmethodID method = inStream_readMethodID(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    error = jvmdi->GetBytecodes(clazz, method, &bytecodeCount, &bytecodes);
    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
    } else {
        outStream_writeByteArray(out, bytecodeCount, bytecodes);
        jdwpFree(bytecodes);
    }
    
    return JNI_TRUE;
}

static jboolean 
isObsolete(PacketInputStream *in, PacketOutputStream *out)
{
    jint error;
    jboolean isObsolete;
    jclass clazz = inStream_readClassRef(in);
    jmethodID method = inStream_readMethodID(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    error = jvmdi->IsMethodObsolete(clazz, method, &isObsolete);
    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
    } else {
        outStream_writeBoolean(out, isObsolete);
    }
    
    return JNI_TRUE;
}

void *Method_Cmds[] = { (void *)0x4
    ,(void *)lineTable
    ,(void *)variableTable
    ,(void *)bytecodes
    ,(void *)isObsolete
};

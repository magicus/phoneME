/*
 * @(#)ClassLoaderReferenceImpl.c	1.12 06/10/10
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
#include "ClassLoaderReferenceImpl.h"
#include "util.h"
#include "inStream.h"
#include "outStream.h"

static jboolean 
visibleClasses(PacketInputStream *in, PacketOutputStream *out)
{
    JNIEnv *env = getEnv();
    jint error;
    jint count;
    jclass *classes;
    int i;

    jobject loader = inStream_readClassLoaderRef(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    error = jvmdi->GetClassLoaderClasses(loader, &count, &classes);
    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
        return JNI_TRUE;
    }

    outStream_writeInt(out, count);
    for (i = 0; i < count; i++) {
        jbyte tag;
        jclass clazz;

        clazz = classes[i];
        tag = referenceTypeTag(clazz);

        outStream_writeByte(out, tag);
        WRITE_GLOBAL_REF(env, out, clazz);
    }

    jdwpFree(classes);

    return JNI_TRUE;
}

void *ClassLoaderReference_Cmds[] = { (void *)0x1
    ,(void *)visibleClasses
};

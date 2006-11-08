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
#include <stdlib.h>
#include <string.h>

#include "ClassObjectReferenceImpl.h"
#include "util.h"
#include "inStream.h"
#include "outStream.h"

static jboolean 
reflectedType(PacketInputStream *in, PacketOutputStream *out)
{
    JNIEnv *env = getEnv();
    jclass clazz;
    jbyte tag;
    jobject object = inStream_readObjectRef(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    /*
     * In our implementation, the reference type id is the same as the
     * class object id, so we bounce it right back.
     *
     * (Both inStream and WRITE_GLOBAL_REF) delete their global references
     * so we need to create a new one to write.)
     */
    clazz = (*env)->NewGlobalRef(env, object);
    if (clazz == NULL) {
        outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
    } else {
        tag = referenceTypeTag(clazz);
        outStream_writeByte(out, tag);
        WRITE_GLOBAL_REF(env, out, clazz);
    }
    return JNI_TRUE;
}

void *ClassObjectReference_Cmds[] = { (void *)1
    ,(void *)reflectedType
};


/* 
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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


#include <JNIUtil.h>

jboolean
initializeFieldIds(jfieldID* dest, JNIEnv* env, jclass classHandle, 
                   const FieldDesc* fields) {
    jboolean retVal = JNI_TRUE;

    while (fields->name != NULL) {
        *dest = (*env)->GetFieldID(env, classHandle, fields->name, 
                                   fields->signature);
        if (*dest == NULL) {
            retVal = JNI_FALSE;
            break;
        }
        ++fields;
        ++dest;
    }

    return retVal;
}

jboolean
initializeStaticFieldIds(jfieldID* dest, JNIEnv* env, jclass classHandle, 
                         const FieldDesc* fields) {
    jboolean retVal = JNI_TRUE;

    while (fields->name != NULL) {
        *dest = (*env)->GetStaticFieldID(env, classHandle, fields->name,
                                         fields->signature);
        if (*dest == NULL) {
            retVal = JNI_FALSE;
            break;
        }
        ++fields;
        ++dest;
    }

    return retVal;
}

void
JNI_ThrowNew(JNIEnv* env, const char* throwable, const char* message) {
    jclass throwableClass = (*env)->FindClass(env, throwable);
    if (throwableClass == NULL) {
        (*env)->FatalError(env, "Failed to load an exception class!");
    }

    if ((*env)->ThrowNew(env, throwableClass, message) != 0) {
        (*env)->FatalError(env, "Failed to throw an exception!");
    }
}

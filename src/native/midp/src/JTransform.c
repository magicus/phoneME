/*
 * 
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
 */


#include <JTransform.h>

#include <KNIUtil.h>

#define TRANSFORM_M00 0
#define TRANSFORM_M01 1
#define TRANSFORM_M10 2
#define TRANSFORM_M11 3
#define TRANSFORM_M02 4
#define TRANSFORM_M12 5
#define TRANSFORM_LAST TRANSFORM_M12

static jfieldID fieldIds[TRANSFORM_LAST + 1];
static jboolean fieldIdsInitialized = KNI_FALSE;

static jboolean initializeTransformFieldIds(jobject objectHandle);

void
transform_get4(Transform4* transform, jobject object) {
    transform->m00 = KNI_GetIntField(object, fieldIds[TRANSFORM_M00]);
    transform->m01 = KNI_GetIntField(object, fieldIds[TRANSFORM_M01]);
    transform->m10 = KNI_GetIntField(object, fieldIds[TRANSFORM_M10]);
    transform->m11 = KNI_GetIntField(object, fieldIds[TRANSFORM_M11]);
}

void
transform_get6(Transform6* transform, jobject object) {
    transform->m00 = KNI_GetIntField(object, fieldIds[TRANSFORM_M00]);
    transform->m01 = KNI_GetIntField(object, fieldIds[TRANSFORM_M01]);
    transform->m10 = KNI_GetIntField(object, fieldIds[TRANSFORM_M10]);
    transform->m11 = KNI_GetIntField(object, fieldIds[TRANSFORM_M11]);
    transform->m02 = KNI_GetIntField(object, fieldIds[TRANSFORM_M02]);
    transform->m12 = KNI_GetIntField(object, fieldIds[TRANSFORM_M12]);
}

void
transform_set4(jobject object, const Transform4* transform) {
    KNI_SetIntField(object, fieldIds[TRANSFORM_M00], transform->m00);
    KNI_SetIntField(object, fieldIds[TRANSFORM_M01], transform->m01);
    KNI_SetIntField(object, fieldIds[TRANSFORM_M10], transform->m10);
    KNI_SetIntField(object, fieldIds[TRANSFORM_M11], transform->m11);
}

void
transform_set6(jobject object, const Transform6* transform) {
    KNI_SetIntField(object, fieldIds[TRANSFORM_M00], transform->m00);
    KNI_SetIntField(object, fieldIds[TRANSFORM_M01], transform->m01);
    KNI_SetIntField(object, fieldIds[TRANSFORM_M10], transform->m10);
    KNI_SetIntField(object, fieldIds[TRANSFORM_M11], transform->m11);
    KNI_SetIntField(object, fieldIds[TRANSFORM_M02], transform->m02);
    KNI_SetIntField(object, fieldIds[TRANSFORM_M12], transform->m12);
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_pisces_Transform6_initialize() {
    KNI_StartHandles(1);
    KNI_DeclareHandle(objectHandle);

    KNI_GetThisPointer(objectHandle);

    if (!initializeTransformFieldIds(objectHandle)) {
        KNI_ThrowNew("java/lang/IllegalStateException", "");
    }

    // don't do anything here (see the throw above)!

    KNI_EndHandles();
    KNI_ReturnVoid();
}

static jboolean
initializeTransformFieldIds(jobject objectHandle) {
    static const FieldDesc transformFieldDesc[] = {
                { "m00", "I"
                },
                { "m01", "I" },
                { "m10", "I" },
                { "m11", "I" },
                { "m02", "I" },
                { "m12", "I" },
                { NULL, NULL }
            };

    jboolean retVal;

    if (fieldIdsInitialized) {
        return KNI_TRUE;
    }

    retVal = KNI_FALSE;

    KNI_StartHandles(1);
    KNI_DeclareHandle(classHandle);

    KNI_GetObjectClass(objectHandle, classHandle);

    if (initializeFieldIds(fieldIds, classHandle, transformFieldDesc)) {
        retVal = KNI_TRUE;
        fieldIdsInitialized = KNI_TRUE;
    }

    KNI_EndHandles();
    return retVal;
}

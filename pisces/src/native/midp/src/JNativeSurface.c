/*
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


#include <JNativeSurface.h>

#include <KNIUtil.h>

#include <PiscesSurface.inl>

#include <pcsl_memory.h>

#include <sni.h>
#include <commonKNIMacros.h>

#define SURFACE_NATIVE_PTR 0
#define SURFACE_LAST SURFACE_NATIVE_PTR 

static jfieldID fieldIds[SURFACE_LAST + 1];
static jboolean fieldIdsInitialized = KNI_FALSE;

static jboolean initializeSurfaceFieldIds(jobject objectHandle);
static void disposeNativeImpl(jobject objectHandle);

Surface* 
surface_get(jobject objectHandle) {
  return (Surface*)JLongToPointer(KNI_GetLongField(objectHandle, 
      fieldIds[SURFACE_NATIVE_PTR]));
}

KNIEXPORT KNI_RETURNTYPE_VOID 
Java_com_sun_pisces_NativeSurface_initialize() {
  KNI_StartHandles(1);
  KNI_DeclareHandle(objectHandle);

  jint width = KNI_GetParameterAsInt(1);
  jint height = KNI_GetParameterAsInt(2);
  
  Surface* surface;

  KNI_GetThisPointer(objectHandle);

  if (initializeSurfaceFieldIds(objectHandle)) {
    surface = surface_create(width, height);

    if((NULL == surface) || 
       (KNI_TRUE == readAndClearMemErrorFlag())) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", 
                     "Allocation of internal renderer buffer failed.");
    } else {
        // success!

        KNI_SetLongField(objectHandle, fieldIds[SURFACE_NATIVE_PTR], 
                         PointerToJLong(surface));
        
        //    KNI_registerCleanup(objectHandle, disposeNativeImpl);
    }

  } else {
    KNI_ThrowNew("java/lang/IllegalStateException", "");
  }
  
  KNI_EndHandles();
  KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID 
Java_com_sun_pisces_NativeSurface_finalize() {
  KNI_StartHandles(1);
  KNI_DeclareHandle(objectHandle);

  KNI_GetThisPointer(objectHandle);

  disposeNativeImpl(objectHandle);

  KNI_EndHandles();
  KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_pisces_NativeSurface_getWidth() {
  jint width;

  KNI_StartHandles(1);
  KNI_DeclareHandle(objectHandle);

  Surface* surface;

  KNI_GetThisPointer(objectHandle);
  surface = (Surface*)JLongToPointer(KNI_GetLongField(objectHandle, 
      fieldIds[SURFACE_NATIVE_PTR]));

  width = surface->width;

  KNI_EndHandles();
  KNI_ReturnInt(width);
}

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_pisces_NativeSurface_getHeight() {
  jint height;

  KNI_StartHandles(1);
  KNI_DeclareHandle(objectHandle);

  Surface* surface;

  KNI_GetThisPointer(objectHandle);
  surface = (Surface*)JLongToPointer(KNI_GetLongField(objectHandle, 
      fieldIds[SURFACE_NATIVE_PTR]));

  height = surface->height;

  KNI_EndHandles();
  KNI_ReturnInt(height);
}

KNIEXPORT KNI_RETURNTYPE_VOID 
Java_com_sun_pisces_NativeSurface_getRGB() {
  KNI_StartHandles(2);
  KNI_DeclareHandle(objectHandle);
  KNI_DeclareHandle(arrayHandle);

  jint offset = KNI_GetParameterAsInt(2);
  jint scanLength = KNI_GetParameterAsInt(3);
  jint x = KNI_GetParameterAsInt(4);
  jint y = KNI_GetParameterAsInt(5);
  jint width = KNI_GetParameterAsInt(6);
  jint height = KNI_GetParameterAsInt(7);
  
  jint dstX = 0;
  jint dstY = 0;
  
  Surface* surface;

  KNI_GetParameterAsObject(1, arrayHandle);
      
  KNI_GetThisPointer(objectHandle);
  surface = (Surface*)JLongToPointer(KNI_GetLongField(objectHandle, 
      fieldIds[SURFACE_NATIVE_PTR]));

  CORRECT_DIMS(surface, x, y, width, height, dstX, dstY);
  
  if ((width > 0) && (height > 0)) {
    jint size = ((height - 1) * scanLength + width) * sizeof(jint);
    jint* tempArray = (jint*)pcsl_mem_malloc(size);
    
    if (NULL == tempArray) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", 
                     "Allocation of temporary renderer memory buffer failed.");
    } else {
      jint* src;
      jint* dst;
      jint srcScanRest = surface->width - width;
      jint dstScanRest = scanLength - width;
          
      src = surface->data + y * surface->width + x;
      dst = tempArray;
      for (; height > 0; --height) {
        jint w2 = width;
        for (; w2 > 0; --w2) {
          *dst++ = *src++;
        }
        src += srcScanRest;
        dst += dstScanRest;
      }

      offset += dstY * scanLength + dstX;
      KNI_SetRawArrayRegion(arrayHandle, offset * sizeof(jint), size, 
          (const jbyte*)tempArray);
      
      pcsl_mem_free(tempArray);
    }
  }

  KNI_EndHandles();
  KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID 
Java_com_sun_pisces_NativeSurface_setRGB() {
  KNI_StartHandles(2);
  KNI_DeclareHandle(objectHandle);
  KNI_DeclareHandle(arrayHandle);

  jint offset = KNI_GetParameterAsInt(2);
  jint scanLength = KNI_GetParameterAsInt(3);
  jint x = KNI_GetParameterAsInt(4);
  jint y = KNI_GetParameterAsInt(5);
  jint width = KNI_GetParameterAsInt(6);
  jint height = KNI_GetParameterAsInt(7);
  
  jint srcX = 0;
  jint srcY = 0;
  
  Surface* surface;

  KNI_GetParameterAsObject(1, arrayHandle);
      
  KNI_GetThisPointer(objectHandle);
  surface = (Surface*)JLongToPointer(KNI_GetLongField(objectHandle, 
      fieldIds[SURFACE_NATIVE_PTR]));

  CORRECT_DIMS(surface, x, y, width, height, srcX, srcY);
  
  if ((width > 0) && (height > 0)) {
      jint *tempArray;
      offset += srcY * scanLength + srcX;

      SNI_BEGIN_RAW_POINTERS;
      
      tempArray = &JavaIntArray(arrayHandle)[offset];

      surface_setRGB(surface, x, y, width, height, tempArray, scanLength);  
      SNI_END_RAW_POINTERS;
  }

  KNI_EndHandles();
  KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID 
Java_com_sun_pisces_NativeSurface_drawSurfaceImpl() {
  KNI_StartHandles(2);
  KNI_DeclareHandle(objectHandle);
  KNI_DeclareHandle(surfaceHandle);
  
  jint srcX = KNI_GetParameterAsInt(2);  
  jint srcY = KNI_GetParameterAsInt(3);  
  jint dstX = KNI_GetParameterAsInt(4);  
  jint dstY = KNI_GetParameterAsInt(5);  
  jint width = KNI_GetParameterAsInt(6);
  jint height = KNI_GetParameterAsInt(7);
  jfloat opacity = KNI_GetParameterAsFloat(8);
    
  Surface* dstSurface;
  Surface* srcSurface;
  
  KNI_GetThisPointer(objectHandle);
  dstSurface = (Surface*)JLongToPointer(KNI_GetLongField(objectHandle, 
      fieldIds[SURFACE_NATIVE_PTR]));
  KNI_GetParameterAsObject(1, surfaceHandle);
  srcSurface = (Surface*)JLongToPointer(
      KNI_GetLongField(surfaceHandle, fieldIds[SURFACE_NATIVE_PTR]));

  CORRECT_DIMS(dstSurface, dstX, dstY, width, height, srcX, srcY);
  CORRECT_DIMS(srcSurface, srcX, srcY, width, height, dstX, dstY);

  if ((width > 0) && (height > 0) && (opacity > 0)) {
    surface_drawSurface(dstSurface, dstX, dstY, width, height, 
        srcSurface, srcX, srcY, opacity);
  }

  KNI_EndHandles();
  KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID 
Java_com_sun_pisces_NativeSurface_drawRGBImpl() {
  KNI_StartHandles(2);
  KNI_DeclareHandle(objectHandle);
  KNI_DeclareHandle(arrayHandle);

  jint offset = KNI_GetParameterAsInt(2);
  jint scanLength = KNI_GetParameterAsInt(3);
  jint x = KNI_GetParameterAsInt(4);
  jint y = KNI_GetParameterAsInt(5);
  jint width = KNI_GetParameterAsInt(6);
  jint height = KNI_GetParameterAsInt(7);
  jfloat opacity = KNI_GetParameterAsFloat(8);  
  
  jint srcX = 0;
  jint srcY = 0;
  
  Surface* surface;

  KNI_GetParameterAsObject(1, arrayHandle);
      
  KNI_GetThisPointer(objectHandle);
  surface = (Surface*)JLongToPointer(KNI_GetLongField(objectHandle, 
      fieldIds[SURFACE_NATIVE_PTR]));

  CORRECT_DIMS(surface, x, y, width, height, srcX, srcY);
  
  if ((width > 0) && (height > 0)) {
      jint* tempArray;
      offset += srcY * scanLength + srcX;
      
      SNI_BEGIN_RAW_POINTERS;
      
      tempArray = &JavaIntArray(arrayHandle)[offset];

      surface_drawRGB(surface, x, y, width, height, tempArray, scanLength, 
          opacity);          
      
      SNI_END_RAW_POINTERS;
  }
  
  KNI_EndHandles();
  KNI_ReturnVoid();
}

static jboolean 
initializeSurfaceFieldIds(jobject objectHandle) {
  static const FieldDesc surfaceFieldDesc[] = {
    { "nativePtr", "J" },
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
  
  if (initializeFieldIds(fieldIds, classHandle, surfaceFieldDesc)) {
    retVal = KNI_TRUE;
    fieldIdsInitialized = KNI_TRUE;
  }

  KNI_EndHandles();  
  return retVal;
}

static void
disposeNativeImpl(jobject objectHandle) {
  Surface* surface;

  if (!fieldIdsInitialized) {
    return;
  }
  
  surface = (Surface*)JLongToPointer(KNI_GetLongField(objectHandle, 
      fieldIds[SURFACE_NATIVE_PTR]));

  if (surface != NULL) {
    surface_dispose(surface);
    KNI_SetLongField(objectHandle, fieldIds[SURFACE_NATIVE_PTR], (jlong)0);

    if(KNI_TRUE == readAndClearMemErrorFlag()) {
        KNI_ThrowNew("java/lang/OutOfMemoryError", 
                     "Allocation of internal renderer buffer failed.");
    }
  }
}

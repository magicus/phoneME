/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

/*
 * Native functions for
 * ..\src\com\sun\jsr239\EGL10Impl.java
 */

#include <lcdlf_export.h>

#include <kni.h>

#include <gl.h>
#include <egl.h>

#include <commonKNIMacros.h>
#include <ROMStructs.h>
#include <jvm.h>

#include "JSR239-KNIInterface.h"

#undef DEBUG

#ifdef DEBUG
#include <stdio.h>
#endif

#ifdef DEBUG
static int contexts = 0;
static int surfaces = 0;
#endif

#define ALL_WINDOWS_ARE_PIXMAPS

/* Called at VM startup to initialize the engine. */
void
JSR239_initialize() {
}

/* Called at VM shutdown to shut down the engine. */
void
JSR239_shutdown() {
}

/*  private native int _eglGetError ( ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1eglGetError() {

    jint returnValue = (jint)eglGetError();
#ifdef DEBUG
    printf("eglGetError() = %d\n", returnValue);
#endif

    KNI_ReturnInt(returnValue);
}

/*  private native int _eglGetDisplay ( int displayID ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1eglGetDisplay() {

    jint displayID = KNI_GetParameterAsInt(1);

    jint returnValue = (jint)eglGetDisplay((NativeDisplayType)displayID);
#ifdef DEBUG
    printf("eglGetDisplay(0x%x) = %d\n", displayID, returnValue);
#endif

    KNI_ReturnInt(returnValue);
}

/*  private native int _eglInitialize ( int display , int [ ] major_minor ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1eglInitialize() {

    jint display = KNI_GetParameterAsInt(1);
    EGLint major, minor;

    jint returnValue;

    KNI_StartHandles(1);
    KNI_DeclareHandle(major_minorHandle);

    KNI_GetParameterAsObject(2, major_minorHandle);

    returnValue = (jint) eglInitialize((EGLDisplay)display, &major, &minor);
#ifdef DEBUG
    printf("eglInitialize(0x%x, major<-%d, minor<-%d) = %d\n",
	   display, major, minor, returnValue);
#endif
    if ((returnValue == EGL_TRUE) && !KNI_IsNullHandle(major_minorHandle)) {
        KNI_SetIntArrayElement(major_minorHandle, 0, major);
        KNI_SetIntArrayElement(major_minorHandle, 1, minor);
    }

    KNI_EndHandles();
    KNI_ReturnInt(returnValue);
}

/*  private native int _eglTerminate ( int display ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1eglTerminate() {

    jint display = KNI_GetParameterAsInt(1);

    jint returnValue = eglTerminate((EGLDisplay)display);
#ifdef DEBUG
    printf("eglTerminate(0x%x) = %d\n", display, returnValue);
#endif

    KNI_ReturnInt(returnValue);
}

/*  private native String _eglQueryString ( int display , int name ) ; */
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_javax_microedition_khronos_egl_EGL10Impl__1eglQueryString() {

    jint display = KNI_GetParameterAsInt(1);
    jint name = KNI_GetParameterAsInt(2);
    const char *string;
    
    KNI_StartHandles(1);
    KNI_DeclareHandle(stringHandle);
    
    string = eglQueryString((EGLDisplay)display, (EGLint)name);
#ifdef DEBUG
    printf("eglQueryString(0x%x, %d) = %s\n", display, name, string);
#endif
    
    if (string) {
        KNI_NewStringUTF(string, stringHandle);
    } else {
        /* Set stringHandle to null. */
        KNI_ReleaseHandle(stringHandle);
    }
    KNI_EndHandlesAndReturnObject(stringHandle);
}

/*  private native int _eglGetConfigs ( int display , int [ ] configs , int config_size , int [ ] num_config ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1eglGetConfigs() {

    jint display = KNI_GetParameterAsInt(1);
    jint config_size = KNI_GetParameterAsInt(3);
    
    EGLConfig *configs = (EGLConfig *)0;
    EGLint num_config = 0;

    jint returnValue = EGL_FALSE;

    KNI_StartHandles(2);
    KNI_DeclareHandle(configsHandle);
    KNI_DeclareHandle(num_configHandle);

    KNI_GetParameterAsObject(2, configsHandle);
    KNI_GetParameterAsObject(4, num_configHandle);

    if (!KNI_IsNullHandle(configsHandle)) {
	configs = (EGLConfig *)JSR239_malloc(config_size*sizeof(EGLint));
	if (!configs) {
            KNI_ThrowNew("java.lang.OutOfMemoryException", "eglGetConfigs");
	    goto exit;
	}
    }

    returnValue = (jint)eglGetConfigs((EGLDisplay)display,
				      configs,
				      (EGLint)config_size,
				      &num_config);

#ifdef DEBUG
    printf("num_config = %d\n", num_config);
#endif

#ifdef ALL_WINDOWS_ARE_PIXMAPS
    {
      EGLBoolean ok;
      EGLint surfaceType;
      int i, j;

        // Remove all configs that are for windows only
        if (returnValue && configs) {
            for (i = 0; i < num_config; i++) {
                ok = eglGetConfigAttrib((EGLDisplay)display, configs[i],
                                        EGL_SURFACE_TYPE, &surfaceType);
                if ((surfaceType & (EGL_PIXMAP_BIT | EGL_PBUFFER_BIT)) == 0) {
                    for (j = i; j < num_config - 1; j++) {
                        configs[j] = configs[j + 1];
                    }
                    configs[--num_config] = 0;
#ifdef DEBUG
                    printf("Deleting a config, num_config = %d\n", num_config);
#endif
                }
            }
        }
    }
#endif

#ifdef DEBUG
    printf("eglGetConfigs(0x%x, configs, %d, num_config<-%d) = %d\n",
	   display, config_size, num_config, returnValue);
#endif
    
    if ((returnValue == EGL_TRUE) && !KNI_IsNullHandle(configsHandle)) {
	KNI_SetRawArrayRegion(configsHandle, 0, config_size*sizeof(EGLint),
			      (const jbyte *)configs);
    }
    if ((returnValue == EGL_TRUE) && !KNI_IsNullHandle(num_configHandle)) {
        KNI_SetIntArrayElement(num_configHandle, 0, num_config);
    }
    
 exit:
    if (configs) {
	JSR239_free(configs);
    }
    KNI_EndHandles();
    KNI_ReturnInt(returnValue);
}

/*  private native int _eglChooseConfig ( int display , int [ ] attrib_list , int [ ] configs , int config_size , int [ ] num_config ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1eglChooseConfig() {
    
    jint display = KNI_GetParameterAsInt(1);
    jint config_size = KNI_GetParameterAsInt(4);
    EGLint *attrib_list = (EGLint *)0;
    EGLConfig *configs = (EGLConfig *)0;
    EGLint num_config;

    jint returnValue = EGL_FALSE;

    KNI_StartHandles(3);
    KNI_DeclareHandle(attrib_listHandle);
    KNI_DeclareHandle(configsHandle);
    KNI_DeclareHandle(num_configHandle);

    KNI_GetParameterAsObject(2, attrib_listHandle);
    KNI_GetParameterAsObject(3, configsHandle);
    KNI_GetParameterAsObject(5, num_configHandle);

    if (!KNI_IsNullHandle(attrib_listHandle)) {
        jint attrib_list_length, attrib_list_size;
        /* Construct attrib_list as a copy of attrib_listHandle */
        attrib_list_length = KNI_GetArrayLength(attrib_listHandle);
        attrib_list_size = (attrib_list_length + 2)*sizeof(jint);
        attrib_list = (EGLint *)JSR239_malloc(attrib_list_size);
        if (!attrib_list) {
            KNI_ThrowNew("java.lang.OutOfMemoryException", "eglChooseConfig");
            goto exit;
        }
        KNI_GetRawArrayRegion(attrib_listHandle, 0, attrib_list_size,
                              (jbyte *)attrib_list);
    }

#ifdef ALL_WINDOWS_ARE_PIXMAPS
    // Force queries with SURFACE_TYPE == WINDOW to actually query pixmap
    if (attrib_list) {
        jint attr, val, idx;

        idx = 0;
        while (1) {
            attr = attrib_list[idx];
#ifdef DEBUG
            printf("attr[%d] = %d\n", idx, attr);
#endif
            if (attr == EGL_NONE) {
#ifdef DEBUG
                printf("attr = EGL_NONE, done\n");
#endif
                break;
            }
            if (attr == EGL_SURFACE_TYPE) {
#ifdef DEBUG
                printf("attr = EGL_SURFACE_TYPE\n");
#endif
                val = attrib_list[idx + 1];
                if ((val & EGL_WINDOW_BIT) != 0) {
#ifdef DEBUG
                    printf("Switching WINDOW_BIT to PIXMAP_BIT\n");
#endif
                    val |= EGL_PIXMAP_BIT;
                    val &= ~EGL_WINDOW_BIT;
                }   
            }
            idx += 2;
        }
    }
#endif

    /* Allocate config_size elements if configsHandle is non-null */
    if (!KNI_IsNullHandle(configsHandle)) {
	configs = (EGLConfig *)JSR239_malloc(config_size*sizeof(EGLint));
	if (!configs) {
            KNI_ThrowNew("java.lang.OutOfMemoryException", "eglChooseConfig");
	    goto exit;
	}
    }
    
    returnValue = (jint)eglChooseConfig((EGLDisplay)display, attrib_list,
					configs, config_size, &num_config);
#ifdef DEBUG
    printf("eglChooseConfig(0x%x, attrib_list, configs, %d, num_config<-%d) = %d\n",
	   display, config_size, num_config, returnValue);
#endif

    /* Copy configs back to Java */
    if (returnValue == EGL_TRUE && !KNI_IsNullHandle(configsHandle)) {
	KNI_SetRawArrayRegion(configsHandle, 0, config_size*sizeof(EGLint),
			      (const jbyte *)configs);
    }
    /* Set output parameter via num_configHandle */
    if ((returnValue == EGL_TRUE) && !KNI_IsNullHandle(num_configHandle)) {
        KNI_SetIntArrayElement(num_configHandle, 0, num_config);
    }

 exit:
    if (attrib_list) {
	JSR239_free(attrib_list);
    }
    if (configs) {
	JSR239_free(configs);
    }

    KNI_EndHandles();
    KNI_ReturnInt(returnValue);
}

/*  private native int _eglGetConfigAttrib ( int display , int config , int attribute , int [ ] value ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1eglGetConfigAttrib() {

    jint display = KNI_GetParameterAsInt(1);
    jint config = KNI_GetParameterAsInt(2);
    jint attribute = KNI_GetParameterAsInt(3);
    EGLint value;

    jint returnValue = EGL_FALSE;

    KNI_StartHandles(1);
    KNI_DeclareHandle(valueHandle);

    KNI_GetParameterAsObject(4, valueHandle);

    returnValue = (jint)eglGetConfigAttrib((EGLDisplay)display,
                                           (EGLConfig)config,
                                           (EGLint)attribute,
                                           &value);
#ifdef DEBUG
    printf("eglGetConfigAttrib(0x%x, 0x%x, %d, value<-%d) = %d\n",
	   display, config, attribute, value, returnValue);
#endif

#ifdef ALL_WINDOWS_ARE_PIXMAPS
    // If all windows are being backed by pixmaps, mark all
    // configs that are pixmap compatible as being window compatible
    if ((attribute == EGL_SURFACE_TYPE) &&
        ((value & EGL_PIXMAP_BIT) != 0)) {
        value |= EGL_WINDOW_BIT;
    }
#endif
    
    /* Set output parameter via valueHandle only if the call was successful */
    if ((returnValue == EGL_TRUE) && !KNI_IsNullHandle(valueHandle)) {
        KNI_SetIntArrayElement(valueHandle, 0, value);
    }

    KNI_EndHandles();
    KNI_ReturnInt(returnValue);
}

/*  private native int _eglCreateWindowSurface ( int display , int config , int win , int [ ] attrib_list ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1eglCreateWindowSurface() {

    jint display = KNI_GetParameterAsInt(1);
    jint config = KNI_GetParameterAsInt(2);
    jint win = KNI_GetParameterAsInt(3);
    EGLint *attrib_list = (EGLint *)0;

    jint returnValue = (jint)EGL_NO_SURFACE;

    KNI_StartHandles(1);
    KNI_DeclareHandle(attrib_listHandle);

    KNI_GetParameterAsObject(4, attrib_listHandle);

    /* Construct attrib_list as a copy of attrib_listHandle if non-null */
    if (!KNI_IsNullHandle(attrib_listHandle)) {
        jint attrib_list_length, attrib_list_size;

	attrib_list_length = KNI_GetArrayLength(attrib_listHandle);
	attrib_list_size = attrib_list_length*sizeof(jint);
	attrib_list = (EGLint *)JSR239_malloc(attrib_list_size);
	if (!attrib_list) {
            KNI_ThrowNew("java.lang.OutOfMemoryException",
			 "eglCreateWindowSurface");
	    goto exit;
	}
	KNI_GetRawArrayRegion(attrib_listHandle, 0, attrib_list_size,
			      (jbyte *) attrib_list);
    }

    returnValue = (jint)eglCreateWindowSurface((EGLDisplay)display,
					       (EGLConfig)config,
					       (NativeWindowType)win,
					       (const EGLint *)attrib_list);
#ifdef DEBUG
    printf("eglCreateWindowSurface(0x%x, 0x%x, 0x%x, attrib_list) = %d\n",
	   display, config, win, returnValue);
#endif

 exit:
    if (attrib_list) {
	JSR239_free(attrib_list);
    }
    
    KNI_EndHandles();
    KNI_ReturnInt(returnValue);
}

/*  private native int _eglCreatePixmapSurface ( int display , int config , int pixmap , int [ ] attrib_list ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1eglCreatePixmapSurface() {

    jint display = KNI_GetParameterAsInt(1);
    jint config = KNI_GetParameterAsInt(2);
    jint pixmap = KNI_GetParameterAsInt(3);
    EGLint *attrib_list = (EGLint *)0;

    jint returnValue = (jint)EGL_NO_SURFACE;

    KNI_StartHandles(1);
    KNI_DeclareHandle(attrib_listHandle);

    KNI_GetParameterAsObject(4, attrib_listHandle);

    /* Construct attrib_list as a copy of attrib_listHandle if non-null */
    if (!KNI_IsNullHandle(attrib_listHandle)) {
        jint attrib_list_length, attrib_list_size;

	attrib_list_length = KNI_GetArrayLength(attrib_listHandle);
	attrib_list_size = attrib_list_length*sizeof(jint);
	attrib_list = (EGLint *)JSR239_malloc(attrib_list_size);
	if (!attrib_list) {
            KNI_ThrowNew("java.lang.OutOfMemoryException",
			 "eglCreatePixmapSurface");
	    goto exit;
	}
	KNI_GetRawArrayRegion(attrib_listHandle, 0, attrib_list_size,
			      (jbyte *) attrib_list);
    }

    returnValue = (jint)eglCreatePixmapSurface((EGLDisplay)display,
					       (EGLConfig)config,
					       (NativePixmapType)pixmap,
					       (EGLint const *) attrib_list);
#ifdef DEBUG
    printf("eglCreatePixmapSurface(%d, %d, 0x%x, attrib_list) = %d\n",
	   display, config, pixmap, returnValue);
#endif

 exit:
    if (attrib_list) {
	JSR239_free(attrib_list);
    }

    KNI_EndHandles();
    KNI_ReturnInt(returnValue);
}

/*  private native int _eglCreatePbufferSurface ( int display , int config , int [ ] attrib_list ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1eglCreatePbufferSurface() {

    jint display = KNI_GetParameterAsInt(1);
    jint config = KNI_GetParameterAsInt(2);
    EGLint *attrib_list = (EGLint *)0;

    jint returnValue = (jint)EGL_NO_SURFACE;

    KNI_StartHandles(2);
    KNI_DeclareHandle(attrib_listHandle);
    KNI_DeclareHandle(thisHandle);

    KNI_GetParameterAsObject(3, attrib_listHandle);
    KNI_GetThisPointer(thisHandle);

    /* Construct attrib_list as a copy of attrib_listHandle if non-null */
    if (!KNI_IsNullHandle(attrib_listHandle)) {
        jint attrib_list_length, attrib_list_size;

	attrib_list_length = KNI_GetArrayLength(attrib_listHandle);
	attrib_list_size = attrib_list_length*sizeof(jint);
	attrib_list = (EGLint *)JSR239_malloc(attrib_list_size);
	if (!attrib_list) {
            KNI_ThrowNew("java.lang.OutOfMemoryException",
			 "eglCreatePbufferSurface");
	    goto exit;
	}
	KNI_GetRawArrayRegion(attrib_listHandle, 0, attrib_list_size,
			      (jbyte *) attrib_list);
    }

#ifdef DEBUG
    {
        int idx = 0;
        int attr;
     
#ifdef DEBUG
        printf("In eglCreatePbufferSurface:\n");
#endif
        if (!attrib_list) {
#ifdef DEBUG
            printf("attrib_list is null!\n");
#endif
        } else {
            while (1) {
                attr = attrib_list[idx++];
#ifdef DEBUG
                printf("attr[%d] = 0x%x\n", idx, attr);
#endif
                if (attr == EGL_NONE) {
                    break;
                }
            }
        }
    }
#endif
        
    returnValue = (jint)eglCreatePbufferSurface((EGLDisplay)display,
						(EGLConfig)config,
						(EGLint const *) attrib_list);

#ifdef DEBUG
    if (returnValue > 0) {
        ++surfaces;
    }

    printf("eglCreatePbufferSurface(0x%x, 0x%x, attrib_list) = %d, surfaces = %d\n",
	   display, config, returnValue, surfaces);

    {
        EGLint error; /* debugging */
        error = eglGetError();
        if (error != EGL_SUCCESS) {
            printf("egl error = 0x%x\n", error);
        }
    }
#endif

 exit:
    if (attrib_list) {
	JSR239_free(attrib_list);
    }

    KNI_EndHandles();
    KNI_ReturnInt(returnValue);
}

/*  private native int _eglDestroySurface ( int display , int surface ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1eglDestroySurface() {

    jint display = KNI_GetParameterAsInt(1);
    jint surface = KNI_GetParameterAsInt(2);

    jint returnValue = (jint)eglDestroySurface((EGLDisplay)display,
					       (EGLSurface)surface);
#ifdef DEBUG
    if (returnValue > 0) {
        --surfaces;
    }

    printf("eglDestroySurface(0x%x, 0x%x) = %d, surfaces = %d\n",
	   display, surface, returnValue, surfaces);
#endif

    KNI_ReturnInt(returnValue);
}

/*  private native int _eglQuerySurface ( int display , int surface , int attribute , int [ ] value ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1eglQuerySurface() {

    jint display = KNI_GetParameterAsInt(1);
    jint surface = KNI_GetParameterAsInt(2);
    jint attribute = KNI_GetParameterAsInt(3);
    EGLint value;

    jint returnValue = EGL_FALSE;

    KNI_StartHandles(1);
    KNI_DeclareHandle(valueHandle);

    KNI_GetParameterAsObject(4, valueHandle);

    returnValue = (jint)eglQuerySurface((EGLDisplay)display,
					(EGLSurface)surface,
					(EGLint)attribute,
					&value);
#ifdef DEBUG
    printf("eglQuerySurface(0x%x, 0x%x, %d, value<-%d) = %d\n",
	   display, surface, attribute, value, returnValue);
#endif

    if ((returnValue == EGL_TRUE) && !KNI_IsNullHandle(valueHandle)) {
        KNI_SetIntArrayElement(valueHandle, 0, value);
    }

    KNI_EndHandles();
    KNI_ReturnInt(returnValue);
}

/*  private native int _eglCreateContext ( int display , int config , int share_context , int [ ] attrib_list ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1eglCreateContext() {

    jint display = KNI_GetParameterAsInt(1);
    jint config = KNI_GetParameterAsInt(2);
    jint share_context = KNI_GetParameterAsInt(3);
    EGLint *attrib_list = (EGLint *)0;

    jint returnValue = 0;

    KNI_StartHandles(1);
    KNI_DeclareHandle(attrib_listHandle);

    KNI_GetParameterAsObject(4, attrib_listHandle);

    /* Construct attrib_list as a copy of attrib_listHandle */
    if (!KNI_IsNullHandle(attrib_listHandle)) {
        jint attrib_list_length, attrib_list_size;

        attrib_list_length = KNI_GetArrayLength(attrib_listHandle);
        attrib_list_size = attrib_list_length*sizeof(jint);
        attrib_list = (EGLint *)JSR239_malloc(attrib_list_size);
        if (!attrib_list) {
            KNI_ThrowNew("java.lang.OutOfMemoryException", "eglCreateContext");
            goto exit;
        }
        KNI_GetRawArrayRegion(attrib_listHandle, 0, attrib_list_size,
                              (jbyte *)attrib_list);
    }

    returnValue = (jint)eglCreateContext((EGLDisplay) display,
					 (EGLConfig) config,
					 (EGLConfig) share_context,
					 attrib_list);
#ifdef DEBUG
    if (returnValue > 0) {
        ++contexts;
    }

    printf("eglCreateContext(0x%x, 0x%x, 0x%x, attrib_list) = %d, contexts = %d\n",
	   display, config, share_context, returnValue, contexts);
#endif

 exit:
    if (attrib_list) {
	JSR239_free(attrib_list);
    }
    KNI_EndHandles();
    KNI_ReturnInt(returnValue);
}

/*  private native int _eglDestroyContext ( int display , int ctx ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1eglDestroyContext() {

    jint display = KNI_GetParameterAsInt(1);
    jint ctx = KNI_GetParameterAsInt(2);

    jint returnValue = eglDestroyContext((EGLDisplay)display,
					 (EGLContext)ctx);
#ifdef DEBUG
    if (returnValue > 0) {
        --contexts;
    }

    printf("eglDestroyContext(0x%x, 0x%x) = %d, contexts = %d\n",
	   display, ctx, returnValue, contexts);
#endif

    KNI_ReturnInt(returnValue);
}

/*  private native int _eglMakeCurrent ( int display , int draw , int read , int ctx ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1eglMakeCurrent() {

    jint display = KNI_GetParameterAsInt(1);
    jint draw = KNI_GetParameterAsInt(2);
    jint read = KNI_GetParameterAsInt(3);
    jint ctx = KNI_GetParameterAsInt(4);

    jint returnValue = (jint)eglMakeCurrent((EGLDisplay)display,
					    (EGLSurface)draw,
					    (EGLSurface)read,
					    (EGLContext)ctx);
#ifdef DEBUG
    printf("eglMakeCurrent(0x%x, 0x%x, 0x%x, 0x%x) = %d\n",  
	   display, draw, read, ctx, returnValue);
#endif
    
    KNI_ReturnInt(returnValue);
}

/*  private native int _getCurrentContext ( ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1getCurrentContext() {

    jint returnValue = (jint)eglGetCurrentContext();
#ifdef DEBUG
    printf("eglGetCurrentContext() = %d\n", returnValue);
#endif

    KNI_ReturnInt(returnValue);
}

/*  private native int _getCurrentSurface ( int readdraw ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1getCurrentSurface() {

    jint readdraw = KNI_GetParameterAsInt(1);

    jint returnValue = (jint)eglGetCurrentSurface((EGLint)readdraw);
#ifdef DEBUG
    printf("eglGetCurrentSurface(%d) = %d\n", readdraw, returnValue);
#endif

    KNI_ReturnInt(returnValue);
}

/*  private native int _getCurrentDisplay ( ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1getCurrentDisplay() {

    jint returnValue = (jint)eglGetCurrentDisplay();
#ifdef DEBUG
    printf("eglGetCurrentDisplay() = 0x%x\n", returnValue);
#endif

    KNI_ReturnInt(returnValue);
}

/*  private native int _eglQueryContext ( int display , int ctx , int attribute , int [ ] value ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1eglQueryContext() {

    jint display = KNI_GetParameterAsInt(1);
    jint ctx = KNI_GetParameterAsInt(2);
    jint attribute = KNI_GetParameterAsInt(3);
    EGLint value;

    jint returnValue = EGL_FALSE;

    KNI_StartHandles(1);
    KNI_DeclareHandle(valueHandle);

    KNI_GetParameterAsObject(4, valueHandle);

    returnValue = (jint)eglQueryContext((EGLDisplay)display,
					(EGLContext)ctx,
					attribute,
					&value);
#ifdef DEBUG
    printf("eglQueryContext(0x%x, 0x%x, %d, value<-%d) = %d\n",
	   display, ctx, attribute, value, returnValue);
#endif
    if ((returnValue == EGL_TRUE) && !KNI_IsNullHandle(valueHandle)) {
        KNI_SetIntArrayElement(valueHandle, 0, value);
    }

    KNI_EndHandles();
    KNI_ReturnInt(returnValue);
}

/*  private native int _eglWaitGL ( ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1eglWaitGL() {

    jint returnValue = (jint)eglWaitGL();
#ifdef DEBUG
    printf("eglWaitGL() = %d\n", returnValue);
#endif

    KNI_ReturnInt(returnValue);
}

/*  private native int _eglWaitNative ( int engine ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1eglWaitNative() {

    jint engine = KNI_GetParameterAsInt(1);

    jint returnValue = (jint)eglWaitNative(engine);
#ifdef DEBUG
    printf("eglWaitNative(%d) = %d\n", engine, returnValue);
#endif

    KNI_ReturnInt(returnValue);
}

/*  private native int _eglSwapBuffers ( int display , int surface ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1eglSwapBuffers() {

    jint display = KNI_GetParameterAsInt(1);
    jint surface = KNI_GetParameterAsInt(2);

    jint returnValue = (jint)eglSwapBuffers((EGLDisplay) display,
					    (EGLSurface) surface);
#ifdef DEBUG
    printf("eglSwapBuffers(0x%x, 0x%x) = %d\n",
	   display, surface, returnValue);
#endif

    KNI_ReturnInt(returnValue);
}

static EGLContext
contextForSurface(EGLDisplay display, EGLSurface surface) {
    EGLConfig config;
    EGLContext context;
    
    eglGetError(); // Remove prior error

    eglQuerySurface(display, surface, EGL_CONFIG_ID, (EGLint *) &config);
    if (eglGetError() != EGL_SUCCESS) {
        // If surface is no longer valid, fail
        return EGL_NO_CONTEXT;
    }

    context = eglCreateContext(display, config,
                               EGL_NO_CONTEXT, (const EGLint *) 0);
    
    return context;
}

static jint
eglCopyBuffersWorkaround(EGLDisplay display, EGLSurface surface,
                         JSR239_Pixmap *pixmap) {
    EGLContext context;
    int hadContext = 0;
#ifdef DEBUG
    GLenum error;
#endif
    
    context = eglGetCurrentContext();
    hadContext = context != EGL_NO_CONTEXT;
            
    if (!hadContext) {
        EGLint success;

        context = contextForSurface((EGLDisplay) display, surface);
        if (context == EGL_NO_CONTEXT) {
            goto cleanup;
        }
        success = eglMakeCurrent(display, surface, surface, context);
        if (!success) {
            goto cleanup;
        }
    }

    glReadPixels(0, 0, pixmap->width, pixmap->height, GL_RGBA,
                 GL_UNSIGNED_BYTE, pixmap->pixels);

#ifdef DEBUG
    error = glGetError();
    if (error != GL_NO_ERROR) {
        printf("glReadPixels: error = 0x%x\n", error);
    }
#endif

 cleanup:
    if (!hadContext) {
        if (context != EGL_NO_CONTEXT) {
            eglDestroyContext(display, context);
        }
        eglMakeCurrent((EGLDisplay) display,
                       EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }
    
    return EGL_TRUE;
}

/*  private native int _eglCopyBuffers ( int display , int surface , Graphics target ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1eglCopyBuffers() {
    EGLDisplay display = (EGLDisplay) KNI_GetParameterAsInt(1);
    EGLSurface surface = (EGLSurface) KNI_GetParameterAsInt(2);
    jint width = KNI_GetParameterAsInt(4);
    jint height = KNI_GetParameterAsInt(5);
    jint delta_height = KNI_GetParameterAsInt(6);
    jint clipX = KNI_GetParameterAsInt(7);
    jint clipY = KNI_GetParameterAsInt(8);
    jint clipWidth = KNI_GetParameterAsInt(9);
    jint clipHeight = KNI_GetParameterAsInt(10);
    JSR239_Pixmap* pixmap;

    // eglCopyBuffers copies the EGL surface color buffer to a native pixmap.
    // As the native pixmaps the Graphics opbject are used. Thus eglCopyBuffers
    // can not be used as the destination buffer can be of a different size as
    // the source surface. In these case call to eglCopyBuffers may lead to
    // the memory corruption.
    // Of course it is more efficient ot use eglCopyBuffers instead of workaround
    // with glReadPixels, but we should do additional checks to do it safely.

    // Let glReadPixels return the data in the following format: 
    const jint bytesPerPixel = 4;
    
    // for data type for glReadPixels we use GL_UNSIGNED_BYTE, one
    // byte for each component
    const jint alphaSize = 8;
    const jint redSize = 8;
    const jint greenSize = 8;
    const jint blueSize = 8;

    KNI_StartHandles(1);
    KNI_DeclareHandle(graphicsHandle);
    KNI_GetParameterAsObject(3, graphicsHandle);

    pixmap = JSR239_getImagePixmap(graphicsHandle, width, height,
                                   bytesPerPixel,
                                   alphaSize, redSize, greenSize, blueSize);

    if (!pixmap) {
        KNI_ThrowNew("java.lang.OutOfMemoryException", "eglCopyBuffers");
        goto exit;
    }

    // Enforce RGBA order of glReadPixels
    pixmap->aOffset = redSize + greenSize + blueSize;
    pixmap->bOffset = greenSize + blueSize;
    pixmap->gOffset = blueSize;
    pixmap->rOffset = 0;

    eglCopyBuffersWorkaround((EGLDisplay)display, (EGLSurface)surface, pixmap);

    // glReadPixels returns pixel data from the color buffer, starting with the
    // pixel whose lower left corner is at location (x, y), into client memory
    // starting at location pixels. Thus we should flip the data returned by
    // glReadPixels
    JSR239_putWindowContents(graphicsHandle, delta_height, 
                             pixmap, 
                             clipX, clipY, clipWidth, clipHeight,
                             1 /*should do flip*/);
    JSR239_destroyPixmap(pixmap);
    
 exit:
    KNI_EndHandles();
    KNI_ReturnInt(EGL_TRUE);
}

/*  private native int _eglSurfaceAttrib ( int display , int surface , int attribute , int value ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1eglSurfaceAttrib() {
    
    jint display = KNI_GetParameterAsInt(1);
    jint surface = KNI_GetParameterAsInt(2);
    jint attribute = KNI_GetParameterAsInt(3);
    jint value = KNI_GetParameterAsInt(4);

    jint returnValue = (jint)eglSurfaceAttrib((EGLDisplay)display,
					      (EGLSurface)surface,
					      (EGLint)attribute,
					      (EGLint)value);
#ifdef DEBUG
    printf("eglSurfaceAttrib(0x%x, 0x%x, %d, %d) = %d\n",
	   display, surface, attribute, value, returnValue);
#endif

    KNI_ReturnInt(returnValue);
}

/*  private native int _eglBindTexImage ( int display , int surface , int buffer ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1eglBindTexImage() {

    jint display = KNI_GetParameterAsInt(1);
    jint surface = KNI_GetParameterAsInt(2);
    jint buffer = KNI_GetParameterAsInt(3);

    jint returnValue = (jint)eglBindTexImage((EGLDisplay)display,
					     (EGLSurface)surface,
					     (EGLint)buffer);
#ifdef DEBUG
    printf("eglBindTexImage(0x%x, 0x%x, %d) = %d\n",
	   display, surface, buffer, returnValue);
#endif

    KNI_ReturnInt(returnValue);
}

/*  private native int _eglReleaseTexImage ( int display , int surface , int buffer ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1eglReleaseTexImage() {

    jint display = KNI_GetParameterAsInt(1);
    jint surface = KNI_GetParameterAsInt(2);
    jint buffer = KNI_GetParameterAsInt(3);

    jint returnValue = (jint)eglReleaseTexImage((EGLDisplay)display,
						(EGLSurface)surface,
						(EGLint)buffer);
#ifdef DEBUG
    printf("eglReleaseTexImage(0x%x, 0x%x, %d) = %d\n",
	   display, surface, buffer, returnValue);
#endif

    KNI_ReturnInt(returnValue);
}

/*  private native int _eglSwapInterval ( int display , int interval ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1eglSwapInterval() {

    jint display = KNI_GetParameterAsInt(1);
    jint interval = KNI_GetParameterAsInt(2);

    jint returnValue = (jint)eglSwapInterval((EGLDisplay)display,
					     (EGLint)interval);
#ifdef DEBUG
    printf("eglSwapInterval(0x%x, %d) = %d\n",
	   display, interval, returnValue);
#endif

    KNI_ReturnInt(returnValue);
}


/* private native int getWindowStrategy( Graphics winGraphics ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1getWindowStrategy() {

    jint returnValue;
    KNI_StartHandles(1);
    KNI_DeclareHandle(graphicsHandle);
    KNI_GetParameterAsObject(1, graphicsHandle);

    returnValue = JSR239_getWindowStrategy(graphicsHandle);
#ifdef DEBUG
    printf("JSR239_getWindowStrategy(0x%x) = %d\n",
	   graphicsHandle, returnValue);
#endif

    KNI_EndHandles();
    KNI_ReturnInt(returnValue);
}

/* private native int getWindowNativeID( Graphics winGraphics ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1getWindowNativeID() {

    jint returnValue;
    KNI_StartHandles(1);
    KNI_DeclareHandle(graphicsHandle);
    KNI_GetParameterAsObject(1, graphicsHandle);

    returnValue = JSR239_getWindowNativeID(graphicsHandle);
#ifdef DEBUG
    printf("JSR239_getWindowNativeID(0x%x) = %d\n",
	   graphicsHandle, returnValue);
#endif

    KNI_EndHandles();
    KNI_ReturnInt(returnValue);
}

/* private native int getWindowPixmap( int displayId , int configId , Graphics winGraphics , int width, int height, int transY ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1getWindowPixmap() {

    jint displayId = KNI_GetParameterAsInt(1);
    jint configId = KNI_GetParameterAsInt(2);
    jint width = KNI_GetParameterAsInt(4);
    jint height = KNI_GetParameterAsInt(5);
    JSR239_Pixmap *pixmap;
    jint returnValue;

    EGLDisplay display = (EGLDisplay)displayId;
    EGLConfig config = (EGLConfig)configId;
    EGLint redSize, greenSize, blueSize, alphaSize, bufferSize;

    KNI_StartHandles(1);
    KNI_DeclareHandle(graphicsHandle);
    KNI_GetParameterAsObject(3, graphicsHandle);
    
    eglGetConfigAttrib(display, config, EGL_RED_SIZE, &redSize);
    eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &greenSize);
    eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &blueSize);
    eglGetConfigAttrib(display, config, EGL_ALPHA_SIZE, &alphaSize);
    eglGetConfigAttrib(display, config, EGL_BUFFER_SIZE, &bufferSize);
  
    pixmap = JSR239_getWindowPixmap(graphicsHandle, width, height,
                                    (jint)((bufferSize + 7)/8),
                                    (jint)redSize,
                                    (jint)greenSize,
                                    (jint)blueSize,
                                    (jint)alphaSize);
    if (pixmap == 0) {
        KNI_ThrowNew("java/lang/OutOfMemoryError",
            "Error creating window pixmap");
    }
    returnValue = (jint) pixmap;

#ifdef DEBUG
    printf("JSR239_getWindowPixmap(0x%x, %d, %d, %d, %d, %d, %d) = 0x%x\n",
	   graphicsHandle, width, height,
           redSize, greenSize, blueSize, alphaSize,
           returnValue);
#endif

    KNI_EndHandles();
    KNI_ReturnInt(returnValue);
}

/* private native int destroyPixmap( int pixmapPtr ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_khronos_egl_EGL10Impl__1destroyPixmap() {

    jint pixmapPtr = KNI_GetParameterAsInt(1);

    JSR239_destroyPixmap((JSR239_Pixmap *)pixmapPtr);
#ifdef DEBUG
    printf("JSR239_destroyPixmap(0x%x)\n", pixmapPtr);
#endif

    KNI_ReturnVoid();
}

/* private native int getImagePixmap( int displayId , int configId , Graphics imageGraphics , int width , int height ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1getImagePixmap() {

    jint displayId = KNI_GetParameterAsInt(1);
    jint configId = KNI_GetParameterAsInt(2);
    jint width = KNI_GetParameterAsInt(4);
    jint height = KNI_GetParameterAsInt(5);
    jint returnValue;

    EGLDisplay display = (EGLDisplay)displayId;
    EGLConfig config = (EGLConfig)configId;
    EGLint redSize, greenSize, blueSize, alphaSize, bufferSize;

    KNI_StartHandles(1);
    KNI_DeclareHandle(graphicsHandle);
    KNI_GetParameterAsObject(3, graphicsHandle);

    eglGetConfigAttrib(display, config, EGL_RED_SIZE, &redSize);
    eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &greenSize);
    eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &blueSize);
    eglGetConfigAttrib(display, config, EGL_ALPHA_SIZE, &alphaSize);
    eglGetConfigAttrib(display, config, EGL_BUFFER_SIZE, &bufferSize);

    returnValue = (jint)JSR239_getImagePixmap(graphicsHandle, width, height,
                                              (jint)((bufferSize + 7)/8),
                                              (jint)redSize,
                                              (jint)greenSize,
                                              (jint)blueSize,
                                              (jint)alphaSize);
#ifdef DEBUG
    printf("JSR239_getImagePixmap(0x%x, %d, %d, %d, %d, %d, %d) = 0x%x\n",
	   graphicsHandle, width, height,
           redSize, greenSize, blueSize, alphaSize,
           returnValue);
#endif

    KNI_EndHandles();
    KNI_ReturnInt(returnValue);
}

/* private native void _getWindowContents( Graphics winGraphics , int pixmapPointer ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_khronos_egl_EGL10Impl__1getWindowContents() {

    jint pixmap = KNI_GetParameterAsInt(1);
    jint srcWidth = KNI_GetParameterAsInt(3);
    jint srcHeight = KNI_GetParameterAsInt(4);
    jint deltaHeight = KNI_GetParameterAsInt(5);

    KNI_StartHandles(1);
    KNI_DeclareHandle(srcGraphicsHandle);
    KNI_GetParameterAsObject(2, srcGraphicsHandle);

#ifdef DEBUG
    printf("JSR239_getWindowContents(0x%x) = 0x%x\n", 
	   graphicsHandle, pixmap);
#endif
    JSR239_getWindowContents((JSR239_Pixmap *)pixmap, srcGraphicsHandle, 
                             srcWidth, srcHeight,
                             deltaHeight);

    KNI_EndHandles();
    KNI_ReturnVoid();
}


/* private native void _putWindowContents( Graphics winGraphics , int pixmapPointer ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_khronos_egl_EGL10Impl__1putWindowContents() {

    jint deltaHeight = KNI_GetParameterAsInt(2);
    jint pixmap = KNI_GetParameterAsInt(3);
    jint clipX = KNI_GetParameterAsInt(4);
    jint clipY = KNI_GetParameterAsInt(5);
    jint clipWidth = KNI_GetParameterAsInt(6);
    jint clipHeight = KNI_GetParameterAsInt(7);

    KNI_StartHandles(1);
    KNI_DeclareHandle(graphicsHandle);
    KNI_GetParameterAsObject(1, graphicsHandle);

#ifdef DEBUG
    printf("JSR239_putWindowContents(0x%x, 0x%x)\n",
	   graphicsHandle, pixmap);
#endif
    JSR239_putWindowContents(graphicsHandle, deltaHeight,
        (JSR239_Pixmap *)pixmap, 
        clipX, clipY, clipWidth, clipHeight,
        0);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/*  private native int _getFullDisplayWidth () ;
    returns the full display width
*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1getFullDisplayWidth() {
    /* Revisit: multiple displays support. Obtain Id of display render surfane is
       bound to. Consider recalculations when display got changed */
    KNI_ReturnInt(lcdlf_get_screen_width(lcdlf_get_current_hardwareId()));
}

/*  private native int _getFullDisplayHeight () ;
    returns the full display height
*/
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_khronos_egl_EGL10Impl__1getFullDisplayHeight() {
    /* Revisit: multiple displays support. Obtain Id of display render surfane is
       bound to. Consider recalculations when display got changed */
    KNI_ReturnInt(lcdlf_get_screen_height(lcdlf_get_current_hardwareId()));
}

/**
 * Garbage collect any zombie Objects to free up their resources.
 * <p>
 * Java declaration:
 * <pre>
 *     garbageCollect(Z)V
 * </pre>
 *
 * @param fullGC boolean indicating whether to do a full GC or not
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_khronos_egl_EGL10Impl__1garbageCollect() {
    jboolean doFullGC = KNI_GetParameterAsBoolean(1);

    if (doFullGC == KNI_TRUE) {
        JVM_GarbageCollect(0, 0);
    } else {
        JVM_GarbageCollect(JVM_COLLECT_YOUNG_SPACE_ONLY, 0);
    }

    KNI_ReturnVoid();
}

/* native private void finalize();*/
/* Macro to retrieve C structure representation of an Object */
typedef struct Java_javax_microedition_khronos_egl_EGLSurfaceImpl 
	_eglsurface_impl;

#define GET_EGL_SURFACE(handle) (unhand(_eglsurface_impl,(handle)))

KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_khronos_egl_EGLSurfaceImpl_finalize() {
    _eglsurface_impl* surface;
    jint pixmap_pointer;

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);

    surface = GET_EGL_SURFACE(thisObject);
    pixmap_pointer = surface->pixmapPointer;

    JSR239_destroyPixmap((JSR239_Pixmap *)pixmap_pointer);

    KNI_EndHandles();
    KNI_ReturnVoid();
}
#undef GET_EGL_SURFACE

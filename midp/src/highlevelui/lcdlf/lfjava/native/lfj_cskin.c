/*
 *  
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

// This file contains the calls to use VM's resource sharing
// api. It holds a static shared object reference that can be
// retrieved by all isolates
#include <stdio.h>

#include <jvmconfig.h>
#include <kni.h>
#include <jvmspi.h>
#include <jvm.h>
#include <sni.h>
#include <ROMStructs.h>
#include <midpMidletSuiteUtils.h>
#include <lfj_image_rom.h>
#include <lcdlf_export.h>

// IMPL_NOTE : Initialize this constant each time the vm restarts!

#if ENABLE_MULTIPLE_ISOLATES    
static int resourcePool = -1;
static int skinProperties = -1;
#endif
 
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_chameleon_skins_resources_SkinResources_shareResourcePool) {

#if ENABLE_MULTIPLE_ISOLATES    
    KNI_StartHandles(1);
    KNI_DeclareHandle(obj);
    KNI_GetParameterAsObject(1, obj);
    
    if (resourcePool == -1) {
        // We only ever allow a single resource pool
        // to be stored. Any future attempts at storing
        // a new one are ignored
        resourcePool = SNI_AddStrongReference(obj);
    }
    
    KNI_EndHandles();                                                                                                
#endif
    KNI_ReturnVoid();

}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_chameleon_skins_resources_SkinResources_shareSkinData) {

#if ENABLE_MULTIPLE_ISOLATES    
    KNI_StartHandles(1);
    KNI_DeclareHandle(obj);
    KNI_GetParameterAsObject(1, obj);
    
    if (skinProperties == -1) {
        // We only ever allow a single resource pool
        // to be stored. Any future attempts at storing
        // a new one are ignored
        skinProperties = SNI_AddStrongReference(obj);
    }
    
    KNI_EndHandles();                                                                                                
#endif
    KNI_ReturnVoid();

}

KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_chameleon_skins_resources_SkinResources_getSharedResourcePool) {
    
#if ENABLE_MULTIPLE_ISOLATES
    KNI_StartHandles(1);
    KNI_DeclareHandle(obj);
    if (resourcePool >= 0) {
        SNI_GetReference(resourcePool, obj);
    }
    KNI_EndHandlesAndReturnObject(obj);
#else
    KNI_StartHandles(1);
    KNI_DeclareHandle(tempHandle);
    KNI_EndHandlesAndReturnObject(tempHandle);
#endif

}

KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_chameleon_skins_resources_SkinResources_getSharedSkinData) {
    
#if ENABLE_MULTIPLE_ISOLATES
    KNI_StartHandles(1);
    KNI_DeclareHandle(obj);
    if (skinProperties >= 0) {
        SNI_GetReference(skinProperties, obj);
    }
    KNI_EndHandlesAndReturnObject(obj);
#else
    KNI_StartHandles(1);
    KNI_DeclareHandle(tempHandle);
    KNI_EndHandlesAndReturnObject(tempHandle);
#endif

}
                                                                                                        
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_chameleon_skins_resources_SkinResources_ifLoadAllResources) {
    
#if ENABLE_MULTIPLE_ISOLATES
    // There is no need to load all images for non AMS
    // isolates, they are to be loaded later on demand
    if (JVM_CurrentIsolateID() == midpGetAmsIsolateId()) {
        KNI_ReturnBoolean(KNI_TRUE);
    } else {
        KNI_ReturnBoolean(KNI_FALSE);
    }
#else
    KNI_ReturnBoolean(KNI_FALSE);
#endif

}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_chameleon_skins_resources_LoadedSkinResources_finalize) {
#if ENABLE_MULTIPLE_ISOLATES
    if ((JVM_CurrentIsolateID() == midpGetAmsIsolateId()) &&
        (resourcePool >= 0)) {
        SNI_DeleteReference(resourcePool);
        resourcePool = -1;
    }
#endif
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_chameleon_skins_resources_LoadedSkinData_finalize) {
#if ENABLE_MULTIPLE_ISOLATES
    if ((JVM_CurrentIsolateID() == midpGetAmsIsolateId()) &&
        (skinProperties >= 0)) {
        SNI_DeleteReference(skinProperties);
        skinProperties = -1;
    }
#endif
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_chameleon_skins_resources_SkinResources_getRomizedImageDataArrayPtr) {
    jint imageId = KNI_GetParameterAsInt(1);
    
    unsigned char* imageData;
    lfj_load_image_from_rom(imageId, &imageData);

    KNI_ReturnInt((int)(void*)imageData);
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_chameleon_skins_resources_SkinResources_getRomizedImageDataArrayLength) {
    jint imageId = KNI_GetParameterAsInt(1);
    
    unsigned char* imageData;
    int len = lfj_load_image_from_rom(imageId, &imageData);

    KNI_ReturnInt(len);
}


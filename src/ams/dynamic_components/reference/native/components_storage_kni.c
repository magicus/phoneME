/*
 *
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

#include <string.h>

#include <midpError.h>
#include <midpUtilKni.h>

#include <suitestore_intern.h>
#include <suitestore_task_manager.h>
#include <suitestore_kni_util.h>

/**
 * Get the class path for the specified dynamic component.
 *
 * @param componentId unique ID of the component
 *
 * @return class path or null if the component does not exist
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_midletsuite_DynamicComponentStorage_getSuiteComponentJarPath) {
#if ENABLE_DYNAMIC_COMPONENTS
    ComponentIdType componentId;
    MIDPError status;
    pcsl_string classPath = PCSL_STRING_NULL;

    KNI_StartHandles(1);
    KNI_DeclareHandle(resultHandle);

    componentId = KNI_GetParameterAsInt(1);

    status = get_jar_path(COMPONENT_DYNAMIC, (jint)componentId,
                          &classPath);
    if (status != ALL_OK) {
        KNI_ThrowNew(midpRuntimeException, NULL);
        KNI_ReleaseHandle(resultHandle);
    } else {
        midp_jstring_from_pcsl_string(KNIPASSARGS &classPath, resultHandle);
        pcsl_string_free(&classPath);
    }

    KNI_EndHandlesAndReturnObject(resultHandle);
#else /*  ENABLE_DYNAMIC_COMPONENTS */
    KNI_StartHandles(1);
    KNI_DeclareHandle(resultHandle);
    KNI_ReleaseHandle(resultHandle);
    KNI_EndHandlesAndReturnObject(resultHandle);
#endif /*  ENABLE_DYNAMIC_COMPONENTS */
}

/**
 * Returns a unique identifier of a dynamic component.
 *
 * @return platform-specific id of the component
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_midletsuite_DynamicComponentStorage_createSuiteComponentID) {
    ComponentIdType componentId = UNUSED_COMPONENT_ID;

#if ENABLE_DYNAMIC_COMPONENTS
    MIDPError rc;

    rc = midp_create_component_id(&componentId);
    if (rc != ALL_OK) {
        if (rc == OUT_OF_MEMORY) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        } else {
            KNI_ThrowNew(midpIOException, NULL);
        }
    }

#endif /* ENABLE_DYNAMIC_COMPONENTS */

    KNI_ReturnInt(componentId);
}

/**
 * Returns the number of the installed components belonging to the given
 * MIDlet suite.
 *
 * @param suiteId ID of the MIDlet suite the information about whose
 *                components must be retrieved
 *
 * @return the number of components belonging to the given suite
 *         or -1 in case of error
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_midletsuite_DynamicComponentStorage_getNumberOfComponents) {
#if ENABLE_DYNAMIC_COMPONENTS
    int numberOfComponents;
    SuiteIdType suiteId = KNI_GetParameterAsInt(1);
    MIDPError status =
        midp_get_number_of_components(suiteId, &numberOfComponents);
    if (status != ALL_OK) {
        numberOfComponents = status;
    }

    KNI_ReturnInt(numberOfComponents);
#else  /* ENABLE_DYNAMIC_COMPONENTS */
    KNI_ReturnInt(0);
#endif /* ENABLE_DYNAMIC_COMPONENTS */
}

/*
 * Reads information about the installed midlet suite's component
 * from the storage.
 *
 * @param componentId unique ID of the component
 * @param ci ComponentInfo object to fill with the information about
 *           the midlet suite's component having the given ID
 *
 * @exception IOException if an the information cannot be read
 * @exception IllegalArgumentException if suiteId is invalid or ci is null
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(
    com_sun_midp_midletsuite_DynamicComponentStorage_getSuiteComponentInfoImpl0) {
#if ENABLE_DYNAMIC_COMPONENTS
    ComponentIdType componentId = KNI_GetParameterAsInt(1);
    MIDPError status = ALL_OK;

    KNI_StartHandles(3);
    KNI_DeclareHandle(componentInfoObject);
    KNI_DeclareHandle(componentInfoClass);
    KNI_DeclareHandle(tmpHandle);

    KNI_GetParameterAsObject(2, componentInfoObject);
    KNI_GetObjectClass(componentInfoObject, componentInfoClass);

    do {
        char *pszError = NULL;
        MidletSuiteData *pData = NULL;

        /* Ensure that suite data are read */
        status = read_suites_data(&pszError);
        storageFreeError(pszError);
        if (status != ALL_OK) {
            break;
        }

        pData = get_component_data(componentId);
        if (!pData) {
            status = NOT_FOUND;
            break;
        }

        KNI_RESTORE_INT_FIELD(componentInfoObject, componentInfoClass,
                              "componentId", componentId);
        KNI_RESTORE_INT_FIELD(componentInfoObject, componentInfoClass,
                              "suiteId", pData->suiteId);
        KNI_RESTORE_PCSL_STRING_FIELD(componentInfoObject, componentInfoClass,
                                     "displayName",
                                      &(pData->varSuiteData.displayName),
                                      tmpHandle);
        KNI_RESTORE_BOOLEAN_FIELD(componentInfoObject, componentInfoClass,
                                  "trusted", pData->isTrusted);
    } while (0);

    if (status != ALL_OK) {
        if (status == NOT_FOUND) {
            KNI_ThrowNew(midpIllegalArgumentException, "bad component ID");
        } else {
            KNI_ThrowNew(midpIOException, NULL);
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
#else /* ENABLE_DYNAMIC_COMPONENTS */
    KNI_ReturnVoid();
#endif /* ENABLE_DYNAMIC_COMPONENTS */
}

/**
 * Reads information about the installed midlet suite's components
 * from the storage.
 *
 * @param suiteId unique ID of the suite
 * @param ci array of ComponentInfo objects to fill with the information
 *           about the installed midlet suite's components
 *
 * @exception IOException if an the information cannot be read
 * @exception IllegalArgumentException if suiteId is invalid or ci is null
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_midletsuite_DynamicComponentStorage_getSuiteComponentsList) {
#if ENABLE_DYNAMIC_COMPONENTS
    SuiteIdType suiteId;
    int numberOfComponents = 0, arraySize;
    MidletSuiteData *pData = g_pSuitesData;

    KNI_StartHandles(4);
    KNI_DeclareHandle(components);
    KNI_DeclareHandle(componentObj);
    KNI_DeclareHandle(componentObjClass);
    KNI_DeclareHandle(tmpHandle);

    suiteId = KNI_GetParameterAsInt(1);
    KNI_GetParameterAsObject(2, components);

    arraySize = (int)KNI_GetArrayLength(components);

    do {
        if (arraySize <= 0) {
            break;
        }

        while (pData) {
            if (pData->suiteId == suiteId && pData->type == COMPONENT_DYNAMIC) {
                KNI_GetObjectArrayElement(components, (jint)numberOfComponents,
                                          componentObj);
                KNI_GetObjectClass(componentObj, componentObjClass);

                KNI_RESTORE_INT_FIELD(componentObj, componentObjClass,
                                      "componentId",
                                      (jint)pData->componentId);
                KNI_RESTORE_INT_FIELD(componentObj, componentObjClass,
                                      "suiteId",
                                      (jint)suiteId);
                KNI_RESTORE_BOOLEAN_FIELD(componentObj,
                                      componentObjClass,
                                      "trusted",
                                      pData->isTrusted);
                KNI_RESTORE_PCSL_STRING_FIELD(componentObj,
                                      componentObjClass,
                                      "displayName",
                                      &(pData->varSuiteData.displayName),
                                      tmpHandle);

                numberOfComponents++;
                if (numberOfComponents == arraySize) {
                    /* IMPL_NOTE: log an error! */
                    break;
                }
            }

            pData = pData->nextEntry;
        }
    } while (0);

    KNI_EndHandles();
    KNI_ReturnVoid();
#else /* ENABLE_DYNAMIC_COMPONENTS */
    KNI_ReturnVoid();
#endif /* ENABLE_DYNAMIC_COMPONENTS */
}

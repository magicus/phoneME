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
KNIDECL(com_sun_midp_midletsuite_DynamicComponentStorage_getComponentJarPath) {
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
}

/**
 * Returns a unique identifier of a dynamic component.
 *
 * @return platform-specific id of the component
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_midletsuite_DynamicComponentStorage_createComponentId) {
    ComponentIdType componentId = UNUSED_COMPONENT_ID;
    MIDPError rc;

    rc = midp_create_component_id(&componentId);
    if (rc != ALL_OK) {
        if (rc == OUT_OF_MEMORY) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        } else {
            KNI_ThrowNew(midpIOException, NULL);
        }
    }

    KNI_ReturnInt(componentId);
}

/**
 * Native method String getComponentId(String, String) of
 * com.sun.midp.midletsuite.DynamicComponentStorage.
 * <p>
 * Gets the unique identifier of the dynamic component defined by its
 * vendor and name.
 *
 * @param suiteId ID of the suite the component belongs to
 * @param vendor name of the vendor that created the component,
 *        as given in a JAD file
 * @param name name of the component, as given in a JAD file
 *
 * @return ID of the component given by vendor and name or
 *         ComponentInfo.UNUSED_COMPONENT_ID if the component
 *         does not exist
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_midletsuite_DynamicComponentStorage_getComponentId) {
    ComponentIdType componentId = UNUSED_COMPONENT_ID;
    MIDPError error;
    SuiteIdType suiteId = KNI_GetParameterAsInt(1);

    KNI_StartHandles(2);

    GET_PARAMETER_AS_PCSL_STRING(2, vendor)
    GET_PARAMETER_AS_PCSL_STRING(3, name)

    error = midp_get_component_id(suiteId, &vendor, &name, &componentId);

    switch(error) {
        case OUT_OF_MEMORY:
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        case SUITE_CORRUPTED_ERROR:
            KNI_ThrowNew(midpIOException, NULL);
            break;
        case NOT_FOUND:
            componentId = UNUSED_COMPONENT_ID;
            break;
        default:
            break;
    }

    RELEASE_PCSL_STRING_PARAMETER
    RELEASE_PCSL_STRING_PARAMETER

    KNI_EndHandles();

    KNI_ReturnInt(componentId);
}

/**
 * Removes the files belonging to the specified component, or to all
 * components owned by the given suite.
 *
 * @param suiteId ID of the suite whose components are being removed
 * @param componentId ID of the component to remove or UNUSED_COMPONENT_ID
 *                    if all components of the suite are being removed 
 *
 * @return status code, ALL_OK if no errors
 */
static MIDPError
delete_components_files(SuiteIdType suiteId, ComponentIdType componentId) {
    pcsl_string componentFileName;
    char* pszError;
    int suiteFound = 0;
    MIDPError status = ALL_OK;
    MidletSuiteData* pData = g_pSuitesData;

    /* handle the list entries having the given suiteId */
    while (pData != NULL) {
        if (pData->suiteId == suiteId) {
            suiteFound = 1;
            
            if (pData->type == COMPONENT_DYNAMIC &&
                    (componentId == UNUSED_COMPONENT_ID ||
                        pData->componentId == componentId)) {
                /* remove the file holding the component */
                status = get_jar_path(COMPONENT_DYNAMIC,
                    (jint)pData->componentId, &componentFileName);
                if (status != ALL_OK) {
                    break;
                }

                storage_delete_file(&pszError, &componentFileName);

                if (pszError != NULL) {
                    storageFreeError(pszError);
                    /* it's an error only if the file exists */
                    if (storage_file_exists(&componentFileName)) {
                        status = IO_ERROR;
                        pcsl_string_free(&componentFileName);
                        break;
                    }
                }

                pcsl_string_free(&componentFileName);
            }
        }

        pData = pData->nextEntry;
    }

    if (status == ALL_OK && suiteFound == 0) {
        /* suite doesn't exist */
        status = NOT_FOUND;
    }

    return status;
}

/**
 * Native method void removeComponent(int componentId) of
 * com.sun.midp.midletsuite.DynamicComponentStorage.
 * <p>
 * Removes a dynamic component given its ID.
 * <p>
 * If the component is in use it must continue to be available
 * to the other components that are using it.
 *
 * @param componentId ID of the component ot remove
 *
 * @throws IllegalArgumentException if the component cannot be found
 * @throws MIDletSuiteLockedException is thrown, if the component is
 *                                    locked
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_midletsuite_DynamicComponentStorage_removeComponent) {
    MIDPError status;
    MidletSuiteData* pData;
    ComponentIdType componentId = KNI_GetParameterAsInt(1);

    do {
        /* check that the component exists */
        pData = get_component_data(componentId);
        if (pData == NULL) {
            status = NOT_FOUND;
            break;
        }

        status = begin_transaction(TRANSACTION_REMOVE_COMPONENT,
                                   componentId, NULL);
        if (status != ALL_OK) {
            break;
        }

        status = delete_components_files(pData->suiteId, componentId);
        if (status != ALL_OK) {
            break;
        }

        (void)remove_from_component_list_and_save(pData->suiteId, componentId);
    } while(0);

    (void)finish_transaction();

    if (status == SUITE_LOCKED) {
        KNI_ThrowNew(midletsuiteLocked, NULL);
    } else if (status == BAD_PARAMS || status == NOT_FOUND) {
        KNI_ThrowNew(midpIllegalArgumentException, "bad component ID");
    } else if (status != ALL_OK) {
        KNI_ThrowNew(midpRuntimeException,
            (status == OUT_OF_MEMORY) ? "Out of memory!" : "Remove failed");
    }

    KNI_ReturnVoid();
}

/**
 * Native method void removeAllComponents(int suiteId) of
 * com.sun.midp.midletsuite.DynamicComponentStorage.
 * <p>
 * Removes all dynamic components belonging to the given suite.
 * <p>
 * If any component is in use, no components are removed, and
 * an exception is thrown.
 *
 * @param suiteId ID of the suite whose components must be removed
 *
 * @throws IllegalArgumentException if there is no suite with
 *                                  the specified ID
 * @throws MIDletSuiteLockedException is thrown, if any component is
 *                                    locked
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_midletsuite_DynamicComponentStorage_removeAllComponents) {
    MIDPError status;
    SuiteIdType suiteId = KNI_GetParameterAsInt(1);

    do {
        status = begin_transaction(TRANSACTION_REMOVE_ALL_COMPONENTS,
                                   suiteId, NULL);
        if (status != ALL_OK) {
            break;
        }

        status = delete_components_files(suiteId, UNUSED_COMPONENT_ID);
        if (status != ALL_OK) {
            break;
        }
        
        (void)remove_from_component_list_and_save(suiteId, UNUSED_COMPONENT_ID);
    } while(0);

    (void)finish_transaction();

    if (status == SUITE_LOCKED) {
        KNI_ThrowNew(midletsuiteLocked, NULL);
    } else if (status == BAD_PARAMS) {
        KNI_ThrowNew(midpIllegalArgumentException, "bad component ID");
    } else if (status != ALL_OK) {
        KNI_ThrowNew(midpRuntimeException,
            (status == OUT_OF_MEMORY) ? "Remove failed" : "Out of memory!");
    }

    KNI_ReturnVoid();
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
    int numberOfComponents;
    SuiteIdType suiteId = KNI_GetParameterAsInt(1);
    MIDPError status =
        midp_get_number_of_components(suiteId, &numberOfComponents);
    if (status != ALL_OK) {
        numberOfComponents = status;
    }

    KNI_ReturnInt(numberOfComponents);
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
    com_sun_midp_midletsuite_DynamicComponentStorage_getComponentInfo) {
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
        KNI_RESTORE_PCSL_STRING_FIELD(componentInfoObject, componentInfoClass,
                                     "version",
                                      &(pData->varSuiteData.suiteVersion),
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
                KNI_RESTORE_PCSL_STRING_FIELD(componentObj,
                                      componentObjClass,
                                      "version",
                                      &(pData->varSuiteData.suiteVersion),
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
}

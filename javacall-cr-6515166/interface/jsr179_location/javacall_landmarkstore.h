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

#ifndef __JAVACALL_LANDMARKSTORE_H
#define __JAVACALL_LANDMARKSTORE_H

/**
 * @defgroup JSR179 JSR179 Location API
 * @ingroup stack
 * Porting interface for native implementation Location API.
 * @{
 */
/** @} */
/**
 * @file javacall_landmarkstore.h
 * @ingroup LandmarkStore
 * @brief Javacall interfaces for JSR-179 Landmark Store
 */
 
#ifdef __cplusplus
extern "C" {
#endif

    
#include "javacall_defs.h" 

/**
 * @defgroup LandmarkStore LandmarkStore API 
 * @ingroup JSR179
 * This API covers accessing a database of known landmarks 
 * stored in the terminal.
 *
 * <b> Accessing a database of known landmarks of the terminal</b>
 *
 * You can use our internal implementation of landmark database because the JSR 179 specification only requires that 
 * a landmark database is shared among Java platform applications.  In this case, 
 * all landmark database APIs could be optional.
 * If the device provides a landmark database used by native application, 
 * the implementation is encouraged to use that database. 
 *
 * Optional features whose availability depends on the landmark store 
 * implementation of the terminal and it's possible relation to landmark 
 * stores shared with native applications:
 *  - creating and deleting landmark stores
 *  - adding and removing landmark categories
 *
 * The porting layer includes:
 *  - Gets landmark Stores
 *   - javacall_landmarkstore_list_open
 *   - javacall_landmarkstore_list_close
 *   - javacall_landmarkstore_list_next
 *  - Gets landmarks from native landmark store
 *   - javacall_landmarkstore_landmarklist_open
 *   - javacall_landmarkstore_landmarklist_close
 *   - javacall_landmarkstore_landmarklist_next
 *  - Gets categories from native landmark store
 *   - javacall_landmarkstore_categorylist_open
 *   - javacall_landmarkstore_categorylist_close
 *   - javacall_landmarkstore_categorylist_next
 *  - Adds/deletes a landmark to landmark store or category
 *   - javacall_landmarkstore_landmark_add_to_landmarkstore
 *   - javacall_landmarkstore_landmark_add_to_category
 *   - javacall_landmarkstore_landmark_delete_from_landmarkstore
 *   - javacall_landmarkstore_landmark_delete_from_category
 *  - Creates/deletes a landmark store or category - Optional
 *   - javacall_landmarkstore_create
 *   - javacall_landmarkstore_delete
 *   - javacall_landmarkstore_category_add
 *   - javacall_landmarkstore_category_delete
 *
 * <b> Implementation Notes </b>
 *
 *  - unicode string representation
 *
 * All unicode strings used in this API should be NULL terminated.
 *
 *  - buffer allocation
 *
 * The buffer for output parameter should be allocated by the caller. 
 * There should be a parameter for the size of the buffer if there is 
 * no predefined maximum size. 
 *
 * The exceptional cases are enumeration functions like *list_next(). 
 * Platform will allocate memory and return its pointer in this case.  
 * The allocated memory will be valid until the enumberation is closed.
 * \par
 *
 */

/******************************************************************************
 ******************************************************************************
 ******************************************************************************
    OPTIONAL FUNCTIONS
 ******************************************************************************
 ******************************************************************************
 ******************************************************************************/
    
/**
 * @defgroup jsrOptionalLandmarkStore Optional LandmarkStore API
 * @ingroup LandmarkStore
 * 
 * @{
 */

#define JAVACALL_LANDMARKSTORE_MAX_LANDMARK_NAME         (32 +1)
#define JAVACALL_LANDMARKSTORE_MAX_CATEGORY_NAME         (32 +1)
#define JAVACALL_LANDMARKSTORE_MAX_LANDMARK_DESCRIPTION  (256 +1)

/**
 * @def SIZE_OF_LANDMARK_INFO( NumberOfAddressInfoFields )
 * 
 * Calculates the size of a landmark information considering the number 
 * of address info fields.
 * This macro should be used instead of sizeof(javacall_landmarkstore_landmark) 
 * because the structure size will be varing with the number of address 
 * fields.
 */
#define SIZE_OF_LANDMARK_INFO(NumberOfAddressInfoFields) ( (int)(&((javacall_landmarkstore_landmark*)(NULL))->fields) + sizeof(javacall_location_addressinfo_fieldinfo)*NumberOfAddressInfoFields )

/**
 * struct javacall_landmarkstore_landmark
 * 
 * @brief Landmark info
 *
 * Description: 
 *    name, description and addressInfo fields are NULL terminated strings
 *    to indicate NULL string the field should contain 0x0000 in the first symbol 
 *    and 0xFFFF in the second symbol
 *    to indicate EMPTY string the field should contain 0x0000 in the first symbol 
 *    and second symbol
 */
typedef struct {
    /** name of landmark, minimum 32 characters */
    javacall_utf16 name[JAVACALL_LANDMARKSTORE_MAX_LANDMARK_NAME];
    /** description of landmark, maximum 256 characters */
    javacall_utf16 description[JAVACALL_LANDMARKSTORE_MAX_LANDMARK_DESCRIPTION];
    /** 
     * Flag indicating whether a coordinates are present or not
     */     
    javacall_bool isValidCoordinate;
    /** latitude in [-90.0, 90,0] */
    double latitude;            
    /** longitude in [-180.0, 180,0) */
    double longitude;
    /** defined as height in meters above the WGS84 ellipsoid, JAVACALL_LOCATION_FLOAT_NAN if unknown */
    float altitude;             
    /** in meters(1-sigma standard deviation), JAVACALL_LOCATION_FLOAT_NAN if unknown */
    float horizontalAccuracy;   
    /** in meters(1-sigma standard deviation), JAVACALL_LOCATION_FLOAT_NAN if unknown */
    float verticalAccuracy;     
    /** number of fields which have value, zero if none */
    int addressInfoFieldNumber;
    /** array of field value */
    javacall_location_addressinfo_fieldinfo fields[1]; 
} javacall_landmarkstore_landmark;


/**
 * Adds a landmark to a landmark store.
 *
 * @param landmarkStoreName where the landmark will be added
 * @param landmark to add
 * @param categoryName where the landmark will belong to, NULL implies that the landmark does not belong to any category.
 * @param outLandmarkID returned id of added landmark
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        on error
 * @retval JAVACALL_INVALID_ARGUMENT  if the category name is invalid or the landmark has a longer name field than the implementation can support.
 */
javacall_result javacall_landmarkstore_landmark_add_to_landmarkstore(
        const javacall_utf16_string landmarkStoreName, 
        const javacall_landmarkstore_landmark* landmark,
        const javacall_utf16_string categoryName,
        javacall_handle* /*OUT*/outLandmarkID);

/**
 * Adds a landmark to a category.
 *
 * @param landmarkStoreName where this landmark belongs
 * @param landmarkID landmark id to add 
 * @param categoryName which the landmark will be added to
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        on error
 * @retval JAVACALL_INVALID_ARGUMENT  if the category name is invalid
 */
javacall_result javacall_landmarkstore_landmark_add_to_category(
        const javacall_utf16_string landmarkStoreName, 
        javacall_handle landmarkID,
        const javacall_utf16_string categoryName);

/**
 * Update existing landmark in the landmark store.
 *
 * @param landmarkStoreName where this landmark belongs
 * @param landmarkID landmark id to update 
 * @param landmark to update
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        on error
 * @retval JAVACALL_INVALID_ARGUMENT  if the landmarkID is invalid
 */
javacall_result javacall_landmarkstore_landmark_update(
        const javacall_utf16_string landmarkStoreName, 
        javacall_handle landmarkID,
        const javacall_landmarkstore_landmark* landmark);

/**
 * Deletes a landmark from a landmark store.
 *
 * This function removes the specified landmark from all categories and deletes the information from this landmark store.
 * If the specified landmark does not belong to this landmark store, then the request is silently ignored and this function call returns without error.
 *
 * @param landmarkStoreName where this landmark belongs
 * @param landmarkID        id of a landmark to delete
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        on error
 */
javacall_result javacall_landmarkstore_landmark_delete_from_landmarkstore(
        const javacall_utf16_string landmarkStoreName,
        javacall_handle landmarkID);
        
/**
 * Deletes a landmark from a category.
 *
 * If the specified landmark does not belong to the specified landmark store or category, then the request is silently ignored and this function call returns without error.
 *
 * @param landmarkStoreName where this landmark belongs
 * @param landmarkID id of a landmark to delete
 * @param categoryName from which the landmark to be removed
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        on error
 */
javacall_result javacall_landmarkstore_landmark_delete_from_category(
        const javacall_utf16_string landmarkStoreName,
        javacall_handle landmarkID,
        const javacall_utf16_string categoryName);

/**
 * Gets a handle for LandmarkStore list.
 *
 * @param pHandle that can be used in javacall_landmarkstore_list_next
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        on other error
 */
javacall_result javacall_landmarkstore_list_open(
        javacall_handle* /*OUT*/pHandle);

/**
 * Closes the specified landmarkstore list. 
 *
 * This handle will no longer be associated with this landmarkstore list.
 *
 * @param handle that is returned by javacall_landmarkstore_list_open
 *
 */
void javacall_landmarkstore_list_close(
        javacall_handle handle);

/**
 * Returns the next landmark store name
 *
 * Assumes that the returned landmarkstore memory block is valid until the next this function call
 *
 * @param handle of landmark store
 * @param pLandmarkStore pointer to UNICODE string for the next landmark store name on success, NULL otherwise
 *      returned param is a pointer to platform specific memory block.
 *      platform MUST BE responsible for allocating and freeing it.
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        on other error
 */
javacall_result javacall_landmarkstore_list_next(
        javacall_handle handle,
        javacall_utf16_string* /*OUT*/pLandmarkStore);


/**
 * Gets a handle for Landmark list.
 *
 * @param landmarkStoreName landmark store to get the landmark from
 * @param categoryName of the landmark to get, NULL implies a wildcard that matches all categories.
 * @param pHandle that can be used in javacall_landmarkstore_list_next
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_INVALID_ARGUMENT  if the category or landmarkStore name is invalid 
 * @retval JAVACALL_FAIL        on other error
 */
javacall_result javacall_landmarkstore_landmarklist_open(
        const javacall_utf16_string landmarkStoreName, 
        const javacall_utf16_string categoryName, 
        javacall_handle* /*OUT*/pHandle);

/**
 * Closes the specified landmark list. 
 *
 * This handle will no longer be associated with this landmark list.
 *
 * @param handle that is returned by javacall_landmarkstore_landmarklist_open
 *
 */
void javacall_landmarkstore_landmarklist_close(
        javacall_handle handle);

/**
 * Returns the next landmark from a landmark store.
 *
 * Assumes that the returned landmark memory block is valid until the next this function call
 *
 * @param handle of landmark store
 * @param pLandmarkID id of returned landmark.
 * @param pLandmark pointer to landmark on sucess, NULL otherwise
 *      returned param is a pointer to platform specific memory block.
 *      platform MUST BE responsible for allocating and freeing it.
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        on other error
 */
javacall_result javacall_landmarkstore_landmarklist_next(
        javacall_handle handle,
        int* /*OUT*/pLandmarkID,
        javacall_landmarkstore_landmark** /*OUT*/pLandmark);

/**
 * Gets a handle to get Category list in specified landmark store.
 *
 * @param landmarkStoreName landmark store to get the categories from
 * @param pHandle that can be used in javacall_landmarkstore_categorylist_next
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        the other error
 * @retval JAVACALL_INVALID_ARGUMENT  if the landmarkStore name is invalid 
 */
javacall_result javacall_landmarkstore_categorylist_open(
        const javacall_utf16_string landmarkStoreName,
        javacall_handle* /*OUT*/pHandle);

/**
 * Closes the specified category list. 
 * This handle will no longer be associated with this category list.
 *
 * @param handle that is returned by javacall_landmarkstore_categorylist_open
 *
 */
void javacall_landmarkstore_categorylist_close(
        javacall_handle handle);

/**
 * Returns the next category name in specified landmark store.
 *
 * Assumes that the returned landmark memory block is valid until the next this function call
 *
 * @param handle of landmark store
 * @param pCategoryName pointer to UNICODE string for the next category name on success, NULL otherwise
 *      returned param is a pointer to platform specific memory block.
 *      platform MUST BE responsible for allocating and freeing it.
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        the other error
 *
 */
javacall_result javacall_landmarkstore_categorylist_next(
        javacall_handle handle,
        javacall_utf16_string* /*OUT*/pCategoryName);


/**
 * Creates a landmark store.
 *
 * If the implementation supports several storage media, this function may e.g. prompt the end user to make the choice.
 *
 * @param landmarkStoreName name of a landmark store.
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        the other error
 * @retval JAVACALL_INVALID_ARGUMENT   if the name is too long or if a landmark store with specified name already exists.
 */
javacall_result /*OPTIONAL*/ javacall_landmarkstore_create(
        const javacall_utf16_string landmarkStoreName);

/**
 * Deletes a landmark store.
 *
 * All the landmarks and categories defined in named landmark store are removed.
 * If a landmark store with the specified name does not exist, this function returns silently without any error.
 *
 * @param landmarkStoreName name of a landmark store.
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        the other error
 * @retval JAVACALL_INVALID_ARGUMENT   if the name is too long 
 */
javacall_result /*OPTIONAL*/ javacall_landmarkstore_delete(
        const javacall_utf16_string landmarkStoreName);

/**
 * Adds a category to a landmark store.
 *
 * This function must support category name that have length up to and 
 * including 32 chracters.
 *
 * @param landmarkStoreName the name of the landmark store.
 * @param categoryName category name - UNICODE string
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        the other error
 * @retval JAVACALL_INVALID_ARGUMENT   if a category name already exists
 */
javacall_result /*OPTIONAL*/ javacall_landmarkstore_category_add(
        const javacall_utf16_string landmarkStoreName,
        const javacall_utf16_string categoryName);

/**
 * Removes a category from a landmark store.
 *
 * The category will be removed from all landmarks that are in that category. However, this function will not remove any of the landmarks. Only the associated category information from the landmarks are removed.
 * If a category with the supplied name does not exist in the specified landmark store, this function returns silently without error.
 *
 * @param landmarkStoreName the name of the landmark store.
 * @param categoryName category name - UNICODE string
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        the other error
 */
javacall_result /*OPTIONAL*/ javacall_landmarkstore_category_delete(
        const javacall_utf16_string landmarkStoreName,
        const javacall_utf16_string categoryName);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __JAVACALL_LANDMARKSTORE_H */



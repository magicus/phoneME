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

#include "javacall_location.h"
#include "javacall_landmarkstore.h"

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
        javacall_handle* /*OUT*/outLandmarkID) {

    return JAVACALL_FAIL;
}

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
        const javacall_utf16_string categoryName) {

    return JAVACALL_FAIL;
}

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
        const javacall_landmarkstore_landmark* landmark) {

    return JAVACALL_FAIL;
}

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
        javacall_handle landmarkID) {

    return JAVACALL_FAIL;
}
        
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
        const javacall_utf16_string categoryName) {

    return JAVACALL_FAIL;
}

/**
 * Gets a handle for Landmark list.
 *
 * @param landmarkStoreName landmark store to get the landmark from
 * @param categoryName of the landmark to get, NULL implies a wildcard that matches all categories.
 * @param pHandle that can be used in javacall_landmarkstore_landmarklist_next
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_INVALID_ARGUMENT  if the category name is invalid 
 * @retval JAVACALL_FAIL        on other error
 */
javacall_result javacall_landmarkstore_landmarklist_open(
        const javacall_utf16_string landmarkStoreName, 
        const javacall_utf16_string categoryName, 
        javacall_handle* /*OUT*/pHandle) {

    return JAVACALL_FAIL;
}

/**
 * Closes the specified landmark list. 
 *
 * This handle will no longer be associated with this landmark list.
 *
 * @param handle that is returned by javacall_landmarkstore_landmarklist_open
 *
 */
void javacall_landmarkstore_landmarklist_close(
                  javacall_handle handle) {
}

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
        javacall_landmarkstore_landmark** /*OUT*/pLandmark) {

    return JAVACALL_FAIL;
}

/**
 * Gets a handle to get Category list in specified landmark store.
 *
 * @param landmarkStoreName landmark store to get the categories from
 * @param pHandle that can be used in javacall_landmarkstore_categorylist_next
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        the other error
 */
javacall_result javacall_landmarkstore_categorylist_open(
        const javacall_utf16_string landmarkStoreName,
        javacall_handle* /*OUT*/pHandle){
    return JAVACALL_FAIL;
}

/**
 * Closes the specified category list. 
 * This handle will no longer be associated with this category list.
 *
 * @param handle that is returned by javacall_landmarkstore_categorylist_open
 *
 */
void javacall_landmarkstore_categorylist_close(
          javacall_handle handle) {
}


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
        javacall_utf16_string* /*OUT*/pCategoryName) {

    return JAVACALL_FAIL;
}

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
                        const javacall_utf16_string landmarkStoreName) {
    return JAVACALL_FAIL;
}

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
                        const javacall_utf16_string landmarkStoreName) {
    return JAVACALL_FAIL;
}

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
        const javacall_utf16_string categoryName) {

    return JAVACALL_FAIL;
}

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
        const javacall_utf16_string categoryName) {

    return JAVACALL_FAIL;
}

/**
 * Gets a handle for LandmarkStore list.
 *
 * @param pHandle that can be used in javacall_landmarkstore_list_next
 * @param pDefLandmarkStore pointer to UNICODE string for the default LandmarkStore name.
 *      default LandmarkStore name can not be NULL
 *      returned param is a pointer to platform specific memory block.
 *      platform MUST BE responsible for allocating and freeing it.
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        on other error
 */
javacall_result javacall_landmarkstore_list_open(
        javacall_handle* /*OUT*/pHandle) {
    
    return JAVACALL_FAIL;
}

/**
 * Closes the specified landmarkstore list. 
 *
 * This handle will no longer be associated with this landmarkstore list.
 *
 * @param handle that is returned by javacall_landmarkstore_list_open
 *
 */
void javacall_landmarkstore_list_close(
                  javacall_handle handle) {
}

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
        javacall_utf16_string* /*OUT*/pLandmarkStore) {

    return JAVACALL_FAIL;
}

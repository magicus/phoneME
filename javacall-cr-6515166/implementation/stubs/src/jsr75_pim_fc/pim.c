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

#include "javacall_pim.h"
#include "javacall_logging.h"
#include "javacall_file.h"
#include "javacall_dir.h"


/* FOR DEBUG */
javacall_result javacall_dir_get_root_path(javacall_utf16*, int *);


/**
 * Return a comma seperated list that contains the names of PIM list
 * the matches the given listType ("Contact" Or "JohnContact,SuziContact")
 *
 * @param listType The pim list type the user wish to obtain
 * @param pimList Pointer to where to store the returned list
 * @param pimListLen The length of the pim list
 *
 * @retval JAVACALL_OK  on sucess   
 * @retval JAVACALL_FAIL  when no list exists or when the buffer size is too small 
 */
javacall_result javacall_pim_get_lists(javacall_pim_type listType,
                                       javacall_utf16 /*OUT*/ *pimList,
                                       int pimListLen) {
    return JAVACALL_FAIL;
}

/**
 * Check to see if a given PIM list type is supported by the platform
 * 
 * @return JAVACALL_TRUE if the list type is supported,
 *         JAVACALL_FALSE otherwise.
 */
javacall_bool javacall_pim_list_is_supported_type(javacall_pim_type listType) {
    return JAVACALL_FALSE;
}

/**
 * Open the request pim list in the given mode.
 *
 * @param listType The pim list type to open
 * @param pimList the name of the list to open , if null the default list will be opened
 * @param mode the open mode for the list
 * @param listHandle a pointer to where to store the listHandle.
 *
 * @retval JAVACALL_OK  on sucess   
 * @retval JAVACALL_INVALID_ARGUMENT  If an invalid mode is provided as a parameter or 
                                      if pimListType is not a valid PIM list type.
 * @retval JAVACALL_FAIL  on other error
 */
javacall_result javacall_pim_list_open(javacall_pim_type listType,
                                       javacall_utf16 *pimList,
                                       javacall_pim_open_mode mode,
                                       javacall_handle *listHandle) {   
      return JAVACALL_FAIL;
}

/**
 * Close the opened pim list
 *
 * @param listHandle a handle of the list to close.
 *
 * @retval JAVACALL_OK  on sucess   
 * @retval JAVACALL_FAIL  in case the list is no longer accessible.
 */
javacall_result javacall_pim_list_close(javacall_handle listHandle) {
     return JAVACALL_FAIL;
}

/**
 * Return the next item in the given pim list 
 * For Contact item the item will be in vCard 2.1 / 3.0 format
 * For Event Todo item the item will be in vCalendar 1.0 format
 *
 * @param listHandle a handle of the list the get the item from.
 * @param item a pointer to where to store the item ,NULL otherwise.
 * @param maxItemLen the maximum size of the item.
 * @param categories a pointer to where to store the item's categories 
 *                   seperated by comma,NULL otherwise.
 * @param maxCategoriesLen the maximum size of the categories buffer.
 * @param itemHandle a pointer to where to store a unique identifier 
 *                   for the returned item.
 *
 * @retval JAVACALL_OK  on sucess
 * @retval JAVACALL_INVALID_ARGUMENT  maxItemLen is too small 
 * @retval JAVACALL_FAIL  in case of reaching the last item in the list
 */
javacall_result javacall_pim_list_get_next_item(javacall_handle listHandle,
                                                unsigned char *item,
                                                int maxItemLen,
                                                javacall_utf16 *categories,
                                                int maxCategoriesLen,
                                                javacall_handle *itemHandle) {
    return JAVACALL_FAIL;
}

/**
 * Modify an item
 * For Contact item the item will be in vCard 2.1 / 3.0 format
 * For Event Todo item the item will be in vCalendar 1.0 format
 *
 * @param itemHandle a handle of the item to modify.
 * @param item a pointer to the item to add to the list
 *
 * @retval JAVACALL_OK  on success   
 * @retval JAVACALL_FAIL  in case of an error
 */
javacall_result javacall_pim_list_modify_item(javacall_handle listHandle,
                                              javacall_handle itemHandle,
                                              const unsigned char *item,
                                              const javacall_utf16 *categories) {
     return JAVACALL_FAIL;
}

/**
 * Add a new item to the given item list
 * For Contact item the item will be in vCard 2.1 / 3.0 format
 * For Event Todo item the item will be in vCalendar 1.0 format
 *
 * @param listHandle a handle of the list to add the new item to.
 * @param item a pointer to the item to add to the list
 * @param categories a pointer to the item's categories seperate by a comma
 * @param itemHandle a pointer to where to store a unique identifier 
 *                   for the new item.
 *
 * @retval JAVACALL_OK  on sucess   
 * @retval JAVACALL_FAIL  in case of an error
 */
javacall_result javacall_pim_list_add_item(javacall_handle listHandle,
                                           const unsigned char *item,
                                           const javacall_utf16 *categories,
                                           javacall_handle *itemHandle) {
    return JAVACALL_FAIL;
}

/**
 * Remove an item from the list
 *
 * @param listHandle a handle of the list to delete the item from.
 * @param itemHandle the item 
 *
 *
 * @retval JAVACALL_OK  on sucess   
 * @retval JAVACALL_INVALID_ARGUMENT  maxItemLen is too small 
 * @retval JAVACALL_FAIL  in case of reaching the last item in the list
 */
javacall_result javacall_pim_list_remove_item(javacall_handle listHandle,
                                              javacall_handle itemHandle) {
      return JAVACALL_FAIL;
}

/**
 * Add the provided category to the PIM list. If the given category already exists 
 * for the list, the method does not add another category and considers that this 
 * method call is successful and returns.
 *
 * The category names are case sensitive in this API, but not necessarily in the 
 * underlying implementation. For example, "Work" and "WORK" map to the same 
 * underlying category if the platform's implementation of categories is case-insensitive; 
 * adding both separately would result in only one category being created in this case.
 *
 * A string with no characters ("") may or may not be a valid category on a particular platform. 
 * If the string is not a valid category as defined by the platform, JAVACALL_FAIL is returned
 * when trying to add it. 
 *
 * @param listHandle a handle of the list to add the new category to.
 * @param categoryName the name of the categroy to be added
 *
 * @retval JAVACALL_OK  on sucess   
 * @retval JAVACALL_FAIL   If categories are unsupported, an error occurs, 
 *                         or the list is no longer accessible or closed.
 */
javacall_result javacall_pim_list_add_category(javacall_handle listHandle,
                                               javacall_utf16 *categoryName) {
    return JAVACALL_FAIL;
}

/**
 * Remove the indicated category from the PIM list. If the indicated category is 
 * not in the PIM list, this method is treated as successfully completing.
 * The category names are case sensitive in this API, but not necessarily in the 
 * underlying implementation. For example, "Work" and "WORK" map to the same underlying 
 * category if the platform's implementation of categories is case-insensitive; 
 * removing both separately would result in only one category being removed in this case. 
 *
 * @param listHandle a handle of the list to remove the new category from.
 * @param categoryName the name of the categroy to be removed
 *
 * @retval JAVACALL_OK  on sucess   
 * @retval JAVACALL_FAIL   If categories are unsupported, an error occurs, 
 *                         or the list is no longer accessible or closed.
 */
javacall_result javacall_pim_list_remove_category(javacall_handle listHandle,
                                                  javacall_utf16 *categoryName) {
    return JAVACALL_FAIL;
}

/**
 * Rename a category from an old name to a new name. All items associated with 
 * the old category name are changed to reference the new category name after 
 * this method is invoked. If the new category name is already an existing category, 
 * then the items associated with the old category name are associated with the existing category.
 * A string with no characters ("") may or may not be a valid category on a particular platform. 
 * If the string is not a category on a platform, a JAVACALL_FAIL should returned when trying 
 * to rename a category to it. 
 *
 * @param listHandle a handle of the list to remove the new category from.
 * @param oldCategoryName the old category name 
 * @param newCategoryName the new category name 
 *
 * @retval JAVACALL_OK  on sucess   
 * @retval JAVACALL_FAIL   in case of an error
 */
javacall_result javacall_pim_list_rename_category(javacall_handle listHandle,
                                                  javacall_utf16 *oldCategoryName,
                                                  javacall_utf16 *newCategoryName) {
    return JAVACALL_FAIL;
}

/**
 * Return the maximum number of categories that this list can have.
 *
 * @param listHandle a handle of the list the get the number from.
 *
 * @retval -1 - ndicates there is no limit the the number of categories that 
 *              this list can have.
 * @retval 0  - indicates no category support and
 * @retval 0 > - in case of a limitation.
 */
int javacall_pim_list_max_categories(javacall_handle listHandle) {
    return -1;
}

/**
 * Return the maximum number of categories this item can be assigned to.
 *
 * @param listHandle a handle to the list from which to the get the number
 * @retval -1 - indicates there is no limit to the number of categories 
 *              this item can be assigned to
 * @retval 0  - indicates no category support
 * @retval >0 - in case a limit exists
 */
int javacall_pim_list_max_categories_per_item(javacall_handle listHandle) {
    return -1;
}

/**
 * Return the categories defined for the PIM list in comma seperate format
 * ("Work,HOME,Friends"). 
 * If there are no categories defined for the PIM list or categories are 
 * unsupported for the list, then JAVACALL_FAIL should be returned
 *
 * @param listHandle a handle of the list the get the item from.
 * @param categoriesName a pointer to where to store the categories to
 * @param maxCategoriesLen the maximum size of the categoriesName.
  *
 * @retval JAVACALL_OK  on sucess   
 * @retval JAVACALL_FAIL  in case no categories found or incase of an error.
 */
javacall_result javacall_pim_list_get_categories(javacall_handle listHandle,
                                                 javacall_utf16 *categoriesName,
                                                 int maxCategoriesLen) {
    return JAVACALL_FAIL;
}

/**
 * Get all fields that are supported in this list. 
 * All fields supported by this list, including both standard 
 * and extended, are returned in this array.
 *
 * in order to identify field, field attributes , field array element
 * that aren't in use the JAVACALL_PIM_INVALID_ID should be set for the
 * member that aren't in use.
 *
 * @param listHandle a handle of the list to get the fields from.
 * @param fields a pointer to where to store the fields to.
 * @param maxItemLen the maximum fields the field buffer can hold.
 *
 * @retval JAVACALL_OK  on success   
 * @retval JAVACALL_FAIL  on any error
 */
javacall_result javacall_pim_list_get_fields(javacall_handle listHandle,
                                             javacall_pim_field *fields,
                                             int maxFields) {
    return JAVACALL_FAIL;
}

/**
 * Get all attributes supported by the list.
 *
 * @retval JAVACALL_OK  on success   
 * @retval JAVACALL_FAIL  on any error
 */
javacall_result javacall_pim_list_get_attributes(javacall_handle listHandle,
                                                 javacall_pim_field_attribute *attributes,
                                                 int maxAttributes) {
    return JAVACALL_FAIL;
}


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
 
#ifndef __JAVACALL_PIM_H
#define __JAVACALL_PIM_H

/**
 * @file javacall_pim.h
 * @ingroup JSR75PIM
 * @brief Javacall interfaces for JSR-75 PIM API
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_defs.h"

/**
 * @defgroup JSR75PIM JSR75 PIM API
 *
 * This API provide porting layer APIs to access the handset address book, 
 * calendar and todo list.
 * 
 * Compliant PIM API implementations:
 *   - MUST include all packages, classes, and interfaces described in this
 *     specification.
 *   - MUST provide support for at least one of the types of PIM lists defined
 *     in this specification
 *   - API: either ContactList, EventList, or ToDoList.
 *   - MUST provide access to one or more actual PIM lists for each supported
 *     PIM list type.
 *   - MAY provide support for all types of PIM lists defined in this API: 
 *     ContactList, EventList, and ToDoList.
 *   - MUST provide a security model for accessing the PIM APIs.
 *   - MUST support at least vCard 2.1 and vCalendar 1.0 data formats for 
 *     importing and exporting items via serial formats mechanism defined 
 *     in javax.microedition.pim.PIM class. Implementations MUST support 
 *     Quoted-Printable and BASE64 encoding formats in import and export.
 *   - MUST support at least the UTF-8 character encoding for APIs that allow
 *     the application to define character encodings.
 *   - MAY support other character encodings.
 *
 * The following functionality is provided by the porting layer:
 * 1.PIM List access:
 * Using the javacall_pim_get_lists() user can retrieve the list of supported
 * PIM list and according to that to access the relevant PIM list using the 
 * javacall_pim_list_open() and to close the PIM list using the 
 * javacall_pim_list_close().
 *
 * 2.Getting PIM items:
 * In order to retrieve the items from the PIM list that user need to open the
 * relevant PIM list using javacall_pim_list_open() with the return list handle
 * the user would than have to call javacall_pim_list_get_next_item() in order
 * to retrieve the next item in the list. 
 * Note: javacall_pim_list_get_next_item() returns the following information
 *       for each item:
 *        - item's data - in vCard/vCalender format.
 *        - a list of categories that the current item is bind to.
 *        - a item handle which is a unique identifier to the item.
 * 
 * 3.Adding new items: 
 * Adding new items to the list also require to open the list first. 
 * With the returned list handle the user will than need to call 
 * javacall_pim_list_add_item() with the following information:
 *   - item's data - in vCard/vCalender format.
 *   - a list of categories that the current item is bind to.
 * upon successful completion the function will return a unique identifier
 * to detect the new item.
 *
 * 4.Removing existing items: 
 * Existing item removal requires to open the PIM list first and with the 
 * return handle to call javacall_pim_list_remove_item with both the PIM 
 * list and the item handles.
 *
 * 5.Getting field information:
 * Field information is retrieve using the javacall_pim_list_get_fields 
 * which returns the following information about each field:
 *   - Field ID - A unique identifier for a field that matches one 
 *     of the values of (javacall_pim_contact_field, javacall_pim_event_field,
 *     javacall_pim_todo)
 *   - Field Type - The storage type of the field us defined in
 *     (javacall_pim_field_type).
 *   - Field Label - String label associated with the given field. 
 *   - Field Attributes - For contact field only all the field's attributes
 *     will be returned. 
 *   - Field Array element - For contact field only of type string all the 
 *                           information regarding array element
 *                           will be returned.
 *
 * 6.Getting categories information:
 * The porting layer API also defines set of APIs to query the categories for 
 * a given list and also APIs for adding a new category ,removing a category
 * and renaming a category.
 *
 * @{
 */

#define JAVACALL_PIM_MAX_ARRAY_ELEMENTS (7)
#define JAVACALL_PIM_MAX_ATTRIBUTES     (11)
#define JAVACALL_PIM_MAX_LABEL_SIZE     (128)
#define JAVACALL_PIM_INVALID_ID         (-1)
#define JAVACALL_PIM_STRING_DELIMITER   ('\n')
#define JAVACALL_PIM_MAX_FIELDS         (19)
#define JAVACALL_PIM_MAX_BUFFER_SIZE    (45536)

/**
 * @enum javacall_pim_type
 * @brief Indicates which personal information the user would like to manage (Contact, Event, Todo)
 */
typedef enum {
    /** Represents a single Contact entry in a PIM Contact database. */
    JAVACALL_PIM_TYPE_CONTACT = 0x10000000,
    /** Represents a single Event entry in a PIM Event database. */
    JAVACALL_PIM_TYPE_EVENT   = 0x20000000,
    /** Represents a single To Do item in a PIM To Do database. */
    JAVACALL_PIM_TYPE_TODO    = 0x40000000
} javacall_pim_type;

/**
 * @enum javacall_pim_contact_field
 * @brief List all the possible contact fields.
 */
typedef enum {
    JAVACALL_PIM_CONTACT_FIELD_ADDR = 100,
    JAVACALL_PIM_CONTACT_FIELD_BIRTHDAY,
    JAVACALL_PIM_CONTACT_FIELD_CLASS,
    JAVACALL_PIM_CONTACT_FIELD_EMAIL,
    JAVACALL_PIM_CONTACT_FIELD_FORMATTED_ADDR,
    JAVACALL_PIM_CONTACT_FIELD_FORMATTED_NAME,
    JAVACALL_PIM_CONTACT_FIELD_NAME,
    JAVACALL_PIM_CONTACT_FIELD_NICKNAME,
    JAVACALL_PIM_CONTACT_FIELD_NOTE,
    JAVACALL_PIM_CONTACT_FIELD_ORG,
    JAVACALL_PIM_CONTACT_FIELD_PHOTO,
    JAVACALL_PIM_CONTACT_FIELD_PHOTO_URL,
    JAVACALL_PIM_CONTACT_FIELD_PUBLIC_KEY,
    JAVACALL_PIM_CONTACT_FIELD_PUBLIC_KEY_STRING,
    JAVACALL_PIM_CONTACT_FIELD_REVISION,
    JAVACALL_PIM_CONTACT_FIELD_TEL,
    JAVACALL_PIM_CONTACT_FIELD_TITLE,
    JAVACALL_PIM_CONTACT_FIELD_UID,
    JAVACALL_PIM_CONTACT_FIELD_URL
} javacall_pim_contact_field;

/**
 * @enum javacall_pim_contact_field_attr
 * @brief List all the possible contact fields attributes.
 */
typedef enum {
    JAVACALL_PIM_CONTACT_FIELD_ATTR_NONE      = 0,
    JAVACALL_PIM_CONTACT_FIELD_ATTR_ASST      = 1,
    JAVACALL_PIM_CONTACT_FIELD_ATTR_AUTO      = 2,
    JAVACALL_PIM_CONTACT_FIELD_ATTR_FAX       = 4,
    JAVACALL_PIM_CONTACT_FIELD_ATTR_HOME      = 8,  
    JAVACALL_PIM_CONTACT_FIELD_ATTR_MOBILE    = 16,
    JAVACALL_PIM_CONTACT_FIELD_ATTR_OTHER     = 32, 
    JAVACALL_PIM_CONTACT_FIELD_ATTR_PAGER     = 64,
    JAVACALL_PIM_CONTACT_FIELD_ATTR_PREFERRED = 128,
    JAVACALL_PIM_CONTACT_FIELD_ATTR_SMS       = 256,
    JAVACALL_PIM_CONTACT_FIELD_ATTR_WORK      = 512
} javacall_pim_contact_field_attr;

/**
 * @enum javacall_pim_contact_field_array_element_addr
 * @brief List all the possible contact fields address array elements.
 */
typedef enum {
    JAVACALL_PIM_CONTACT_FIELD_ARRAY_ELEMENT_ADDR_POBOX,
    JAVACALL_PIM_CONTACT_FIELD_ARRAY_ELEMENT_ADDR_EXTRA,
    JAVACALL_PIM_CONTACT_FIELD_ARRAY_ELEMENT_ADDR_STREET,
    JAVACALL_PIM_CONTACT_FIELD_ARRAY_ELEMENT_ADDR_LOCALITY,  
    JAVACALL_PIM_CONTACT_FIELD_ARRAY_ELEMENT_ADDR_REGION,
    JAVACALL_PIM_CONTACT_FIELD_ARRAY_ELEMENT_ADDR_POSTALCODE,
    JAVACALL_PIM_CONTACT_FIELD_ARRAY_ELEMENT_ADDR_COUNTRY
} javacall_pim_contact_field_array_element_addr;

/**
 * @enum javacall_pim_contact_field_array_element_name
 * @brief List all the possible contact fields name array elements.
 */
typedef enum {
    JAVACALL_PIM_CONTACT_FIELD_ARRAY_ELEMENT_NAME_FAMILY,
    JAVACALL_PIM_CONTACT_FIELD_ARRAY_ELEMENT_NAME_GIVEN,
    JAVACALL_PIM_CONTACT_FIELD_ARRAY_ELEMENT_NAME_OTHER,
    JAVACALL_PIM_CONTACT_FIELD_ARRAY_ELEMENT_NAME_PREFIX,
    JAVACALL_PIM_CONTACT_FIELD_ARRAY_ELEMENT_NAME_SUFFIX
} javacall_pim_contact_field_array_element_name;

/**
 * @enum javacall_pim_event_field
 * @brief List all the possible event fields.
 */
typedef enum {
    JAVACALL_PIM_EVENT_FIELD_ALARM = 100,
    JAVACALL_PIM_EVENT_FIELD_CLASS,
    JAVACALL_PIM_EVENT_FIELD_END,
    JAVACALL_PIM_EVENT_FIELD_LOCATION,
    JAVACALL_PIM_EVENT_FIELD_NOTE,
    JAVACALL_PIM_EVENT_FIELD_REVISION,
    JAVACALL_PIM_EVENT_FIELD_START,
    JAVACALL_PIM_EVENT_FIELD_SUMMARY,
    JAVACALL_PIM_EVENT_FIELD_UID
} javacall_pim_event_field;

/**
 * @enum javacall_pim_todo_field
 * @brief List all the possible todo fields.
 */
typedef enum {  
    JAVACALL_PIM_TODO_FIELD_CLASS = 100,
    JAVACALL_PIM_TODO_FIELD_COMPLETED,
    JAVACALL_PIM_TODO_FIELD_COMPLETION_DATE,
    JAVACALL_PIM_TODO_FIELD_DUE,
    JAVACALL_PIM_TODO_FIELD_NOTE,
    JAVACALL_PIM_TODO_FIELD_PRIORITY,  
    JAVACALL_PIM_TODO_FIELD_REVISION,
    JAVACALL_PIM_TODO_FIELD_SUMMARY,
    JAVACALL_PIM_TODO_FIELD_UID
} javacall_pim_todo_field;

/**
 * @enum javacall_pim_field_type
 * @brief The type of field
 */
typedef enum {
    JAVACALL_PIM_FIELD_TYPE_BINARY,
    JAVACALL_PIM_FIELD_TYPE_BOOLEAN,
    JAVACALL_PIM_FIELD_TYPE_DATE,
    JAVACALL_PIM_FIELD_TYPE_INT,
    JAVACALL_PIM_FIELD_TYPE_STRING,
    JAVACALL_PIM_FIELD_TYPE_STRING_ARRAY
} javacall_pim_field_type;

/**
 * struct javacall_pim_field_attribute
 * @brief Holds the field attribute information.
 */
typedef struct {
    int id;
    javacall_utf16 label[JAVACALL_PIM_MAX_LABEL_SIZE];  
} javacall_pim_field_attribute;

/**
 * struct javacall_pim_field_array_element
 * @brief Holds the field array elemet information information.
 */
typedef struct {
    int id;
    javacall_utf16 label[JAVACALL_PIM_MAX_LABEL_SIZE];  
} javacall_pim_field_array_element;

/**
 * struct javacall_pim_field
 * @brief Holds all the required information regarding a field.
 */
typedef struct {
    int id;
    int maxValues;
    javacall_pim_field_type type;
    javacall_utf16 label[JAVACALL_PIM_MAX_LABEL_SIZE];
    int attributes;
    javacall_pim_field_array_element arrayElements[JAVACALL_PIM_MAX_ARRAY_ELEMENTS];
} javacall_pim_field;

/**
 * @enum javacall_pim_open_mode
 * @brief The access mode to the relevant PIM list.
 */
typedef enum {
    JAVACALL_PIM_OPEN_MODE_READ_ONLY,
    JAVACALL_PIM_OPEN_MODE_WRITE_ONLY,
    JAVACALL_PIM_OPEN_MODE_READ_WRITE
} javacall_pim_open_mode;

/**
 * @defgroup jsrMandatoryPIM Mandatory PIM API
 * @ingroup JSR75PIM
 * @{
 */

/**
 * Return a JAVACALL_PIM_STRING_DELIMITER seperated list that contains the names
 * of PIM lists that match the given listType
 * ("Contact" Or "JohnContact\nSuziContact")
 *
 * @param listType The PIM list type the user wish to obtain
 * @param pimList Pointer in which to store the returned list. 
 *                the list should be delimited by JAVACALL_PIM_STRING_DELIMITER).
 *                The defualt list name should appear in the first place. 
 * @param pimListLen The length of the PIM list
 * @retval JAVACALL_OK on success   
 * @retval JAVACALL_FAIL when no list exists or when the buffer size is too small 
 */
javacall_result javacall_pim_get_lists(javacall_pim_type listType,
                                       javacall_utf16* /*OUT*/ pimList,
                                       int pimListLen);

/**
 * Checks to see if a given PIM list type is supported by the platform
 * 
 * @return JAVACALL_TRUE if the list type is supported,
 *         JAVACALL_FALSE otherwise.
 */
javacall_bool javacall_pim_list_is_supported_type(javacall_pim_type listType);

/**
 * Open the requested PIM list in the given mode.
 *
 * @param listType The PIM list type to open
 * @param pimListName the name of the list to open,
 *        if pimList is null the handle of default dummy
 *        list will be returned; this kind of list may
 *        be used only for getting default list structure
 * @param mode the open mode for the list
 * @param listHandle a pointer in which to store the listHandle
 * @retval JAVACALL_OK on success   
 * @retval JAVACALL_INVALID_ARGUMENT If an invalid mode is provided as a 
 *         parameter or if listType is not a valid PIM list type.
 * @retval JAVACALL_FAIL on other error
 */
javacall_result javacall_pim_list_open(javacall_pim_type listType,
                                       javacall_utf16* pimListName,
                                       javacall_pim_open_mode mode,
                                       javacall_handle* /* OUT */ listHandle);

/**
 * Close an open PIM list
 *
 * @param listHandle a handle of the list to close.
 * @retval JAVACALL_OK on success   
 * @retval JAVACALL_FAIL in case the list is no longer accessible.
 */
javacall_result javacall_pim_list_close(javacall_handle listHandle);

/**
 * Return the next item in the given PIM list 
 * For Contact item the item will be in vCard 2.1 / 3.0 format
 * For Event/Todo item the item will be in vCalendar 1.0 format
 *
 * @param listHandle a handle of the list from which to the get the item.
 * @param item a pointer to a buffer in which to store the item, 
 *             NULL in case an item was not found.
 * @param maxItemLen the maximum size of the item.
 * @param categories a pointer to a buffer in which to store the item's 
 *                   categories seperated by JAVACALL_PIM_STRING_DELIMITER,
 *                   NULL in case the item does not have any categories.
 * @param maxCategoriesLen the maximum size of the categories buffer.
 * @param itemHandle a pointer in which to store a unique identifier 
 *                   for the returned item.
 * @retval JAVACALL_OK on success   
 * @retval JAVACALL_INVALID_ARGUMENT  maxItemLen is too small 
 * @retval JAVACALL_FAIL in case reached the last item in the list
 */
javacall_result javacall_pim_list_get_next_item(javacall_handle listHandle,
                                                unsigned char* item,
                                                int maxItemLen,
                                                javacall_utf16* categories,
                                                int maxCategoriesLen,
                                                javacall_handle* itemHandle);

/**
 * Modify an item
 * For Contact item the item will be in vCard 2.1 / 3.0 format
 * For Event/Todo item the item will be in vCalendar 1.0 format
 *
 * @param listHandle a handle to the list to which the item belongs
 * @param itemHandle a handle of the item to modify.
 * @param item a pointer to the item to add to the list
 * @param categories a pointer to the item's categories seperate by a comma
 * @retval JAVACALL_OK on success   
 * @retval JAVACALL_FAIL in case of an error
 */
javacall_result javacall_pim_list_modify_item(javacall_handle listHandle,
                                              javacall_handle itemHandle,
                                              const unsigned char* item,
                                              const javacall_utf16* categories);

/**
 * Add a new item to the given item list
 * For Contact item the item will be in vCard 2.1 / 3.0 format
 * For Event/Todo item the item will be in vCalendar 1.0 format
 *
 * @param listHandle a handle to the list to which to add the new item
 * @param item a pointer to the item to add to the list
 * @param categories a pointer to the item's categories seperate by a comma
 * @param itemHandle a pointer in which to store a unique identifier 
 *                   for the new item.
 * @retval JAVACALL_OK on success   
 * @retval JAVACALL_FAIL in case of an error
 */
javacall_result javacall_pim_list_add_item(javacall_handle listHandle,
                                           const unsigned char* item,
                                           const javacall_utf16* categories,
                                           javacall_handle* itemHandle);

/**
 * Remove an item from the list
 *
 * @param listHandle a handle to the list from which to delete the item
 * @param itemHandle a handle to the item to remove
 * @retval JAVACALL_OK on success   
 * @retval JAVACALL_INVALID_ARGUMENT if either of the handles is not valid 
 * @retval JAVACALL_FAIL in case of reaching the last item in the list
 */
javacall_result javacall_pim_list_remove_item(javacall_handle listHandle,
                                              javacall_handle itemHandle);

/**
 * Add the given category to the PIM list. 
 * If the given category already exists in the list, the method does not add 
 * another category, considers this method call successful and returns.
 *
 * The category names are case sensitive in this API, but not necessarily in 
 * the underlying implementation. For example, "Work" and "WORK" map to the 
 * same underlying category if the platform's implementation of categories is 
 * case-insensitive; adding both separately would result in only one category
 * being created in this case.
 *
 * A string with no characters ("") may or may not be a valid category 
 * on a particular platform. 
 * If the string is not a valid category as defined by the platform, 
 * JAVACALL_FAIL is returned when trying to add it. 
 *
 * @param listHandle a handle of the list to add the new category to.
 * @param categoryName the name of the category to be added
 * @retval JAVACALL_OK  on success   
 * @retval JAVACALL_FAIL   If categories are unsupported, an error occurs, 
 *                         or the list is no longer accessible or closed.
 */
javacall_result javacall_pim_list_add_category(javacall_handle listHandle,
                                               javacall_utf16* categoryName);

/**
 * Remove the indicated category from the PIM list. If the indicated category
 * is not in the PIM list, this method is treated as successfully completed.
 * The category names are case sensitive in this API, but not necessarily in 
 * the underlying implementation. For example, "Work" and "WORK" map to the 
 * same underlying category if the platform's implementation of categories 
 * is case-insensitive; removing both separately would result in only one 
 * category being removed in this case. 
 *
 * @param listHandle a handle to the list from which to remove the given category
 * @param categoryName the name of the category to be removed
 * @retval JAVACALL_OK on success   
 * @retval JAVACALL_FAIL If categories are unsupported, an error occurs, 
 *                       or the list is no longer accessible or closed.
 */
javacall_result javacall_pim_list_remove_category(javacall_handle listHandle,
                                                  javacall_utf16* categoryName);

/**
 * Rename a category from an old name to a new name. All items associated with 
 * the old category name are changed to reference the new category name after 
 * this method is invoked. If the new category name is already an existing 
 * category, then the items associated with the old category name are associated
 * with the existing category.
 * A string with no characters ("") may or may not be a valid category on a 
 * particular platform. 
 * If the string is not a category on a platform, a JAVACALL_FAIL should 
 * returned when trying to rename a category to it. 
 *
 * @param listHandle a handle to the list from which to remove the new category
 * @param oldCategoryName the old category name 
 * @param newCategoryName the new category name 
 * @retval JAVACALL_OK on success   
 * @retval JAVACALL_FAIL in case of an error
 */
javacall_result javacall_pim_list_rename_category(javacall_handle listHandle,
                                                  javacall_utf16* oldCategoryName,
                                                  javacall_utf16* newCategoryName);

/**
 * Return the maximum number of categories that this list can have.
 *
 * @param listHandle a handle to the list from which to the get the number
 * @retval -1 - indicates there is no limit to the number of categories that 
 *              this list can have
 * @retval 0  - indicates no category support
 * @retval >0 - in case a limit exists
 */
int javacall_pim_list_max_categories(javacall_handle listHandle);

/**
 * Return the maximum number of categories this item can be assigned to.
 *
 * @param listHandle a handle to the list from which to the get the number
 * @retval -1 - indicates there is no limit to the number of categories 
 *              this item can be assigned to
 * @retval 0  - indicates no category support
 * @retval >0 - in case a limit exists
 */
int javacall_pim_list_max_categories_per_item(javacall_handle listHandle);

/**
 * Return a list of the categories defined for the given PIM list in 
 * ','-seperated format ("Work,HOME,Friends"). 
 * If there are no categories defined for the PIM list or categories are 
 * unsupported for the list, then JAVACALL_FAIL should be returned
 *
 * @param listHandle a handle to the list from which the get the categories list
 * @param categoriesName a pointer to a buffer in which to store the 
 *                       categories list
 * @param maxCategoriesLen the maximum size of the categoriesName buffer
  *
 * @retval JAVACALL_OK on success   
 * @retval JAVACALL_FAIL in case no categories found or incase of an error.
 */
javacall_result javacall_pim_list_get_categories(javacall_handle listHandle,
                                                 javacall_utf16* categoriesName,
                                                 int maxCategoriesLen);

/**
 * Get all fields that are supported in this list. 
 * All fields supported by this list, including both standard 
 * and extended, are returned in the given array.
 *
 * In order to identify field, field attributes , field array element
 * that aren't in use the JAVACALL_PIM_INVALID_ID should be set for the
 * member that aren't in use.
 *
 * @param listHandle a handle to the list from which to get the field
 * @param fields a pointer to a buffer in which to store the fields
 * @param maxFields the maximum fields the field buffer can hold.
 *
 * @retval JAVACALL_OK  on success   
 * @retval JAVACALL_FAIL  in case of reaching the last item in the list
 */
javacall_result javacall_pim_list_get_fields(javacall_handle listHandle,
                                             javacall_pim_field* fields,
                                             int maxFields);

/**
 * Get all attributes supported by the given list
 * 
 * @param listHandle a handle to the list from which to get the attributes
 * @param attributes a pointer to a buffer in which to store the attributes
 * @param maxAttributes the maximum attributes the attributes buffer can hold
 * @retval JAVACALL_OK on success   
 * @retval JAVACALL_FAIL in case reached the last item in the list
 */
javacall_result javacall_pim_list_get_attributes(javacall_handle listHandle,
                                                 javacall_pim_field_attribute* attributes,
                                                 int maxAttributes);

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __JAVACALL_PIM_H */


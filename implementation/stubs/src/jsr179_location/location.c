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

/**
 * Gets the values of the specified property.
 *
 * If there are more than one items in a property string, the items are separated by comma.
 *
 * The following properties should be provided:
 *  - JAVACALL_LOCATION_PROVIDER_LIST
 *   The lists of location providers.
 *  - JAVACALL_LOCATION_ORIENTATION_LIST
 *   The lists of orientation providers. An empty string means that orientation is not supported.
 *  - JAVACALL_LOCATION_LANDMARKSTORE_LIST       
 *   The lists of native landmark stores. An empty string means that there is no native landmark store.
 *   The first item should be the default landmark sotre.
 *
 * @param property id of property
 * @param outPropertyValue UNICODE string value.
 *        Size of this buffer should be JAVACALL_LOCATION_MAX_PROPERTY_LENGTH
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_location_property_get(
        javacall_location_property property,
        javacall_utf16* /*OUT*/outPropertyValue) {
    return JAVACALL_FAIL;
}

/**
 * Get the information for the given name of location provider.
 *
 * This function only gets information and is intended to return it quickly. 
 *
 * The valid provider name is listed in JAVACALL_LOCATION_PROVIDER_LIST property.
 * 
 * @param name of the location provider, NULL implies the default location provider
 * @param pInfo  the information of the location provider
 * 
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_INVALID_ARGUMENT  if the given name of location provider is not found.
 * @retval JAVACALL_FAIL    otherwise, fail
 */
javacall_result javacall_location_provider_getinfo(
        const javacall_utf16_string name,
        javacall_location_provider_info* /*OUT*/pInfo) {
    return JAVACALL_FAIL;
}

/**
 * Initializes a provider.
 *
 * The name will be the loaction or orientation provider.
 * The name of the location provider is in JAVACALL_LOCATION_PROVIDER_LIST property. 
 * Orientation device name is in JAVACALL_LOCATION_ORIENTATION_LIST property. 
 *
 * see javanotify_location_event
 *
 * @param name  of the location provider
 * @param pProvider handle of the location provider
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_WOULD_BLOCK  javanotify_location_event needs to be called to notify completion
 * @retval JAVACALL_INVALID_ARGUMENT  if the specified provider is not found.
 *
 * @retval JAVACALL_FAIL    out of service or other error
 */
javacall_result javacall_location_provider_open(
        const javacall_utf16_string name,
        /*OUT*/ javacall_handle* pProvider) {
    return JAVACALL_FAIL;
}

    
/**
 * Closes the opened provider.
 *
 * This function must free all resources allocated for the specified provider.
 *
 * @param pProvider handle of a provider
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an error
 */
javacall_result javacall_location_provider_close(
        javacall_handle provider) {
    return JAVACALL_FAIL;
}

/**
 * Gets the status of the location provider.
 * This function only get the current state and is intended to return it quickly. 
 *
 * @param provider handle of the location provider
 * @param pState returns state of the specified provider.
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an error
 */
javacall_result javacall_location_provider_state(
        javacall_handle provider,
        /*OUT*/ javacall_location_state* pState) {
    return JAVACALL_FAIL;
}

/**
 * Requests a location acquisition.
 *
 * This function only requests location update and is intended to return it quickly. 
 * The location update will be get through javanotify_location_event() with JAVACALL_EVENT_LOCATION_UPDATE_ONCE type.
 * This function will not be called again before javanotify_location_event is called for the previous request completion.
 * If timeout expires before obtaining the location result, javanotify_location_event() will be called with JAVACALL_LOCATION_RESULT_TIMEOUT reason.
 *
 * see javanotify_location_event
 *
 * @param provider handle of the location provider
 * @param timeout timeout in milliseconds. -1 implies default value.
 *
 * @retval JAVACALL_OK                  success
 * @retval JAVACALL_FAIL                if gets a invalid location or other error
 */
javacall_result javacall_location_update_set(javacall_handle provider, javacall_int64 timeout) {
    return JAVACALL_FAIL;
}

/**
 * Cancels the current location acquisition.
 * 
 * This function will incur calling javanotify_location_event() with JAVACALL_LOCATION_RESULT_CANCELED reason.
 *
 * see javanotify_location_event
 *
 * @param provider handle of the location provider
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there is an  error
 */
javacall_result javacall_location_update_cancel(javacall_handle provider) {
    return JAVACALL_FAIL;
}

/**
 * Gets a location information after location update notification.
 *
 * The location update will be notified through javanotify_location_event() with 
 * JAVACALL_EVENT_LOCATION_UPDATE_ONCE type. 
 *
 * see javanotify_location_event
 *
 * @param provider handle of the location provider
 * @param pLocationInfo location info
 *
 * @retval JAVACALL_OK                  success
 * @retval JAVACALL_FAIL                if gets a invalid location or other error
 */
javacall_result javacall_location_get(javacall_handle provider, 
        /*OUT*/ javacall_location_location* pLocationInfo) {
    return JAVACALL_FAIL;
}

/******************************************************************************
 ******************************************************************************
 ******************************************************************************
    OPTIONAL FUNCTIONS
 ******************************************************************************
 ******************************************************************************
 ******************************************************************************/
    
/**
 * Gets the extra info
 *
 * This information is from the current location result. 
 * This function will be called in the callback function of location updates,
 * so the implementation should be fast enough.
 *
 * @param provider handle of the location provider
 * @param mimetype MIME type of extra info
 * @param maxUnicodeStringBufferLen length of value. The length
 *          should be equal or larger than extraInfoSize 
 *          of the acquired location.
 * @param outUnicodeStringBuffer contents of extrainfo 
 * @param outMimeTypeBuffer name of Other MIME type extraInfo
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result /*OPTIONAL*/ javacall_location_get_extrainfo(
        javacall_handle provider,
        javacall_location_extrainfo_mimetype mimetype,
        int maxUnicodeStringBufferLen,
        /*OUT*/javacall_utf16_string outUnicodeStringBuffer,
        /*OUT*/javacall_utf16_string outMimeTypeBuffer) {
    return JAVACALL_FAIL;
}
    
/**
 * Gets the address info
 *
 * This information is from the current location result. 
 *
 * @param provider handle of the location provider
 * @param pAddresInfoFieldNumber used for both input and output number of array elements. 
 *          Input number should be equal or larger than
 *          addressInfoFieldNumber of the acquired location.
 * @param fields array of address info field
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result /*OPTIONAL*/ javacall_location_get_addressinfo(
        javacall_handle provider,
        /*IN and OUT*/ int* pAddresInfoFieldNumber,
        /*OUT*/javacall_location_addressinfo_fieldinfo fields[]) {
    return JAVACALL_FAIL;
}

/**
 * Computes atan2(y, x) for the two double values.
 *
 * The atan2 math function is used to perform azimuth
 * and distance calculations.
 *
 * @param x first double
 * @param y second double
 *
 * @retval atan2 for the two double values
 */
double /*OPTIONAL*/ javacall_location_atan2(
      double x, double y) {
    return 0;
}

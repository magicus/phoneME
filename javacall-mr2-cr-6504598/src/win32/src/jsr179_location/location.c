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

#ifdef __cplusplus
extern "C" {
#endif
    
#include <stdio.h>
#include <windows.h>
#include <math.h>
#include <time.h>
#include "javacall_location.h"
#include "javacall_time.h"
#include "javacall_file.h"
#include "javacall_logging.h"

#define MAX_PROVIDERS 2
javacall_utf16 provider_name[MAX_PROVIDERS][10] = {{'P','r','o','v','_','N','1',0},{'P','r','o','v','_','N','2',0}};
int provider_name_len[MAX_PROVIDERS] = {7, 7};
int provider_descr[MAX_PROVIDERS] = {1, 2};
javacall_bool provider_opened[MAX_PROVIDERS] = {JAVACALL_FALSE, JAVACALL_FALSE};
javacall_location_provider_info provider_info[MAX_PROVIDERS] = {
    {JAVACALL_FALSE, JAVACALL_TRUE, JAVACALL_TRUE,  JAVACALL_TRUE,
        0, 1,  1,  1000, 3000, 10000, 500,  1000},
    {JAVACALL_TRUE,  JAVACALL_TRUE, JAVACALL_FALSE, JAVACALL_TRUE,
        0, 10, 10, 2000, 5000, 10000, 1000, 1000}};

javacall_location_location providers_location[MAX_PROVIDERS] = {
    {JAVACALL_TRUE, 1, 1, 1, 1, 1, 1, 1, 1, 0, 7, 2},
    {JAVACALL_TRUE, 2, 2, 2, 2, 2, 2, 2, 2, 0, 7, 0}};
javacall_bool update_cancel[MAX_PROVIDERS] = {JAVACALL_FALSE, JAVACALL_FALSE};

/**
 * Open Provider thread
 */
static DWORD WINAPI open_provider(void* pArg)
{
    int ind = (int)pArg;
    javacall_handle fd;
    unsigned char pstate = JAVACALL_LOCATION_AVAILABLE + '0';
    javacall_file_open(provider_name[ind], 7, JAVACALL_FILE_O_CREAT | JAVACALL_FILE_O_RDWR, &fd);
    javacall_file_write(fd, (const unsigned char *)&pstate, sizeof(pstate));
    javacall_file_close(fd);
    Sleep(provider_info[ind].averageResponseTime*2);
    provider_opened[ind] = JAVACALL_TRUE;
    update_cancel[ind] = JAVACALL_FALSE;
    javanotify_location_event( JAVACALL_EVENT_LOCATION_OPEN_COMPLETED, (javacall_handle)provider_descr[ind], JAVACALL_OK);
    return 0;
}

/**
 * Update Location thread
 */
static DWORD WINAPI update_location(void* pArg)
{
    int ind = (int)pArg;
    Sleep(provider_info[ind].averageResponseTime);
    if (!update_cancel[ind]) {
        (javacall_int64)providers_location[ind].timestamp = javacall_time_get_milliseconds_since_1970();
        javanotify_location_event( JAVACALL_EVENT_LOCATION_UPDATE_ONCE, (javacall_handle)provider_descr[ind], JAVACALL_OK);
    }
    return 0;
}

/**
 * Cancel Update Location thread
 */
static DWORD WINAPI cancel_update_location(void* pArg)
{
    int ind = (int)pArg;
    update_cancel[ind] = JAVACALL_TRUE;
    Sleep(1000);
    javanotify_location_event( JAVACALL_EVENT_LOCATION_UPDATE_ONCE, (javacall_handle)provider_descr[ind], JAVACALL_LOCATION_RESULT_CANCELED);
    update_cancel[ind] = JAVACALL_FALSE;
    return 0;
}

/** external function */
javacall_result get_landmarkStore_list(int, javacall_utf16_string);

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
 *
 * @param property id of property
 * @param outPropertyValue UNICODE string value.
 *        Size of this buffer should be JAVACALL_LOCATION_MAX_PROPERTY_LENGTH
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_location_property_get(
        javacall_location_property property,
        javacall_utf16_string /*OUT*/outPropertyValue) {
    int i,j,k=0;
    switch(property) {
        case JAVACALL_LOCATION_PROVIDER_LIST:
            for(i=0; i<MAX_PROVIDERS; i++) {
                for(j=0; j<provider_name_len[i]; j++) {
                    outPropertyValue[k++] = provider_name[i][j];
                }
                outPropertyValue[k++] = ',';
            }
            outPropertyValue[k-1] = 0;
        return JAVACALL_OK;
    default:
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
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

    int i,j;
    javacall_bool found = JAVACALL_FALSE;

    if(name != NULL) {
        for(i=0; i<MAX_PROVIDERS && found == JAVACALL_FALSE; i++) {
            for(j=0; j<provider_name_len[i]; j++) {
                if(name[j] != provider_name[i][j]) {
                    break;
                }
            }
            if((j == provider_name_len[i]) && (name[j] == 0)) {
                found = JAVACALL_TRUE;
                break;
            }
        }
    } else {
        found = JAVACALL_TRUE;
        i = 0;
    }
    
    if(found == JAVACALL_TRUE) {
        *pInfo = provider_info[i];
        return JAVACALL_OK;
    }
    return JAVACALL_INVALID_ARGUMENT;
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
        /*OUT*/ javacall_handle* pProvider){

    int i,j;
    javacall_bool found = JAVACALL_FALSE;

    for(i=0; i<MAX_PROVIDERS && found == JAVACALL_FALSE; i++) {
        for(j=0; j<provider_name_len[i]; j++) {
            if(name[j] != provider_name[i][j]) {
                break;
            }
        }
        if((j == provider_name_len[i]) && (name[j] == 0)) {
            found = JAVACALL_TRUE;
            break;
        }
    }

    if(found == JAVACALL_TRUE) {
        if(provider_opened[i] == JAVACALL_FALSE) {
            /* Create Win32 thread to open Provider - non blocking */
            CreateThread(NULL, 0, open_provider, (void *)i, 0, NULL);
            *pProvider = (javacall_handle)provider_descr[i];
            return JAVACALL_WOULD_BLOCK;
        }
        /* Provider already opened */
        *pProvider = (javacall_handle)provider_descr[i];
        return JAVACALL_OK;
    }
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
    Sleep(500);

    if(JAVACALL_FALSE == provider_opened[(int)provider]) {
        javacall_print("the provider is already closed\n");
    } else {
        provider_opened[(int)provider] = JAVACALL_FALSE;
    }

    return JAVACALL_OK;
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
    unsigned char pstate = JAVACALL_LOCATION_AVAILABLE;
    int i;
    javacall_handle fd;
    javacall_bool found = JAVACALL_FALSE;

    for(i=0; i<MAX_PROVIDERS && found == JAVACALL_FALSE; i++) {
        if(provider_descr[i] == (int)provider) {
            found = JAVACALL_TRUE;
            break;
        }
    }
    if(found == JAVACALL_TRUE) {
        javacall_file_open(provider_name[i], 7, JAVACALL_FILE_O_RDWR, &fd);
        javacall_file_read(fd, (unsigned char *)&pstate, sizeof(pstate));
        javacall_file_close(fd);
        pstate  -= '0';
    }
    *pState = pstate;
    return JAVACALL_OK;
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
    int i;
    javacall_bool found = JAVACALL_FALSE;

    for(i=0; i<MAX_PROVIDERS && found == JAVACALL_FALSE; i++) {
        if(provider_descr[i] == (int)provider) {
            found = JAVACALL_TRUE;
            break;
        }
    }
    if(found == JAVACALL_TRUE) {
        /* Create Win32 thread to open Provider - non blocking */
        CreateThread(NULL, 0, update_location, (void *)i, 0, NULL);
        return JAVACALL_OK;
    }


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
    int i;
    javacall_bool found = JAVACALL_FALSE;

    for(i=0; i<MAX_PROVIDERS && found == JAVACALL_FALSE; i++) {
        if(provider_descr[i] == (int)provider) {
            found = JAVACALL_TRUE;
            break;
        }
    }
    if(found == JAVACALL_TRUE) {
        /* Create Win32 thread to open Provider - non blocking */
        CreateThread(NULL, 0, cancel_update_location, (void *)i, 0, NULL);
        return JAVACALL_OK;
    }

    return JAVACALL_FAIL;
}

/**
 * Gets a location information after location update notification.
 *
 * The location update will be notified through javanotify_location_event() with 
 * JAVACALL_EVENT_LOCATION_UPDATE_ONCE or JAVACALL_EVENT_LOCATION_UPDATE_PERIODICALLY type. 
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
    int i;
    javacall_bool found = JAVACALL_FALSE;

    for(i=0; i<MAX_PROVIDERS && found == JAVACALL_FALSE; i++) {
        if(provider_descr[i] == (int)provider) {
            found = JAVACALL_TRUE;
            break;
        }
    }
    if(found == JAVACALL_TRUE) {
        if(providers_location[i].timestamp == 0) {
            return JAVACALL_FAIL;
        } else {
            *pLocationInfo = providers_location[i];
            return JAVACALL_OK;
        }
    }
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
 * @defgroup jsrOptionalLocation OPTIONAL misc location API
 * @ingroup Location
 * 
 * @{
 */

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
    if(maxUnicodeStringBufferLen > 0) {
        outUnicodeStringBuffer[0] = 0;
    }
    outMimeTypeBuffer[0] = 0;
    if (mimetype == JAVACALL_LOCATION_EXTRAINFO_PLAINTEXT) {
        if(maxUnicodeStringBufferLen >= 7) {
            outUnicodeStringBuffer[0] = 'H';
            outUnicodeStringBuffer[1] = 'e';
            outUnicodeStringBuffer[2] = 'l';
            outUnicodeStringBuffer[3] = 'l';
            outUnicodeStringBuffer[4] = 'o';
            outUnicodeStringBuffer[5] = '1';
            outUnicodeStringBuffer[6] = 0;
            if((int)provider == provider_descr[0]) {    
                outUnicodeStringBuffer[5] = '1';
            } else {
                outUnicodeStringBuffer[5] = '2';
            }
        }
    }
    if ((mimetype == JAVACALL_LOCATION_EXTRAINFO_OTHER) && ((int)provider == provider_descr[0])) {
        if(maxUnicodeStringBufferLen >= 7) {
            outUnicodeStringBuffer[0] = 'H';
            outUnicodeStringBuffer[1] = 'e';
            outUnicodeStringBuffer[2] = 'l';
            outUnicodeStringBuffer[3] = 'l';
            outUnicodeStringBuffer[4] = 'o';
            outUnicodeStringBuffer[5] = '1';
            outUnicodeStringBuffer[6] = 0;
            outMimeTypeBuffer[0] = 'O';
            outMimeTypeBuffer[1] = 't';
            outMimeTypeBuffer[2] = 'h';
            outMimeTypeBuffer[3] = 'e';
            outMimeTypeBuffer[4] = 'r';
            outMimeTypeBuffer[5] = 0;
        }
    }
    return JAVACALL_OK;
};
    
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
    if((int)provider == provider_descr[0]) {    
        if(*pAddresInfoFieldNumber >= 2){
            fields[0].filedId = JAVACALL_LOCATION_ADDRESSINFO_COUNTRY;
            fields[0].data[0] = 'R';
            fields[0].data[1] = 'u';
            fields[0].data[2] = 's';
            fields[0].data[3] = 's';
            fields[0].data[4] = 'i';
            fields[0].data[5] = 'a';
            fields[0].data[6] = 0;
            fields[1].filedId = JAVACALL_LOCATION_ADDRESSINFO_CITY;
            fields[1].data[0] = 'S';
            fields[1].data[1] = '-';
            fields[1].data[2] = 'P';
            fields[1].data[3] = 'e';
            fields[1].data[4] = 't';
            fields[1].data[5] = 'e';
            fields[1].data[6] = 'r';
            fields[1].data[7] = 's';
            fields[1].data[8] = 'b';
            fields[1].data[9] = 'u';
            fields[1].data[10] = 'r';
            fields[1].data[11] = 'g';
            fields[1].data[12] = 0;
            *pAddresInfoFieldNumber = 2;
            return JAVACALL_OK;
        }
    }

    *pAddresInfoFieldNumber = 0;
    return JAVACALL_OK;
};

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
    return atan2(x, y);
};

#ifdef __cplusplus
} //extern "C"
#endif


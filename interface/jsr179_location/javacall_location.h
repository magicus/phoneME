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

#ifndef __JAVACALL_LOCATION_H
#define __JAVACALL_LOCATION_H
/**
 * @defgroup JSR179 JSR179 Location API
 * @ingroup stack
 * Porting interface for native implementation Location API.
 * @{
 */
/** @} */

/**
 * @file javacall_location.h
 * @ingroup Location
 * @brief Javacall interfaces for JSR-179 location
 */
 
#ifdef __cplusplus
extern "C" {
#endif

    
#include "javacall_defs.h" 

/**
 * @defgroup Location Location API
 * @ingroup JSR179
 * This API covers obtaining information about the present geographic location
 * and orientation of the terminal.
 *
 * <b> Acquisition of present geographic location of the terminal</b>
 *
 * To calculate distance/azimuth, the platform independent atan2 
 * function implementation will be provided. However, 
 * if the target platform provides atan2 functionality 
 * by means of a native math library, it will be more efficient to utilize it.
 *
 * The acquired location should include following information:
 *  - Only Latitude and Longitude are mandatory.
 *  - Altitude - Float.NAN if unknown    
 *  - HorizontalAccuracy and VerticalAccuracy in meters - Float.NaN if unknown
 *  - LocationMethod - zero if unknown
 *  - Speed, Course - Float.NAN if unknown
 *  - AddressInfo - provided if supported.
 *  - ExtraInfo - provided if supported.
 *
 * For the specified location provider, the following infomation should be provided:
 *  - Default time related properties of a location provider
 *  - The default interval - how often updates for listener.
 *  - The default maximum age - acceptable age for cached info for listener
 *  - The default timeout  - how late update can be from interval for listener
 *  - The average time of receiving next location
 *  - The recommended time interval for asking a new state
 *  - The other properties for Critera.
 * 
 * Optional feature whose availability depends on the used location methods.
 *  - provide altitude information
 *  - provide accuracy of altitude
 *  - provide course and speed information
 *  - provide textual address information related to the location.
 *  - provide proximity monitoring.
 *
 * The porting layer includes:
 *  - Opens a location provider and gets the information   
 *    - javacall_location_provider_open
 *    - javacall_location_provider_close
 *    - javacall_location_provider_getinfo
 *    - javanotify_location_event
 *  - Gets the current Status of a location provider.
 *    - javacall_location_provider_state
 *  - Gets the location info on request basis.
 *    - javacall_location_update_set
 *    - javacall_location_update_cancel
 *  - Gets the proximity events - Optional
 *    - javacall_location_proximity_monitoring_add
 *    - javacall_location_proximity_monitoring_cancel
 *    - javanotify_location_proximity
 *
 * <b> Acquisition of orientation of the terminal</b>
 *
 * The following features are optionally supported:
 *  - compass azimuth of the terminal orientation
 *  - pitch and roll 3D terminal orientation information
 * If the implementation chooses to support orientation, it must 
 * provide the azimuth information. Providing pitch and roll is optional.
 *
 * The porting layer includes:
 *  - Opens and closes a orientation provider
 *    - javacall_location_provider_open
 *    - javacall_location_provider_close
 *  - Gets a orientation information
 *    - javacall_location_orientation_start
 *    - javacall_location_orientation_finish
 *    - javanotify_location_event
 *
 *
 * <b> Implementation Notes </b>
 *
 *  - asynchronous operation
 *
 * In the asynchronous operation, the operation returns quickly with JAVACALL_WOULD_BLOCK. When the operation has been completed, an
 * event is required to be sent from Platform. See section on event for related
 * information.
 *
 * The asynchronous functions are
 *       opening location provider,
 *       orientation update,
 *       location update once,
 *       proximity update and
 *
 *  - utf16 string representation
 *
 * All utf16 strings used in this API should be NULL terminated.
 *
 *  - buffer allocation
 *
 * The buffer for output parameter should be allocated by the caller. 
 * There should be a parameter for the size of the buffer if there is 
 * no predefined maximum size. 
 * \par
 *
 */

/**
 * @defgroup jsrMandatoryLocation Mandatory Location API
 * @ingroup Location
 * @{
 */

#define JAVACALL_LOCATION_MAX_ADDRESSINFO_FIELD     (32 +1)
#define JAVACALL_LOCATION_MAX_PROPERTY_LENGTH       (64 +1)
#define JAVACALL_LOCATION_MAX_MIMETYPE_LENGTH       (32 +1)

/**
 * @enum javacall_location_property
 * @brief Property ids
 */
typedef enum {
    /** location provider name list */
    JAVACALL_LOCATION_PROVIDER_LIST               = 0, 
    /** orientation device name list */
    JAVACALL_LOCATION_ORIENTATION_LIST            = 1
} javacall_location_property;

/**
 * @enum javacall_location_power_consumption
 * @brief Power consumption level
 */
typedef enum {
    /** Constant indicating no requirements for the parameter. */
    JAVACALL_LOCATION_NO_IDEA               = 0,
    /** Level indicating only low power consumption allowed. */
    JAVACALL_LOCATION_USAGE_LOW             = 1,
    /** Level indicating average power consumption allowed. */
    JAVACALL_LOCATION_USAGE_MEDIUM          = 2,
    /** Level indicating high power consumption allowed. */
    JAVACALL_LOCATION_USAGE_HIGH            = 3
} javacall_location_power_consumption;

/**
 * @enum javacall_location_state
 * @brief State of a location provider
 */
typedef enum {
    /** Availability status code: the location provider is available. */
    JAVACALL_LOCATION_AVAILABLE               = 0,
    /** Availability status code: the location provider is out of service. */
    JAVACALL_LOCATION_OUT_OF_SERVICE          = 1,
    /** Availability status code: the location provider is temporarily unavailable. */
    JAVACALL_LOCATION_TEMPORARILY_UNAVAILABLE = 2
} javacall_location_state;

/**
 * @enum javacall_location_extrainfo_mimetype
 * @brief MIME type id of extra information
 */
typedef enum {
    /** Mime type is "application/X-jsr179-location-nmea" */
    JAVACALL_LOCATION_EXTRAINFO_NMEA               = 0,
    /** Mime type is "application/X-jsr179-location-lif" */
    JAVACALL_LOCATION_EXTRAINFO_LIF,
    /** Mime type is "text/plain" */
    JAVACALL_LOCATION_EXTRAINFO_PLAINTEXT,
    /** Other mime types */
    JAVACALL_LOCATION_EXTRAINFO_OTHER
} javacall_location_extrainfo_mimetype;

/**
 * @enum javacall_location_method 
 * @brief location acquisition method
 */
typedef enum {
    /** Location method using satellites. */
    JAVACALL_LOCATION_MTE_SATELLITE = 0x00000001,
    /** Location method Time Difference for cellular / terrestrial RF system. */
    JAVACALL_LOCATION_MTE_TIMEDIFFERENCE = 0x00000002,
    /** Location method Time of Arrival (TOA) for cellular / terrestrial RF system. */
    JAVACALL_LOCATION_MTE_TIMEOFARRIVAL = 0x00000004,
    /** Location method Cell-ID for cellular. */
    JAVACALL_LOCATION_MTE_CELLID = 0x00000008,
    /** Location method Short-range positioning system (for example, Bluetooth LP). */
    JAVACALL_LOCATION_MTE_SHORTRANGE = 0x00000010,
    /** Location method Angle of Arrival for cellular / terrestrial RF system. */
    JAVACALL_LOCATION_MTE_ANGLEOFARRIVAL = 0x00000020,
    /** Location method is of type terminal based. */
    JAVACALL_LOCATION_MTY_TERMINALBASED = 0x00010000,
    /** Location method is of type network based. */
    JAVACALL_LOCATION_MTY_NETWORKBASED = 0x00020000,
    /** Location method is assisted by the other party 
        (Terminal assisted for Network based, Network assisted for terminal based). */
    JAVACALL_LOCATION_MTA_ASSISTED = 0x00040000,
    /** Location method is unassisted. */
    JAVACALL_LOCATION_MTA_UNASSISTED = 0x00080000
} javacall_location_method;

/**
 * @enum javacall_location_addressinfo_field
 * @brief filed ids of location's address information 
 */
typedef enum {
    /** Address field denoting address extension, e.g. flat number. */
    JAVACALL_LOCATION_ADDRESSINFO_EXTENSION = 1,
    /** Address field denoting street name and number. */
    JAVACALL_LOCATION_ADDRESSINFO_STREET = 2,
    /** Address field denoting zip or postal code. */
    JAVACALL_LOCATION_ADDRESSINFO_POSTAL_CODE = 3,
    /** Address field denoting town or city name. */
    JAVACALL_LOCATION_ADDRESSINFO_CITY = 4,
    /** Address field denoting country. */
    JAVACALL_LOCATION_ADDRESSINFO_COUNTY = 5,
    /** Address field denoting state or province. */
    JAVACALL_LOCATION_ADDRESSINFO_STATE = 6,
    /** Address field denoting country. */
    JAVACALL_LOCATION_ADDRESSINFO_COUNTRY = 7,
    /** Address field denoting country as a two-letter ISO 3166-1 code. */
    JAVACALL_LOCATION_ADDRESSINFO_COUNTRY_CODE = 8,
    /** Address field denoting a municipal district. */
    JAVACALL_LOCATION_ADDRESSINFO_DISTRICT = 9,
    /** Address field denoting a building name. */
    JAVACALL_LOCATION_ADDRESSINFO_BUILDING_NAME = 10,
    /** Address field denoting a building floor. */
    JAVACALL_LOCATION_ADDRESSINFO_BUILDING_FLOOR = 11,
    /** Address field denoting a building room. */
    JAVACALL_LOCATION_ADDRESSINFO_BUILDING_ROOM = 12,
    /** Address field denoting a building zone */
    JAVACALL_LOCATION_ADDRESSINFO_BUILDING_ZONE = 13,
    /** Address field denoting a street in a crossing. */
    JAVACALL_LOCATION_ADDRESSINFO_CROSSING1 = 14,
    /** Address field denoting a street in a crossing. */
    JAVACALL_LOCATION_ADDRESSINFO_CROSSING2 = 15,
    /** Address field denoting a URL for this place. */
    JAVACALL_LOCATION_ADDRESSINFO_URL = 16,
    /** Address field denoting a phone number for this place. */
    JAVACALL_LOCATION_ADDRESSINFO_PHONE_NUMBER = 17
} javacall_location_addressinfo_field;


/**
 * @enum javacall_location_result
 * @brief result code extention to javacall_result
 */
typedef enum {
   /* javacall_result end at -7 */
   /** Operation is canceled */
   JAVACALL_LOCATION_RESULT_CANCELED = -10,  
   /** Operation is timeout */
   JAVACALL_LOCATION_RESULT_TIMEOUT = -11,   
   /** Provider is out of service */
   JAVACALL_LOCATION_RESULT_OUT_OF_SERVICE = -12, 
   /** Provider is out of service */
   JAVACALL_LOCATION_RESULT_TEMPORARILY_UNAVAILABLE = -13 
} javacall_location_result;

/* defines Float.NaN, NEED REVISIT: may depend on system */
#ifdef __arm /* ADS compiler */
#define JAVACALL_LOCATION_FLOAT_NAN   (0f_7FC00000)
#elif __GNUC__ /* GNU C compiler */
#define JAVACALL_LOCATION_FLOAT_NAN (((union { unsigned __l; float __d; })  { __l: 0x7fc00000U }).__d)
#else
static const unsigned int __nan_int = 0x7fc00000;
#define JAVACALL_LOCATION_FLOAT_NAN (*(float*)(void*)&__nan_int)
#endif

/**
 * struct javacall_location_provider_info
 * 
 * @brief The information of location provider including the criteria information which the location provider could meet.
 */
typedef struct {
    javacall_bool incurCost;
    javacall_bool canReportAltitude;
    javacall_bool canReportAddressInfo;
    javacall_bool canReportSpeedCource;
    javacall_location_power_consumption powerConsumption;
    int horizontalAccuracy;     /* measured in meter */
    int verticalAccuracy;       /* measured in meter */
    int defaultTimeout;         /* in milliseconds. The default listener timeout to obtain a new location result. */
    int defaultMaxAge;          /* in milliseconds. The default listener maximum age defines that how old the location result is allowed to be provided when the listener update is made. */
    int defaultInterval;        /* in milliseconds. The default listener interval between cyclic location updates. */ 
    int averageResponseTime;    /* in milliseconds. The average response time for obtaining a new location */
    int defaultStateInterval;   /* in milliseconds. The recommended interval for querying provider's state. */
} javacall_location_provider_info;

/**
 * struct javacall_location_addressinfo_fieldinfo
 * @brief Address infomation's field value.
 */
typedef struct {
    javacall_location_addressinfo_field filedId;
    javacall_utf16 data[JAVACALL_LOCATION_MAX_ADDRESSINFO_FIELD];
} javacall_location_addressinfo_fieldinfo;

/**
 * struct javacall_location_location
 * @brief Location info
 */
typedef struct {
    /** whether a valid location with coordinates or an invalid one, invalid may have the extra information for the reason  */ 
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
    /** in meters per second, JAVACALL_LOCATION_FLOAT_NAN if unknown */
    float speed;                
    /** in degree, JAVACALL_LOCATION_FLOAT_NAN if unknown */
    float course;               
    /** bitwise combination of javacall_location_method, 0 if unknown */
    int method;                 
    /** value of javacall_time_get_milliseconds_since_1970() */
    javacall_int64 timestamp; 
    /** 0 if none or the length of the extra infomation string which includes NULL termination */
    int extraInfoSize;          
    /** number of address info fields which have value, zero if none */
    int addressInfoFieldNumber; 
} javacall_location_location;


/**
 * struct javacall_location_orientation
 * 
 * @brief Orientation info
 */
typedef struct {
    /** in degree within [0.0, 360.0) */
    float compassAzimuth;   
    /** in degree within [-90.0, 90.0], JAVACALL_LOCATION_FLOAT_NAN if unknown */
    float pitch;            
    /** in degree within [-180.0, 180.0), JAVACALL_LOCATION_FLOAT_NAN if unknown */
    float roll;
    /** If true, the compassAzimuth and pitch are relative to the magnetic field of the Earth. 
        If false, the compassAzimuth is relative to true north and pitch is relative to gravity. */
    javacall_bool isMagnetic; 
} javacall_location_orientation;

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
 * @param outPropertyValue utf16 string value.
 *        Size of this buffer should be JAVACALL_LOCATION_MAX_PROPERTY_LENGTH
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_location_property_get(
        javacall_location_property property,
        javacall_utf16* /*OUT*/outPropertyValue);

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
        javacall_location_provider_info* /*OUT*/pInfo);

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
        /*OUT*/ javacall_handle* pProvider);

    
/**
 * Closes the opened provider.
 *
 * This function must free all resources allocated for the specified provider.
 *
 * @param provider handle of a provider
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an error
 */
javacall_result javacall_location_provider_close(
        javacall_handle provider);

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
        /*OUT*/ javacall_location_state* pState);

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
javacall_result javacall_location_update_set(javacall_handle provider, javacall_int64 timeout);

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
javacall_result javacall_location_update_cancel(javacall_handle provider);

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
        /*OUT*/ javacall_location_location* pLocationInfo);

/** @} */
    


/******************************************************************************
 ******************************************************************************
 ******************************************************************************

  NOTIFICATION FUNCTIONS
  - - - -  - - - - - - -  
  The following functions are implemented by Sun.
  Platform is required to invoke these function for each occurrence of the
  undelying event.
  The functions need to be executed in platform's task/thread

 ******************************************************************************
 ******************************************************************************
 ******************************************************************************/
    
/**
 * @defgroup MiscNotification_location Notification API for Location
 * @ingroup Location
 * @{
 */


/**
 * @enum javacall_location_callback_type
 * @brief Location callback event type
 */
typedef enum {
    JAVACALL_EVENT_LOCATION_OPEN_COMPLETED,
    JAVACALL_EVENT_LOCATION_ORIENTATION_COMPLETED,
    JAVACALL_EVENT_LOCATION_UPDATE_ONCE
} javacall_location_callback_type;
    
/**
 * A callback function to be called for notification of non-blocking 
 * location related events.
 * The platform will invoke the call back in platform context for
 * each provider related occurrence. 
 *
 * @param event type type of indication: Either
 *          - JAVACALL_EVENT_LOCATION_OPEN_COMPLETED
 *          - JAVACALL_EVENT_LOCATION_ORIENTATION_COMPLETED
 *          - JAVACALL_EVENT_LOCATION_UPDATE_ONCE
 * @param provider handle of provider related to the notification
 * @param operation_result operation result: Either
 *      - JAVACALL_OK if operation completed successfully, 
 *      - JAVACALL_LOCATION_RESULT_CANCELED if operation is canceled 
 *      - JAVACALL_LOCATION_RESULT_TIMEOUT  if operation is timeout 
 *      - JAVACALL_LOCATION_RESULT_OUT_OF_SERVICE if provider is out of service
 *      - JAVACALL_LOCATION_RESULT_TEMPORARILY_UNAVAILABLE if provider is temporarily unavailable
 *      - otherwise, JAVACALL_FAIL
 */
void javanotify_location_event(
        javacall_location_callback_type event,
        javacall_handle provider,
        javacall_location_result operation_result);


/**
 * A callback function to be called for notification of proximity monitoring updates.
 *
 * This function will be called only once when the terminal enters the proximity of the registered coordinate. 
 *
 * @param provider handle of provider related to the notification
 * @param latitude of registered coordinate.
 * @param longitude of registered coordinate.
 * @param proximityRadius of registered coordinate.
 * @param pLocationInfo location info
 * @param operation_result operation result: Either
 *      - JAVACALL_OK if operation completed successfully, 
 *      - JAVACALL_LOCATION_RESULT_CANCELED if operation is canceled 
 *      - JAVACALL_LOCATION_RESULT_OUT_OF_SERVICE if provider is out of service
 *      - JAVACALL_LOCATION_RESULT_TEMPORARILY_UNAVAILABLE if provider is temporarily unavailable
 *      - otherwise, JAVACALL_FAIL
 */
void /*OPTIONAL*/javanotify_location_proximity(
        javacall_handle provider,
        double latitude,
        double longitude,
        float proximityRadius,
        javacall_location_location* pLocationInfo,
        javacall_location_result operation_result);

/** @} */
    
/******************************************************************************
 ******************************************************************************
 ******************************************************************************
    OPTIONAL FUNCTIONS
 ******************************************************************************
 ******************************************************************************
 ******************************************************************************/
    
/**
 * @defgroup jsrOptionalLocation Optional Location API
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
        /*OUT*/javacall_utf16_string outMimeTypeBuffer);
    
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
        /*OUT*/javacall_location_addressinfo_fieldinfo fields[]);

/**
 * Initiates getting orientation
 *
 * see javanotify_location_event
 *
 * @param provider handle of a location provider
 * @param pOrientationInfo orientation info
 * @param pContext address of a pointer variable to receive the context;
 *        this is set only when the function returns JAVACALL_WOULD_BLOCK.
 *
 * @retval JAVACALL_OK                  success
 * @retval JAVACALL_WOULD_BLOCK  javanotify_location_event needs to be called to notify completion. The type of event will be JAVACALL_EVENT_LOCATION_ORIENTATION_COMPLETED
 * @retval JAVACALL_FAIL                if gets a invalid orientation or other error
 */
javacall_result /*OPTIONAL*/ javacall_location_orientation_start(
        javacall_handle provider,
        /*OUT*/ javacall_location_orientation* pOrientationInfo,
        /*OUT*/ void **pContext);

/**
 * Finishes a pending orientation operation.
 * 
 * see javanotify_location_event
 *
 * @param provider handle of a location provider
 * @param pOrientationInfo orientation info
 * @param context the context returned by the orientation_start function
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_WOULD_BLOCK if the caller must call the finish function 
 *         again to complete the operation
 * @retval JAVACALL_FAIL        if there is an  error
 */
javacall_result /*OPTIONAL*/ javacall_location_orientation_finish(
        javacall_handle provider,
        /*OUT*/ javacall_location_orientation* pOrientationInfo,
        void *context);

/**
 * Adds proximity monitoring for the specified coordinate
 * 
 * This function could monitor more than one coordinate. 
 * javanotify_location_proximity() is called only once when the terminal enters the proximity of the resigered coordinate. 
 * The monitoring state change is reported as well through the operation_result parameter of javanotify_location_proximity(). 
 *
 * @param provider handle of the location provider
 * @param latitude of the location to monitor
 * @param longitude of the location to monitor
 * @param proximityRadius the radius in meters that is considered to be the threshold for being in the proximity of the specified location. ( >= 0 )
 *
 * @retval JAVACALL_OK                  success
 * @retval JAVACALL_FAIL                the already monitored location is provided, or if the platform does not have resources to add a new location to be monitored.
 * @retval JAVACALL_INVALID_ARGUMENT    if latitude or longitude are not within the valid range. if proximityRadius < 0
 * 
 */
javacall_result /*OPTIONAL*/ javacall_location_proximity_monitoring_add(
        javacall_handle provider,
        double latitude,
        double longitude,
        float proximityRadius);
/**
 * Removes a proximity monitoring for the specified coordinate
 * 
 * @param provider handle of the location provider
 * @param latitude of the location to monitor
 * @param longitude of the location to monitor
 * @param proximityRadius the radius in meters that is considered to be the threshold for being in the proximity of the specified location. ( >= 0 )
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        the other error
 * @retval JAVACALL_INVALID_ARGUMENT   if the given location is not found
 */
javacall_result /*OPTIONAL*/ javacall_location_proximity_monitoring_cancel(
        javacall_handle provider,
        double latitude,
        double longitude,
        float proximityRadius);

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
        double x, double y);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __JAVACALL_LOCATION_H */



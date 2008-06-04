/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */ 
#ifndef __JAVACALL_OEMSETTING_H_
#define __JAVACALL_OEMSETTING_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file javacall_oemsetting.h
 * @ingroup OEM Setting
 * @brief Javacall interfaces for properties configurable by OEM.
 */

#include "javacall_defs.h"

/**
 * @defgroup OEMSetting OEMSetting API
 * @ingroup JTWI
 * @{
 */

/**
 * Get default network profile used by Java
 *
 * @param buf       [OUT] Buffer that will contains default nework string
 * @param bufSize   [IN/OUT] Buf size and returned buf data size
 *
 * @retval JAVACALL_OK
 * @retval JAVACALL_FAIL
 */
javacall_result javacall_oem_get_defaultnetwork(char* buf, int* bufSize);

/**
 * Set default network profile used by Java
 *
 * @param buf       [IN] Buffer that will contains default nework string
 * @param bufSize   Size of this buffer
 *
 * @retval JAVACALL_OK
 * @retval JAVACALL_FAIL
 */
javacall_result javacall_oem_set_defaultnetwork(const char* buf, int bufSize);

/**
 * Get OEM's user agent string for HTTP header
 *
 * @return HTTP UserAgent string or NULL
 */
const char* javacall_oem_get_useragent();

/**
 * Get OEM's x-wap-profile string for HTTP header
 *
 * @return HTTP x-wap-profile or NULL
 */
const char* javacall_oem_get_xwapprofile();

/**
 * Get number of ams folders
 */
javacall_result javacall_oem_ams_get_folderCount(int* folderCount);

/**
 * Get customed midlet group name regarding to a specific index
 *
 */
javacall_result javacall_oem_ams_get_foldername(char* folderName,
                                                int* nameLength,
                                                int index);

/**
 * Get customed download regarding to a specific index
 *
 */
javacall_result javacall_oem_ams_get_folderurl(char* URL,
                                               int* bufLength,
                                               int index);

/**
 * Get customed default midlet group name
 *
 */
javacall_result javacall_oem_ams_get_defaultfolder(char* folderName,
                                                   int* nameLength);

/**
 * Get jvm heap size from native configuration
 */
int javacall_oem_get_java_heapsize(void);


/**
 * Get customed ams font size, return value
 *
 *  JAVACALL_FONT_SIZE_SMALL - small font
 *  JAVACALL_FONT_SIZE_SMALL - medium font
 *  JAVACALL_FONT_SIZE_LARGE - large font
 *
 * if no customization, return default setting "medium font"
 *
 */
javacall_result javacall_oem_ams_get_fontSize(int* fontSize);

/**
 *
 * get size info for specific type of font, type might be
 *
 * 0 - small font
 * 1 - medium font
 * 2 - large font
 *
 */
javacall_result javacall_oem_font_getSizeInfo(int fontSize,
                                              int* height,
                                              int* descent,
                                              int* leading);


/**
 * get status of a specific certificate
 * status could be
 * 
 * enabled -- return JAVACALL_TRUE
 * disabled -- return JAVACALL_FALSE
 *
 */
javacall_bool javacall_oem_secutirySetting_getKeyStatus(const char *owner);

/**
 * to enable or disable a specific certificate
 *
 */
void javacall_oem_secutirySetting_saveKeyStatus(unsigned short* keyOwner,
                                                javacall_bool enabled);

void oem_securitySetting_getCertsForDomainManufacturer(char** domain,
                                                       int maxCertsInDomain);

void oem_securitySetting_getCertsForDomainOperator(char** domain,
                                                   int maxCertsInDomain);

void oem_securitySetting_getCertsForDomainThirdParty(char** domain,
                                                     int maxCertsInDomain);

void oem_securitySetting_addCertToDomainThirdParty(wchar_t* caOwnerStr);

javacall_bool oem_securitySetting_trustAllRootCerts(void);

/** @} */

/** @} */
    
#ifdef __cplusplus
}
#endif

#endif 



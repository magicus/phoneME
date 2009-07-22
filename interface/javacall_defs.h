/**
 * @mainpage Javacall API
 * @image html Java.png
 * @image html sun.jpg
 * <h3>Reference Documentation for Javacall Porting API</h3>
 *
 * <p>These pages specify the Javacall porting APIs. They
 * describe the header files' contents, including function
 * signatures, globals, and data structures. The pages
 * organize the files both functionally by subsystem and
 * service, and alphabetically. They also index functions,
 * globals, and data structures for easier information access.
 * </p>
 *

 *
 */

/*
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
#ifndef __JAVACALL_DEFINE_H_
#define __JAVACALL_DEFINE_H_

/**
 * @file javacall_defs.h
 * @ingroup Common
 * @brief Common definitions for javacall
 */

/**
 * @defgroup Common Common Javacall API Definitions
 *
 * The common javacall definitions are defined in this file
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif 

/*
 * To use javacall wrappers 
 *   turn USE_JAVACALL_WRAPPERS on  for javacall files and 
 *   turn USE_JAVACALL_WRAPPERS off for native test files.
 * See the the document below for details:
 *   porting_api/tools/wrappers/README.txt
 */
#ifdef USE_JAVACALL_WRAPPERS
#include "javacall_wrappers_define.h"
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

/**
 * @enum javacall_result
 * @brief javacall error results
 */
typedef enum {
   /** Generic success */
   JAVACALL_OK = 0,
   /** Generic failure */
   JAVACALL_FAIL = -1,
   /** Not implemented */
   JAVACALL_NOT_IMPLEMENTED = -2,
   /** Out of memory */
   JAVACALL_OUT_OF_MEMORY = -3,
   /** Invalid argument */
   JAVACALL_INVALID_ARGUMENT = -4,
   /** Would block */
   JAVACALL_WOULD_BLOCK = -5,
   /** Connection not found */
   JAVACALL_CONNECTION_NOT_FOUND = -6,
   /** Operation is interrupted */
   JAVACALL_INTERRUPTED = -7,
   /** Return by javacall read on  
       SoS connections or socket in
       Non-Delay mode. Caller should
       reinvoke the read function 
       to retry reading data */
   JAVACALL_NO_DATA_AVAILABLE = -8,
   /** File not found in the given path */
   JAVACALL_FILE_NOT_FOUND = -9,
   /** bad file name */
   JAVACALL_BAD_FILE_NAME = -10,
   /** End of file */
   JAVACALL_END_OF_FILE = -11,
   /** I/O error occured */
   JAVACALL_IO_ERROR = -12,
   /** bad properties in jad file, 
    * either a missing required property or
    * incorrectly formatted property */
   JAVACALL_BAD_JAD_PROPERTIES = -13,
    /** javacall properties db value not found */
   JAVACALL_VALUE_NOT_FOUND = -14,
    /** Invalid state */
   JAVACALL_INVALID_STATE = -15,
   /** Timeout elapsed */
   JAVACALL_TIMEOUT = -16,

   /** 
     * No audio device found. Please return this code only in case you want to
     * reject playback, i.e. when the content is audio only. If some playback
     * is still possible (e.g. mute video), please return JAVACALL_OK instead
     */
   JAVACALL_NO_AUDIO_DEVICE = -17
} javacall_result;

#define JAVACALL_SUCCEEDED(Status) ((javacall_result)(Status) >= 0) 

/**
 * @enum javacall_bool
 * @brief javacall boolean type
 */
typedef enum {
    /** FALSE */
    JAVACALL_FALSE = 0,
    /** TRUE */
    JAVACALL_TRUE  = 1
} javacall_bool;

/**
 * @typedef javacall_handle
 * @brief general handle type
 */
typedef void* javacall_handle;

/**
 * Platform-dependent defines,
 * check JAVACALL_PLATFORM_INC_DIR environment variable
 */
#include <javacall_platform_defs.h>

/**
 * @typedef javacall_suite_id
 * @brief suite unique ID
 */
typedef javacall_int32 javacall_suite_id;

/**
 * @brief unique storage ID
 */
typedef javacall_int32 javacall_storage_id;

/**
 * @brief unique AMS folder ID
 */
typedef javacall_int32 javacall_folder_id;

/**
 * @brief unique running midlet ID
 */
typedef javacall_int32 javacall_app_id;

/**
 * @brief unique RMS ID
 */
typedef javacall_int32 javacall_rms_id;

/**
 * @brief unique RMS record ID
 */
typedef javacall_int32 javacall_record_id;

/**
 * @define JAVACALL_INVALID_SUITE_ID
 * @brief The suite_id that doesn't correspond to any midlet suite
 *
 * IMPL_NOTE: value -1 is reserved for internal (rommized) MIDlet suites
 */
#define JAVACALL_INVALID_SUITE_ID (-2)

/**
 * @define JAVACALL_INVALID_FOLDER_ID
 * @brief The folder id that doesn't correspond to any folder
 */
#define JAVACALL_INVALID_FOLDER_ID (-1)

/**
 * @define JAVACALL_ROOT_FOLDER_ID
 * @brief ID of the root folder
 */
#define JAVACALL_ROOT_FOLDER_ID (-2)

/**
 * @define JAVACALL_INVALID_STORAGE_ID
 * @brief The storage id that doesn't correspond to any storage
 */
#define JAVACALL_INVALID_STORAGE_ID (-1)

/**
 * @define JAVACALL_INVALID_APP_ID
 * @brief The application id that doesn't correspond to any running application
 */
#define JAVACALL_INVALID_APP_ID (-1)

/**
 * @typedef javacall_utf16_string
 * @brief general utf16 string type, this type is null terminated string
 */
typedef javacall_utf16* javacall_utf16_string;

/**
 * @typedef javacall_const_utf16_string
 * @brief general constant utf16 string type, this type is constant null
 * terminated string
 */
typedef const javacall_utf16* javacall_const_utf16_string;

/**
 * @typedef javacall_utf8_string
 * @brief general utf8 string type, this type is null terminated string
 */
typedef unsigned char* javacall_utf8_string;

/**
 * @typedef javacall_const_utf8_string
 * @brief general constant utf8 string type, this type is constant null
 * terminated string
 */
typedef const unsigned char* javacall_const_utf8_string;

/**
 * @typedef javacall_ascii_string
 * @brief general eight-bit ASCII string type,
 *        this type is null terminated string
 */
typedef char* javacall_ascii_string;

/**
 * @typedef javacall_const_ascii_string
 * @brief general constant eight-bit ASCII string type,
 *        this type is constant null terminated string
 */
typedef const char* javacall_const_ascii_string;

/**
 * @typedef javacall_pixel
 * @brief Pixel type for LCD graphics
 */
#if ENABLE_DYNAMIC_PIXEL_FORMAT
typedef unsigned short javacall_pixel16;
typedef unsigned int javacall_pixel32;
typedef javacall_pixel32 javacall_pixel;
#elif ENABLE_32BITS_PIXEL_FORMAT
typedef unsigned int javacall_pixel;
#else
typedef unsigned short javacall_pixel;
typedef unsigned short javacall_pixel16;
#endif

/**
 * A list of properties that can be searched by a key.
 *
 * IMPL_NOTE: should be moved to nams/javacall_ams_common.h
 */
typedef struct _javacall_ams_properties {
    /**
     * Number of properties, there are 2 Strings (key/value)
     * for each property.
     */
    int numberOfProperties;
    javacall_utf16_string* pStringArr;
} javacall_ams_properties;

/**
 * @define JAVACALL_UNKNOWN_LENGTH
 * @brief Corresponds to unknown length
 * @note  Be careful with bit-depth context with signed-unsigned conversion: <br>
 * May happen that (\c unsigned \c int)JAVACALL_UNKNOWN_LENGTH != (\c long \c long)JAVACALL_UNKNOWN_LENGTH 
 */
#define JAVACALL_UNKNOWN_LENGTH (-1)

/**
 * @}
 */
#ifdef __cplusplus
} // extern "C"
#endif
/**
 * @defgroup JTWI JTWI API
 *
 * This document describes the requirements for implementing JTWI 1.0 
 * ( Java Technology for the Wireless Industry, JSR 185). <br>
 * JTWI specifies the technologies that must be included in all JTWI-compliant devices: 
 * CLDC 1.0 (JSR 30), 
 * MIDP 2.0 (JSR 118), 
 * WMA 1.1 (JSR 120), 
 * as well as CLDC 1.1 (JRS 139) and 
 * MMAPI (JSR 135) where applicable.
 *
 * The specifications be found at: http://jcp.org/en/jsr/detail?id=185
 * 
 * @{
 * @}
 */

#endif /* __JAVACALL_DEFINE_H_ */

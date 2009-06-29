/*
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
#ifndef __JAVACALL_PLATFORM_DEFINE_H_
#define __JAVACALL_PLATFORM_DEFINE_H_

/**
 * @file javacall_platform_defs.h
 * @ingroup Common
 * @brief Platform-dependent definitions for javacall
 */

/**
 * @defgroup Platform Platfrom specific Common Javacall API Definitions
 * @ingroup Common
 * The platform specific common javacall definitions are defined in this file
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif 

/**
 * @typedef javacall_utf16
 * @brief general unicode string type
 */
typedef unsigned short javacall_utf16;

/**
 * @typedef javacall_uint8
 * @brief 8 bit unsigned interger type
 */
typedef unsigned __int8 javacall_uint8;

/**
 * @typedef javacall_uint16
 * @brief 16 bit unsigned interger type
 */
typedef unsigned __int16 javacall_uint16;

/**
 * @typedef javacall_uint32
 * @brief 32 bit unsigned interger type
 */
typedef unsigned __int32 javacall_uint32;

/**
 * @typedef javacall_uint64
 * @brief 64 bit unsigned integer type
 */
typedef unsigned __int64 javacall_uint64;

/**
 * @typedef javacall_int16
 * @brief 16 bit signed interger type
 */
typedef signed short javacall_int16;

/**
 * @typedef javacall_int32
 * @brief 32 bit interger type
 */
typedef __int32 javacall_int32;

/**
 * @typedef javacall_int64
 * @brief 64 bit interger type
 */
typedef __int64 javacall_int64;

/**
 * @typedef javacall_filename_string
 * @brief general file name string type, this type is null terminated string
 */
typedef javacall_utf16* javacall_filename_string;

/**
 * @typedef javacall_const_filename_string
 * @brief general constant file name string type, this type is constant null
 * terminated string
 */
typedef const javacall_utf16* javacall_const_filename_string;

/**
 * @def JAVACALL_MAX_FILE_NAME_LENGTH
 * Maximal length of filename supported 
 */
#define JAVACALL_MAX_FILE_NAME_LENGTH         256

/**
 * @def JAVACALL_MAX_ILLEGAL_FILE_NAME_CHARS
 * Maximal number of illegal chars
 */
#define JAVACALL_MAX_ILLEGAL_FILE_NAME_CHARS  256

/**
 * @def JAVACALL_MAX_ROOTS_LIST_LENGTH
 * Maximal length of a list of file system roots
 */
#define JAVACALL_MAX_ROOTS_LIST_LENGTH  1024

/**
 * @def JAVACALL_MAX_ROOT_PATH_LENGTH
 * Maximal length of a file system root path
 */
#define JAVACALL_MAX_ROOT_PATH_LENGTH   256

/**
 * @def JAVACALL_MAX_LOCALIZED_ROOTS_LIST_LENGTH
 * Maximal length of a list of localized names of file system roots
 */
#define JAVACALL_MAX_LOCALIZED_ROOTS_LIST_LENGTH  1024

/**
 * @def JAVACALL_MAX_LOCALIZED_DIR_NAME_LENGTH
 * Maximal length of a localized name of a special directory
 */
#define JAVACALL_MAX_LOCALIZED_DIR_NAME_LENGTH    512

/**
 * @def JAVACALL_PIM_MAX_ARRAY_ELEMENTS
 * 
 */
#define JAVACALL_PIM_MAX_ARRAY_ELEMENTS (10)   
/**
 * @def JAVACALL_PIM_MAX_ATTRIBUTES
 * 
 */
#define JAVACALL_PIM_MAX_ATTRIBUTES     (15)
/**
 * @def JAVACALL_PIM_MAX_FIELDS
 * 
 */
#define JAVACALL_PIM_MAX_FIELDS         (19)

/** 
 * @def JAVACALL_FONT_SIZE_SMALL
 *  
 */
#define JAVACALL_FONT_SIZE_SMALL    8
/** 
 * @def JAVACALL_FONT_SIZE_MEDIUM
 *  
 */
#define JAVACALL_FONT_SIZE_MEDIUM   12
/** 
 * @def JAVACALL_FONT_SIZE_LARGE
 *  
 */
#define JAVACALL_FONT_SIZE_LARGE    16

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif 



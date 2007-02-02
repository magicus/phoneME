/*
 * $LastChangedDate: 2006-08-10 18:32:51 +0400 (Thu, 10 Aug 2006) $ 
 *
 * Copyright (c) 2005 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */ 
#ifndef __JAVACALL_PLATFORM_DEFINE_H_
#define __JAVACALL_PLATFORM_DEFINE_H_

/**
 * @file javacall_platform_defs.h
 * @brief Platform-dependent definitions for javacall
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
 * @typedef javacall_int32
 * @brief 32 bit interger type
 */
typedef signed int javacall_int32;

/**
 * @typedef javacall_int64
 * @brief 64 bit interger type
 */
typedef __int64 javacall_int64;  // This type shall be redefined for non MSC compiler!!

/**
 * @def JAVACALL_MAX_FILE_NAME_LENGTH
 * Maximal length of filename supported 
 */
#define JAVACALL_MAX_FILE_NAME_LENGTH         128

/**
 * @def JAVACALL_MAX_ILLEGAL_FILE_NAME_CHARS
 * Maximal number of illegal chars
 */
#define JAVACALL_MAX_ILLEGAL_FILE_NAME_CHARS  128

#ifdef __cplusplus
}
#endif

#endif 



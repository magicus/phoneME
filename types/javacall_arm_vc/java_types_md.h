/*
 * @(#)java_types_md.h	1.7 06/04/12 06:24:46
 *
 * Copyright 2006 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/**
 * @file
 * @ingroup types
 * @brief Basic types for armsd_arm_ads configuration
 */ 

#if !defined _JAVA_TYPES_H_
# error "Never include <java_types_md.h> directly; use <java_types.h> instead."
#endif

#ifndef _JAVA_TYPES_MD_H_
#define _JAVA_TYPES_MD_H_

/** Byte parameter type. */
typedef signed char jbyte;

/** Char parameter type. */
typedef unsigned short jchar;

/** Integer parameter type. */
typedef int         jint;

/** Long parameter type. */
typedef long long   jlong;

/** Platform-specific type specifier for 64-bit integer */
#define PCSL_LLD "%lld"

#endif /* !_JAVA_TYPES_MD_H_ */

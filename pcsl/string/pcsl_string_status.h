/*
 *  
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

#ifndef _PCSL_STRING_STATUS_H_
#define _PCSL_STRING_STATUS_H_

/**
 * @file
 * @ingroup string
 */

/**
 * Return status values for PCSL string library.
 */
typedef enum {
  PCSL_STRING_OK     =  0,                /* success */
  PCSL_STRING_ERR    = (int)0xFFFFFFFF,   /* unknown error */
  PCSL_STRING_ENOMEM = -4,                /* not enough memory */
  PCSL_STRING_EINVAL = -6,                /* invalid arguments */
  PCSL_STRING_EILSEQ = -7,                /* malformed sequence of bytes */
  PCSL_STRING_BUFFER_OVERFLOW = -8,       /* specified buffer is too small */
  PCSL_STRING_OVERFLOW        = -9        /* arithmetic overflow  */
} pcsl_string_status;

#endif /* _PCSL_STRING_STATUS_H_ */



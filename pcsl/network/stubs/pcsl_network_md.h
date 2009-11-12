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
#ifndef _PCSL_NETWORK_MD_H
#define _PCSL_NETWORK_MD_H
/**
 * @file
 * @ingroup network
 *
 * Stubs definitions for pcsl network network
 */ 

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The value which valid handle returned by pcsl network functions cannot have
 */
#define INVALID_HANDLE_MD ((void*)-1)

/**
 * Maximum length of array of bytes sufficient to hold IP address
 */
#define MAX_ADDR_LENGTH_MD 256

/**
 * Maximum host name length
 */
#define MAX_HOST_LENGTH_MD 256

#ifdef __cplusplus
}
#endif


#endif /* _PCSL_NETWORK_MD_H */

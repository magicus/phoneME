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
#ifndef _MIDP_RESOURCE_LIMIT_H
#define _MIDP_RESOURCE_LIMIT_H

/**
 * @defgroup core_resmanager Resource Manager External Interface
 * @ingroup core
 */

/**
 * @file
 * @ingroup core_resmanager
 *
 * @brief This file declares interfaces to resource consumption table. 
 * 
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enumerated resource types
 */
typedef enum {
    RSC_TYPE_TCP_CLI = 0,
    RSC_TYPE_TCP_SER,
    RSC_TYPE_UDP,
    RSC_TYPE_FILE,
    RSC_TYPE_AUDIO_CHA,
    RSC_TYPE_IMAGE_MUT,
    RSC_TYPE_IMAGE_IMMUT,
    RSC_TYPE_COUNT /* Total number of resources */
} RscType; 

/**
 * Allocate reserved resources and initiate limits for current Isolate.
 *
 * @return true if the resources are available, false otherwize
 */
extern int midpAllocateReservedResources();

/**
 * Free the reserved resources for current Isolate.
 */
extern void midpFreeReservedResources();

/**
 * Verify that the resource limit is not crossed. IsolateID will 
 * internally be fetched from getCurrentIsolateId() as defined in midpServices.h
 *
 * @param type Resource type
 * @param requestSize Requesting size
 *
 * @return 1 if resource limit is not crossed, otherwise 0
 *
 */
extern int midpCheckResourceLimit (RscType type, int requestSize);

/**
 * Increment the resource consumption count. IsolateID will 
 * internally be fetched from getCurrentIsolateId() as defined in midpServices.h
 * This function should be called strictly from the Java native function 
 * which would have been triggered by corresponding Java thread.
 *
 * @param type Resource type
 * mode the resource limit is always checked against the global limit.  
 * @param delta requesting size
 *
 * @return 1 if count is successfully incremented, otherwise 0
 *
 */
extern int midpIncResourceCount(RscType type, int delta);

/**
 * Decrement the resource consumption count. IsolateID will 
 * internally be fetched from getCurrentIsolateId() as defined in midpServices.h
 * This function should be called strictly from the Java native function 
 * which would have been triggered by corresponding Java thread.
 *
 * @param type Resource type
 * mode the resource limit is always checked against the global limit.  
 * @param delta requesting size
 *
 * @return 1 if count is successfully decremented, otherwise 0
 *
 */
extern int midpDecResourceCount(RscType type, int delta);

/**
 * Finalize the Resource limit structures.
 */
extern void midpFinalizeResourceLimit(void);

#ifdef __cplusplus
}
#endif

#endif /* _MIDP_RESOURCE_LIMIT_H_ */

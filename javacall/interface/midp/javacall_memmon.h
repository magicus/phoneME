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

#ifndef __JAVACALL_MEMMON_H_
#define __JAVACALL_MEMMON_H_

/**
 * @file javacall_memmon.h
 * @ingroup Input
 * @brief Javacall interfaces for memory monitoring
 */

#ifdef __cplusplus
extern "C" {
#endif
    
/******************************************************************************
 ******************************************************************************
 ******************************************************************************

  NOTIFICATION FUNCTIONS
  - - - -  - - - - - - -  
  The following functions are implemented by Sun.
  Platform is required to invoke these function for each occurence of the
  undelying event.
  The functions need to be executed in platform's task/thread

 ******************************************************************************
 ******************************************************************************
 ******************************************************************************/
    
/**
 * @defgroup WtkSdkNotification API for processing events from WTK or SDK 
 * @ingroup Input
 *
 * The following callback function must be called by the platform when an event from WTK or SDK is received
 *
 * @{
 */        
 
/**
 * @enum javacall_wtk_sdk_type
 * @brief Event from WTK or SDK type
 */
typedef enum {
   /** Run GC */
   runGC            = -1000,
   /** Stop memory monitoring */
   stopMonMemory    = -1001
} javacall_wtk_sdk_type;

/**
 * The notification function to be called by CLDC garbage 
 * collection.
 */
void javanotify_run_GC(void);

/**
 * The notification function to be stopped by CLDC memory
 * monitoring.
 */
void javanotify_stop_memmon(void);
    
/** @} */    
    
#ifdef __cplusplus
}
#endif

#endif 


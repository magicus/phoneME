/*
 * 
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

#ifndef _JSR211_NAMS_INSTALLER_H_
#define _JSR211_NAMS_INSTALLER_H_

/**
 * @file
 * @defgroup chapi JSR 211 Content Handler API (CHAPI)
 * @ingroup stack
 * @brief This is the JSR211 API definition for Native AMS mode. 
 * Some functionality added for processing JSR211 specific attributes during
 * MIDlet suite installation.
 * 
 * @{
 */

#include <pcsl_string.h>

#ifdef __cplusplus
extern "C" {
#endif/*__cplusplus*/

/**
 * Variable for error codes generated from jsr211 functions.
 */
extern int jsr211_errCode;

/**
 * Finds JSR 211 declared attributes, verifies its and 
 * prepares Content Handlers structures for registration.
 *
 * @param jadsmp JAD properties
 * @param mfsmp Manifest properties
 * @param jarHandle processed jar handle for class presence test
 * @param trusted whether installing suit is trusted
 *
 * @return number of Conntent Handlers found 
 * or -1 in case of error. Error code is placed to jsr211_errCode variable.
 */
int jsr211_verify_handlers(MidpProperties jadsmp, MidpProperties mfsmp,
                                            void* jarHandle, jchar trusted);

/**
 * Stores installing Content Handlers in the JSR211 complaint registry.
 * @param suiteId installing suite Id
 * @return 0 if handlers are stored successfully.
 */
int jsr211_store_handlers(SuiteIdType suiteId);

/**
 * Removes registered Content Handlers from the JSR211 Registry.
 * @param suiteId removing suite Id
 */
void jsr211_remove_handlers(SuiteIdType suiteId);

#ifdef __cplusplus
}
#endif/*__cplusplus*/

/** @} */

#endif  /* _JSR211_NAMS_INSTALLER_H_ */

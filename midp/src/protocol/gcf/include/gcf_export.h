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

/**
 * @defgroup network Networking - Porting Interface
 * @ingroup subsystems
 */

/**
 * @file
 * @ingroup network
 * 
 * Prototypes for supporting sockets and serversockets on top of CLDC's
 * Generic Connection Framework (GCF).  A target platform that supports these
 * protocols must have a platform-specific implementation of this file's
 * functions.
 */

#ifndef _GCF_EXPORT_H_
#define _GCF_EXPORT_H_

#ifdef __cplusplus
extern "C" {
#endif

/*=========================================================================
 * Function prototypes for the native networking operations
 *=======================================================================*/

/**
 * Gets the name of the local device from the system. This method is
 * called when the <tt>microedition.hostname</tt> system property
 * is retrieved.
 *
 * @return the name of this device
 */
extern char* getLocalHostName();


#ifdef __cplusplus
}
#endif

#endif /* _GCF_EXPORT_H_ */

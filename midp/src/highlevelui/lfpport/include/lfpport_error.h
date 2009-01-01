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

#ifndef _LFPPORT_ERROR_H_
#define _LFPPORT_ERROR_H_

/**
 * @file
 * @ingroup highui_lfpport
 *
 * @brief Error codes for the platform dependent functions.
 */

#include <kni.h>

/**
 * Return values for the LCDUI porting-layer functions.
 * Because KNI invokes many of the LCDUI porting-layer functions,
 * they reuse the the following KNI return values.
 * <ul>
 *  <li>KNI_OK - success</li>
 *  <li>KNI_ERR - unknown error</li>
 *  <li>KNI_ENOMEM - not enough memory</li>
 *  <li>KNI_EINVAL - invalid arguments</li>
 * </ul>
 * 
 * Ports that do not use KNI need to change this type to remove the
 * KNI dependency.
 *
 */
typedef jint MidpError;

#endif /* _LFPPORT_ERROR_H_ */

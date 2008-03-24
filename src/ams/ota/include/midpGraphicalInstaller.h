/*
 *
 * Copyright 1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
#ifndef _GRAPHICAL_INSTALLER_H_
#define _GRAPHICAL_INSTALLER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "kni.h"

/**
 * Notify the platform of the installation status
 * 
 * @param status the installation status
 *        <code>KNI_TRUE</code> if the installation was successful,
 *        <code>KNI_FALSE</code> otherwise  
 */
void midpNotifyInstallStatus(jboolean status);

#ifdef __cplusplus
}
#endif

#endif /* _GRAPHICAL_INSTALLER_H_ */


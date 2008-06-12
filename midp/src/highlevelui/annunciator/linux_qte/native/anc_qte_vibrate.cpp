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
 * 
 * This source file is specific for Qt-based configurations.
 */

#include <kni.h>
#include <anc_vibrate.h>


/**
 * @file
 *
 * @brief Native code to handle vibrate control
 *
 * @note If the target platform does not have vibrate capability 
 * then there is no need to modify this file.  The reason why 
 * this file exists in a common directory is because currently 
 * all modules under LCDUI handle the vibrate the same 
 * way - i.e. no implementation.
 */


/**
 * Platform dependent implementation of startVibrate
 *
 * @note start vibrate is not implemented, as planned.
 * @return KNI_FALSE:  this device does not support vibrate
 */
jboolean anc_start_vibrate()
{
    return KNI_FALSE;
}


/**
 * Platform dependent implementation of stopVibrate.
 *
 * @note stop vibrate is not implemented, as planned.
 * @return KNI_FALSE:  this device does not support vibrate
 */
jboolean anc_stop_vibrate(void)
{
    return KNI_FALSE;  // vibrate not supported
}


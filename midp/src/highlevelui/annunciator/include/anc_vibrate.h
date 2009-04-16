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

#ifndef _ANC_VIBRATE_H_
#define _ANC_VIBRATE_H_

/**
 * @file
 * @ingroup highui_anc
 *
 * @brief Interface for the LCDUI vibrate functions
 */

#include <kni.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Vibrate porting function: Stop vibration.
 *
 * Platform should stop the vibration at once when 
 * this function is called.
 * @return KNI_TRUE when the system supports
 * vibrate control via this function,
 * KNI_FALSE otherwise
 */ 
extern jboolean anc_stop_vibrate(void);

/**
 * Vibrate porting function: Start vibration.
 *
 * Platform should start the vibration at once when 
 * this function is called.
 *
 * @return KNI_TRUE when the system supports 
 * vibrate control via this function, 
 * KNI_FALSE otherwise
 */
extern jboolean anc_start_vibrate();

#ifdef __cplusplus
}
#endif

#endif /* _ANC_VIBRATE_H_ */

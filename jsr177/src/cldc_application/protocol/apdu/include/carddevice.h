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
#ifndef _CARDDEVICE_H_
#define _CARDDEVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @defgroup satsa JSR177 Security and Trust Services API (SATSA)
 * @ingroup stack
 */

/**
 * @defgroup carddevice SATSA Native Card Driver Interface
 * @ingroup satsa
 * @brief SATSA smart card driver porting interface. \n
 * ##include <carddevice.h>
 * @{
 *
 * This file defines the SATSA porting interface to a smart card
 * device driver.
 * The functions defined here are called
 * from the SATSA native card access implementation to handle
 * specific operations for locking device communication
 * and performing data exchange with smart card applets.
 */

/* This constant is used only in javacall_carddevice_reset_start and javacall_carddevice_reset_finish functions. */
#define SIGNAL_RESET    0x7781

/* This constant is used only in javacall_carddevice_xfer_data_start and javacall_carddevice_xfer_data_finish functions. */
#define SIGNAL_XFER     0x7782

/* This constant is used only in javacall_carddevice_lock and javacall_carddevice_unlock functions. */
#define SIGNAL_LOCK     0x7783

#ifdef __cplusplus
}
#endif

#endif /* ifndef _CARDDEVICE_H_ */

/*
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
#ifndef __JAVACALL_ODD_H_
#define __JAVACALL_ODD_H_

/**
 * @file javacall_odd.h
 * @ingroup OnDeviceDebug
 * @brief Javacall interfaces for On Device Debug
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_defs.h"

/**
 * @defgroup OnDeviceDebug API
 * @ingroup JTWI
 *
 * OnDeviceDebug APIs define the functionality for:
 * 
 * - Enabling the ODTAgent midlet
 * 
 *  @{
 */

/**
 * The platform calls this function to inform VM that
 * ODTAgent midlet must be enabled.
 */
void javanotify_enable_odd(void);

/**
 * debugger calls this function on start-up to allow ODT to initialize any global resources
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_odt_initialize();

/**
 * debugger calls this function when debugged Isolate starts.
 * ODT should open all required connections e.g., KDWP, stdout
 *
 * @param port KDWP port
 * @param pHandle address of variable to receive the handle
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_odt_open_channel(int port, void **pHandle);

/**
 * debugger calls this function when debugged Isolate exits.
 * ODT should close all open connections e.g., KDWP, stdout
 *
 * @param handle address of variable to receive the handle
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_odt_close_channel(javacall_handle handle);

/**
 * debugger calls this function before reading a full KDWP packet
 *
 * @param handle address of variable to receive the handle
 * @param pBytesAvailable number of bytes available
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_odt_is_available(javacall_handle handle, int *pBytesAvailable);

/**
 * debugger calls this function to write a KDWP packet
 *
 * @param handle handle of an open connection
 * @param pData base of buffer containing data to be written
 * @param len number of bytes to attempt to write
 * @param pBytesWritten returns the number of bytes written after
 *        successful write operation; only set if this function returns
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_odt_write_bytes(javacall_handle handle, char *pData, int len, int *pBytesWritten);

/**
 * debugger calls this function to read a KDWP packet
 *
 * @param handle handle of an open connection
 * @param pData base of buffer to receive read data
 * @param len number of bytes to attempt to read
 * @param pBytesRead returns the number of bytes actually read; it is
 *        set only when this function returns JAVACALL_OK
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_odt_read_bytes(javacall_handle handle, unsigned char *pData, int len, int *pBytesRead);

/**
 * debugger calls this function to redirect system output
 *
 * @param handle handle of an open connection
 * @param pData base of buffer containing data to be written
 * @param len number of bytes to attempt to write
 * @param pBytesWritten returns the number of bytes written after
 *        successful write operation; only set if this function returns
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_odt_redirect_output(javacall_handle handle, char *pData, int len, int *pBytesWritten);


/** @} */

#ifdef __cplusplus
}
#endif

#endif


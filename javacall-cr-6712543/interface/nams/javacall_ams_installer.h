/*
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
#ifndef __JAVACALL_AMS_INSTALLER_H
#define __JAVACALL_AMS_INSTALLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_defs.h"

/*
 * Functions declared in this file are exported by the Installer.
 * They should be used in case if the Installer is implemented on
 * the MIDP side and the Application Manager UI - on the Platform's
 * side, or vice-versa.
 */


/**
 * @file javacall_ams_installer.h
 * @ingroup NAMS
 * @brief Javacall interfaces of the installer
 */

/**
 * @defgroup InstallerAPI API of the installer
 * @ingroup NAMS
 *
 *
 * @{
 */

/**
 * App Manager invokes this function to start a suite installation.
 *
 * @param pUrl null-terminated http url string of MIDlet's jad file.
 *             The url is of the following form:
 *             http://www.website.com/a/b/c/d.jad
 *             or
 *             file:////a/b/c/d.jad
 *             or
 *             c:\a\b\c\d.jad
 * @param invokeGUI <tt>JAVACALL_TRUE</tt> to invoke Graphical Installer,
 *                  <tt>JAVACALL_FALSE</tt> otherwise
 *
 * @return error code: <tt>JAVACALL_OK</tt> if the installation was
 *                                          successfully started,
 *                     <tt>JAVACALL_FAIL</tt> otherwise
 *
 * The return of this function only tells if the install process is started
 * successfully. The actual result of if the installation (status and ID of
 * the newly installed suite) will be reported later by
 * java_ams_operation_completed().
 */
javacall_result
java_ams_install_suite(const char *pUrl, javacall_bool invokeGUI);

/**
 * Installer invokes this function to inform the task manager about
 * the current installation progress.
 *
 * Note: when installation is completed, javacall_ams_operation_completed()
 *       will be called to report installation status.
 *
 * @param installPercent percents completed (0 - 100)
 */
void
java_ams_installation_percentage(int installPercent);

/**
 * @defgroup ImageCache	Image Cache
 * @ingroup NAMS
 * @brief Java cache the images in JAR file to increase the performance
 *
 *
 * @{
 */

/**
 * Installer informs MIDP that the image cache should be created.
 *
 * @param suiteId unique ID of the MIDlet suite
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
java_ams_create_resource_cache(javacall_suite_id suiteId);

/** @} */
/** @} */

#ifdef __cplusplus
}
#endif

#endif  /* __JAVACALL_AMS_INSTALLER_H */

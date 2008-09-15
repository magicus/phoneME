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

/*
 * This file presents here for consistency only.
 *
 * Indeed, javacall_ams_config.h is generated when building Javacall and is
 * placed in $JAVACALL_OUTPUT_DIR/inc/ directory. It contains AMS
 * configuration-dependent macros used to provide proper javacall/javanotify
 * calling convention suitable for this particular configuration.
 *
 * For example, if the installer is located on the Platform's side and
 * the Application Manager - on the Java side, the declaration of
 * "install a midlet suite" function that is exported by the Installer and
 * is called by the Application Manager will be:
 *
 * javacall_result JCDECL_APPMGR_INST(install_suite)(...);
 *
 * and it will be expanded into:
 *
 * javacall_result javacall_ams_install_suite(...);
 *
 * For the configuration where the Installer is located on the Platform's
 * side and the Application Manager - on the Java side, this definition will
 * be expanded as:
 *
 * javacall_result javanotify_ams_install_suite(...);
 *
 */

/*
 * Configuration for Application Manager UI on the Platform's side + all other
 * components on the Java side:
 */
#define JCDECL_APPMGR_INST(x)       javanotify_ams_##x
#define JCDECL_INST_APPMGR(x)       javacall_ams_##x

#define JCDECL_SUITESTORE(x)        javanotify_ams_##x
#define JCDECL_INST_SUITESTORE(x)   NOT_USED##x
#define JCDECL_APPMGR_SUITESTORE(x) javanotify_ams_##x
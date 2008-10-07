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
 * This file contains AMS configuration-dependent macros used to
 * provide proper javacall/javanotify calling convention suitable
 * for this particular configuration.
 *
 * Macros expansion depends on the Javacall build options defining
 * which AMS components will be implemented on the Java side and
 * which - on the Platform's side.
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
 * IMPL_NOTE: javacall_options.h generation will be implemented later,
 *            for now switch manually.
 */
#if 1 /*ENABLE_NATIVE_AMS_UI*/

/*
 * Configuration for Application Manager UI on the Platform's side + all other
 * components on the Java side:
 */
#define JCDECL_APPMGR_INST(x)       javanotify_ams_##x
#define JCDECL_INST_APPMGR(x)       javacall_ams_##x

#define JCDECL_SUITESTORE(x)        javanotify_ams_##x
#define JCDECL_INST_SUITESTORE(x)   NOT_USED##x
#define JCDECL_APPMGR_SUITESTORE(x) javanotify_ams_##x

#else

/*
 * IMPL_NOTE: currently Javacall build options allow to automatically implement
 *            macros for only two configurations: (1) a "big" monolithics NAMS
 *            (all components are on the Platform's side) and (2) Native UI:
 *            only Application Manager UI is implemented on the Platform's side.
 *
 *            More options will be added in future.
 */
#define JCDECL_APPMGR_INST(x)       NOT_USED##x
#define JCDECL_INST_APPMGR(x)       NOT_USED##x

#define JCDECL_SUITESTORE(x)        NOT_USED##x
#define JCDECL_INST_SUITESTORE(x)   NOT_USED##x
#define JCDECL_APPMGR_SUITESTORE(x) NOT_USED##x

#endif /* ENABLE_NATIVE_AMS_UI */

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
 * @mainpage MIDP Optimized Implementation
 *
 * <h3>Reference Documentation for Native Code</h3>
 *
 * <p>These pages specify the product's native APIs. They
 * describe the header files' contents, including function
 * signatures, globals, and data structures. The pages
 * organize the files both functionally by subsystem and
 * service, and alphabetically. They also index functions,
 * globals, and data structures for easier information access.
 * </p>
 *
 * <p>See the <a href="../../index.html">Documentation Overview</a>
 * for the complete list of product documentation.</p>
 */

/**
 * @defgroup subsystems MIDP Subsystems
 */

 /**
  * @defgroup stack
  */

/**
 * @defgroup ams Application Management System
 * @ingroup subsystems
 */

/**
 * @defgroup cams Common
 * @ingroup ams
 */

/**
 * @defgroup cams_port Porting Interface
 * @ingroup cams
 */

/**
 * @defgroup cams_extern External Interface
 * @ingroup cams
 */

/**
 * @defgroup jams Java Platform AMS - External Interface
 * @ingroup ams
 */

/**
 * @defgroup nams Native AMS
 * @ingroup ams
 */

/**
 * @defgroup nams_extern External Interface
 * @ingroup nams
 */

/**
 * @defgroup nams_port Porting Interface
 * @ingroup nams
 */

/**
 * @defgroup push Push Registry
 * @ingroup subsystems
 */

/**
 * @file
 * @ingroup jams
 *
 * @brief Interface to access application management functions provided by
 * platform independent layer.  Used when integrating Java platform system with
 * native application manager.
 *
 *  Functions defined here do NOT need to be ported and are implemented in
 *  share code.
 */

#ifndef _MIDP_H_
#define _MIDP_H_

/* Include the types needed by this file */
#include <kni.h>
#include <jvm.h>
#include <midpString.h>
#include <suitestore_common.h>

#include <midp_constants_data.h> /* to include the debug option values */
/*
 * Possible debug options are:
 * MIDP_NO_DEBUG, MIDP_DEBUG_SUSPEND, MIDP_DEBUG_NO_SUSPEND.
 * See share/constants.xml.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name Status codes
 * Describe the reasons why the main class is terminated or not run.
 * (Note: Some functions also use 0 for normal.)
 * @{
 */
/** the system is shutting down */
#define MIDP_SHUTDOWN_STATUS 1
/** the system was initialized properly. Used for slave mode */
#define MIDP_RUNNING_STATUS 0
/** single error code for various errors */
#define MIDP_ERROR_STATUS (-1)
/** the MIDlet suite was not found */
#define SUITE_NOT_FOUND_STATUS (-2)
/** @} */

/**
 * The public MIDP initialization function. If not running from the MIDP
 * home directory, midpSetAppDir should be called first with the
 * directory of the MIDP system. The functions must be called before any
 * other MIDP function except midpSetAppDir.
 */
int midpInitialize();

/**
 * Sets the home directory for MIDP if needed.
 * So the suites and other MIDP persistent
 * state can be found. Only had an effect when called before 
 * any other method except midpInitialize is called.
 * If not called directory specified by midpSetConfigDir will 
 * be used.
 * @param dir home directory of MIDP
 */
void midpSetAppDir(const char* dir);

/**
 * Sets the config directory for MIDP if needed.
 * In this directory static data as images and 
 * configuration files are located. If not called
 * directory specified by midpSetAppDir will be used.
 * Only had an effect when called before any
 * other method except midpInitialize is called.
 *
 * @param dir config directory of MIDP
 */
void midpSetConfigDir(const char* dir);

/**
 * Runs the given MIDlet from the specified MIDlet suite with the
 * given arguments. Up to 3 arguments will be made available to
 * the MIDlet as properties <tt>arg-&lt;num&gt;</tt>, where
 * <tt><i>num</i></tt> is <tt>0</tt> for the first argument, etc.
 *
 * @param suiteId The MIDlet Suite ID that the MIDlet is in
 * @param midletClassName The class name of MIDlet to run
 * @param arg0 The first argument for the MIDlet to be run.
 * @param arg1 The second argument for the MIDlet to be run.
 * @param arg2 The third argument for the MIDlet to be run.
 * @param debugOption 0 for no debug, 1 debug: suspend the VM until the
 *   debugger sends a continue command, 2 debug: do not wait for the debugger
 * @param classPathExt additional path to be passed to the VM
 *
 * @return <tt>0</tt> if successful,
 *         <tt>MIDP_SHUTDOWN_STATUS</tt> if the system is shutting down,
 *         <tt>MIDP_ERROR_STATUS</tt> if an error,
 *         <tt>SUITE_NOT_FOUND_STATUS</tt> if the MIDlet suite not found
 */
int midp_run_midlet_with_args_cp(SuiteIdType suiteId,
                                 const pcsl_string* midletClassName,
                                 const pcsl_string* arg0,
                                 const pcsl_string* arg1,
                                 const pcsl_string* arg2,
                                 int debugOption,
                                 char* classPathExt);

/**
 * Runs a MIDlet that has arguments. Up to 3 arguments will be available
 * to the MIDlet as properties arg-&lt;n&gt;, when n is 0 for the first
 * argument.
 *
 * @param suiteId ID of the suite the MIDlet is in
 * @param midletClassName class name of MIDlet to run
 * @param arg0 argument to for the MIDlet to be run.
 * @param arg1 argument to for the MIDlet to be run.
 * @param arg2 argument to for the MIDlet to be run.
 * @param debugOption 0 for no debug, 1 debug: suspend the VM until the
 *   debugger sends a continue command, 2 debug: do not wait for the debugger
 *
 * @return 0 if successful,
 *          1 (MIDP_SHUTDOWN_STATUS) system is shutting down,
 *         -1 (MIDP_ERROR_STATUS) if an error,
 *         -2 (SUITE_NOT_FOUND_STATUS) if MIDlet suite not found
 */
int midp_run_midlet_with_args(SuiteIdType suiteId,
                              const pcsl_string* midletClassName,
                              const pcsl_string* arg0,
                              const pcsl_string* arg1,
                              const pcsl_string* arg2,
                              int debugOption);
#if !ENABLE_CDC
/**
 * Starts the system and instructs the VM to run the main() method of
 * the specified class. Does not return until the system is stopped.
 *
 * @param classPath string containing the class path
 * @param mainClass string containing the main class for the VM to run.
 * @param argc the number of arguments to pass to the main method
 * @param argv the arguments to pass to the main method
 *
 * @return <tt>MIDP_SHUTDOWN_STATUS</tt> if the system is shutting down or
 *         <tt>MIDP_ERROR_STATUS</tt> if an error
 */
int midpRunMainClass(JvmPathChar *classPath,
                     char *mainClass,
                     int argc,
                     char **argv);
#endif

/**
 * Cleans up MIDP resources. This should be last MIDP function called or
 * midpInitialize should be called again if another MIDP function
 * is needed such as running MIDP in a loop.
 */
void midpFinalize();

#ifdef __cplusplus
}
#endif

#endif /* _MIDP_H_ */

/*
 *
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <midp_logging.h>
#include <midpAMS.h>
#include <midpMalloc.h>
#include <jvm.h>
#include <findMidlet.h>
#include <midpUtilKni.h>
#include <suitestore_task_manager.h>
#include <commandLineUtil.h>
#include <commandLineUtil_md.h>
#include <heap.h>
#include <ams_params.h>
#include <midp_properties_port.h>

/** Maximum number of command line arguments. */
#define RUNMIDLET_MAX_ARGS 32

/** Usage text for the run MIDlet executable. */
static const char* const runUsageText =
"\n"
"Usage: runMidlet [<VM args>] [-debug] [-loop] [-classpathext <path>]\n"
"           (-ordinal <suite number> | <suite ID>)\n"
"           [<classname of MIDlet to run> [<arg0> [<arg1> [<arg2>]]]]\n"
"         Run a MIDlet of an installed suite. If the classname\n"
"         of the MIDlet is not provided and the suite has multiple MIDlets,\n"
"         the first MIDlet from the suite will be run.\n"
"          -debug: start the VM suspended in debug mode\n"
"          -loop: run the MIDlet in a loop until system shuts down\n"
"          -classpathext <path>: append <path> to classpath passed to VM\n"
"             (can access classes from <path> as if they were romized)\n"
"\n"
"  where <suite number> is the number of a suite as displayed by the\n"
"  listMidlets command, and <suite ID> is the unique ID a suite is \n"
"  referenced by\n\n";

/**
 * Runs a MIDlet from an installed MIDlet suite. This is an example of
 * how to use the public MIDP API.
 *
 * @param argc The total number of arguments
 * @param argv An array of 'C' strings containing the arguments
 *
 * @return <tt>0</tt> for success, otherwise <tt>-1</tt>
 *
 * IMPL_NOTE:determine if it is desirable for user targeted output
 *       messages to be sent via the log/trace service, or if
 *       they should remain as printf calls
 */
int
runMidlet(int argc, char** commandlineArgs) {
    int status = -1;
    SuiteIdType suiteId   = UNUSED_SUITE_ID;
    pcsl_string classname = PCSL_STRING_NULL;
    pcsl_string arg0 = PCSL_STRING_NULL;
    pcsl_string arg1 = PCSL_STRING_NULL;
    pcsl_string arg2 = PCSL_STRING_NULL;
    int repeatMidlet = 0;
    char* argv[RUNMIDLET_MAX_ARGS];
    int i, used;
    int debugOption = MIDP_NO_DEBUG;
    char *progName = commandlineArgs[0];
    char* appDir = NULL;
    char* confDir = NULL;
    char* additionalPath;
    SuiteIdType* pSuites = NULL;
    int numberOfSuites = 0;
    int ordinalSuiteNumber = -1;
    char* chSuiteNum = NULL;
    MIDPError errCode;
    char** ppParamsFromPlatform;
    char** ppSavedParams = NULL;
    int savedNumberOfParams = 0, numberOfParams = 0;

    JVM_Initialize(); /* It's OK to call this more than once */

    /* get midp application directory, set it */
    appDir = getApplicationDir(argv[0]);
    if (appDir == NULL) {
        REPORT_ERROR(LC_AMS, "Failed to recieve midp application directory");
        return -1;
    }

    midpSetAppDir(appDir);

    /* get midp configuration directory, set it */
    confDir = getConfigurationDir(argv[0]);
    if (confDir == NULL) {
        REPORT_ERROR(LC_AMS, "Failed to recieve midp configuration directory");
        return -1;
    }
    
    midpSetConfigDir(confDir);

    if (midpInitialize() != 0) {
        REPORT_ERROR(LC_AMS, "Not enough memory");
        return -1;
    }

    /* Set Java heap parameters now so they can been overridden from command line */
    setHeapParameters();

    /*
     * Check if there are some parameters passed to us from the platform
     * (i.e., in the current implementation, they are read from a file).
     */
    errCode = ams_get_startup_params(&ppParamsFromPlatform, &numberOfParams);
    if (errCode == ALL_OK && numberOfParams > 0) {
        savedNumberOfParams = numberOfParams;
        ppSavedParams = ppParamsFromPlatform;

        while ((used = JVM_ParseOneArg(numberOfParams,
                                       ppParamsFromPlatform)) > 0) {
            numberOfParams -= used;
            ppParamsFromPlatform += used;
        }

        if (numberOfParams + 1 > RUNMIDLET_MAX_ARGS) {
            REPORT_ERROR(LC_AMS, "(1) Number of arguments exceeds supported limit");
	        ams_free_startup_params(ppSavedParams, savedNumberOfParams);
	        return -1;
	    }

        argv[0] = progName; 
	    for (i = 0; i < numberOfParams; i++) {
            /* argv[0] is the program name */
            argv[i + 1] = ppParamsFromPlatform[i];
        }
    }

    /* if savedNumberOfParams > 0, ignore the command-line parameters */
    if (savedNumberOfParams <= 0) {
        /* 
         * Debugger port: command-line argument overrides 
         * configuration settings. 
         */
        {
            char* debuggerPortString =
              midpRemoveCommandOption("-port", commandlineArgs, &argc);
            if (debuggerPortString != NULL) {
                int debuggerPort;
                if (sscanf(debuggerPortString, "%d", &debuggerPort) != 1) {
                    REPORT_ERROR(LC_AMS, "Invalid debugger port format");
                    return -1;
                }

                setInternalProperty("VmDebuggerPort", debuggerPortString);
            }
        }

        /*
         * Parse options for the VM. This is desirable on a 'development' platform
         * such as linux_qte. For actual device ports, copy this block of code only
         * if your device can handle command-line arguments.
         */

        /*
         * JVM_ParseOneArg expects commandlineArgs[0] to contain the first actual
         * parameter
         */
        argc --;
        commandlineArgs ++;

        while ((used = JVM_ParseOneArg(argc, commandlineArgs)) > 0) {
            argc -= used;
            commandlineArgs += used;
        }

        /* Restore commandlineArgs[0] to contain the program name. */
        argc ++;
        commandlineArgs --;
        commandlineArgs[0] = progName;
    }

    /*
     * Not all platforms allow rewriting the command line arg array,
     * make a copy
     */
    if ((numberOfParams <= 0 && argc > RUNMIDLET_MAX_ARGS) ||
        (numberOfParams > RUNMIDLET_MAX_ARGS)) {
        REPORT_ERROR(LC_AMS, "Number of arguments exceeds supported limit");
        ams_free_startup_params(ppSavedParams, savedNumberOfParams);
        return -1;
    }

    if (savedNumberOfParams <= 0) {
        for (i = 0; i < argc; i++) {
            argv[i] = commandlineArgs[i];
        }
    } else {
        /*
         * if savedNumberOfParams is greater than zero, command-line parameters
         * are ignored
         */
        argc = numberOfParams + 1; /* +1 because argv[0] is the program name */
    }

    /*
     * IMPL_NOTE: corresponding VM option is called "-debugger"
     */
    if (midpRemoveOptionFlag("-debug", argv, &argc) != NULL) {
        debugOption = MIDP_DEBUG_SUSPEND;
    }

    if (midpRemoveOptionFlag("-loop", argv, &argc) != NULL) {
        repeatMidlet = 1;
    }

    /* run the midlet suite by its ordinal number */
    if ((chSuiteNum = midpRemoveCommandOption("-ordinal",
                                              argv, &argc)) != NULL) {
        /* the format of the string is "number:" */
        if (sscanf(chSuiteNum, "%d", &ordinalSuiteNumber) != 1) {
            REPORT_ERROR(LC_AMS, "Invalid suite number format");
            ams_free_startup_params(ppSavedParams, savedNumberOfParams);
            return -1;
        }
    }

    /* additionalPath gets appended to the classpath */
    additionalPath = midpRemoveCommandOption("-classpathext", argv, &argc);

    if (argc == 1 && ordinalSuiteNumber == -1) {
        REPORT_ERROR(LC_AMS, "Too few arguments given.");
        ams_free_startup_params(ppSavedParams, savedNumberOfParams);
        return -1;
    }

    if (argc > 6) {
        REPORT_ERROR(LC_AMS, "Too many arguments given\n");
        ams_free_startup_params(ppSavedParams, savedNumberOfParams);
        return -1;
    }

    do {
        int onlyDigits;
        int len;
        int i;

        if (argc > 5) {
            if (PCSL_STRING_OK != pcsl_string_from_chars(argv[5], &arg2)) {
                REPORT_ERROR(LC_AMS, "Out of Memory");
                break;
            }
        }

        if (argc > 4) {
            if (PCSL_STRING_OK != pcsl_string_from_chars(argv[4], &arg1)) {
                REPORT_ERROR(LC_AMS, "Out of Memory");
                break;
            }
        }

        if (argc > 3) {
            if (PCSL_STRING_OK != pcsl_string_from_chars(argv[3], &arg0)) {
                REPORT_ERROR(LC_AMS, "Out of Memory");
                break;
            }
        }

        if (argc > 2) {
            if (PCSL_STRING_OK != pcsl_string_from_chars(argv[2], &classname)) {
                REPORT_ERROR(LC_AMS, "Out of Memory");
                break;
            }

        }

        /* if the storage name only digits, convert it */
        onlyDigits = 1;
        len = strlen(argv[1]);
        for (i = 0; i < len; i++) {
            if (!isdigit((argv[1])[i])) {
                onlyDigits = 0;
                break;
            }
        }

        if (ordinalSuiteNumber != -1 || onlyDigits) {
            /* load IDs of the installed suites */
            MIDPError err = midp_get_suite_ids(&pSuites, &numberOfSuites);
            if (err != ALL_OK) {
                REPORT_ERROR1(LC_AMS, "Error in midp_get_suite_ids(), code %d",
                              err);
                break;
            }
        }

        if (ordinalSuiteNumber != -1) {
            /* run the midlet suite by its ordinal number */
            if (ordinalSuiteNumber > numberOfSuites || ordinalSuiteNumber < 1) {
                REPORT_ERROR(LC_AMS, "Suite number out of range");
                midp_free_suite_ids(pSuites, numberOfSuites);
                break;
            }

            suiteId = pSuites[ordinalSuiteNumber - 1];
        } else if (onlyDigits) {
            /* run the midlet suite by its ID */
            int i;

            /* the format of the string is "number:" */
            if (sscanf(argv[1], "%d", &suiteId) != 1) {
                REPORT_ERROR(LC_AMS, "Invalid suite ID format");
                break;
            }

            for (i = 0; i < numberOfSuites; i++) {
                if (suiteId == pSuites[i]) {
                    break;
                }
            }

            if (i == numberOfSuites) {
                REPORT_ERROR(LC_AMS, "Suite with the given ID was not found");
                break;
            }
        } else {
            /* Run by ID */
            suiteId = INTERNAL_SUITE_ID;

            if (strcmp(argv[1], "internal") &&
                strcmp(argv[1], "-1") && additionalPath == NULL) {
                /*
                 * If the argument is not a suite ID, it might be a full
                 * path to the midlet suite's jar file.
                 * In this case this path is added to the classpath and
                 * the suite is run without installation (it is useful
                 * for internal test and development purposes).
                 */
                additionalPath = argv[1];
            }
        }

        if (pcsl_string_is_null(&classname)) {
            int res = find_midlet_class(suiteId, 1, &classname);
            if (OUT_OF_MEM_LEN == res) {
                REPORT_ERROR(LC_AMS, "Out of Memory");
                break;
            }

            if (NULL_LEN == res) {
                REPORT_ERROR(LC_AMS, "Could not find the first MIDlet");
                break;
            }
        }

        do {
            status = midp_run_midlet_with_args_cp(suiteId, &classname,
                                                  &arg0, &arg1, &arg2,
                                                  debugOption, additionalPath);
        } while (repeatMidlet && status != MIDP_SHUTDOWN_STATUS);

        if (pSuites != NULL) {
            midp_free_suite_ids(pSuites, numberOfSuites);
            suiteId = UNUSED_SUITE_ID;
        }
    } while (0);

    pcsl_string_free(&arg0);
    pcsl_string_free(&arg1);
    pcsl_string_free(&arg2);
    pcsl_string_free(&classname);

    switch (status) {
    case MIDP_SHUTDOWN_STATUS:
        break;

    case MIDP_ERROR_STATUS:
        REPORT_ERROR(LC_AMS, "The MIDlet suite could not be run.");
        break;

    case SUITE_NOT_FOUND_STATUS:
        REPORT_ERROR(LC_AMS, "The MIDlet suite was not found.");
        break;

    default:
        break;
    }

    if (JVM_GetConfig(JVM_CONFIG_SLAVE_MODE) == KNI_FALSE) {	
        midpFinalize();
    }

    ams_free_startup_params(ppSavedParams, savedNumberOfParams);
    
    return status;
}

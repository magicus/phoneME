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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <midp_logging.h>
#include <midpAMS.h>
#include <midpStorage.h>
#include <midpMalloc.h>
#include <midpUtilKni.h>
#include <suitestore_task_manager.h>
#include <commandLineUtil_md.h>

/**
 * @file
 *
 * Example of how the public MIDP API can be used to delete an installed
 * MIDlet Suite.
 */

/* Dummy implementations of functions needed to run in NamsTestService mode. */
#if !ENABLE_I3_TEST
void nams_test_service_setup_listeners() {}
void nams_test_service_remove_listeners() {}
#endif

/** Usage text for the remove MIDlet executable. */
static const char* const removeUsageText =
"\n"
"Usage: removeMidlet (<suite number> | <suite ID> | all)\n"
"         Remove an installed MIDlet suite or all suites.\n"
"\n"
"  where <suite number> is the number of a suite as displayed by the\n"
"  listMidlets command, and <suite ID> is the unique ID a suite is \n"
"  referenced by\n\n";

/**
 * Deletes an installed MIDlet suite. This is an example of how to use
 * the public MIDP API.
 *
 * @param argc The total number of arguments
 * @param argv An array of 'C' strings containing the arguments
 *
 * @return <tt>0</tt> for success, otherwise <tt>-1</tt>
 *
 * IMPL_NOTE: determine if it is desirable for user targeted output
 *       messages to be sent via the log/trace service, or if
 *       they should remain as printf calls
 */
int removeMidlet(int argc, char* argv[]) {
    int   status = -1;
    char* appDir = NULL;

    if (argc == 1) {
        fprintf(stderr, removeUsageText);
        return -1;
    }

    if (argc > 2) {
        REPORT_ERROR1(LC_AMS, "Too many arguments given\n%s", removeUsageText);
        fprintf(stderr, "Too many arguments given\n%s", removeUsageText);
        return -1;
    }

    /* get midp home directory, set it */
    appDir = getApplicationDir(argv[0]);
    if (appDir == NULL) {
        return -1;
    }
    /* set up appDir before calling initialize */
    midpSetAppDir(appDir);

    if (midpInitialize() != 0) {
        REPORT_ERROR(LC_AMS, "Not enough memory");
        fprintf(stderr, "Not enough memory\n");
        return -1;
    }

    do {
        int onlyDigits;
        int len;
        int i;
        SuiteIdType* pSuites = NULL;
        int numberOfSuites = 0;
        MIDPError err;

        /* if the storage name only digits, convert it */
        onlyDigits = 1;
        len = strlen(argv[1]);
        for (i = 0; i < len; i++) {
            if (!isdigit((argv[1])[i])) {
                onlyDigits = 0;
                break;
            }
        }

        if (onlyDigits) {
            /* Remove by number */
            int suiteNumber;

            /* the format of the string is "number:" */
            if (sscanf(argv[1], "%d", &suiteNumber) != 1) {
                REPORT_ERROR(LC_AMS, "Invalid suite number format");
                fprintf(stderr, "Invalid suite number format\n");
                break;
            }

            err = midp_get_suite_ids(&pSuites, &numberOfSuites);
            if (err != ALL_OK) {
                REPORT_ERROR1(LC_AMS, "Error in midp_get_suite_ids(), code %d",
		              err);
                fprintf(stderr, "Error in midp_get_suite_ids(), code %d.\n",
		        err);
                break;
            }

            if (suiteNumber > numberOfSuites || suiteNumber < 1) {
                REPORT_ERROR(LC_AMS, "Suite number out of range");
                fprintf(stderr, "Suite number out of range\n");
                midp_free_suite_ids(pSuites, numberOfSuites);
                break;
            }

            /* The suite number for the first suite is 1. */
            midp_remove_suite(pSuites[suiteNumber - 1]);

            midp_free_suite_ids(pSuites, numberOfSuites);
        } else if (strcmp(argv[1], "all") == 0) {
            /* Remove all */
            err = midp_get_suite_ids(&pSuites, &numberOfSuites);

            if (err != ALL_OK) {
                REPORT_ERROR1(LC_AMS, "Error in midp_get_suite_ids(), code %d",
                              err);
                fprintf(stderr, "Error in midp_get_suite_ids(), code %d.\n",
                        err);
                break;
            }

            for (i = 0; i < numberOfSuites; i++) {
                midp_remove_suite(pSuites[i]);
            }

            midp_free_suite_ids(pSuites, numberOfSuites);
        } else {
            REPORT_ERROR2(LC_AMS, "Invalid argument: '%s'.\n%s",
                          argv[1], removeUsageText);
            fprintf(stderr, "Invalid argument: '%s'.\n%s",
                    argv[1], removeUsageText);
        }

        status = 0;
    } while (0);

    midpFinalize();

    return status;
}


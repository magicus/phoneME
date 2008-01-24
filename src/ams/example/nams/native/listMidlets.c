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

#include <string.h>
#include <stdio.h>
#include <midp_logging.h>
#include <midp_ams_status.h>
#include <midpAMS.h>
#include <midpMalloc.h>
#include <midpUtilKni.h>
#include <suitestore_installer.h>
#include <suitestore_task_manager.h>
#include <commandLineUtil_md.h>

/**
 * @file
 *
 * Example of how the public MIDP API can be used to list installed
 * MIDlet Suite.
 */

/* Dummy implementations of functions needed to run in NamsTestService mode. */
#if !ENABLE_I3_TEST
void nams_test_service_setup_listeners() {}
void nams_test_service_remove_listeners() {}
#endif

/**
 * Prints a field to <tt>stdout</tt>.
 *
 * IMPL_NOTE:decide if this should be sent to the log instead stdout
 *
 * @param pszLabel A 'C' string label to be printed before the field
 * @param field The field to print
 */
static void
print_field(char* pszLabel, const pcsl_string* field) {
    printf("%s",pszLabel);
    PRINTF_PCSL_STRING("%s", field);
    putchar('\n');
}

/**
 * Prints a property value to <tt>stdout</tt>.
 *
 * @param pszLabel A 'C' string label to be printed before the
 *                 property value
 * @param key The property key of the value to print
 * @param props The properties to search for <tt>key</tt>
 */
static void
printProperty(char* pszLabel, const pcsl_string * key, MidpProperties props) {
    print_field(pszLabel, midp_find_property(&props, key));
}

/**
 * Lists all installed MIDlet suites. This is an example of how to use
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
int
listMidlets(int argc, char* argv[]) {
    int   status = -1;
    int   i;
    long  size;
    char* appDir = NULL;

    (void)argv;                                   /* Avoid compiler warnings */
    if (argc > 1) {
        REPORT_ERROR(LC_AMS, "Too many arguments given");
        fprintf(stderr, "Too many arguments given\n");
        return -1;
    }

    /* get application directory, set it */
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
        SuiteIdType* pSuites = NULL;
        int numberOfSuites = 0;
        MIDPError err;

        err = midp_get_suite_ids(&pSuites, &numberOfSuites);
        if (err != ALL_OK) {
            REPORT_ERROR1(LC_AMS, "Error in midp_get_suite_ids(), code %d",
                          err);
            fprintf(stderr, "Error in midp_get_suite_ids(), code %d.\n", err);
            break;
        }

        if (numberOfSuites == 0) {
            REPORT_ERROR(LC_AMS, "No MIDlet Suites installed on phone");
            printf("** No MIDlet Suites installed on phone\n");
            status = 0;
            break;
        }

        for (i = 0; i < numberOfSuites; i++) {
            MidpInstallInfo info;
            MidpProperties properties;

            info = midp_get_suite_install_info(pSuites[i]);
            if (BAD_ID_INFO_STATUS(info)) {
                REPORT_ERROR(LC_AMS, "Suite list is corrupt");
                fprintf(stderr, "Suite list is corrupt\n");
                break;
            }

            if (OUT_OF_MEM_INFO_STATUS(info)) {
                REPORT_ERROR(LC_AMS, "Out Of Memory for Info");
                fprintf(stderr, "Out Of Memory for Info\n");
                break;
            }

            if (SUITE_CORRUPTED_ERR_STATUS(info)) {
                /*
                 * Installinfo is not initialized in case of an error
                 * so no need to free it
                 */
                REPORT_ERROR1(LC_AMS, "Error : Suite %d is corrupted", (i+1));
                fprintf(stderr, "Error : Suite %d is corrupted\n", (i+1));
                continue;
            }

            if (READ_ERROR_INFO_STATUS(info)) {
                REPORT_ERROR(LC_AMS, "Corrupt install info");
                fprintf(stderr, "Corrupt install info\n");
                break;
            }

            properties = midp_get_suite_properties(pSuites[i]);
            if (OUT_OF_MEM_PROPERTY_STATUS(properties)) {
                midp_free_install_info(&info);
                midp_free_properties(&properties);
                REPORT_ERROR(LC_AMS, "Out Of Memory for properties");
                fprintf(stderr, "Out Of Memory for properties\n");
                break;
            }

            if (CORRUPTED_PROPERTY_STATUS(properties)) {
                midp_free_install_info(&info);
                midp_free_properties(&properties);
                REPORT_ERROR1(LC_AMS, "Error : Suite %d is corrupted", (i+1));
                fprintf(stderr, "Error : Suite %d is corrupted\n", (i+1));
                continue;
            }

            if (READ_ERROR_PROPERTY_STATUS(properties)) {
                midp_free_install_info(&info);
                midp_free_properties(&properties);
                REPORT_ERROR(LC_AMS, "Corrupt properties");
                fprintf(stderr, "Corrupt properties\n");
                break;
            }

            printf("[%d]\n", (i + 1));
            printProperty("  Name: ", &SUITE_NAME_PROP, properties);
            printProperty("  Vendor: ", &SUITE_VENDOR_PROP, properties);
            printProperty("  Version: ", &SUITE_VERSION_PROP, properties);
            printProperty("  Description: ", &SUITE_DESC_PROP, properties);

            if (info.authPathLen > 0) {
                int j;

                puts("  Authorized by: ");
                for (j = 0; j < info.authPathLen; j++) {
                    print_field("    ", &info.authPath_as[j]);
                }
            }

            print_field("  SecurityDomain: ", &info.domain_s);
            printf("  Verified: %s\n",
                (info.pVerifyHash != NULL ? "true" : "false"));
            printf("  Suite ID: %ld\n", (long)pSuites[i]);
            print_field("  JAD URL: ", &info.jadUrl_s);
            print_field("  JAR URL: ", &info.jarUrl_s);
            size = midp_get_suite_storage_size(pSuites[i]);
            if (size < 0) {
                fprintf(stderr, "Ran out of memory getting the size\n");
            } else {
                printf("  Size: %ldK\n", (size + 1023) / 1024);
            }

            midp_free_install_info(&info);
            midp_free_properties(&properties);
        }

        midp_free_suite_ids(pSuites, numberOfSuites);

        status = 0;
    } while (0);

    midpFinalize();

    return status;
}

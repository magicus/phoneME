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

#include <string.h>
#include <stdio.h>
#include <midp_logging.h>
#include <midp_ams_status.h>
#include <midpAMS.h>
#include <suitestore_export.h>
#include <midpMalloc.h>

/**
 * @file
 *
 * Example of how the public MIDP API can be used to list installed
 * MIDlet Suite.
 */

extern char* midpFixMidpHome(char *cmd);

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
    print_field(pszLabel, midp_find_property(props, key));
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
 * IMPL_NOTE:determine if it is desirable for user targeted output 
 *       messages to be sent via the log/trace service, or if 
 *       they should remain as printf calls
 */
int
main(int argc, char* argv[]) {
    int   status = -1;
    int i;
    long size;
    char* midpHome;


    (void)argv;                                   /* Avoid compiler warnings */
    if (argc > 1) {
        REPORT_ERROR(LC_AMS, "Too many arguments given");
        fprintf(stderr, "Too many arguments given\n");
        return -1;
    }

    /* For development platforms MIDP_HOME is dynamic. */
    midpHome = midpFixMidpHome(argv[0]);
    if (midpHome == NULL) {
        return -1;
    }
    /* set up midpHome before calling initialize */
    midpSetHomeDir(midpHome);
    
    if (midpInitialize() != 0) {
        REPORT_ERROR(LC_AMS, "Not enough memory");
        fprintf(stderr, "Not enough memory\n");
        return -1;
    }

    do {
        pcsl_string* pSuites;
        int numberOfSuites = midpGetSuiteIDs(&pSuites);

        if (numberOfSuites < 0) {
            REPORT_ERROR(LC_AMS, "Out of memory for suite ID");
            fprintf(stderr, "Out Of Memory for suite ID\n");
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

            info = midp_get_suite_install_info(&pSuites[i]);
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

            if (READ_ERROR_INFO_STATUS(info)) {
                REPORT_ERROR(LC_AMS, "Corrupt install info");
                fprintf(stderr, "Corrupt install info\n");
                break;
            }

            properties = midp_get_suite_properties(&pSuites[i]);
            if (OUT_OF_MEM_PROPERTY_STATUS(properties)) {
                midpFreeInstallInfo(info);
                REPORT_ERROR(LC_AMS, "Out Of Memory for properties");
                fprintf(stderr, "Out Of Memory for properties\n");
                break;
            }

            if (READ_ERROR_PROPERTY_STATUS(properties)) {
                midpFreeInstallInfo(info);
                REPORT_ERROR(LC_AMS, "Corrupt properties");
                fprintf(stderr, "Corrupt properties\n");
                break;
            }

            printf("[%d]\n", (i + 1));
            printProperty("  Name: ", &SUITE_NAME_PROP, properties);
            printProperty("  Vendor: ", &SUITE_VENDOR_PROP, properties);
            printProperty("  Version: ", &SUITE_VERSION_PROP, properties);
            printProperty("  Description: ", &SUITE_DESC_PROP, properties);
            /*
             * IMPL_NOTE: remember what was meant by info.ca field, which
             * doesn't exist any more.
             */
            /* printField("  Authorized by: ", info.ca); */
            print_field("  SecurityDomain: ", &info.domain_s);
            print_field("  Suite ID: ", &pSuites[i]);
            print_field("  JAD URL: ", &info.jadUrl_s);
            print_field("  JAR URL: ", &info.jarUrl_s);
            size = midp_get_suite_storage_size(&pSuites[i]);
            if (size < 0) {
                fprintf(stderr, "Ran out of memory getting the size\n");
            } else {
                printf("  Size: %ldK\n", (size + 1023) / 1024);
            }

            midpFreeInstallInfo(info);
            midpFreeProperties(properties);
        }

        midpFreeSuiteIDs(pSuites, numberOfSuites);

        status = 0;
    } while (0);

    midpFinalize();

    return status;
}

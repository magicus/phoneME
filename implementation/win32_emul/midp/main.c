/*
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
#include "javacall_lifecycle.h"
#include "lime.h"
#include "stdio.h"
#include "windows.h"
#include "javacall_properties.h"
#include "javacall_events.h"
#include "javacall_logging.h"

#if ENABLE_JSR_120
extern javacall_result finalize_wma_emulator();
#endif
extern void javanotify_set_vm_args(int argc, char* argv[]);
extern void javanotify_set_heap_size(int heapsize);
extern int isNetworkMonitorActive();
extern void javanotify_start_local(char* classname, char* descriptor,
                            char* classpath, javacall_bool debug);
extern void javanotify_start_suite(char* suiteId);
extern void javanotify_install_midlet_wparams(const char* httpUrl,
                                       int silentInstall, int forceUpdate);
extern void javanotify_remove_suite(char* suite_id);
extern void javanotify_transient(char* url);
extern void javanotify_list_midlets(void);
extern void javanotify_list_storageNames(void);
extern void InitializeLimeEvents();
extern void FinalizeLimeEvents();


/** Usage text for the run emulator executable. */
/* IMPL_NOTE: Update usage according to main(...) func */
static const char* const emulatorUsageText =
            "\n"
            "Syntax:\n"
            "\n"
            "emulator [arguments] <Application>\n"
            "\n"
            "Arguments are:\n"
            "\n"
            "-classpath, -cp    The class path for the VM\n"
            "-D<property=value> Property definitions\n"
            "-version           \n"
            "Display version information about the emulator\n"
            "-help              Display list of valid arguments\n"
            "-Xverbose[: allocation | gc | gcverbose | class | classverbose |\n"
            "         verifier | stackmaps | bytecodes | calls | \n"
            "         callsverbose | frames | stackchunks | exceptions | \n"
            "         events | threading | monitors | networking | all\n"
            "                   enable verbose output\n"
            "-Xquery\n"
            "                   Query options\n"
            "-Xdebug            Use a remote debugger\n"
            "-Xrunjdwp:[transport=<transport>,address=<address>,server=<y/n>\n"
            "           suspend=<y/n>]\n"
            "                   Debugging options\n"
            "-Xdevice:<device name>\n"
            "                   Name of the device to be emulated\n"
            "-Xdescriptor:<JAD file name>\n"
            "                   The JAD file to be executed\n"
            "-Xjam[:install=<JAD file url> | force | list | storageNames |\n"
            "           run=[<storage name> | <storage number>] |\n"
            "           remove=[<storage name> | <storage number> | all] |\n"
            "           transient=<JAD file url>]\n"
            "                   Java Application Manager and support\n"
            "                   for Over The Air provisioning (OTA)\n"
            "-Xautotest:<JAD file url>\n"
            "                   Run in autotest mode\n"
            "-Xheapsize:<size>  (e.g. 65536 or 128k or 1M)\n"
            "                   specifies the VM heapsize\n"
            "                   (overrides default value)\n"
            "-Xprefs:<filename> Override preferences by properties in file\n"
            "-Xnoagent          Supported for backwards compatibility\n"
            "-Xdomain:<domain_name>\n"
            "                   Set the MIDlet suite's security domain\n\n";

typedef enum {
    RUN_OTA,
    RUN_LOCAL,
    AUTOTEST,
} execution_mode;

typedef enum {
    RUN,
    INSTALL,
    INSTALL_FORCE,
    REMOVE,
    TRNASIENT,
    LIST,
    STORAGE_NAMES
} execution_parameter;

/* global varaiable to determine if the application
 * is running locally or via OTA */
javacall_bool isRunningLocal = JAVACALL_FALSE;

main(int argc, char *argv[]) {
    int i, vmArgc = 0;
    char *vmArgv[100]; /* CLDC parameters */
    int executionMode      = -1;
    int executionParameter = -1;
    char *url              = NULL; /* URL for installation, autotest */
    char* domainStr        = NULL;
    char *storageName      = NULL;
    char *descriptor       = NULL;
    char *device           = NULL;
    int heapsize           = -1;
    char *className        = NULL;
    char *classPath        = NULL;
    char *debugPort        = NULL;

    /* uncomment this like to force the debugger to start */
    /* _asm int 3; */

    if (JAVACALL_OK != javacall_initialize_configurations()) {
        return -1;
    }

    for (i = 1; i < argc; i++) {
        javautil_debug_print(JAVACALL_LOG_INFORMATION, "core",
                             "argv[%d] = %s", i, argv[i]);
    }

    /* parse LIME environment variables */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--") == 0) {
            /* end of environment definitions */
            break;
        } else {
            char *equalsIndex = strstr(argv[i], "=");
            if (equalsIndex == 0) {
                javautil_debug_print(JAVACALL_LOG_CRITICAL, "core",
                                     "Illegal argument to KVM: %s\n", argv[i]);
                return 0;
            } else {
                /* These Variables are required by lime,
                 * and need to be set before calling lime initialization
                 * (Done in javacall_events_init) */
                _putenv(argv[i]);
            }
        }
    }

    javacall_events_init();

    i++;
    /* parse MIDP/CLDC arguments */
    for (i; i < argc; i++) {
        if (strcmp(argv[i], "-classpath") == 0) {
            classPath = vmArgv[vmArgc++] = argv[++i];
            /* run local application */
            if (executionMode == -1) {
                executionMode = RUN_LOCAL;
            }
        } else if (strncmp(argv[i],"-D", 2) == 0) {
            /* It is a CLDC arg, add to CLDC arguments list */
            /* vmArgv[vmArgc++] = argv[i]; */
            /* set system property */
            char *key;
            char *value;
            javacall_property_type property_type = JAVACALL_APPLICATION_PROPERTY;

            key = argv[i] + 2;
            for (value = key; *value ; value++) {
                if (*value == '=') {
                    *value++ = 0;
                    break;
                }
            }

            /* ignore microedition.encoding for now,
             * since only ISO8859_1 encoding is currently supported */
            if (strcmp(key,"microedition.encoding") == 0) {
                continue;
            }

            /* Use the key name used in JSR177 implementation */
            if (strcmp(key,"com.sun.midp.io.j2me.apdu.hostsandports") == 0 ) {
                key = "com.sun.io.j2me.apdu.hostsandports";
                property_type = JAVACALL_INTERNAL_PROPERTY;
            }
            javacall_set_property(key, value, JAVACALL_TRUE,property_type);
        } else if (strcmp(argv[i], "-monitormemory") == 0) {
            /* old argument  - ignore it */
        } else if (strcmp(argv[i], "-memory_profiler") == 0) {

            /* It is a CLDC arg, add to CLDC arguments list */
            vmArgv[vmArgc++] = argv[i++]; /* -memory_profiler */
            vmArgv[vmArgc++] = argv[i++]; /* -port */
            vmArgv[vmArgc++] = argv[i]; /* port number */
        } else if (strcmp(argv[i], "-jprof") == 0) {

            /* It is a CLDC arg, add to CLDC arguments list */
            vmArgv[vmArgc++] = "+UseExactProfiler";
            i++;
            javacall_set_property("profiler.filename",
                                  argv[i], JAVACALL_TRUE,
                                  JAVACALL_APPLICATION_PROPERTY);
            i++;
        } else if (strcmp(argv[i], "-tracegarbagecollection") == 0) {

            /* It is a CLDC arg, add to CLDC arguments list */
            vmArgv[vmArgc++] = "+TraceGC";
        } else if (strcmp(argv[i], "-traceclassloading") == 0) {

            /* It is a CLDC arg, add to CLDC arguments list */
            vmArgv[vmArgc++] = "+TraceClassLoading";
        } else if (strcmp(argv[i], "-tracemethodcalls") == 0) {

        } else if (strcmp(argv[i], "-traceexceptions") == 0) {

            /* It is a CLDC arg, add to CLDC arguments list */
            vmArgv[vmArgc++] = "+TraceExceptions";
        } else if (strcmp(argv[i], "-domain") == 0) {

            i++;
            domainStr = argv[i];
        } else if (strcmp(argv[i], "-autotest") == 0) {

            executionMode = AUTOTEST;
            i++;
            url = malloc(sizeof(char)*(strlen(argv[i])+1));
            strcpy(url, argv[i]);
        } else if (strcmp(argv[i], "-descriptor") == 0) {

            /* run local application */
            executionMode = RUN_LOCAL;
            i++;
            descriptor = malloc(sizeof(char)*(strlen(argv[i])+1));
            strcpy(descriptor, argv[i]);
        } else if (strcmp(argv[i], "-install") == 0) {

            /* install an application onto the emulator from url */
            executionMode = RUN_OTA;
            i++;
            if (strcmp(argv[i], "-force") == 0) {
                /* force update without user confirmation */
                executionParameter = INSTALL_FORCE;
                i++;
            } else {
                executionParameter = INSTALL;
            }
            if (strncmp(argv[i], "http://", 7) != 0) {
                /* URL must start with http */
                javautil_debug_print (JAVACALL_LOG_INFORMATION, "main",
                                      "The JAD URL %s is not valid, "
                                      "it must be an http URL", argv[i]);
                return 0;
            }
            url = malloc(sizeof(char)*(strlen(argv[i])+1));
            strcpy(url, argv[i]);
        } else if (strcmp(argv[i], "-list") == 0) {

            /* list all applications installed on the device and exit */
            executionMode = RUN_OTA;
            executionParameter = LIST;
        } else if (strcmp(argv[i], "-storageNames") == 0) {

            /* list all applications installed on the device and exit
             * each line contains one storage name */
            executionMode = RUN_OTA;
            executionParameter = STORAGE_NAMES;
        } else if (strcmp(argv[i], "-run") == 0) {

            /* run a previously installed application */
            executionMode = RUN_OTA;
            executionParameter = RUN;
            i++;
            storageName = malloc(sizeof(char)*(strlen(argv[i])+1));
            strcpy(storageName, argv[i]);
        } else if (strcmp(argv[i], "-remove") == 0) {

            /* remove a previously installed application */
            executionMode = RUN_OTA;
            executionParameter = REMOVE;
            i++;
            storageName = malloc(sizeof(char)*(strlen(argv[i])+1));
            strcpy(storageName, argv[i]);
        } else if (strcmp(argv[i], "-transient") == 0) {

            /* install, run and remove an application */
            executionMode = RUN_OTA;
            executionParameter = TRNASIENT;
            i++;
            url = malloc(sizeof(char)*(strlen(argv[i])+1));
            strcpy(url, argv[i]);
        } else if (strcmp(argv[i], "-debugger") == 0) {

            /* debug an application */
            /* It is a CLDC arg, add to CLDC arguments list */
            /* vmArgv[vmArgc++] = argv[i]; */
            i++;
            if (strcmp(argv[i],"-port") == 0) {
                /* It is a CLDC arg, add to CLDC arguments list */
                /* vmArgv[vmArgc++] = argv[i]; */
                i++;
                /* It is a CLDC arg, add to CLDC arguments list */
                /* vmArgv[vmArgc++] = argv[i]; */
                javacall_set_property("vmdebuggerport",
                                      argv[i], JAVACALL_TRUE,
                                      JAVACALL_APPLICATION_PROPERTY);
                debugPort = malloc(sizeof(char)*(strlen(argv[i])+1));
                strcpy(debugPort, argv[i]);
            }
        } else if (strcmp(argv[i], "-heapsize") == 0) {

            /* Set the emulator's heap size to be a maximum of size bytes */
            char* token;
            i++;
            token = strstr(argv[i], "kB");
            if (token != NULL) {
                token = strtok(argv[i], "kB");
                heapsize = atoi(token)*1024; /* convert KiloBytes to bytes */
            } else {
                heapsize = atoi(argv[i]);
            }

            /* Override JAVA_HEAP_SIZE internal property used
            /* by MIDP initialization code */
            if (heapsize > 0) {
                /* 1GB heap size fits 10-digits number in bytes */
                #define HEAPSIZE_BUFFER_SIZE 11

                char heapsizeStr[HEAPSIZE_BUFFER_SIZE];
                _snprintf(heapsizeStr, HEAPSIZE_BUFFER_SIZE, "%d", heapsize);
                javacall_set_property(
                    "JAVA_HEAP_SIZE", heapsizeStr,
                    JAVACALL_TRUE, JAVACALL_INTERNAL_PROPERTY);
            }

        } else if (strncmp(argv[i], "-", 1) == 0) {
            javautil_debug_print (JAVACALL_LOG_INFORMATION, "main",
                                  "Illegal argument %s", argv[i]);
            javautil_debug_print (JAVACALL_LOG_INFORMATION, "main",
                                  "%s", emulatorUsageText);
            return 0;
        } else if (i == argc - 1) {
            /* The last argument in the list, assume it is
             * the MIDlet class name to run when running locally */
            className = malloc(sizeof(char)*(strlen(argv[i])+1));
            strcpy(className, argv[i]);
        } else {
            javautil_debug_print (JAVACALL_LOG_INFORMATION, "main",
                                  "Illegal argument %s", argv[i]);
            javautil_debug_print (JAVACALL_LOG_INFORMATION, "main",
                                  "%s", emulatorUsageText);
            return 0;
        }
    }

    if (vmArgc > 0 ) {
        /* set VM args */
        javanotify_set_vm_args(vmArgc, vmArgv);
    }

    if (heapsize != -1) {
        /* set heapsize */
        javanotify_set_heap_size(heapsize);
    }

    /* Check if network monitor is active,
     * If yes, set system property javax.microedition.io.Connector.protocolpath
     * to com.sun.kvem.io
     */
    if (isNetworkMonitorActive()) {
        javacall_set_property("javax.microedition.io.Connector.protocolpath",
                              "com.sun.kvem.io",
                              JAVACALL_TRUE,
                              JAVACALL_APPLICATION_PROPERTY);
    } else {
        javacall_set_property("javax.microedition.io.Connector.protocolpath",
                              "com.sun.midp.io",
                              JAVACALL_TRUE,
                              JAVACALL_APPLICATION_PROPERTY);
    }

    javacall_set_property("running_local",
                          "false",
                          JAVACALL_TRUE,
                          JAVACALL_APPLICATION_PROPERTY);
    /* check executionMode and call appropriate javanotify function */
    if (executionMode == RUN_LOCAL) {
        /* set property so we know we run in local mode and not in ota mode. */
        isRunningLocal = JAVACALL_TRUE;
        javacall_set_property("running_local",
                              "true",
                              JAVACALL_TRUE,
                              JAVACALL_APPLICATION_PROPERTY);
        if (debugPort != NULL) {
            javanotify_start_local(className, descriptor,
                                   classPath, JAVACALL_TRUE);
        } else {
            javanotify_start_local(className, descriptor,
                                   classPath, JAVACALL_FALSE);
        }
    } else if (executionMode == RUN_OTA) {
        /* check the executionParameter and call the
         * appropriate javanotify function */
        switch (executionParameter) {
        case RUN:
            {
                javanotify_start_suite(storageName);
                break;
            }
        case INSTALL:
            {
                javanotify_install_midlet_wparams(url, 1, 0);
                break;
            }
        case INSTALL_FORCE:
            {
                javanotify_install_midlet_wparams(url, 1, 1);
                break;
            }
        case REMOVE:
            {
                javanotify_remove_suite(storageName);
                break;
            }
        case TRNASIENT:
            {
                javanotify_transient(url);
                break;
            }
        case LIST:
            {
                javanotify_list_midlets();
                break;
            }
        case STORAGE_NAMES:
            {
                javanotify_list_storageNames();
                break;
            }
        default:
            {
                javanotify_start();
                break;
            }
        }

    } else if (executionMode == AUTOTEST) {
        char *argv1[5] = {"runMidlet", "-1",
            "com.sun.midp.installer.AutoTester", url, domainStr};
        int numargs = (domainStr!=NULL) ? 5 : 4;
        javanotify_start_java_with_arbitrary_args(numargs, argv1);
    } else { /* no execution mode, invalid arguments */
        javanotify_start();
    }

#if ENABLE_JSR_120
    /* initializeWMASupport(); */
#endif

    InitializeLimeEvents();

    JavaTask();

    javacall_events_finalize();

#if ENABLE_JSR_120
    finalize_wma_emulator();
#endif

    /* free allocated memory */
    free(descriptor);
    free(className);
    free(url);
    free(storageName);

    FinalizeLimeEvents();
    return 1;
}

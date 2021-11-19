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
#include "javacall_memory.h"

#ifdef ENABLE_OUTPUT_REDIRECTION
#include "io_sockets.h"
#endif /* ENABLE_OUTPUT_REDIRECTION */

#ifdef ENABLE_MONITOR_PARENT_PROCESS
#include "parproc_monitor.h"
#endif /* ENABLE_MONITOR_PARENT_PROCESS */

#if ENABLE_JSR_120
extern javacall_result finalize_wma_emulator();
#endif
extern void javanotify_set_vm_args(int argc, char* argv[]);
extern void javanotify_set_heap_size(int heapsize);
#ifdef USE_NETMON
extern int isNetworkMonitorActive();
#endif
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

static void reportOutOfMemoryError();

#if ENABLE_MULTIPLE_ISOLATES
static char* constructODTAgentMVMManagerArgument(
        const char* odtAgentSettings);

static int extendClasspath(const char* classpath);
#endif /* ENABLE_MULTIPLE_ISOLATES */

static javacall_utf16* get_properties_file_name(int* fileNameLen, 
                                                const char* argv[], int argc);
static char* ansi_to_utf8(char *str);

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

#define ODT_AGENT_OPTION "-Xodtagent"
#define ODT_AGENT_OPTION_LEN \
        (sizeof(ODT_AGENT_OPTION) / sizeof(char) - 1)

#define PREFS_OPTION "-Xprefs"
#define PREFS_OPTION_LEN \
        (sizeof(PREFS_OPTION) / sizeof(char) - 1)

#define RUN_ODT_AGENT_PREFIX "-runodtagent"
#define RUN_ODT_AGENT_PREFIX_LEN \
        (sizeof(RUN_ODT_AGENT_PREFIX) / sizeof(char) - 1)
        
typedef enum {
    RUN_OTA,
    RUN_LOCAL,
    AUTOTEST,
    ODTAGENT
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
    int stdoutPort         = -1;
    int stderrPort         = -1;
    char* odtAgentSettings = NULL;
    char* mvmManagerArgument = NULL;
    javacall_utf16* propFileName;
    int propFileNameLen = 0;
    int isMemoryProfiler = 0;
    char *utf8Value;

#ifdef USE_MEMMON
	int isMemoryMonitor = 0;
#endif

    /* uncomment this like to force the debugger to start */
    /* _asm int 3; */

    /* get the configuration file name */
    propFileName = get_properties_file_name(&propFileNameLen, 
                                            argv + 1, argc - 1);

    if (javacall_initialize_configurations_from_file(
            propFileName, propFileNameLen) != JAVACALL_OK) {
        if (propFileName != NULL) {
            javacall_free(propFileName);
        }
    
        return -1;
    }

    if (propFileName != NULL) {
        javacall_free(propFileName);
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
            if (i < argc-1) { /* just ignore it if no actual path is given */
                vmArgv[vmArgc++] = argv[i++];               /* -classpath */
                classPath = vmArgv[vmArgc++] = argv[i];     /* the actual path */
                /* run local application */
                if (executionMode == -1) {
                    executionMode = RUN_LOCAL;
                }
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
            } else if (!isMemoryProfiler && strcmp(key,"memprofport") == 0) {
                int j = strlen(value);
                if (j > 0 && j < 6) { /* port length is correct */
				    j++;
                    debugPort = malloc(sizeof(char)*j);
                    memcpy(debugPort, value, j);
                    javacall_set_property("VmDebuggerPort",
                                      debugPort, JAVACALL_TRUE,
                                      JAVACALL_INTERNAL_PROPERTY);
                    javacall_set_property("VmMemoryProfiler",
                                      "true", JAVACALL_TRUE,
                                      JAVACALL_INTERNAL_PROPERTY);
                    isMemoryProfiler = 1;
                }
#ifdef USE_MEMMON
            } else if (strcmp(key,"monitormemory") == 0) {
                javacall_set_property("MemoryMonitor",
                                  "true", JAVACALL_TRUE,
                                  JAVACALL_INTERNAL_PROPERTY);
	            isMemoryMonitor=1;
#endif
			}
            utf8Value = ansi_to_utf8(value);
            if (utf8Value == NULL)
                return 0;
            javacall_set_property(key, utf8Value, JAVACALL_TRUE,property_type);
            javacall_free(utf8Value);
        } else if (strcmp(argv[i], "-profile") == 0) {
                /* ROM profile name is passed here */ 
                vmArgv[vmArgc++] = argv[i++];
                vmArgv[vmArgc++] = argv[i];
        } else if (strcmp(argv[i], "-memory_profiler") == 0) {

            /* It is a CLDC arg, add to CLDC arguments list */
            vmArgv[vmArgc++] = argv[i++]; /* -memory_profiler */
            vmArgv[vmArgc++] = argv[i++]; /* -port */
            vmArgv[vmArgc++] = argv[i]; /* port number */
        } else if (strcmp(argv[i], "-jprof") == 0) {

            /* It is a CLDC arg, add to CLDC arguments list */
            vmArgv[vmArgc++] = "+UseExactProfiler";
            i++;
            utf8Value = ansi_to_utf8(argv[i]);
            if (utf8Value == NULL)
                return 0;
            javacall_set_property("profiler.filename",
                                  utf8Value, JAVACALL_TRUE,
                                  JAVACALL_APPLICATION_PROPERTY);
            javacall_free(utf8Value);
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
        } else if (strcmp(argv[i], "-stdoutport") == 0) {
            if ((i + 1) < argc) {
                ++i;
                stdoutPort = atoi(argv[i]);
            }
        } else if (strcmp(argv[i], "-stderrport") == 0) {
            if ((i + 1) < argc) {
                ++i;
                stderrPort = atoi(argv[i]);
            }
        } else if (strcmp(argv[i], ODT_AGENT_OPTION) == 0) {
            /* handle "-Xodtagent" */
            executionMode = ODTAGENT;
        } else if (strncmp(argv[i], ODT_AGENT_OPTION ":", 
                           ODT_AGENT_OPTION_LEN + 1) == 0) {
            /* handle "-Xodtagent:<settings>" */
            
            /* settings for the ODT agent start after the ':' */
            const char* settingsString = argv[i] + ODT_AGENT_OPTION_LEN + 1;
            int settingsStringLen = strlen(settingsString);

            odtAgentSettings = 
                    (char*)malloc((settingsStringLen + 1) * sizeof(char));
            if (odtAgentSettings == NULL) {
                reportOutOfMemoryError();
                return -1;
            }
            
            strcpy(odtAgentSettings, settingsString);

            executionMode = ODTAGENT;
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
            if (!isMemoryProfiler && strcmp(argv[i], "-port") == 0) {
                /* It is a CLDC arg, add to CLDC arguments list */
                /* vmArgv[vmArgc++] = argv[i]; */
                i++;
                /* It is a CLDC arg, add to CLDC arguments list */
                /* vmArgv[vmArgc++] = argv[i]; */
                javacall_set_property("VmDebuggerPort",
                                      argv[i], JAVACALL_TRUE,
                                      JAVACALL_INTERNAL_PROPERTY);
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

        } else if (strncmp(argv[i], PREFS_OPTION ":", PREFS_OPTION_LEN + 1) 
                           == 0) {
            /* already processed in get_properties_file_name */
            /* don't pass the argument further */
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

    if (!isMemoryProfiler) {
	    javacall_set_property("VmMemoryProfiler",
                            NULL, JAVACALL_TRUE,
                            JAVACALL_INTERNAL_PROPERTY);
    }

#ifdef USE_MEMMON
    if (!isMemoryMonitor) {
	    javacall_set_property("MemoryMonitor",
                            NULL, JAVACALL_TRUE,
                            JAVACALL_INTERNAL_PROPERTY);
    }
#endif
	
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

#ifdef USE_NETMON

    if (isNetworkMonitorActive()) {
        javacall_set_property("javax.microedition.io.Connector.protocolpath",
                              "com.sun.kvem.io",
                              JAVACALL_TRUE,
                              JAVACALL_APPLICATION_PROPERTY);
        javacall_set_property("javax.microedition.io.Connector.protocolpath.fallback",
                              "com.sun.midp.io",
                              JAVACALL_TRUE,
                              JAVACALL_APPLICATION_PROPERTY);
    } else {
#endif
        javacall_set_property("javax.microedition.io.Connector.protocolpath",
                              "com.sun.midp.io",
                              JAVACALL_TRUE,
                              JAVACALL_APPLICATION_PROPERTY);
        javacall_set_property("javax.microedition.io.Connector.protocolpath.fallback",
                              NULL,
                              JAVACALL_TRUE,
                              JAVACALL_APPLICATION_PROPERTY);
#ifdef USE_NETMON
    }
#endif

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
    } else if (executionMode == ODTAGENT) {
    
#if ENABLE_MULTIPLE_ISOLATES
        
        /* run the MVM manager and instruct it run the ODT agent */

        char* argv1[4] = { 
                "runMidlet", "-1",
                "com.sun.midp.appmanager.MVMManager", 
                RUN_ODT_AGENT_PREFIX }; 

        if ((classPath != NULL) && !extendClasspath(classPath)) {
            return -1;
        }
    
        if (odtAgentSettings != NULL) {
            mvmManagerArgument = 
                    constructODTAgentMVMManagerArgument(odtAgentSettings);
            if (mvmManagerArgument == NULL) {
                reportOutOfMemoryError();
                return -1;
            }
            
            /* replace the first mvm manager argument */
            argv1[3] = mvmManagerArgument;
        }

        javanotify_start_java_with_arbitrary_args(4, argv1);

#else /* ENABLE_MULTIPLE_ISOLATES */

        /* run the ODT agent directly */

        char *argv1[4] = {
                "runMidlet", "-1",
                "com.sun.midp.odd.ODTAgentMIDlet", 
                odtAgentSettings };
        int numargs = (odtAgentSettings != NULL) ? 4 : 3;
        javanotify_start_java_with_arbitrary_args(numargs, argv1);

#endif /* ENABLE_MULTIPLE_ISOLATES */

    } else { /* no execution mode, invalid arguments */
        javanotify_start();
    }

#if ENABLE_JSR_120
    /* initializeWMASupport(); */
#endif

    InitializeLimeEvents();

    if ((stdoutPort != -1) || (stderrPort != -1)) {
#ifdef ENABLE_OUTPUT_REDIRECTION
        /* enable redirection of output to sockets */
        SIOInit(stdoutPort, stderrPort);
#else /* ENABLE_OUTPUT_REDIRECTION */
        javautil_debug_print(JAVACALL_LOG_INFORMATION, "main",
                             "Output redirection is not supported.");
#endif /* ENABLE_OUTPUT_REDIRECTION */
    }

#ifdef ENABLE_MONITOR_PARENT_PROCESS
    startParentProcessMonitor();
#endif /* ENABLE_MONITOR_PARENT_PROCESS */

    JavaTask();

#ifdef ENABLE_MONITOR_PARENT_PROCESS
    stopParentProcessMonitor();
#endif /* ENABLE_MONITOR_PARENT_PROCESS */

#ifdef ENABLE_OUTPUT_REDIRECTION
    if ((stdoutPort != -1) || (stderrPort != -1)) {
        /* stop redirection of output to sockets */
        SIOStop();
    }
#endif /* ENABLE_OUTPUT_REDIRECTION */

    javacall_events_finalize();

#if ENABLE_JSR_120
    finalize_wma_emulator();
#endif

    /* free allocated memory */
    free(descriptor);
    free(className);
    free(url);
    free(storageName);
    free(odtAgentSettings);
    free(mvmManagerArgument);

    FinalizeLimeEvents();
    return 1;
}

/**
 * Logs an out of memory error.
 */ 
static void 
reportOutOfMemoryError() {
    javautil_debug_print(JAVACALL_LOG_CRITICAL, "main",
                         "Out of memory error");
}

#if ENABLE_MULTIPLE_ISOLATES

/**
 * Constructs a new string in the form which is accepted as a parameter to the
 * MVM application manager and causes the manager to run ODT agent automatically 
 * with the specified settings. The returned value is a pointer to the newly 
 * allocated string. Its responsibility of the caller to free it after use.
 * 
 * @param odtAgentSettings pointer to settings string to be passed to ODT agent
 * @return pointer to the allocated argument string or <code>NULL</code> if
 *      the string allocation failed 
 */  
static char*
constructODTAgentMVMManagerArgument(const char* odtAgentSettings) {
    int mvmManagerParamLen = 
            RUN_ODT_AGENT_PREFIX_LEN + 1 + strlen(odtAgentSettings);
    char* mvmManagerParamString = 
            (char*)malloc((mvmManagerParamLen + 1) * sizeof(char));
    if (mvmManagerParamString == NULL) {
        return NULL;
    }
    
    strcpy(mvmManagerParamString, RUN_ODT_AGENT_PREFIX ":");
    strcat(mvmManagerParamString, odtAgentSettings);
    
    return mvmManagerParamString;
}

/**
 * Inserts the specified classpath in front of the classpath stored in the 
 * "classpathext" system property. In this way the input classpath will have
 * higher priority than the previously stored classpath.
 * 
 * @param classpath the classpath to insert in front of "classpathext"
 * @return <code>1</code> on success, <code>0</code> on failure  
 */   
static int 
extendClasspath(const char* classpath) {
    char* oldClasspathext = NULL;
    char* pathSeparator = NULL;
    int classpathLength;
    int separatorLength;
    int offset;
    char* newClasspathext;
    char* utf8Classpath;

    utf8Classpath = ansi_to_utf8(classpath);
    if (utf8Classpath == NULL) {
        return 0;
    }
    
    javacall_get_property("classpathext", JAVACALL_APPLICATION_PROPERTY,
                          &oldClasspathext);
    if ((oldClasspathext == NULL) || (oldClasspathext[0] == '\0')) {
        if (javacall_set_property(
                "classpathext", utf8Classpath, 1, 
                JAVACALL_APPLICATION_PROPERTY) != JAVACALL_OK) {
            javautil_debug_print(JAVACALL_LOG_ERROR, "main",
                                 "Failed to set classpath!");
            javacall_free(utf8Classpath);
            return 0;
        }
    
        return 1;
    }

    javacall_get_property("path.separator", JAVACALL_APPLICATION_PROPERTY, 
                          &pathSeparator);
    if (pathSeparator == NULL) {
        javautil_debug_print(JAVACALL_LOG_ERROR, "main",
                             "Failed to get path separator!");
        javacall_free(utf8Classpath);
        return 0;
    }

    classpathLength = strlen(utf8Classpath);
    separatorLength = strlen(pathSeparator);
    newClasspathext = (char*)malloc((classpathLength + separatorLength 
                                            + strlen(oldClasspathext) + 1) 
                                        * sizeof(char));
    if (newClasspathext == NULL) {
        reportOutOfMemoryError();
        javacall_free(utf8Classpath);
        return 0;
    }        
    
    /* prepend classpath to make it of higher priority */
    offset = 0;
    strcpy(newClasspathext, utf8Classpath);
    
    offset += classpathLength;
    strcpy(newClasspathext + offset, pathSeparator);
    
    offset += separatorLength;
    strcpy(newClasspathext + offset, oldClasspathext);

    if (javacall_set_property("classpathext", newClasspathext, 1, 
                              JAVACALL_APPLICATION_PROPERTY) != JAVACALL_OK) {
        javautil_debug_print(JAVACALL_LOG_ERROR, "main",
                             "Failed to set classpath!");

        javacall_free(utf8Classpath);
        free(newClasspathext);
        return 0;
    }

    javacall_free(utf8Classpath);
    free(newClasspathext);
    return 1;    
}

#endif /* ENABLE_MULTIPLE_ISOLATES */

/**
 * Returns the properties file name found in the given argument list or
 * <tt>NULL</tt> if the name is not specified in the list.
 * 
 * @param fileNameLen pointer to the length of the returned string
 * @param argv pointer to the array of arguments
 * @param argc the number of arguments in the array
 * @return the property file name or <code>NULL</code> if the name is not
 *      specified in the list
 */      
static javacall_utf16* 
get_properties_file_name(int* fileNameLen, const char* argv[], int argc) {
    int i;
    
    for (i = 0; i < argc; ++i) {
        if (strncmp(argv[i], PREFS_OPTION ":", PREFS_OPTION_LEN + 1) == 0) {
            /* handle "-Xprefs:<filename>" */
            
            /* the file name starts after ':' */
            const char* mbFileName = argv[i] + PREFS_OPTION_LEN + 1;
            javacall_utf16* wideFileName;
            int wideFileNameLen;
            
            /* get the length */
            wideFileNameLen = 
                    MultiByteToWideChar(CP_ACP, 0, mbFileName, -1, NULL, 0);
            if (wideFileNameLen <= 1) {
                return NULL;
            }
                        
            wideFileName = (javacall_utf16*)javacall_malloc(
                                   wideFileNameLen * sizeof(javacall_utf16));
            if (wideFileName == NULL) {
                return NULL;
            }
            
            /* do the conversion, assuming javacall_utf16 ~ WCHAR */
            if (wideFileNameLen != 
                    MultiByteToWideChar(CP_ACP, 0, mbFileName, -1, 
                                        wideFileName, wideFileNameLen)) {
                javacall_free(wideFileName);
                return NULL;
            }

            *fileNameLen = wideFileNameLen - 1;
            return wideFileName;                        
        }
    }
    
    /* no properties file argument */
    return NULL;
} 

static char* ansi_to_utf8(char *str) {
    int length = strlen(str);
    javacall_utf16* w_buf;
    int w_length;
    char *res;
    int r_length;

    if (str == NULL)
        return str;

    if (*str == '\0') {
        res = (char*)javacall_malloc(1);
        *res = '\0';
    } else {
        w_length = MultiByteToWideChar(CP_ACP, 0, str, length, NULL, 0);
        if (w_length == 0) {
            javautil_debug_print(JAVACALL_LOG_ERROR, "main",
                                 "Cannot convert argv into unicode: %d",
                                 GetLastError());
            return NULL;
        }

        w_buf = (javacall_utf16*)javacall_malloc(w_length * sizeof(javacall_utf16));
        if (w_buf == NULL) {
            javautil_debug_print(JAVACALL_LOG_ERROR, "main",
                                 "Cannot convert argv into unicode: not enough memory",
                                 GetLastError());
            return NULL;
        }

        if (MultiByteToWideChar(CP_ACP, 0, str, length, w_buf, w_length) == 0) {
            javautil_debug_print(JAVACALL_LOG_ERROR, "main",
                                 "Cannot convert argv into unicode: %d",
                                 GetLastError());
            javacall_free(w_buf);
            return NULL;
        }

        r_length = WideCharToMultiByte(CP_UTF8, 0, w_buf, w_length, NULL, 0, NULL, 0);
        if (r_length == 0) {
            javautil_debug_print(JAVACALL_LOG_ERROR, "main",
                                 "Cannot convert argv into unicode: %d",
                                 GetLastError());
            javacall_free(w_buf);
            return NULL;
        }

        res = (char*)javacall_malloc(r_length + 1);
        if (res == NULL) {
            javautil_debug_print(JAVACALL_LOG_ERROR, "main",
                                 "Cannot convert argv into unicode: not enough memory",
                                 GetLastError());
            javacall_free(w_buf);
            return NULL;
        }

        r_length = WideCharToMultiByte(CP_UTF8, 0, w_buf, w_length, res, r_length, NULL, 0);
        if (r_length == 0) {
            javautil_debug_print(JAVACALL_LOG_ERROR, "main",
                                 "Cannot convert argv into unicode: %d",
                                 GetLastError());
            javacall_free(w_buf);
            javacall_free(res);
            return NULL;
        }

        javacall_free(w_buf);
        res[r_length] = '\0';
    }

    return res;
}

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
#include <ctype.h>

#include <jvm.h>
#include <kni.h>

#include <findMidlet.h>
#include <midpAMS.h>
#include <suitestore_export.h>
#include <midp_run_vm.h>
#include <midpInit.h>
#include <midpMalloc.h>

#include <midpStorage.h>
#include <midpServices.h>
#include <midpNativeThread.h>
#include <midpNativeAppManager.h>
#include <midpUtilKni.h>

#if ENABLE_MULTIPLE_ISOLATES
#define MIDP_HEAP_REQUIREMENT (4 * 1024 * 1024)
#else
#define MIDP_HEAP_REQUIREMENT (1024 * 1024)
#endif

/**
 * @file
 *
 * Example of how the public MIDP API can be used to list installed
 * MIDlet Suite.
 */

extern char* midpFixMidpHome(char *cmd);

#if ENABLE_NATIVE_AMS
extern int findNextEmptyMIDlet(int appId);
extern void initNams(void);
#if ENABLE_I3_TEST
extern void initNamsCommands(int argn, char* args[]);
extern midp_ThreadRoutine midlet_starter_routine;
#endif
#endif

/** Usage text for the runNams executable. */
static const char* const runUsageText =
"\n"
"Usage: runNams [<VM args>] (-namsTestService |\n"
"           -runMainClass <mainClass> [<args>] |\n"
"           (-runMidlet | -jamsTestMode (<suite number> | <suite ID>)\n"
"              [<classname of MIDlet to run> [<arg0> [<arg1> [<arg2>]]]]))\n"
"\n"
"Options are:\n"
"    -namsTestService - runs NAMS Test Service;\n"
"    -runMainClass - runs an alternate main class;\n"
"    -runMidlet    - runs a MIDlet using NAMS API;\n"
"    -jamsTestMode - runs a MIDlet in the AMS isolate using the JAMS mode API\n"
"\n"
"  where <suite number> is the number of a suite as displayed by the\n"
"  listMidlets command, and <suite ID> is the unique ID a suite is \n"
"  referenced by\n\n";

static jchar internalSuiteId[] = {'i', 'n', 't', 'e', 'r', 'n', 'a', 'l'};
static jint internalSuiteIdLen = sizeof (internalSuiteId) / sizeof (jchar);
static jchar discoverClassName[] =
    {'c', 'o', 'm', '.', 's', 'u', 'n', '.', 'm', 'i', 'd', 'p',
    '.', 'i', 'n', 's', 't', 'a', 'l', 'l', 'e', 'r', '.',
    'D', 'i', 's', 'c', 'o', 'v', 'e', 'r', 'y', 'A', 'p', 'p'};
static jchar selectorClassName[] =
    {'c', 'o', 'm', '.', 's', 'u', 'n', '.', 'm', 'i', 'd', 'p',
    '.', 'm', 'i', 'd', 'l', 'e', 't', 's', 'u', 'i', 't', 'e',
    '.', 'S', 'e', 'l', 'e', 'c', 't', 'o', 'r'};
#if ENABLE_NATIVE_AMS
static jchar namsManagerClassName[] =
    {'c', 'o', 'm', '.', 's', 'u', 'n', '.', 'm', 'i', 'd', 'p',
    '.', 'm', 'a', 'i', 'n',
    '.', 'N', 'a', 'm', 's', 'M', 'a', 'n', 'a', 'g', 'e', 'r'};
#if ENABLE_I3_TEST
static jchar i3frameworkClassName[] =
    {'c', 'o', 'm', '.', 's', 'u', 'n', '.', 'm', 'i', 'd', 'p',
    '.', 'i', '3', 't', 'e', 's', 't',
    '.', 'N', 'a', 'm', 's', 'F', 'r', 'a', 'm', 'e', 'w', 'o', 'r', 'k'};
#endif
#endif

static pcsl_string argsForMidlet[3] = {
    PCSL_STRING_NULL_INITIALIZER,
    PCSL_STRING_NULL_INITIALIZER,
    PCSL_STRING_NULL_INITIALIZER
};
static pcsl_string suiteIDToRun = PCSL_STRING_NULL_INITIALIZER;
static pcsl_string* const asuiteIDToRun = &suiteIDToRun;
static pcsl_string classNameToRun = PCSL_STRING_NULL_INITIALIZER;
static pcsl_string* const aclassNameToRun = &classNameToRun;
static pcsl_string* pSuiteIds = NULL;
static jint numberOfSuiteIds = 0;
static jint *pSuiteRunState = NULL;
static jint foregroundAppId = 0;


static pcsl_string* getSuiteId(int index) {
    if (index >= 0 && index < numberOfSuiteIds) {
        return (pSuiteIds+index);
    }

    return NULL;
}

static void loadSuiteIds() {
    int i;

    /*
     * This is a public API which can be called without the VM running
     * so we need automatically init anything needed, to make the
     * caller's code less complex.
     *
     * Initialization is performed in steps so that we do use any
     * extra resources such as the VM for the operation being performed.
     */
    if (midpInit(LIST_LEVEL) != 0) {
        return;
    }

    numberOfSuiteIds = midpGetSuiteIDs(&pSuiteIds);
    if (numberOfSuiteIds < 0) {
        REPORT_ERROR(LC_AMS, "Can't load suite IDs.");
        fprintf(stderr, "Can't load suite IDs.\n");
        return;
    }

    pSuiteRunState = (jint*)midpMalloc(numberOfSuiteIds*sizeof(jint));
    if (pSuiteRunState == NULL) {
        REPORT_ERROR(LC_AMS, "Out of Memory");
        fprintf(stderr, "Out Of Memory\n");
        return;
    }

    for (i = 0; i < numberOfSuiteIds; i++) {
        pSuiteRunState[i] = MIDP_MIDLET_STATE_DESTROYED;
    }
}

static void unloadSuiteIds() {
    if (pSuiteIds != NULL) {
        midpFreeSuiteIDs(pSuiteIds, numberOfSuiteIds);
        pSuiteIds = NULL;
        numberOfSuiteIds = 0;
    }

    if (pSuiteRunState != NULL) {
        midpFree(pSuiteRunState);
        pSuiteRunState = NULL;
    }
}

void nams_process_command(int command, int param) {
    jint classNameLen = -1;
    jchar* pClassName = NULL;

    printf("* Received nams command (%d, %d)\n", command, param);

    switch (command) {
    case -1:
        midp_system_stop();
        break;

    case 1: {
        /* Run by number */
        pcsl_string *suiteId = getSuiteId(param - 1);
	if (suiteId == NULL) {
            printf("invalid suite index [%d]\n", param);
            break;
        }

        GET_PCSL_STRING_DATA_AND_LENGTH(suiteId)
        midp_midlet_create_start(/* midlet suite id */
                                 (jchar*)suiteId_data,
                                 suiteId_len,
                                 /* midlet class name */
                                 NULL_MIDP_STRING.data,
                                 NULL_MIDP_STRING.len,
                                 param
                                );
        RELEASE_PCSL_STRING_DATA_AND_LENGTH
        break;
    }

    case 2:
        midp_midlet_pause(param);
        break;

    case 3:
        midp_midlet_resume(param);
        break;

    case 4:
        midp_midlet_destroy(param);
        break;

    case 5:
        midp_midlet_set_foreground(param);
        break;

    case 6: {

        switch (param) {
#if ENABLE_NATIVE_AMS
        case 0:
            pClassName = namsManagerClassName;
            classNameLen = sizeof (namsManagerClassName) / sizeof (jchar);
            break;

#if ENABLE_I3_TEST
        case 1:
            pClassName = i3frameworkClassName;
            classNameLen = sizeof (i3frameworkClassName) / sizeof (jchar);
            break;
#endif
#endif
        case 2:
            pClassName = discoverClassName;
            classNameLen = sizeof (discoverClassName) / sizeof (jchar);
            break;

        default:
            pClassName = selectorClassName;
            classNameLen = sizeof (selectorClassName) / sizeof (jchar);
            break;
        }

        midp_midlet_create_start(internalSuiteId, internalSuiteIdLen,
                                 pClassName, classNameLen,
#if ENABLE_NATIVE_AMS && ENABLE_I3_TEST
                                 (param != 0)
                                     ? findNextEmptyMIDlet(0) : 0
#else
                                 -param
#endif
                                );

        break;
    }

    default:
        printf("* Received WM_TEST(%d, %d)\n", command, param);
        break;
    }
}

/**
 * The function that will be called when Java system state
 * changes.
 *
 * @param state The new system state as defined in this file
 */
void system_state_listener(jint state) {
    printf("--- system_state_listener(%d)\n", state);

    if (state == MIDP_SYSTEM_STATE_STARTED) {
        int i;
        const jchar *jchArgsForMidlet[3];
        jint  argsLen[3];

        // Currently we support up to 3 arguments.
        for (i = 0; i < 3; i++) {
            jchArgsForMidlet[i] = pcsl_string_get_utf16_data(&argsForMidlet[i]);
            argsLen[i] = pcsl_string_utf16_length(&argsForMidlet[i]);
        }
        GET_PCSL_STRING_DATA_AND_LENGTH(asuiteIDToRun)
        GET_PCSL_STRING_DATA_AND_LENGTH(aclassNameToRun)

        midp_midlet_create_start_with_args((jchar*)asuiteIDToRun_data,
                                           asuiteIDToRun_len,
                                           (jchar*)aclassNameToRun_data,
                                           aclassNameToRun_len,
                                           (jchar**)jchArgsForMidlet,
                                           argsLen,
                                           3,
                                           findNextEmptyMIDlet(0));
        RELEASE_PCSL_STRING_DATA_AND_LENGTH
        RELEASE_PCSL_STRING_DATA_AND_LENGTH
        for (i = 0; i < 3; i++) {
            pcsl_string_release_utf16_data(jchArgsForMidlet[i],
                                           &argsForMidlet[i]);
        }
    }
}

/**
 * The typedef of the background listener that is notified
 * when the background system changes.
 *
 * @param reason              The reason the background change happened
 */
void background_listener(jint reason) {
    int i = 0;
    printf("--- background_listener(%d)\n", reason);

    for (i = 0; i < numberOfSuiteIds; i++) {
        if (pSuiteRunState[i] == MIDP_MIDLET_STATE_STARTED &&
            i+1 != foregroundAppId) {

            printf("midp_midlet_set_foreground(%d)  reason = %d\n",
                   i+1, reason);
            midp_midlet_set_foreground(i+1);
            break;
        }
    }
}

/**
 * The typedef of the foreground listener that is notified
 * when the foreground midlet changes.
 *
 * @param appId               The application id used to identify the app
 * @param reason              The reason the foreground change happened
 */
void foreground_listener(jint appId, jint reason) {
    printf("--- foreground_listener(%d, %d)\n", appId, reason);

    foregroundAppId = appId;
    if (appId > 0 && appId <= numberOfSuiteIds) {
        printf("[%d] \"", appId);
        PRINTF_PCSL_STRING("%s",pSuiteIds+appId-1);
        printf("\"  has the foreground   reason = %d\n", reason);
    }
}

/**
 * The typedef of the midlet state listener that is notified
 * with the midlet state changes.
 *
 * @param appId               The application id used to identify the app
 * @param state               The new state of the application
 * @param reason              The reason the state change happened
 */
void state_change_listener(jint appId, jint state, jint reason) {
    printf("--- state_change_listener(%d, %d, %d)\n", appId, state, reason);

    if (appId > 0 && appId <= numberOfSuiteIds) {
        pSuiteRunState[appId-1] = state;

        printf("[%d] \"", appId);
        PRINTF_PCSL_STRING("%s",pSuiteIds+appId-1);
        printf("\"  changed state - (%d) \"", state);

	switch (state) {
        case MIDP_MIDLET_STATE_STARTED: printf("STARTED\""); break;
        case MIDP_MIDLET_STATE_PAUSED: printf("PAUSED\""); break;
        case MIDP_MIDLET_STATE_DESTROYED: printf("DESTROYED\""); break;
        case MIDP_MIDLET_STATE_ERROR: printf("ERROR\""); break;
        default: printf("INVALID!!!\""); break;
        }

        printf("  reason = %d\n", reason);
    }
}

/**
 * Print out a suite termination message containing the given suite ID.
 *
 * @param suiteId ID of the suite
 * @param suiteIdLen length of the suite ID
 */
void suite_termination_listener(jchar* suiteId, jint suiteIdLen) {
    int i;

    printf("Suite has terminated. ID = ");

    for (i = 0; i < suiteIdLen; i++) {
        putchar(suiteId[i]);
    }

    puts("");
}

#if ENABLE_NATIVE_AMS && ENABLE_I3_TEST
void initNamsCommands(int argn, char* args[]) {
    int i;
    int* cmd;

    if (argn <= 0) {
        return;
    }

    cmd = midpMalloc(sizeof(int) * (1 + 2 * (argn - 1)));
    cmd[0] = argn - 1;

    for (i = 1; i < argn; ++i) {
        cmd[1 + 2 * (i - 1) + 0] = 6;
        cmd[1 + 2 * (i - 1) + 1] = atoi(args[i]);
    };

    for (i = 1; i < (1 + 2 * (argn - 1)); i+=2) {
        /*
        printf("DEBUG: midlet starter: cmd = %i, param = %i\n",
            cmd[i+0], cmd[i+1]);
        */
        if (cmd[i + 0] != 0 && cmd[i + 1] >= 0) {
            midp_startNativeThread(
                (midp_ThreadRoutine*)&midlet_starter_routine,
                (midp_ThreadRoutineParameter)&cmd[i]);
        }
    }
    /*
     * Now cmd is not destroyed my midpFree(cmd) -
     * this storage is needed by spawned threads after this routine returns...
     */
}

#endif

/**
 * Sets up the arguments required to start a midlet:
 * suiteIDToRun, classNameToRun, argsForMidlet[]
 *
 * @param argc The total number of arguments
 * @param argv An array of 'C' strings containing the arguments
 *
 * @return <tt>0</tt> if successful,
 *         <tt>MIDP_ERROR_STATUS</tt> if an error
*/
int setupArgToStartMidlet(int argc, char* argv[]) {
    int status = MIDP_ERROR_STATUS;

    do {
        int i, len;

        /* if the storage name only digits, convert it */
        int onlyDigits = 1;
        len = strlen(argv[0]);
        for (i = 0; i < len; i++) {
            if (!isdigit((argv[0])[i])) {
                onlyDigits = 0;
                break;
            }
        }

        if (onlyDigits) {
            /* Run by number */
            int suiteNumber;

            /* the format of the string is "number:" */
            if (sscanf(argv[0], "%d", &suiteNumber) != 1) {
                REPORT_ERROR(LC_AMS, "Invalid suite number format");
                fprintf(stderr, "Invalid suite number format\n");
                break;
            }

            if (suiteNumber > numberOfSuiteIds || suiteNumber < 1) {
                REPORT_ERROR(LC_AMS, "Suite number out of range");
                fprintf(stderr, "Suite number out of range\n");
                break;
            }

            suiteIDToRun = pSuiteIds[suiteNumber - 1];
        } else {
            /* Run by ID */
            pcsl_string_status rc =
                pcsl_string_from_chars(argv[0], &suiteIDToRun);

            if (PCSL_STRING_OK != rc) {
                REPORT_ERROR(LC_AMS, "Out of Memory");
                fprintf(stderr, "Out Of Memory\n");
                break;
            }
        }

        /* Setting up a class name of the midlet to be run */
        if (argc > 1) {
            if (PCSL_STRING_OK !=
                pcsl_string_from_chars(argv[1], &classNameToRun)) {
                REPORT_ERROR(LC_AMS, "Out of Memory");
                fprintf(stderr, "Out Of Memory\n");
                break;
            }
        }

        /* Setting up arguments for the midlet */
        for (i = 0; i < 3; i ++) {
            if (argc > i + 2) {
                if (PCSL_STRING_OK !=
                    pcsl_string_from_chars(argv[i + 2], &argsForMidlet[i])) {
                    REPORT_ERROR(LC_AMS, "Out of Memory");
                    fprintf(stderr, "Out Of Memory\n");
                    break;
                }
            } else {
                argsForMidlet[i] = PCSL_STRING_NULL;
            }
        }

        if (pcsl_string_is_null(&classNameToRun)) {
            int res = find_midlet_class(&suiteIDToRun, 1, &classNameToRun);
            if (OUT_OF_MEM_LEN == res) {
                REPORT_ERROR(LC_AMS, "Out of Memory");
                fprintf(stderr, "Out Of Memory\n");
                break;
            }

            if (NULL_LEN == res) {
                REPORT_ERROR(LC_AMS, "Could not find the first MIDlet");
                fprintf(stderr, "Could not find the first MIDlet\n");
                break;
            }
        }

        status = 0; // ok
    } while (0);

    return status;
}

/**
 * Mode 1. NAMS test service:<br>
 * runNams [&lt;VM args&gt;] -namsTestService<br>
 * Does not return until the system is stopped.
 *
 * @param argc The total number of arguments
 * @param argv An array of 'C' strings containing the arguments
 *
 * @return <tt>MIDP_SHUTDOWN_STATUS</tt> if the system is shutting down or
 *         <tt>MIDP_ERROR_STATUS</tt> if an error
 */
int runNamsTestService(int argc, char* argv[]) {
#if ENABLE_NATIVE_AMS && ENABLE_I3_TEST
    initNams();
    initNamsCommands(argc - 1, argv + 1);
#else
    (void)argc;
    (void)argv;
#endif

    return midp_system_start();
}

/**
 * Mode 2. Run a MIDlet using the NAMS API:<br>
 * runNams [&lt;VM args&gt;] -runMidlet &lt;suiteId or suite number&gt;<br>
 *   &lt;MIDlet classname&gt; [[[&lt;arg1&gt;] &lt;arg2&gt;] &lt;arg3&gt;]<br>
 *
 * @param argc The total number of arguments
 * @param argv An array of 'C' strings containing the arguments
 *
 * @return <tt>0</tt> if successful,
 *         <tt>MIDP_SHUTDOWN_STATUS</tt> if the system is shutting down,
 *         <tt>MIDP_ERROR_STATUS</tt> if an error,
 *         <tt>SUITE_NOT_FOUND_STATUS</tt> if the MIDlet suite not found
 */
int runMidletWithNAMS(int argc, char* argv[]) {
    int status;

    status = setupArgToStartMidlet(argc, argv);
    if (status != 0) {
        return status;
    }

    /* set the listeners before starting the system */
    midp_system_set_state_change_listener(system_state_listener);
    midp_system_set_background_listener(background_listener);
    midp_midlet_set_foreground_listener(foreground_listener);
    midp_midlet_set_state_change_listener(state_change_listener);
    midp_suite_set_termination_listener(suite_termination_listener);

    return midp_system_start();
}

/**
 * Mode 3. Run a MIDlet in the AMS isolate using the JAMS mode API:<br>
 * runNams [&lt;VM args&gt;] -jamsTestMode &lt;suiteId or suite number&gt;<br>
 *     &lt;MIDlet classname&gt; [[[&lt;arg1&gt;] <arg2>] &lt;arg3&gt;]<br>
 *
 * @param argc The total number of arguments
 * @param argv An array of 'C' strings containing the arguments
 *
 * @return <tt>0</tt> if successful,
 *         <tt>MIDP_SHUTDOWN_STATUS</tt> if the system is shutting down,
 *         <tt>MIDP_ERROR_STATUS</tt> if an error,
 *         <tt>SUITE_NOT_FOUND_STATUS</tt> if the MIDlet suite not found
 */
int runMidletWithJAMS(int argc, char* argv[]) {
    int status = MIDP_ERROR_STATUS;

    if (argc < 1) {
        return status;
    }

    status = setupArgToStartMidlet(argc, argv);
    if (status != 0) {
        return status;
    }

    status = midp_run_midlet_with_args_cp(&suiteIDToRun, &classNameToRun,
        &argsForMidlet[0], &argsForMidlet[1], &argsForMidlet[2], 0, NULL);

    return status;
}

/**
 * Mode 4. Run an alternate main class:<br>
 * runNams [&lt;VM args&gt;] -runMainClass mainClass [&lt;args&gt;]
 *
 * @param argc The total number of arguments
 * @param argv An array of 'C' strings containing the arguments
 *
 * @return <tt>0</tt> if successful,
 *         <tt>MIDP_SHUTDOWN_STATUS</tt> if the system is shutting down,
 *         <tt>MIDP_ERROR_STATUS</tt> if an error,
 *         <tt>SUITE_NOT_FOUND_STATUS</tt> if the MIDlet suite not found
 */
int runMainClass(int argc, char* argv[]) {
    char* mainClass; // the class name of midlet to run
    int status = MIDP_ERROR_STATUS;

    if (argc < 1) {
        return status;
    }

    if (midpInitialize() != 0) {
        return status;
    }

    mainClass = argv[0];
    status = midpRunMainClass(NULL, mainClass, argc, argv);
    midpFinalize();

    return status;
}

/**
 * Start the MT MIDP. Waits until it shuts down, and then exits by default.
 * If the -restart option is given, and no VM error occurred, MIDP is
 * restarted.
 *
 * @param argc The total number of arguments
 * @param argv An array of 'C' strings containing the arguments
 *
 * @return <tt>0</tt> for success, otherwise <tt>-1</tt>
 */
int
main(int argc, char* argv[]) {
    int status = -1;
    char* midpHome;
    int used;
    int savedArgc;
    char **savedArgv;
    int restart = 0;

    savedArgc = argc;
    savedArgv = argv;

    do {
        argc = savedArgc;
        argv = savedArgv;

        /* For development platforms MIDP_HOME is dynamic. */
        midpHome = midpFixMidpHome(argv[0]);
        if (midpHome == NULL) {
            /* midpFixMidpHome has already issued an error message */
            return -1;
        }

        /* set up midpHome before calling midp_system_start */
        midpSetHomeDir(midpHome);

        JVM_Initialize();

        /*
         * Set Java heap capacity now so it can been overridden from command
         * line.
         */
        JVM_SetConfig(JVM_CONFIG_HEAP_CAPACITY, MIDP_HEAP_REQUIREMENT);

        /* Parse VM arguments */
        argc--;
        argv++;
        while ((used = JVM_ParseOneArg(argc, argv)) > 0) {
            argc -= used;
            argv += used;
        }

        /*
         * Parse runNams arguments. The following options are allowed:
         *
         * -namsTestService
         * -runMidlet <suiteId or suite number> <MIDlet classname>
         *     [[[<arg1>] <arg2>] <arg3>]
         * -jamsTestMode <suiteId or suite number> <MIDlet classname>
         *     [[[<arg1>] <arg2>] <arg3>]
         * -runMainClass mainClass [<args>]
         */
        if (argc > 0 && 0 == strcmp(argv[0], "-restart")) {
            restart = 1;
            argc--;
            argv++;
        }

        if (argc < 1) {
            fprintf(stderr, runUsageText);
            break;
        } else {
            int i;
            int mode = -1;
            char *options[] = {
                "-namsTestService", "-runMidlet",
                "-jamsTestMode", "-runMainClass"
            };
            int (*handlers[])(int argc, char* argv[]) = {
                runNamsTestService, runMidletWithNAMS,
                runMidletWithJAMS, runMainClass
            };

            for(i = 0; i < (int)(sizeof(options) / sizeof(options[0])); i++) {
                if (!strcmp(argv[0], options[i])) {
                    mode = i;
                    break;
                }
            }

            if (mode == -1) {
                fprintf(stderr, runUsageText);
                break;
            } else {
                /* load the suite id's */
                loadSuiteIds();
                status = handlers[i](--argc, ++argv);
            }
        }

        /* clean up */
        unloadSuiteIds();

        if (status != 0 && status != MIDP_SHUTDOWN_STATUS) {
            fprintf(stderr, "VM startup failed (%d)\n", status);
            return -1;
        }
    } while (restart);

    /* it is safe to call it more than once */
    unloadSuiteIds();

    return 0;
}

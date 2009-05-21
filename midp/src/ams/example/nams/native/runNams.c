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

#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <jvm.h>
#include <kni.h>

#include <findMidlet.h>
#include <midpAMS.h>
#include <midp_run_vm.h>
#include <midpInit.h>
#include <midpMalloc.h>
#include <midpStorage.h>
#include <midpServices.h>
#include <midpNativeThread.h>
#include <midpNativeAppManager.h>
#include <midpUtilKni.h>
#include <suitestore_task_manager.h>
#include <commandLineUtil_md.h>

/**
 * @file
 *
 * Example of how the public MIDP API can be used to run an installed
 * MIDlet Suite.
 */

#define MIDLET_DESTROY_DEFAULT_TIMEOUT 5000

#define NAMS_TEST_SERVICE_CLASS_NAME "com.sun.midp.main.NamsTestServiceMidlet"

#if ENABLE_I3_TEST
extern void initNams(void);
extern int findNextEmptyMIDlet(int appId);
extern midp_ThreadRoutine midlet_starter_routine;
static void initNamsCommands(int argn, char* args[]);
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

static jchar discoverClassName[] =
    {'c', 'o', 'm', '.', 's', 'u', 'n', '.', 'm', 'i', 'd', 'p',
    '.', 'i', 'n', 's', 't', 'a', 'l', 'l', 'e', 'r', '.',
    'D', 'i', 's', 'c', 'o', 'v', 'e', 'r', 'y', 'A', 'p', 'p'};
static jchar selectorClassName[] =
    {'c', 'o', 'm', '.', 's', 'u', 'n', '.', 'm', 'i', 'd', 'p',
    '.', 'a', 'p', 'p', 'm', 'a', 'n', 'a', 'g', 'e', 'r',
    '.', 'M', 'I', 'D', 'l', 'e', 't', 'S', 'e', 'l', 'e', 'c', 't', 'o', 'r'};

#if ENABLE_I3_TEST
static jchar namsManagerClassName[] =
    {'c', 'o', 'm', '.', 's', 'u', 'n', '.', 'm', 'i', 'd', 'p',
    '.', 'm', 'a', 'i', 'n',
    '.', 'N', 'a', 'm', 's', 'M', 'a', 'n', 'a', 'g', 'e', 'r'};
static jchar i3frameworkClassName[] =
    {'c', 'o', 'm', '.', 's', 'u', 'n', '.', 'm', 'i', 'd', 'p',
    '.', 'i', '3', 't', 'e', 's', 't',
    '.', 'N', 'a', 'm', 's', 'F', 'r', 'a', 'm', 'e', 'w', 'o', 'r', 'k'};
#endif

static pcsl_string argsForMidlet[3] = {
    PCSL_STRING_NULL_INITIALIZER,
    PCSL_STRING_NULL_INITIALIZER,
    PCSL_STRING_NULL_INITIALIZER
};
static SuiteIdType suiteIDToRun = UNUSED_SUITE_ID;
static pcsl_string classNameToRun = PCSL_STRING_NULL_INITIALIZER;
static pcsl_string* const aclassNameToRun = &classNameToRun;
static SuiteIdType* pSuiteIds = NULL;
static int numberOfSuiteIds = 0;
static jint *pSuiteRunState = NULL;
static jint foregroundAppId = 0;


static SuiteIdType getSuiteId(int index) {
    if (index >= 0 && index < numberOfSuiteIds) {
        return pSuiteIds[index];
    }

    return UNUSED_SUITE_ID;
}

static MIDPError loadSuiteIds() {
    int i;
    MIDPError status;

    /*
     * This is a public API which can be called without the VM running
     * so we need automatically init anything needed, to make the
     * caller's code less complex.
     *
     * Initialization is performed in steps so that we do use any
     * extra resources such as the VM for the operation being performed.
     */
    if (midpInit(LIST_LEVEL) != 0) {
        return OUT_OF_MEMORY;
    }

    status = midp_get_suite_ids(&pSuiteIds, &numberOfSuiteIds);
    if (status != ALL_OK) {
        REPORT_ERROR(LC_AMS, "Can't load suite IDs.");
        fprintf(stderr, "Can't load suite IDs: error %d.\n", status);
        return status;
    }

    pSuiteRunState = (jint*)midpMalloc(numberOfSuiteIds*sizeof(jint));
    if (pSuiteRunState == NULL) {
        REPORT_ERROR(LC_AMS, "Out of Memory");
        fprintf(stderr, "Out Of Memory\n");
        return OUT_OF_MEMORY;
    }

    for (i = 0; i < numberOfSuiteIds; i++) {
        pSuiteRunState[i] = MIDP_MIDLET_STATE_DESTROYED;
    }

    return ALL_OK;
}

static void unloadSuiteIds() {
    if (pSuiteIds != NULL) {
        midp_free_suite_ids(pSuiteIds, numberOfSuiteIds);
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
        SuiteIdType suiteId = getSuiteId(param - 1);
	if (suiteId == UNUSED_SUITE_ID) {
            printf("invalid suite index [%d]\n", param);
            break;
        }

        midp_midlet_create_start(/* midlet suite id */
                                 suiteId,
                                 /* midlet class name */
                                 NULL_MIDP_STRING.data,
                                 NULL_MIDP_STRING.len,
                                 param,
                                 NULL
                                );
        break;
    }

    case 2:
        midp_midlet_pause(param);
        break;

    case 3:
        midp_midlet_resume(param);
        break;

    case 4:
        midp_midlet_destroy(param, MIDLET_DESTROY_DEFAULT_TIMEOUT);
        break;

    case 5:
        midp_midlet_set_foreground(param);
        break;

    case 6: {

        switch (param) {
#if ENABLE_I3_TEST
        case 0:
            pClassName = namsManagerClassName;
            classNameLen = sizeof (namsManagerClassName) / sizeof (jchar);
            break;

        case 1:
            pClassName = i3frameworkClassName;
            classNameLen = sizeof (i3frameworkClassName) / sizeof (jchar);
            break;
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

        midp_midlet_create_start(INTERNAL_SUITE_ID,
                                 pClassName, classNameLen,
#if ENABLE_I3_TEST
                                 ((param != 0)
                                     ? findNextEmptyMIDlet(0) : 0)
#else
                                 -param
#endif
                                 , NULL
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
 * @param pEventData
 */
void system_state_listener(const NamsEventData* pEventData) {
    printf("--- system_state_listener(event = %d, state = %d)\n",
        pEventData->event, pEventData->state);

    if (pEventData->event == MIDP_NAMS_EVENT_STATE_CHANGED &&
            pEventData->state == MIDP_SYSTEM_STATE_ACTIVE) {
        int i;
        const jchar *jchArgsForMidlet[3];
        jint  argsLen[3];

        /* Currently we support up to 3 arguments. */
        for (i = 0; i < 3; i++) {
            jchArgsForMidlet[i] = pcsl_string_get_utf16_data(&argsForMidlet[i]);
            argsLen[i] = pcsl_string_utf16_length(&argsForMidlet[i]);
        }

        GET_PCSL_STRING_DATA_AND_LENGTH(aclassNameToRun)
        (void) midp_midlet_create_start_with_args(suiteIDToRun,
                                           (const jchar*)aclassNameToRun_data,
                                           aclassNameToRun_len,
                                           (const jchar**)jchArgsForMidlet,
                                           argsLen,
                                           3,
#if ENABLE_I3_TEST
                                           findNextEmptyMIDlet(0),
#else
                                           /*
                                            * There is only one application
                                            * excepting namsTestService mode
                                            */
                                           1,
#endif
                                           NULL);
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
 * @param pEventData
 */
void background_listener(const NamsEventData* pEventData) {
    int i = 0;

    if (pEventData == NULL) {
        printf("--- background_listener(): NULL event data!\n");
        return;
    }

    if (pEventData->state != MIDP_DISPLAY_STATE_BACKGROUND &&
        pEventData->state != MIDP_DISPLAY_STATE_BACKGROUND_REQUEST) {
        /* probably foreground request - don't handle */
        return;
    }

    printf("--- background_listener(appId = %d, reason = %d)\n",
           pEventData->appId, pEventData->reason);

    for (i = 0; i < numberOfSuiteIds; i++) {
        if (pSuiteRunState[i] == MIDP_MIDLET_STATE_ACTIVE &&
            i+1 != foregroundAppId) {

            printf("midp_midlet_set_foreground(suiteId = %d)\n", i+1);
            midp_midlet_set_foreground(i+1);
            break;
        }
    }
}

/**
 * The typedef of the foreground listener that is notified
 * when the foreground midlet changes.
 *
 * @param pEventData
 */
void foreground_listener(const NamsEventData* pEventData) {
    if (pEventData == NULL) {
        printf("--- foreground_listener(): NULL event data!\n");
        return;
    }

    if (pEventData->state != MIDP_DISPLAY_STATE_FOREGROUND &&
        pEventData->state != MIDP_DISPLAY_STATE_FOREGROUND_REQUEST) {
        /* probably background request - don't handle */
        return;
    }

    printf("--- foreground_listener(appId = %d, reason = %d)\n",
           pEventData->appId, pEventData->reason);

    foregroundAppId = pEventData->appId;
    if (pEventData->appId > 0 && pEventData->appId <= numberOfSuiteIds) {
        printf("[%d] ", pEventData->appId);
        printf("Suite ID = %ld", (long)pSuiteIds[pEventData->appId-1]);
        printf(" has the foreground   reason = %d\n", pEventData->reason);
    }
}

/**
 * The typedef of the midlet state listener that is notified
 * with the midlet state changes.
 *
 * @param pEventData
 */
void state_change_listener(const NamsEventData* pEventData) {
    if (!pEventData || !pEventData->pSuiteData) {
        printf("--- state_change_listener(): invalid pEventData!\n");
        return;
    }

    printf("--- state_change_listener(appId = %d, state = %d, reason = %d)\n",
           pEventData->appId, pEventData->state, pEventData->reason);

    if (pEventData->event != MIDP_NAMS_EVENT_STATE_CHANGED) {
        printf("Dropping event: %d\n", pEventData->event);
        return;
    }

    if (pEventData->appId > 0) {
        SuiteIdType suiteId = pEventData->pSuiteData->suiteId;

        printf("[%d] ", pEventData->appId);
        printf("Suite ID = %ld", (long)suiteId);

        if (pEventData->appId <= numberOfSuiteIds) {
            pSuiteRunState[pEventData->appId-1] = pEventData->state;
        }

        printf(" changed state - (%d) \"", pEventData->state);

        switch (pEventData->state) {
            case MIDP_MIDLET_STATE_ACTIVE: printf("ACTIVE\""); break;
            case MIDP_MIDLET_STATE_PAUSED: printf("PAUSED\""); break;
            case MIDP_MIDLET_STATE_DESTROYED:
                printf("DESTROYED\"");
                if (pEventData->reason == MIDP_REASON_TERMINATED) {
                    printf("Suite has terminated. ID = %ld\n", (long)suiteId);
                }
                break;
            case MIDP_MIDLET_STATE_ERROR: printf("ERROR\""); break;
            default: printf("INVALID!!!\""); break;
        }

        printf("  reason = %d\n", pEventData->reason);
    }
}

#if ENABLE_I3_TEST
static void initNamsCommands(int argn, char* args[]) {
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
 * @return error code (<tt>ALL_OK</tt> if successful)
 */
static MIDPError
setupArgToStartMidlet(int argc, char* argv[]) {
    MIDPError status = BAD_PARAMS;
    SuiteIdType tmpSuiteId = UNUSED_SUITE_ID;

    if (argc == 0) {
        return status;
    }

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
            /* run the midlet suite by its ID */
            int i;

            /* the format of the string is "number:" */
            if (sscanf(argv[0], "%d", &tmpSuiteId) != 1) {
                REPORT_ERROR(LC_AMS, "Invalid suite ID format");
                fprintf(stderr, "Invalid suite ID format\n");
                break;
            }

            for (i = 0; i < numberOfSuiteIds; i++) {
                if (tmpSuiteId == pSuiteIds[i]) {
                    break;
                }
            }

            if (i == numberOfSuiteIds) {
                REPORT_ERROR(LC_AMS, "Suite with the given ID was not found");
                fprintf(stderr, "Suite with the given ID was not found\n");
                break;
            }

            suiteIDToRun = tmpSuiteId;
        } else {
            /* Run by ID */
            suiteIDToRun = INTERNAL_SUITE_ID;
        }

        /* Setting up a class name of the midlet to be run */
        if (argc > 1) {
            if (PCSL_STRING_OK !=
                pcsl_string_from_chars(argv[1], &classNameToRun)) {
                status = OUT_OF_MEMORY;
                break;
            }
        }

        /* Setting up arguments for the midlet */
        for (i = 0; i < 3; i ++) {
            if (argc > i + 2) {
                if (PCSL_STRING_OK !=
                    pcsl_string_from_chars(argv[i + 2], &argsForMidlet[i])) {
                    status = OUT_OF_MEMORY;
                    break;
                }
            } else {
                argsForMidlet[i] = PCSL_STRING_NULL;
            }
        }

        if (pcsl_string_is_null(&classNameToRun)) {
            int res = find_midlet_class(suiteIDToRun, 1, &classNameToRun);
            if (OUT_OF_MEM_LEN == res) {
                status = OUT_OF_MEMORY;
                break;
            }

            if (NULL_LEN == res) {
                REPORT_ERROR(LC_AMS, "Could not find the first MIDlet");
                fprintf(stderr, "Could not find the first MIDlet\n");
                break;
            }
        }

        status = ALL_OK;
    } while (0);

    if (status == OUT_OF_MEMORY) {
        REPORT_ERROR(LC_AMS, "Out of Memory");
        fprintf(stderr, "Out Of Memory\n");
    }

    return status;
}

static MIDPError runMidletWithNAMS(int argc, char* argv[]);

/**
 * Mode 1. NAMS test service:<br>
 * runNams [&lt;VM args&gt;] -namsTestService<br>
 * Does not return until the system is stopped.
 *
 * @param argc The total number of arguments
 * @param argv An array of 'C' strings containing the arguments
 *
 * @return error code (<tt>ALL_OK</tt> if successful)
 */
static MIDPError runNamsTestService(int argc, char* argv[]) {
    int numOfArgsToRunTestService = 2;
    char* ppArgsToRunTestService[] = {
        "-1", /* internal suite id */
        NAMS_TEST_SERVICE_CLASS_NAME
    };

#if ENABLE_I3_TEST
    initNams();
    initNamsCommands(argc - 1, argv + 1);
#else
    (void)argc;
    (void)argv;
#endif

    return runMidletWithNAMS(numOfArgsToRunTestService,
                             ppArgsToRunTestService);
}

/**
 * Mode 2. Run a MIDlet using the NAMS API:<br>
 * runNams [&lt;VM args&gt;] -runMidlet &lt;suiteId or suite number&gt;<br>
 *   &lt;MIDlet classname&gt; [[[&lt;arg1&gt;] &lt;arg2&gt;] &lt;arg3&gt;]<br>
 *
 * @param argc The total number of arguments
 * @param argv An array of 'C' strings containing the arguments
 *
 * @return error code (<tt>ALL_OK</tt> if successful)
 */
static MIDPError runMidletWithNAMS(int argc, char* argv[]) {
    MIDPError status;

    status = setupArgToStartMidlet(argc, argv);
    if (status != ALL_OK) {
        return status;
    }

    /* set the listeners before starting the system */
    midp_add_event_listener(system_state_listener, SYSTEM_EVENT_LISTENER);
    midp_add_event_listener(background_listener, DISPLAY_EVENT_LISTENER);
    midp_add_event_listener(foreground_listener, DISPLAY_EVENT_LISTENER);
    midp_add_event_listener(state_change_listener, MIDLET_EVENT_LISTENER);

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
 * @return <tt>ALL_OK</tt> if successful,
 *         <tt>BAD_PARAMS</tt> if some parameter is invalid,
 *         <tt>NOT_FOUND</tt> if the MIDlet suite not found,
 *         <tt>GENERAL_ERROR</tt> if another error
 */
static MIDPError runMidletWithJAMS(int argc, char* argv[]) {
    int retCode = MIDP_ERROR_STATUS;
    MIDPError status;

    if (argc < 1) {
        return BAD_PARAMS;
    }

    status = setupArgToStartMidlet(argc, argv);
    if (status != ALL_OK) {
        return status;
    }

    retCode = midp_run_midlet_with_args_cp(suiteIDToRun, &classNameToRun,
        &argsForMidlet[0], &argsForMidlet[1], &argsForMidlet[2], 0, NULL);

    /*
     * IMPL_NOTE: this code can be removed after migration of the status codes
     * returned from midp_run_midlet_with_args_cp() to MIDPError.
     */
    if (retCode == 0 || retCode == MIDP_SHUTDOWN_STATUS) {
        status = ALL_OK;
    } else if (status == SUITE_NOT_FOUND_STATUS) {
        status = NOT_FOUND;
    } else if (status != ALL_OK) {
        status = GENERAL_ERROR;
    }

    return status;
}

/**
 * Mode 4. Run an alternate main class:<br>
 * runNams [&lt;VM args&gt;] -runMainClass mainClass [&lt;args&gt;]
 *
 * @param argc The total number of arguments
 * @param argv An array of 'C' strings containing the arguments
 *
 * @return <tt>ALL_OK</tt> if successful,
 *         <tt>BAD_PARAMS</tt> if some parameter is invalid,
 *         <tt>GENERAL_ERROR</tt> if another error
 */
static MIDPError runMainClass(int argc, char* argv[]) {
    char* mainClass; /* the class name of midlet to run */
    int retCode = MIDP_ERROR_STATUS;
    MIDPError status = ALL_OK;

    if (argc < 1) {
        return BAD_PARAMS;
    }

    if (midpInitialize() != 0) {
        return OUT_OF_MEMORY;
    }

    mainClass = argv[0];
    retCode = midpRunMainClass(NULL, mainClass, argc, argv);
    midpFinalize();

    if (retCode == 0 || retCode == MIDP_SHUTDOWN_STATUS) {
        status = ALL_OK;
    } else if (retCode != ALL_OK) {
        status = GENERAL_ERROR;
    }

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
int runNams(int argc, char* argv[]) {
    MIDPError status;
    int used;
    int savedArgc;
    char **savedArgv;
    int restart = 0;

    savedArgc = argc;
    savedArgv = argv;

    /* initialize the system */
    status = midp_system_initialize();
    if (status != ALL_OK) {
        fprintf(stderr, "midp_system_initialize() failed (%d)\n", status);
        return status;
    }

    do {
        argc = savedArgc;
        argv = savedArgv;

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
            MIDPError (*handlers[])(int argc, char* argv[]) = {
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
                status = loadSuiteIds();
                if (status != ALL_OK) {
                    fprintf(stderr, "Failed to load suite IDs (%d)\n", status);
                    break;
                }
                status = handlers[i](--argc, ++argv);
            }
        }

        /* clean up */
        unloadSuiteIds();

        if (status != ALL_OK) {
            fprintf(stderr, "VM startup failed (%d)\n", status);
            break;
        }
    } while (restart);

    /* it is safe to call it more than once */
    unloadSuiteIds();

    return status;
}

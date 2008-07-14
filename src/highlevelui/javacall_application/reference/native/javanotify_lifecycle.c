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
 * @file
 *
 * Implementation of javacall lifecycle notification functions.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <midpServices.h>
#include <midp_logging.h>
#include <localeMethod.h>
#include <midp_jc_event_defs.h>

#include <javacall_events.h>
#include <javacall_lifecycle.h>
#include <javacall_time.h>
#include <javautil_unicode.h>

static char urlAddress[BINARY_BUFFER_MAX_LEN];

/**
 * A helper function to
 * @param event a pointer to midp_javacall_event_union
 * @return javacall_event_send() operation result
 */
static javacall_result
midp_jc_event_send(midp_jc_event_union *event) {
    return javacall_event_send((unsigned char *)event,
                               sizeof(midp_jc_event_union));
}

/**
 * The platform should invoke this function in platform context to start
 * Java.
 */
void javanotify_start(void) {
    midp_jc_event_union e;
    midp_jc_event_start_arbitrary_arg *data = &e.data.startMidletArbitraryArgEvent;

    REPORT_INFO(LC_CORE, "javanotify_start() >>\n");

    e.eventType = MIDP_JC_EVENT_START_ARBITRARY_ARG;

    data->argc = 0;
    data->argv[data->argc++] = "runMidlet";
    data->argv[data->argc++] = "-1";
    data->argv[data->argc++] =
#if ENABLE_MULTIPLE_ISOLATES
    "com.sun.midp.appmanager.MVMManager";
#else
    "com.sun.midp.appmanager.Manager";
#endif

    midp_jc_event_send(&e);
}

/**
 * The platform should invoke this function in platform context to start
 * a specified MIDlet suite.
 *
 * @param suiteId the ID of the suite to start
 */
void javanotify_start_suite(char* suiteId) {
    int length;
    midp_jc_event_union e;
    midp_jc_event_start_arbitrary_arg *data = &e.data.startMidletArbitraryArgEvent;

    REPORT_INFO(LC_CORE, "javanotify_start_suite() >>\n");

    e.eventType = MIDP_JC_EVENT_START_ARBITRARY_ARG;

    data->argc = 0;
    data->argv[data->argc++] = "runMidlet";
    data->argv[data->argc++] = "-1";
    data->argv[data->argc++] = "com.sun.midp.appmanager.MIDletSuiteLauncher";

    length = strlen(suiteId);
    if (length >= BINARY_BUFFER_MAX_LEN) {
        return;
    }

    memset(urlAddress, 0, BINARY_BUFFER_MAX_LEN);
    memcpy(urlAddress, suiteId, length);
    data->argv[data->argc++] = urlAddress;

    midp_jc_event_send(&e);
}

/**
 * The platform should invoke this function in platform context to start
 * Java in local context (not OTA).
 */
void javanotify_start_local(char* classname, char* descriptor,
                            char* classpath, javacall_bool debug) {
    midp_jc_event_union e;
    midp_jc_event_start_arbitrary_arg *data = &e.data.startMidletArbitraryArgEvent;

    REPORT_INFO2(LC_CORE,"javanotify_start_local() >> classname=%s, descriptor=%d \n",
                 classname, descriptor);

    e.eventType = MIDP_JC_EVENT_START_ARBITRARY_ARG;

    data->argc = 0;
    data->argv[data->argc++] = "runMidlet";
    data->argv[data->argc++] = "-1";

    if (classname == NULL) {
        data->argv[data->argc++] = "internal";
    } else {
        data->argv[data->argc++] = classname;
    }

    if (descriptor != NULL) {
        data->argv[data->argc++] = descriptor;
    } else {
        data->argv[data->argc++] = classpath;
    }

    if (classpath != NULL) {
        data->argv[data->argc++] = "-classpathext";
        data->argv[data->argc++] = classpath;
    }

    midp_jc_event_send(&e);
}

/**
 * The platform should invoke this function in platform context to start
 * the Java VM and run TCK.
 *
 * @param url the http location of the TCK server
 *            the url should be of the form: "http://host:port"
 * @param domain the TCK execution domain
 */
void javanotify_start_tck(char *tckUrl, javacall_lifecycle_tck_domain domain_type) {
    int length;
    midp_jc_event_union e;
    midp_jc_event_start_arbitrary_arg *data = &e.data.startMidletArbitraryArgEvent;

    REPORT_INFO2(LC_CORE,"javanotify_start_tck() >> tckUrl=%s, domain_type=%d \n",tckUrl,domain_type);

    e.eventType = MIDP_JC_EVENT_START_ARBITRARY_ARG;

    data->argc = 0;
    data->argv[data->argc++] = "runMidlet";
    data->argv[data->argc++] = "-1";
    data->argv[data->argc++] = "com.sun.midp.installer.AutoTester";

    length = strlen(tckUrl);
    if (length >= BINARY_BUFFER_MAX_LEN)
        return;

    memset(urlAddress, 0, BINARY_BUFFER_MAX_LEN);
    memcpy(urlAddress, tckUrl, length);
    if (strcmp(urlAddress, "none") != 0) {
        data->argv[data->argc++] = urlAddress;
    }


    if (domain_type == JAVACALL_LIFECYCLE_TCK_DOMAIN_UNTRUSTED) {
        data->argv[data->argc++] = "untrusted";
    } else if (domain_type == JAVACALL_LIFECYCLE_TCK_DOMAIN_TRUSTED) {
        data->argv[data->argc++] = "trusted";
    } else if (domain_type == JAVACALL_LIFECYCLE_TCK_DOMAIN_UNTRUSTED_MIN) {
        data->argv[data->argc++] = "minimum";
    } else if (domain_type == JAVACALL_LIFECYCLE_TCK_DOMAIN_UNTRUSTED_MAX) {
        data->argv[data->argc++] = "maximum";
    } else {
        return;
    }

    midp_jc_event_send(&e);
}

/**
 * The platform should invoke this function in platform context to start
 * the Java VM and run i3test framework.
 *
 * @param arg1 optional argument 1
 * @param arg2 optional argument 2
 *
 * @note allowed argument description can be obtained by '-help' value as arg1.
 */
void javanotify_start_i3test(char* arg1, char* arg2) {
    midp_jc_event_union e;
    midp_jc_event_start_arbitrary_arg *data = &e.data.startMidletArbitraryArgEvent;

    REPORT_INFO2(LC_CORE,"javanotify_start_i3test() >> %s %s\n",arg1,arg2);

    e.eventType = MIDP_JC_EVENT_START_ARBITRARY_ARG;

    data->argc = 0;
    data->argv[data->argc++] = "runMidlet";
    data->argv[data->argc++] = "-1";
    data->argv[data->argc++] = "com.sun.midp.i3test.Framework";
    if (NULL != arg1) {
        data->argv[data->argc++] = arg1;
        if (NULL != arg2)
            data->argv[data->argc++] = arg2;
    }

    midp_jc_event_send(&e);
}

/**
 * The platform should invoke this function in platform context to start
 * the Java VM and run installed Java Content Handler.
 *
 * @param handlerID launched Content Handler ID
 * @param url Invocation parameter: URL
 * @param action optional Invocation parameter: Action
 *
 * @note allowed argument description can be obtained by '-help' value as arg1.
 */
void javanotify_start_handler(char* handlerID, char* url, char* action) {
    midp_jc_event_union e;
    midp_jc_event_start_arbitrary_arg *data = &e.data.startMidletArbitraryArgEvent;

    REPORT_INFO3(LC_CORE,"javanotify_start_handler() >> %s %s %s\n",
                 handlerID, url, action);

    e.eventType = MIDP_JC_EVENT_START_ARBITRARY_ARG;

    data->argc = 0;
    data->argv[data->argc++] = "runMidlet";
    data->argv[data->argc++] = "-1";
    data->argv[data->argc++] = "com.sun.midp.content.Invoker";
    if (NULL != handlerID) {
        data->argv[data->argc++] = handlerID;
        if (NULL != url) {
            data->argv[data->argc++] = url;
            if (NULL != action)
                data->argv[data->argc++] = action;
        }
    }

    midp_jc_event_send(&e);
}

/**
 * A notification function for telling Java to perform installation of
 * a MIDlet
 *
 * If the given url is of the form http://www.sun.com/a/b/c/d.jad then
 * java will start a graphical installer will download the MIDlet
 * fom the internet.
 * If the given url is a file url (see below, file:///a/b/c/d.jar or
 * file:///a/b/c/d/jad) installation will be performed
 * in the backgroudn without launching the graphic installer application
 *
 *
 * @param url of MIDlet to install, can be either one of the following
 *   1. A full path to the jar file, of the following form file:///a/b/c/d.jar
 *   2. A full path to the JAD file, of the following form file:///a/b/c/d.jad
 *   3. An http url of jad file, of the following form,
 *      http://www.sun.com/a/b/c/d.jad
 */
void javanotify_install_midlet(const char *httpUrl) {
    int length;
    midp_jc_event_union e;
    midp_jc_event_start_arbitrary_arg *data = &e.data.startMidletArbitraryArgEvent;

    REPORT_INFO1(LC_CORE,"javanotify_install_midlet() >> httpUrl = %s\n", httpUrl);

    e.eventType = MIDP_JC_EVENT_START_ARBITRARY_ARG;

    data->argc = 0;
    data->argv[data->argc++] = "runMidlet";
    data->argv[data->argc++] = "-1";
    data->argv[data->argc++] = "com.sun.midp.installer.GraphicalInstaller";
    data->argv[data->argc++] = "I";

    length = strlen(httpUrl);
    if (length >= BINARY_BUFFER_MAX_LEN)
        return;

    memset(urlAddress, 0, BINARY_BUFFER_MAX_LEN);
    memcpy(urlAddress, httpUrl, length);
    data->argv[data->argc++] = urlAddress;

    midp_jc_event_send(&e);
}

/**
 * A notification function for telling Java to perform installation of
 * a MIDlet from filesystem,
 *
 * The installation will be performed in the background without launching
 * the graphic installer application.
 *
 * The given path is the full path to MIDlet's jad file or jad.
 * In case the MIDlet's jad file is specified, then
 * the MIDlet's jar file muts reside in the same directory as the jad
 * file.
 *
 * @param jadFilePath full path the jad (or jar) file which is of the form:
 *        file://a/b/c/d.jad
 * @param jadFilePathLen length of the file path
 * @param userWasAsked a flag indicating whether the platform already asked
 *        the user for permission to download and install the application
 *        so there's no need to ask again and we can immediately install.
 */
void javanotify_install_midlet_from_filesystem(const javacall_utf16* jadFilePath,
                                               int jadFilePathLen, int userWasAsked) {
    midp_jc_event_union e;
    midp_jc_event_start_arbitrary_arg *data = &e.data.startMidletArbitraryArgEvent;

    REPORT_INFO(LC_CORE, "javanotify_install_midlet_from_filesystem() >>\n");

    e.eventType = MIDP_JC_EVENT_START_ARBITRARY_ARG;

    data->argc = 0;
    data->argv[data->argc++] = "runMidlet";
    data->argv[data->argc++] = "-1";
    data->argv[data->argc++] = "com.sun.midp.scriptutil.CommandLineInstaller";
    data->argv[data->argc++] = "I";

    if (jadFilePathLen >= BINARY_BUFFER_MAX_LEN)
        return;

    memset(urlAddress, 0, BINARY_BUFFER_MAX_LEN);
    unicodeToNative(jadFilePath, jadFilePathLen,
                    (unsigned char *)urlAddress, BINARY_BUFFER_MAX_LEN);
    data->argv[data->argc++] = urlAddress;

    midp_jc_event_send(&e);
}

/**
 * A notification function for telling Java to perform installation of
 * a MIDlet with parameters
 *
 * If the given url is of the form http://www.sun.com/a/b/c/d.jad then
 * java will start a graphical installer will download the MIDlet
 * fom the internet.
 * If the given url is a file url (see below, file:///a/b/c/d.jar or
 * file:///a/b/c/d/jad) installation will be performed
 * in the backgroudn without launching the graphic installer application
 *
 *
 * @param url of MIDlet to install, can be either one of the following
 *   1. A full path to the jar file, of the following form file:///a/b/c/d.jar
 *   2. A full path to the JAD file, of the following form file:///a/b/c/d.jad
 *   3. An http url of jad file, of the following form,
 *      http://www.sun.com/a/b/c/d.jad
 * @param silentInstall install the MIDlet without user interaction
 * @param forceUpdate install the MIDlet even if it already exist regardless
 *                    of version
 */
void javanotify_install_midlet_wparams(const char* httpUrl,
                                       int silentInstall, int forceUpdate) {
    int length;
    midp_jc_event_union e;
    midp_jc_event_start_arbitrary_arg *data = &e.data.startMidletArbitraryArgEvent;

    REPORT_INFO2(LC_CORE,"javanotify_install_midlet_wparams() >> "
                         "httpUrl = %s, silentInstall = %d\n",
                 httpUrl, silentInstall);

    e.eventType = MIDP_JC_EVENT_START_ARBITRARY_ARG;

    data->argc = 0;
    data->argv[data->argc++] = "runMidlet";
    data->argv[data->argc++] = "-1";
    data->argv[data->argc++] = "com.sun.midp.installer.GraphicalInstaller";

    if (silentInstall == 1) {
        if (forceUpdate == 1) {
            data->argv[data->argc++] = "FU";
        } else {
            data->argv[data->argc++] = "FI";
        }
    } else {
        data->argv[data->argc++] = "I";
    }

    length = strlen(httpUrl);
    if (length >= BINARY_BUFFER_MAX_LEN) {
        return;
    }

    memset(urlAddress, 0, BINARY_BUFFER_MAX_LEN);
    memcpy(urlAddress, httpUrl, length);
    data->argv[data->argc++] = urlAddress;

    midp_jc_event_send(&e);
}

/**
 * The platform should invoke this function in platform context to start
 * the Java VM with arbitrary arguments.
 *
 * @param argc number of command-line arguments
 * @param argv array of command-line arguments
 *
 * @note This is a service function and it is introduced in the javacall
 *       interface for debug purposes. Please DO NOT CALL this function without
 *       being EXPLICITLY INSTRUCTED to do so.
 */
void javanotify_start_java_with_arbitrary_args(int argc, char* argv[]) {
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_start_java_with_arbitrary_args() >>\n");

    if (argc > MIDP_RUNMIDLET_MAXIMUM_ARGS)
        argc = MIDP_RUNMIDLET_MAXIMUM_ARGS;
    e.eventType = MIDP_JC_EVENT_START_ARBITRARY_ARG;
    e.data.startMidletArbitraryArgEvent.argc = argc;
    memcpy(e.data.startMidletArbitraryArgEvent.argv, argv, argc * sizeof(char*));

    midp_jc_event_send(&e);
}

/**
 * Parse options for the VM
 *
 * @param argc number of command-line arguments
 * @param argv array of command-line arguments
 */
void javanotify_set_vm_args(int argc, char* argv[]) {
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_set_vm_args() >>\n");

    if (argc > MIDP_RUNMIDLET_MAXIMUM_ARGS) {
        argc = MIDP_RUNMIDLET_MAXIMUM_ARGS;
    }

    e.eventType = MIDP_JC_EVENT_SET_VM_ARGS;
    e.data.startMidletArbitraryArgEvent.argc = argc;
    memcpy(e.data.startMidletArbitraryArgEvent.argv, argv, argc * sizeof(char*));

    midp_jc_event_send(&e);
}

/**
 * Set VM heapsize
 * @param heapsize the heap size in bytes
 */
void javanotify_set_heap_size(int heapsize) {
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_set_heap_size() >>\n");

    e.eventType = MIDP_JC_EVENT_SET_HEAP_SIZE;
    e.data.heap_size.heap_size = heapsize;
    midp_jc_event_send(&e);
}

/**
 * list the MIDlet suites installed
 */
void javanotify_list_midlets(void) {
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_list_midlets() >>\n");

    e.eventType = MIDP_JC_EVENT_LIST_MIDLETS;
    midp_jc_event_send(&e);
}

/**
 * list the MIDlet suites installed
 * Each line contains one storage name
 */
void javanotify_list_storageNames(void) {
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_list_storageName() >>\n");

    e.eventType = MIDP_JC_EVENT_LIST_STORAGE_NAMES;
    midp_jc_event_send(&e);
}

/**
 * Remove a MIDlet suite according to the given suite ID
 */
void javanotify_remove_suite(char* suite_id) {
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_remove_suite() >>\n");

    e.eventType = MIDP_JC_EVENT_REMOVE_MIDLET;
    e.data.removeMidletEvent.suiteID = suite_id;
    midp_jc_event_send(&e);
}

/**
 * Install, run, and remove the application with the specified JAD file
 */
void javanotify_transient(char* url) {
    int length;
    midp_jc_event_union e;
    midp_jc_event_start_arbitrary_arg *data = &e.data.startMidletArbitraryArgEvent;

    REPORT_INFO(LC_CORE,"javanotify_transient() >>\n");

    e.eventType = MIDP_JC_EVENT_START_ARBITRARY_ARG;

    data->argc = 0;
    data->argv[data->argc++] = "runMidlet";
    data->argv[data->argc++] = "-1";
    data->argv[data->argc++] = "com.sun.midp.installer.AutoTester";

    length = strlen(url);
    if (length >= BINARY_BUFFER_MAX_LEN)
        return;

    memset(urlAddress, 0, BINARY_BUFFER_MAX_LEN);
    memcpy(urlAddress, url, length);
    if (strcmp(urlAddress, "none") != 0) {
        data->argv[data->argc++] = urlAddress;
    }

    data->argv[data->argc++] = "1"; /* loop count */

    midp_jc_event_send(&e);
}

/**
 * The platform should invoke this function in platform context to end Java.
 */
void javanotify_shutdown(void) {
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_shutdown() >>\n");

    e.eventType = MIDP_JC_EVENT_END;

    midp_jc_event_send(&e);
}

/**
 * The platform should invoke this function in platform context to pause
 * Java.
 */
void javanotify_pause(void) {
    midp_jc_event_union e; 

    REPORT_INFO(LC_CORE, "javanotify_pause() >>\n");

    e.eventType = MIDP_JC_EVENT_PAUSE;

    midp_jc_event_send(&e);
}

/**
 * The platform should invoke this function in platform context to end pause
 * and resume Java.
 */
void javanotify_resume(void) {
    midp_jc_event_union e; 

    REPORT_INFO(LC_CORE, "javanotify_resume() >>\n");

    e.eventType = MIDP_JC_EVENT_RESUME;

    midp_jc_event_send(&e);
}

/**
 * The platform should invoke this function in platform context
 * to select another running application to be the foreground.
 */
void javanotify_select_foreground_app(void) {
#if ENABLE_MULTIPLE_ISOLATES
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_switchforeground() >>\n");

    e.eventType = MIDP_JC_EVENT_SWITCH_FOREGROUND;

    midp_jc_event_send(&e);
#endif /* ENABLE_MULTIPLE_ISOLATES */
}

/**
 * The platform should invoke this function in platform context
 * to bring the Application Manager Screen to foreground.
 */
void javanotify_switch_to_ams(void) {
#if ENABLE_MULTIPLE_ISOLATES
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_selectapp() >>\n");

    e.eventType = MIDP_JC_EVENT_SELECT_APP;

    midp_jc_event_send(&e);
#endif /* ENABLE_MULTIPLE_ISOLATES */
}

/**
 * The platform should invoke this function in platform context to pause
 * Java bytecode execution (without invoking pauseApp)
 */
void javanotify_internal_pause(void) {
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_internal_pause() >>\n");

    /* IMPL_NOTE: currently this event is not handled anywhere */
    e.eventType = MIDP_JC_EVENT_INTERNAL_PAUSE;

    midp_jc_event_send(&e);
}

/**
 * The platform should invoke this function in platform context to end
 * an internal pause and resume Java bytecode processing
 */
void javanotify_internal_resume(void) {
    midp_jc_event_union e;

    REPORT_INFO(LC_CORE, "javanotify_internal_resume() >>\n");

    /* IMPL_NOTE: currently this event is not handled anywhere */
    e.eventType = MIDP_JC_EVENT_INTERNAL_RESUME;

    midp_jc_event_send(&e);
}

#ifdef __cplusplus
}
#endif

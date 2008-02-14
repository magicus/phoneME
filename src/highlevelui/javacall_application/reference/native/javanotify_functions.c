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
 * Implementation of javacall notification functions.
 */

#include <string.h>
#include <midpServices.h>
#include <midp_logging.h>
#include <localeMethod.h>
#include <midp_jc_event_defs.h>

#include <javacall_datagram.h>
#include <javacall_events.h>
#include <javacall_input.h>
#include <javacall_keypress.h>
#include <javacall_lifecycle.h>
#include <javacall_network.h>
#include <javacall_penevent.h>
#include <javacall_security.h>
#include <javacall_socket.h>
#include <javacall_time.h>
#include <javautil_unicode.h>

#ifdef ENABLE_JSR_120
#include <javacall_sms.h>
#include <javacall_cbs.h>
#endif

#ifdef ENABLE_JSR_205
#include <javacall_mms.h>
#endif

#ifdef ENABLE_JSR_177
#include <javacall_carddevice.h>
#endif

#ifdef USE_VSCL
#include <javacall_vscl.h>
#endif

#ifdef ENABLE_JSR_179
#include <javacall_location.h>
#endif
#ifdef ENABLE_ON_DEVICE_DEBUG
#include <javacall_odd.h>
#endif /* ENABLE_ON_DEVICE_DEBUG */

static char urlAddress[BINARY_BUFFER_MAX_LEN];

#define MAX_PHONE_NUMBER_LENGTH 48
static char selectedNumber[MAX_PHONE_NUMBER_LENGTH];

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
 * The notification function to be called by platform for keypress
 * occurences.
 * The platform will invoke the call back in platform context for
 * each key press, key release and key repeat occurence
 * @param key the key that was pressed
 * @param type <tt>JAVACALL_KEYPRESSED</tt> when key was pressed
 *             <tt>JAVACALL_KEYRELEASED</tt> when key was released
 *             <tt>JAVACALL_KEYREPEATED</tt> when to be called repeatedly
 *             by platform during the duration that the key was held
 */
void javanotify_key_event(javacall_key key, javacall_keypress_type type) {
    midp_jc_event_union e;

    REPORT_INFO2(LC_CORE,"javanotify_key_event() >> key=%d , type=%d\n",key,type);

    e.eventType = MIDP_JC_EVENT_KEY;
    e.data.keyEvent.key = key;
    e.data.keyEvent.keyEventType = type;

    midp_jc_event_send(&e);
}

/**
* The notification function to be called by platform for pen 
* press/release/drag occurences.
* The platform will invoke the call back in platform context for
* each pen press, pen release and pen dragg occurence
* @param x the x positoin when the pen was pressed/released/dragged
* @param y the y positoin when the pen was pressed/released/dragged
* @param type <tt>JAVACALL_PENPRESSED</tt> when pen was pressed
*             <tt>JAVACALL_PENRELEASED</tt> when pen was released
*             <tt>JAVACALL_PENDRAGGED</tt> when pen was dragged
*/
void javanotify_pen_event(int x, int y, javacall_penevent_type type) {
    midp_jc_event_union e;

        REPORT_INFO3(LC_CORE,"javanotify_pen_event() >> x=%d, y=%d type=%d\n",x,y,type);

    e.eventType = MIDP_JC_EVENT_PEN;
    e.data.penEvent.type = type;
    e.data.penEvent.x = x;
    e.data.penEvent.y = y;

    midp_jc_event_send(&e);
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

    e.eventType = MIDP_JC_EVENT_INTERNAL_RESUME;

    midp_jc_event_send(&e);
}


/**
 * A callback function to be called for notification of network
 * conenction related events, such as network going down or up.
 * The platform will invoke the call back in platform context.
 * @param event the type of network-related event that occured
 *              JAVACALL_NETWORK_DOWN if the network became unavailable
 *              JAVACALL_NETWORK_UP if the network is now available
 *
 */
void javanotify_network_event(javacall_network_event netEvent) {
    midp_jc_event_union e;
    e.eventType = MIDP_JC_EVENT_NETWORK;
    if (netEvent == JAVACALL_NETWORK_UP) {
        e.data.networkEvent.netType = MIDP_NETWORK_UP;
    } else {
        e.data.networkEvent.netType = MIDP_NETWORK_DOWN;
    }

    midp_jc_event_send(&e);
}

#if ENABLE_JSR_120
    #include <jsr120_sms_pool.h>
    #include <jsr120_cbs_pool.h>
    #include <javacall_memory.h>
#endif
#if ENABLE_JSR_205
    #include <jsr205_mms_pool.h>
    #include <string.h>
#endif

#ifdef ENABLE_JSR_120

static SmsMessage* jsr120_sms_new_msg_javacall(jchar  encodingType,
                                        unsigned char  msgAddr[MAX_ADDR_LEN],
                                        jchar  sourcePortNum,
                                        jchar  destPortNum,
                                        jlong  timeStamp,
                                        jchar  msgLen,
                                        unsigned char* msgBuffer) {

    SmsMessage *sms = (SmsMessage*)javacall_malloc(sizeof(SmsMessage));
    memset(sms, 0, sizeof(SmsMessage));

    sms->msgAddr   = (char*)javacall_malloc(MAX_ADDR_LEN);
    sms->msgBuffer = (char*)javacall_malloc(msgLen);

    sms->encodingType  = encodingType;
    sms->sourcePortNum = sourcePortNum;
    sms->destPortNum   = destPortNum;
    sms->timeStamp     = timeStamp;
    sms->msgLen        = msgLen;

    memcpy(sms->msgAddr, msgAddr, MAX_ADDR_LEN);
    memcpy(sms->msgBuffer, msgBuffer, msgLen);

    return sms;
}

/**
 * callback that needs to be called by platform to handover an incoming SMS intended for Java 
 *
 * After this function is called, the SMS message should be removed from platform inbox
 * 
 * @param msgType JAVACALL_SMS_MSG_TYPE_ASCII, or JAVACALL_SMS_MSG_TYPE_BINARY or JAVACALL_SMS_MSG_TYPE_UNICODE_UCS2  1002
 * @param sourceAddress the source SMS address for the message.  The format of the address  parameter  
 *                is  expected to be compliant with MSIDN, for example,. +123456789 
 * @param msgBuffer payload of incoming sms 
 *        if msgType is JAVACALL_SMS_MSG_TYPE_ASCII then this is a 
 *        pointer to char* ASCII string. 
 *        if msgType is JAVACALL_SMS_MSG_TYPE_UNICODE_UCS2, then this
 *        is a pointer to javacall_utf16 UCS-2 string. 
 *        if msgType is JAVACALL_SMS_MSG_TYPE_BINARY, then this is a 
 *        pointer to binary octet buffer. 
 * @param msgBufferLen payload len of incoming sms 
 * @param sourcePortNum the port number that the message originated from
 * @param destPortNum the port number that the message was sent to
 * @param timeStamp SMS service center timestamp
 */
void javanotify_incoming_sms(javacall_sms_encoding msgType,
                        char *sourceAddress,
                        unsigned char *msgBuffer,
                        int msgBufferLen,
                        unsigned short sourcePortNum,
                        unsigned short destPortNum,
                        javacall_int64 timeStamp) {
    midp_jc_event_union e;
        SmsMessage* sms;

    e.eventType = MIDP_JC_EVENT_SMS_INCOMING;

    sms = jsr120_sms_new_msg_javacall(
             msgType, sourceAddress, sourcePortNum, destPortNum, timeStamp, msgBufferLen, msgBuffer);

    e.data.smsIncomingEvent.stub = (int)sms;

    midp_jc_event_send(&e);
    return;
}
#endif

#ifdef ENABLE_JSR_205

static char* javacall_copystring(char* src) {
    int length = strlen(src)+1;
    char* result = javacall_malloc(length);
    memcpy(result, src, length);
    return (char*)result;
}

static MmsMessage* jsr205_mms_new_msg_javacall(char* fromAddress, char* appID,
    char* replyToAppID, int msgLen, unsigned char* msgBuffer) {

    MmsMessage* message = (MmsMessage*)javacall_malloc(sizeof(MmsMessage));
    memset(message, 0, sizeof(MmsMessage));

    message->fromAddress  = javacall_copystring(fromAddress);
    message->appID        = javacall_copystring(appID);
    message->replyToAppID = javacall_copystring(replyToAppID);

    message->msgLen = msgLen;
    if (msgLen > 0) {
        message->msgBuffer = (char*)memcpy((void*)javacall_malloc(msgLen), msgBuffer, msgLen);
    }

    return message;
}

/*
 * See javacall_mms.h for description
 */
void javanotify_incoming_mms(
        char* fromAddress, char* appID, char* replyToAppID,
        int bodyLen, unsigned char* body) {
    midp_jc_event_union e;
    MmsMessage* mms;

    e.eventType = MIDP_JC_EVENT_MMS_INCOMING;

    mms = jsr205_mms_new_msg_javacall(fromAddress, appID, replyToAppID, bodyLen, body);

    e.data.mmsIncomingEvent.stub = (int)mms;

    midp_jc_event_send(&e);
    return;
}

void javanotify_incoming_mms_available(
        char* fromAddress, char* appID, char* replyToAppID, 
        javacall_handle handle) {

    midp_jc_event_union e;
    MmsMessage* mms;

    e.eventType = MIDP_JC_EVENT_MMS_INCOMING;

    //bodyLen=-1
    mms = jsr205_mms_new_msg_javacall(fromAddress, appID, replyToAppID, -1, (char*)handle);

    e.data.mmsIncomingEvent.stub = (int)mms;

    midp_jc_event_send(&e);
    return;
}

#endif

#ifdef ENABLE_JSR_120

static CbsMessage* jsr120_cbs_new_msg_javacall(jchar encodingType,
                               jchar msgID,
                               jchar msgLen,
                               unsigned char* msgBuffer) {

    CbsMessage* message = (CbsMessage*)javacall_malloc(sizeof(CbsMessage));
    memset(message, 0, sizeof(CbsMessage));

    message->encodingType = encodingType;
    message->msgID        = msgID;

    message->msgLen       = (msgLen > MAX_CBS_MESSAGE_SIZE) ? MAX_CBS_MESSAGE_SIZE : msgLen;
    message->msgBuffer    = (unsigned char*)javacall_malloc(msgLen);
    memcpy(message->msgBuffer, msgBuffer, msgLen);

    return message;
}

/**
 * callback that needs to be called by platform to handover an incoming CBS intended for Java 
 *
 * After this function is called, the CBS message should be removed from platform inbox
 * 
 * @param msgType JAVACALL_CBS_MSG_TYPE_ASCII, or JAVACALL_CBS_MSG_TYPE_BINARY or JAVACALL_CBS_MSG_TYPE_UNICODE_UCS2
 * @param msgID message ID
 * @param msgBuffer payload of incoming cbs 
 *        if msgType is JAVACALL_CBS_MSG_TYPE_ASCII then this is a 
 *        pointer to char* ASCII string. 
 *        if msgType is JAVACALL_CBS_MSG_TYPE_UNICODE_UCS2, then this
 *        is a pointer to javacall_utf16 UCS-2 string. 
 *        if msgType is JAVACALL_CBS_MSG_TYPE_BINARY, then this is a 
 *        pointer to binary octet buffer. 
 * @param msgBufferLen payload len of incoming cbs 
 */
void javanotify_incoming_cbs(
        javacall_cbs_encoding  msgType,
        unsigned short         msgID,
        unsigned char*         msgBuffer,
        int                    msgBufferLen) {
    midp_jc_event_union e;
        CbsMessage* cbs;

    e.eventType = MIDP_JC_EVENT_CBS_INCOMING;

    cbs = jsr120_cbs_new_msg_javacall(msgType, msgID, msgBufferLen, msgBuffer);

    e.data.cbsIncomingEvent.stub = (int)cbs;

    midp_jc_event_send(&e);
    return;    
}
#endif

#ifdef ENABLE_JSR_120
/**
 * A callback function to be called by platform to notify that an SMS 
 * has completed sending operation.
 * The platform will invoke the call back in platform context for
 * each sms sending completion. 
 *
 * @param result indication of send completed status result: Either
 *         <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> on failure
 * @param handle Handle value returned from javacall_sms_send
 */
void javanotify_sms_send_completed(javacall_result result,
                                   int handle) {
    midp_jc_event_union e;

    e.eventType = MIDP_JC_EVENT_SMS_SENDING_RESULT;
    e.data.smsSendingResultEvent.handle = (void *) handle;
    e.data.smsSendingResultEvent.result
        = JAVACALL_OK == result ? WMA_OK : WMA_ERR;

    midp_jc_event_send(&e);
    return;
}
#endif

#ifdef ENABLE_JSR_205
/**
 * A callback function to be called by platform to notify that an MMS 
 * has completed sending operation.
 * The platform will invoke the call back in platform context for
 * each mms sending completion. 
 *
 * @param result indication of send completed status result: Either
 *         <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> on failure
 * @param handle of available MMS
 */
void javanotify_mms_send_completed(javacall_result result,
                                   int handle) {
    midp_jc_event_union e;

    e.eventType = MIDP_JC_EVENT_MMS_SENDING_RESULT;
    e.data.mmsSendingResultEvent.handle = (void *) handle;
    e.data.mmsSendingResultEvent.result = 
        (JAVACALL_OK == result) ? WMA_OK : WMA_ERR;

    midp_jc_event_send(&e);
    return;
}
#endif

#ifdef ENABLE_JSR_177
/**
 * 
 */ 
void javanotify_carddevice_event(javacall_carddevice_event event,
                                 void *context) {
    midp_jc_event_union e;

    e.eventType = MIDP_JC_EVENT_CARDDEVICE;
    switch (event) {
    case JAVACALL_CARDDEVICE_RESET:
        e.data.carddeviceEvent.eventType = MIDP_CARDDEVICE_RESET;
        e.data.carddeviceEvent.handle = (int)context;
        break;
    case JAVACALL_CARDDEVICE_XFER:
        e.data.carddeviceEvent.eventType = MIDP_CARDDEVICE_XFER;
        e.data.carddeviceEvent.handle = (int)context;
        break;
    case JAVACALL_CARDDEVICE_UNLOCK:
        e.data.carddeviceEvent.eventType = MIDP_CARDDEVICE_UNLOCK;
        e.data.carddeviceEvent.handle = 0;
        break;
    default:
        /* TODO: report error */
        return;
    }
    javacall_event_send((unsigned char *) &e, sizeof(midp_jc_event_union));
    return;
}
#endif /* ENABLE_JSR_177 */

/**
 * A callback function to be called for notification of non-blocking 
 * client/server socket related events, such as a socket completing opening or , 
 * closing socket remotely, disconnected by peer or data arrived on 
 * the socket indication.
 * The platform will invoke the call back in platform context for
 * each socket related occurrence. 
 *
 * @param type type of indication: Either
 *          - JAVACALL_EVENT_SOCKET_OPEN_COMPLETED
 *          - JAVACALL_EVENT_SOCKET_CLOSE_COMPLETED
 *          - JAVACALL_EVENT_SOCKET_RECEIVE
 *          - JAVACALL_EVENT_SOCKET_SEND
 *          - JAVACALL_EVENT_SOCKET_REMOTE_DISCONNECTED
 *          - JAVACALL_EVENT_NETWORK_GETHOSTBYNAME_COMPLETED  
 * @param socket_handle handle of socket related to the notification
 * @param operation_result <tt>JAVACALL_OK</tt> if operation 
 *        completed successfully, 
 *        <tt>JAVACALL_FAIL</tt> or negative value on failure
 */
void javanotify_socket_event(javacall_socket_callback_type type,
                        javacall_handle socket_handle,
                        javacall_result operation_result) {

    midp_jc_event_union e;
    e.eventType = MIDP_JC_EVENT_SOCKET;
    e.data.socketEvent.handle = socket_handle;
    e.data.socketEvent.status = operation_result;

    e.data.socketEvent.extraData = NULL;

    switch (type) {
        case JAVACALL_EVENT_SOCKET_OPEN_COMPLETED:
        case JAVACALL_EVENT_SOCKET_SEND:
            e.data.socketEvent.waitingFor = NETWORK_WRITE_SIGNAL;
            break;

        case JAVACALL_EVENT_SOCKET_RECEIVE:
            e.data.socketEvent.waitingFor = NETWORK_READ_SIGNAL;
            break;

        case JAVACALL_EVENT_NETWORK_GETHOSTBYNAME_COMPLETED:
            e.data.socketEvent.waitingFor = HOST_NAME_LOOKUP_SIGNAL;
            break;

        case JAVACALL_EVENT_SOCKET_CLOSE_COMPLETED:
            e.data.socketEvent.waitingFor = NETWORK_EXCEPTION_SIGNAL;
            break;

        case JAVACALL_EVENT_SOCKET_REMOTE_DISCONNECTED:
            e.data.socketEvent.waitingFor = NETWORK_EXCEPTION_SIGNAL;
            break;                 

        default:
            /* IMPL_NOTE: decide what to do */
            return;                 /* do not send event to java */


            /* IMPL_NOTE: NETWORK_EXCEPTION_SIGNAL is not assigned by any indication */
    }

    midp_jc_event_send(&e);
}

/**
 * A callback function to be called for notification of non-blocking 
 * server socket only related events, such as a accept completion.
 * The platform will invoke the call back in platform context for
 * each socket related occurrence. 
 *
 * @param type type of indication: Either
 *          JAVACALL_EVENT_SERVER_SOCKET_ACCEPT_COMPLETED
 * @param socket_handle handle of socket related to the notification.
 *                          If the platform is not able to provide the socket 
 *                          handle in the callback, it should pass 0 as the new_socket_handle
 *                          and the implementation will call javacall_server_socket_accept_finish
 *                          to retrieve the handle for the accepted connection.
 * @param new_socket_handle in case of accept the socket handle for the 
 *                          newly created connection
 *               
 * @param operation_result <tt>JAVACALL_OK</tt> if operation 
 *        completed successfully, 
 *        <tt>JAVACALL_FAIL</tt> or negative value on failure
 */
void /* OPTIONAL */ javanotify_server_socket_event(javacall_server_socket_callback_type type,
                               javacall_handle socket_handle,
                               javacall_handle new_socket_handle,
                               javacall_result operation_result) {
    midp_jc_event_union e;
    e.eventType = MIDP_JC_EVENT_SOCKET;
    e.data.socketEvent.handle = socket_handle;
    e.data.socketEvent.status = operation_result;

    e.data.socketEvent.extraData = NULL;

    if (type == JAVACALL_EVENT_SERVER_SOCKET_ACCEPT_COMPLETED) {
        e.data.socketEvent.waitingFor = NETWORK_READ_SIGNAL;
        /* IMPL_NOTE: how about using  extraData instead of status? Then, malloc required. */


        /* If the platform is not able to provide the socket handle in the callback,
           it should pass 0. */
        if (operation_result == JAVACALL_OK) {
            (javacall_handle) e.data.socketEvent.status = new_socket_handle;
        } else {
            e.data.socketEvent.status = operation_result;
        }
    } else {
        /* IMPL_NOTE: decide what to do */
        return;                 /* do not send event to java */
    }

    midp_jc_event_send(&e);
}

/**
 * A callback function to be called for notification of non-blocking 
 * client/server socket related events, such as a socket completing opening or , 
 * closing socket remotely, disconnected by peer or data arrived on 
 * the socket indication.
 * The platform will invoke the call back in platform context for
 * each socket related occurrence. 
 *
 * @param type type of indication: Either
 *     - JAVACALL_EVENT_DATAGRAM_RECVFROM_COMPLETED
 *     - JAVACALL_EVENT_DATAGRAM_SENDTO_COMPLETED
 * @param handle handle of datagram related to the notification
 * @param operation_result <tt>JAVACALL_OK</tt> if operation 
 *        completed successfully, 
 *        <tt>JAVACALL_FAIL</tt> or negative value on failure
 */
void javanotify_datagram_event(javacall_datagram_callback_type type,
                               javacall_handle handle,
                               javacall_result operation_result) {

    midp_jc_event_union e;
    e.eventType = MIDP_JC_EVENT_SOCKET;
    e.data.socketEvent.handle = handle;
    e.data.socketEvent.status = operation_result;

    e.data.socketEvent.extraData = NULL;

    switch (type) {
        case JAVACALL_EVENT_DATAGRAM_SENDTO_COMPLETED:
            e.data.socketEvent.waitingFor = NETWORK_WRITE_SIGNAL;
            break;
        case JAVACALL_EVENT_DATAGRAM_RECVFROM_COMPLETED:
            e.data.socketEvent.waitingFor = NETWORK_READ_SIGNAL;
            break;
        default:
            /* IMPL_NOTE: decide what to do */
            return;                 /* do not send event to java */

            /* IMPL_NOTE: NETWORK_EXCEPTION_SIGNAL is not assigned by any indication */
    }

    midp_jc_event_send(&e);
}

#ifdef ENABLE_JSR_135
/**
 * Post native media event to Java event handler
 * 
 * @param type          Event type
 * @param appId         Application ID
 * @param playerId      Player ID
 * @param status        Event status
 * @param data          Data for this event type
 */
void javanotify_on_media_notification(javacall_media_notification_type type,
                                      int appId,
                                      int playerId, 
                                      javacall_result status,
                                      void *data) {
#if ENABLE_JSR_135
    midp_jc_event_union e;

    REPORT_INFO4(LC_MMAPI, "javanotify_on_media_notification type=%d appId=%d playerId%d status=%d\n", type, appId, playerId, status);

    e.eventType = MIDP_JC_EVENT_MULTIMEDIA;
    e.data.multimediaEvent.mediaType = type;
    e.data.multimediaEvent.appId = appId;
    e.data.multimediaEvent.playerId = playerId;
    e.data.multimediaEvent.status = (int) status;
    e.data.multimediaEvent.data = (int) data;

    midp_jc_event_send(&e);
#endif
}
#endif

#if ENABLE_JSR_234
/**
 * Post native advanced multimedia event to Java event handler
 * 
 * @param type          Event type
 * @param processorId   Processor ID that came from javacall_media_processor_create 
 * @param data          Data for this event type
 */
void javanotify_on_amms_notification(javacall_amms_notification_type type,
                                     javacall_int64 processorId,
                                     void *data) {
    midp_jc_event_union e;

    e.eventType = MIDP_JC_EVENT_ADVANCED_MULTIMEDIA;
    e.data.multimediaEvent.mediaType = type;
    e.data.multimediaEvent.appId = (int)((processorId >> 32) & 0xFFFF);
    e.data.multimediaEvent.playerId = (int)(processorId & 0xFFFF);
    e.data.multimediaEvent.data = (int) data;

    REPORT_INFO1(LC_NONE, 
            "[javanotify_on_amms_notification] type=%d\n", type);

    midp_jc_event_send(&e);
}
#endif

/**
 * The implementation call this callback notify function when image decode done
 *
 * @param handle Handle that is returned from javacall_image_decode_start
 * @param result the decoding operation result
 */
void javanotify_on_image_decode_end(javacall_handle handle, javacall_result result) {
#ifdef ENABLE_EXTERNAL_IMAGE_DECODER
    midp_jc_event_union e;

    e.eventType = MIDP_JC_EVENT_IMAGE_DECODER;
    e.data.imageDecoderEvent.handle = handle;
    e.data.imageDecoderEvent.result = result;

    midp_jc_event_send(&e);
#else
   (void)handle;
   (void)result;
#endif
}

/**
 * A callback function to be called by the platform in order to notify
 * about changes in the available file system roots (new root was added/
 * a root on removed).
 */
void javanotify_fileconnection_root_changed(void) {
#ifdef ENABLE_JSR_75
    midp_jc_event_union e;
    e.eventType = JSR75_FC_JC_EVENT_ROOTCHANGED;

    midp_jc_event_send(&e);
#endif
}

#ifdef ENABLE_JSR_179
/**
 * A callback function to be called for notification of non-blocking 
 * location related events.
 * The platform will invoke the call back in platform context for
 * each provider related occurrence. 
 *
 * @param type type of indication: Either
 * <pre>
 *          - <tt>JAVACALL_EVENT_LOCATION_OPEN_COMPLETED</tt>
 *          - <tt>JAVACALL_EVENT_LOCATION_ORIENTATION_COMPLETED</tt>
 *          - <tt>JAVACALL_EVENT_LOCATION_UPDATE_PERIODICALLY</tt>
 *          - <tt>JAVACALL_EVENT_LOCATION_UPDATE_ONCE</tt>
 * </pre>
 * @param handle handle of provider related to the notification
 * @param operation_result operation result: Either
 * <pre>
 *      - <tt>JAVACALL_OK</tt> if operation completed successfully, 
 *      - <tt>JAVACALL_LOCATION_RESULT_CANCELED</tt> if operation is canceled 
 *      - <tt>JAVACALL_LOCATION_RESULT_TIMEOUT</tt>  if operation is timeout 
 *      - <tt>JAVACALL_LOCATION_RESULT_OUT_OF_SERVICE</tt> if provider is out of service
 *      - <tt>JAVACALL_LOCATION_RESULT_TEMPORARILY_UNAVAILABLE</tt> if provider is temporarily unavailable
 *      - otherwise, <tt>JAVACALL_FAIL</tt>
 * </pre>
 */
void javanotify_location_event(
        javacall_location_callback_type event,
        javacall_handle provider,
        javacall_location_result operation_result) {
    midp_jc_event_union e;

    e.eventType = JSR179_LOCATION_JC_EVENT;

    switch(event){
    case JAVACALL_EVENT_LOCATION_ORIENTATION_COMPLETED:
        e.data.jsr179LocationEvent.event = JSR179_ORIENTATION_SIGNAL;   
        break;
    default:
        e.data.jsr179LocationEvent.event = JSR179_LOCATION_SIGNAL;
        break;
    }   
    
    e.data.jsr179LocationEvent.provider = provider;
    e.data.jsr179LocationEvent.operation_result = operation_result;

    midp_jc_event_send(&e);
}

/**
 * A callback function to be called for notification of proximity monitoring updates.
 *
 * This function will be called only once when the terminal enters the proximity of the registered coordinate. 
 *
 * @param provider handle of provider related to the notification
 * @param latitude of registered coordinate.
 * @param longitude of registered coordinate.
 * @param proximityRadius of registered coordinate.
 * @param pLocationInfo location info
 * @param operation_result operation result: Either
 * <pre>
 *      - <tt>JAVACALL_OK</tt> if operation completed successfully, 
 *      - <tt>JAVACALL_LOCATION_RESULT_CANCELED</tt> if operation is canceled 
 *      - <tt>JAVACALL_LOCATION_RESULT_OUT_OF_SERVICE</tt> if provider is out of service
 *      - <tt>JAVACALL_LOCATION_RESULT_TEMPORARILY_UNAVAILABLE</tt> if provider is temporarily unavailable
 *      - otherwise, <tt>JAVACALL_FAIL</tt>
 * </pre>
 */
void /*OPTIONAL*/javanotify_location_proximity(
        javacall_handle provider,
        double latitude,
        double longitude,
        float proximityRadius,
        javacall_location_location* pLocationInfo,
        javacall_location_result operation_result) {

    midp_jc_event_union e;

    e.eventType = JSR179_PROXIMITY_JC_EVENT;
    e.data.jsr179ProximityEvent.provider = provider;
    e.data.jsr179ProximityEvent.latitude = latitude;
    e.data.jsr179ProximityEvent.longitude = longitude;
    e.data.jsr179ProximityEvent.proximityRadius = proximityRadius;
    e.data.jsr179ProximityEvent.operation_result = operation_result;
    if (pLocationInfo)
        memcpy(&(e.data.jsr179ProximityEvent.location), pLocationInfo , sizeof(javacall_location_location));

    midp_jc_event_send(&e);
}

#endif /* ENABLE_JSR_179 */

#ifdef ENABLE_JSR_211

/*
 * Copies source javacall_utf16_string string into newly allocated memory buffer
 * @param src source javacall_utf16_string
 * @return pointer to newly allocated memory containing copy of source string
 */
static javacall_utf16_string
copy_jc_utf16_string(javacall_const_utf16_string src) {
    int length = 0;
    javacall_utf16_string result;
    if (JAVACALL_OK != javautil_unicode_utf16_ulength (src, &length)) {
        length = 0;
    }
    result = javacall_calloc(length + 1, sizeof(javacall_utf16));
    memcpy(result, src, (length + 1) * sizeof(javacall_utf16));
    return result;
}

/*
 * Called by platform to notify java VM that invocation of native handler 
 * is finished. This is <code>ContentHandlerServer.finish()</code> substitute 
 * after platform handler completes invocation processing.
 * @param invoc_id processed invocation Id
 * @param url if not NULL, then changed invocation URL
 * @param argsLen if greater than 0, then number of changed args
 * @param args changed args if @link argsLen is greater than 0
 * @param dataLen if greater than 0, then length of changed data buffer
 * @param data the data
 * @param status result of the invocation processing. 
 */

void javanotify_chapi_platform_finish(
        int invoc_id,
        javacall_utf16_string url,
        int argsLen, javacall_utf16_string* args,
        int dataLen, void* data,
        javacall_chapi_invocation_status status
) {
    midp_jc_event_union e;
    javacall_chapi_invocation *inv;
    int i;

    e.eventType = JSR211_JC_EVENT_PLATFORM_FINISH;
    e.data.jsr211PlatformEvent.invoc_id    = invoc_id;
    e.data.jsr211PlatformEvent.jsr211event =
        javacall_malloc (sizeof(*e.data.jsr211PlatformEvent.jsr211event));
    if (NULL != e.data.jsr211PlatformEvent.jsr211event) {
        e.data.jsr211PlatformEvent.jsr211event->status     = status;
        e.data.jsr211PlatformEvent.jsr211event->handler_id = NULL;
        
        inv = &e.data.jsr211PlatformEvent.jsr211event->invocation;
        inv->url               = copy_jc_utf16_string (url);
        inv->type              = NULL;
        inv->action            = NULL;
        inv->invokingAppName   = NULL;
        inv->invokingAuthority = NULL;
        inv->username          = NULL;
        inv->password          = NULL;
        inv->argsLen           = argsLen;
        inv->args              =
            javacall_calloc (sizeof(javacall_utf16_string), inv->argsLen);
        if (NULL != inv->args) {
            for (i = 0; i < inv->argsLen; i++) {
                inv->args[i] = copy_jc_utf16_string(args[i]);
            }
        }
        inv->dataLen           = dataLen;
        inv->data              = javacall_malloc (inv->dataLen);
        if (NULL != inv->data) {
            memcpy (inv->data, data, inv->dataLen);
        }
        inv->responseRequired  = 0;
    }
    
    midp_jc_event_send(&e);
}

/**
 * Receives invocation request from platform. <BR>
 * This is <code>Registry.invoke()</code> substitute for Platform->Java call.
 * @param handler_id target Java handler Id
 * @param invocation filled out structure with invocation params
 * @param invoc_id invocation Id for further references
 */
void javanotify_chapi_java_invoke(
        const javacall_utf16_string handler_id,
        javacall_chapi_invocation* invocation,
        int invoc_id) {
    midp_jc_event_union e;
    javacall_chapi_invocation *inv;
    int i;

    e.eventType = JSR211_JC_EVENT_JAVA_INVOKE;
    e.data.jsr211PlatformEvent.invoc_id    = invoc_id;
    e.data.jsr211PlatformEvent.jsr211event =
        javacall_malloc (sizeof(*e.data.jsr211PlatformEvent.jsr211event));
    if (NULL != e.data.jsr211PlatformEvent.jsr211event) {
        e.data.jsr211PlatformEvent.jsr211event->handler_id = 
            copy_jc_utf16_string(handler_id);
        e.data.jsr211PlatformEvent.jsr211event->status     =
            INVOCATION_STATUS_ERROR;
        
        inv = &e.data.jsr211PlatformEvent.jsr211event->invocation;
        inv->url             = copy_jc_utf16_string(invocation->url);
        inv->type            = copy_jc_utf16_string(invocation->type);
        inv->action          = copy_jc_utf16_string(invocation->action);
        inv->invokingAppName = 
            copy_jc_utf16_string(invocation->invokingAppName);
        inv->invokingAuthority = 
            copy_jc_utf16_string(invocation->invokingAuthority);
        inv->username        = copy_jc_utf16_string(invocation->username);
        inv->password        = copy_jc_utf16_string(invocation->password);
        inv->argsLen         = invocation->argsLen;
        inv->args            =
            javacall_calloc (sizeof(javacall_utf16_string), inv->argsLen);
        if (NULL != inv->args) {
            for (i = 0; i < inv->argsLen; i++) {
                inv->args[i] = copy_jc_utf16_string(invocation->args[i]);
            }
        }
        inv->dataLen           = invocation->dataLen;
        inv->data              = javacall_malloc (inv->dataLen);
        if (NULL != inv->data) {
            memcpy (inv->data, invocation->data, inv->dataLen);
        }
        inv->responseRequired  = invocation->responseRequired;
    }

    midp_jc_event_send(&e);
}

#endif /* ENABLE_JSR_211 */

/**
 * The platform calls this callback notify function when the permission dialog
 * is dismissed. The platform will invoke the callback in platform context.
 *  
 * @param userPermssion the permission level the user chose
 */
void javanotify_security_permission_dialog_finish(
                         javacall_security_permission_type userPermission) {
    midp_jc_event_union e;

    e.eventType = MIDP_JC_EVENT_PERMISSION_DIALOG;
    e.data.permissionDialog_event.permission_level = userPermission;

    midp_jc_event_send(&e);
}

/**
 * A notification function for telling Java to perform installation of
 * a content via http, for SprintAMS.
 *
 * This function requires that the descriptor (JADfile, or GCDfile)
 * has already been downloaded and resides somewhere on the file system.
 * The function also requires the full URL that was used to download the
 * file.
 * 
 * The given URL should be of the form http://www.sun.com/a/b/c/d.jad
 * or http://www.sun.com/a/b/c/d.gcd.  
 * Java will start a graphical installer which will download the content
 * fom the Internet.
 *
 * @param httpUrl null-terminated http URL string of the content 
 *        descriptor. The URL is of the following form:
 *        http://www.website.com/a/b/c/d.jad
 * @param descFilePath full path of the descriptor file which is of the 
 *        form:
 *        /a/b/c/d.jad  or /a/b/c/d.gcd
 * @param descFilePathLen length of the file path
 * @param isJadFile set to TRUE if the mime type of of the downloaded
 *        descriptor file is <tt>text/vnd.sun.j2me.app-descriptor</tt>. If 
 *        the mime type is anything else (e.g., <tt>text/x-pcs-gcd</tt>), 
 *        this must be set to FALSE.
 * @param isSilent set to TRUE if the content is to be installed silently,
 *        without intervention from the user. (e.g., in the case of SL
 *        or SI messages)
 * 
 */
void javanotify_install_content(const char * httpUrl,
                                const javacall_utf16* descFilePath,
                                int descFilePathLen,
                                javacall_bool isJadFile,
                                                                javacall_bool isSilent) {
    const static int SchemaLen = 16;
    const static javacall_utf16 SchemaFile[] = {'f','i','l','e',':','/','/','/'};

    midp_jc_event_union e;
    int httpUrlLength, dscFileOffset;
        

    if ((httpUrl == NULL) || (httpUrl == NULL)) {
        return; /* mandatory parameter is NULL */
    }

    httpUrlLength = strlen(httpUrl) + 1;
    dscFileOffset = (httpUrlLength + 1) & (-2); /* align to WORD boundary */

    if ((descFilePathLen <= 0) ||
        (descFilePathLen * 2 + dscFileOffset > sizeof(urlAddress))) {
        return; /* static buffer so small */
    }

    /* Is it nessasary to add schema file like: "file:///"? */
    /* Let move the input strings to the static buffer - urlAddress */
    memcpy(urlAddress, httpUrl, httpUrlLength);
    memcpy(urlAddress + dscFileOffset, descFilePath, descFilePathLen * 2);

    e.eventType = MIDP_JC_EVENT_INSTALL_CONTENT;
    e.data.install_content.httpUrl = urlAddress;
    e.data.install_content.descFilePath = (javacall_utf16*) urlAddress + dscFileOffset;
    e.data.install_content.descFilePathLen = descFilePathLen;
    e.data.install_content.isJadFile = isJadFile;
    e.data.install_content.isSilent = isSilent;

    midp_jc_event_send(&e);
}

#ifdef USE_VSCL

/*
 * See javacall_vscl.h for description
 */
void javanotify_vscl_incoming_event(javacall_vscl_event_type type, 
                                    javacall_utf16* str1,
                                    javacall_utf16* str2) {
    midp_jc_event_union e;

    switch (type) {
    case JAVACALL_VSCL_INCOMING_CALL:
        e.eventType = MIDP_JC_EVENT_VSCL_INCOMING_CALL;
                REPORT_INFO(LC_VSCL,"javanotify_vscl_incoming_event() got EVENT=INCOMING_CALL\n");        
        break;
    case JAVACALL_VSCL_CALL_DROPPED:
        e.eventType = MIDP_JC_EVENT_VSCL_CALL_DROPPED;
                REPORT_INFO(LC_VSCL,"javanotify_vscl_incoming_event() got EVENT=CALL_DROPPED\n");        
        break;
    case JAVACALL_VSCL_INCOMING_MAIL_CBS:
    case JAVACALL_VSCL_INCOMING_MAIL_DELIVERY_CONF:
    case JAVACALL_VSCL_INCOMING_MAIL_MMS:
    case JAVACALL_VSCL_INCOMING_MAIL_SMS:
    case JAVACALL_VSCL_INCOMING_MAIL_WAP_PUSH:
    case JAVACALL_VSCL_SCHEDULED_ALARM:
                REPORT_INFO1(LC_VSCL,"javanotify_vscl_incoming_event() EVENT=%d NOT IMPLEMENTED!\n", (int)type);        
        return;
    case JAVACALL_VSCL_FLIP_OPEN:
                REPORT_INFO(LC_VSCL,"javanotify_vscl_incoming_event() got EVENT=FLIP_OPEN\n");
        e.eventType = MIDP_JC_EVENT_VSCL_FLIP_OPENED;
        break;
    case JAVACALL_VSCL_FLIP_CLOSED:
                REPORT_INFO(LC_VSCL,"javanotify_vscl_incoming_event() got EVENT=FLIP_CLOSED\n");
        e.eventType = MIDP_JC_EVENT_VSCL_FLIP_CLOSED;
        break;
    default:
                REPORT_INFO1(LC_VSCL,"javanotify_vscl_incoming_event() EVENT=%d NOT IMPLEMENTED!\n", (int)type);       
        return;
    }

    midp_jc_event_send(&e);
}
#endif /* USE_VSCL */

/**
 * Notify Java that the user has made a selection
 * 
 * @param phoneNumber a string representing the phone number the user selected.
 *                    The string will be copied so it can be freed after this
 *                    function call returns.
 *                    In case the user dismissed the phonebook without making a
 *                    selection, this sting is <tt>NULL</tt>
 */
void /* OPTIONAL */ javanotify_textfield_phonenumber_selection(char* phoneNumber) {
    midp_jc_event_union e;
    int length;

    e.eventType = MIDP_JC_EVENT_PHONEBOOK;
    memset(selectedNumber, 0, MAX_PHONE_NUMBER_LENGTH);

    if (phoneNumber != NULL) {
        length = strlen(phoneNumber) + 1;
        if (length <= MAX_PHONE_NUMBER_LENGTH) {
            memcpy(selectedNumber, phoneNumber, length);
        } else {
            memcpy(selectedNumber, phoneNumber, MAX_PHONE_NUMBER_LENGTH);
        }
    }

    e.data.phonebook_event.phoneNumber = selectedNumber;

    midp_jc_event_send(&e);
}

void /* OPTIONAL */ javanotify_rotation() {
     midp_jc_event_union e;
     int length;
  
     e.eventType = MIDP_JC_EVENT_ROTATION;
     midp_jc_event_send(&e);
}

#if ENABLE_ON_DEVICE_DEBUG
/**
 * The platform calls this function to inform VM that
 * ODTAgent midlet must be enabled.
 */
void javanotify_enable_odd() {
     midp_jc_event_union e;

     e.eventType = MIDP_JC_ENABLE_ODD_EVENT;    
     midp_jc_event_send(&e);
}
#endif

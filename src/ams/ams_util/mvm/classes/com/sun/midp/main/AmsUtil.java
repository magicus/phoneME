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

package com.sun.midp.main;

import com.sun.cldc.isolate.*;

import com.sun.midp.midlet.MIDletStateHandler;
import com.sun.midp.midlet.MIDletSuite;

import com.sun.midp.midletsuite.MIDletSuiteImpl;
import com.sun.midp.midletsuite.MIDletSuiteStorage;

import com.sun.midp.configurator.Constants;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

/** 
 * Implements utilities that are different for SVM and MVM modes.
 * Utilities to start a MIDlet in a suite. If not the called from the 
 * AMS Isolate the request is forwarded to the AMS Isolate.
 * See {@link #executeWithArgs}.
 * <p> 
 * In the AMS Isolate, a check is make that the MIDlet is
 * not already running and is not in the process of starting.
 * If the MIDlet is already running or is starting the request
 * returns immediately. See {@link #startMidletCommon}
 * <p>
 */
public class AmsUtil {
    /** Cached reference to the MIDletExecuteEventProducer. */
    private static MIDletExecuteEventProducer midletExecuteEventProducer;
        
    /** Cached reference to the MIDletControllerEventProducer. */
    private static MIDletControllerEventProducer midletControllerEventProducer;

    /** Cached reference to the MIDletProxyList. */
    private static MIDletProxyList midletProxyList;
        
     /** The unique ID of the next MIDlet suite to run. */
    static String nextMidletSuiteToRun;

    /** The class of the next MIDlet to run. */
    static String nextMidletToRun;

    /**
     * If not null, this will be available to the MIDlet to run as
     * application property arg-0.
     */
    static String arg0ForNextMidlet;

    /**
     * If not null, this will be available to the MIDlet to run as
     * application property arg-1.
     */
    static String arg1ForNextMidlet;

    /**
     * If not null, this will be available to the MIDlet to run as
     * application property arg-2.
     */
    static String arg2ForNextMidlet;

    /**
     * Initializes AmsUtil class. shall only be called from 
     * MIDletSuiteLoader's main() in MVM AMS isolate
     * or in SVM main isolate.
     * No need in security checks since it is package private method.
     *
     * @param theMIDletProxyList MIDletController's container
     * @param theMidletControllerEventProducer utility to send events
     */
    static void initClass(MIDletProxyList theMIDletProxyList,
            MIDletControllerEventProducer theMidletControllerEventProducer) {
            
        midletProxyList = theMIDletProxyList;
        midletControllerEventProducer = theMidletControllerEventProducer;

        IsolateMonitor.initClass(theMIDletProxyList);
        StartMIDletMonitor.initClass(theMIDletProxyList);
    }
    
    /**
     * Initializes AmsUtil class. shall only be called from 
     * AppIsolateMIDletSuiteLoader's main() in 
     * non-AMS isolates.
     * No need in security checks since it is package private method.
     *
     * @param theMIDletExecuteEventProducer event producer 
     *        to be used by this class to send events
     */
    static void initClassInAppIsolate(
        MIDletExecuteEventProducer theMIDletExecuteEventProducer) {
            
        midletExecuteEventProducer = theMIDletExecuteEventProducer;
    }

    /**
     * Starts a MIDlet in a new Isolate.
     *
     * @param midletSuiteStorage reference to a MIDletStorage object
     * @param externalAppId ID of MIDlet to invoke, given by an external
     *                      application manager
     * @param id ID of an installed suite
     * @param midlet class name of MIDlet to invoke
     * @param displayName name to display to the user
     * @param arg0 if not null, this parameter will be available to the
     *             MIDlet as application property arg-0
     * @param arg1 if not null, this parameter will be available to the
     *             MIDlet as application property arg-1
     * @param arg2 if not null, this parameter will be available to the
     *             MIDlet as application property arg-2
     *
     * @return false to signal that the MIDlet suite does not have to exit
     * before the MIDlet is run
     */
    static boolean executeWithArgs(MIDletSuiteStorage midletSuiteStorage,
            int externalAppId, String id, String midlet,
            String displayName, String arg0, String arg1, String arg2) {
        int amsIsolateId;

        if (id == null) {
            // this was just a cancel request meant for SVM mode
            return false;
        }

        if (midlet == null) {
            throw new IllegalArgumentException("MIDlet class cannot be null");
        }

        amsIsolateId = MIDletSuiteLoader.getAmsIsolateId();

        if (MIDletSuiteLoader.getIsolateId() != amsIsolateId) {
            /*
             * This is not the AMS isolate so send the request to the
             * AMS isolate.
             */
            midletExecuteEventProducer.sendMIDletExecuteEvent(
                externalAppId,
                id, 
                midlet, 
                displayName, 
                arg0, 
                arg1, 
                arg2);
            return false;
        }

        // Don't start the MIDlet if it is already running.
        if (midletProxyList.isMidletInList(id, midlet)) {
            return false;
        }

        try {
            startMidletCommon(midletSuiteStorage, externalAppId, id, midlet,
                              displayName, arg0, arg1, arg2);
        } catch (Throwable t) {
            /*
             * This method does not throw exceptions for start errors,
             * (just like the SVM case), for errors, MVM callers rely on
             * start error events.
             */
        }

        return false;
    }

    /**
     * Starts a MIDlet in a new Isolate. Called from the AMS MIDlet suite.
     *
     * @param id ID of an installed suite
     * @param midlet class name of MIDlet to invoke
     * @param displayName name to display to the user
     * @param arg0 if not null, this parameter will be available to the
     *             MIDlet as application property arg-0
     * @param arg1 if not null, this parameter will be available to the
     *             MIDlet as application property arg-1
     * @param arg2 if not null, this parameter will be available to the
     *             MIDlet as application property arg-2
     *
     * @return Isolate that the MIDlet suite was started in
     */
    public static Isolate startMidletInNewIsolate(String id, String midlet,
            String displayName, String arg0, String arg1, String arg2) {
        // Note: getMIDletSuiteStorage performs an AMS permission check
        return startMidletCommon(MIDletSuiteStorage.getMIDletSuiteStorage(),
            0, id, midlet, displayName, arg0, arg1, arg2);
    }

    /**
     * Starts a MIDlet in a new Isolate.
     * Check that the MIDlet is is not realy running and is not already
     * being started.  If so, return immediately.
     * 
     *
     * @param midletSuiteStorage midletSuiteStorage for obtaining a
     *      classpath
     * @param externalAppId ID of MIDlet to invoke, given by an external
     *                      application manager
     * @param id ID of an installed suite
     * @param midlet class name of MIDlet to invoke
     * @param displayName name to display to the user
     * @param arg0 if not null, this parameter will be available to the
     *             MIDlet as application property arg-0
     * @param arg1 if not null, this parameter will be available to the
     *             MIDlet as application property arg-1
     * @param arg2 if not null, this parameter will be available to the
     *             MIDlet as application property arg-2
     *
     * @return Isolate that the MIDlet suite was started in;
     *             <code>null</code> if the MIDlet is already running
     */
    private static Isolate startMidletCommon(MIDletSuiteStorage
            midletSuiteStorage, int externalAppId, String id, String midlet,
            String displayName, String arg0, String arg1, String arg2) {
        Isolate isolate;
        String[] args = {id, midlet, displayName, arg0, arg1, arg2,
                         Integer.toString(externalAppId)};
        String[] classpath = midletSuiteStorage.getMidletSuiteClassPath(id);

        if (classpath[0] == null) {
            /*
             * Avoid a null pointer exception, rommized midlets don't need
             * a classpath.
             */
            classpath[0] = "";
        }

        try {
	    StartMIDletMonitor app = StartMIDletMonitor.okToStart(id, midlet);
	    if (app == null) {
		// Isolate is already running; don't start it again
		return null;
	    }

            isolate =
                new Isolate("com.sun.midp.main.AppIsolateMIDletSuiteLoader",
                    args, classpath);
	    app.setIsolate(isolate);
        } catch (Throwable t) {
            t.printStackTrace();
            throw new RuntimeException("Can't create Isolate****");
        }

        try {
            int reserved = Constants.SUITE_MEMORY_RESERVED;
            int limit = Constants.SUITE_MEMORY_LIMIT;

            if (limit < 0) {
                limit = isolate.totalMemory();
            } else {
                limit = limit * 1024;
            }

            reserved = reserved * 1024;

            isolate.setMemoryQuota(reserved, limit);
            isolate.setAPIAccess(true);
            isolate.start();
        } catch (Throwable t) {
            int errorCode;
            String msg;

            if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                t.printStackTrace();
            }

            if (t instanceof IsolateStartupException) {
                /*
                 * An error occured in the
                 * initialization or configuration of the new isolate
                 * before any application code is invoked, or if this
                 * Isolate was already started or is terminated.
                 */
                errorCode = Constants.MIDLET_ISOLATE_CONSTRUCTOR_FAILED;
                msg = "Can't start Application.";
            } else if (t instanceof IsolateResourceError) {
                /* The system has exceeded the maximum Isolate count. */
                errorCode = Constants.MIDLET_ISOLATE_RESOURCE_LIMIT;
                msg = "No more concurrent applications allowed.";
            } else if (t instanceof OutOfMemoryError) {
                /* The reserved memory cannot be allocated */
                errorCode = Constants.MIDLET_OUT_OF_MEM_ERROR;
                msg = "Not enough memory to run the application.";
            } else {
                errorCode = Constants.MIDLET_ISOLATE_CONSTRUCTOR_FAILED;
                msg = t.toString();
            }

            midletControllerEventProducer.sendMIDletStartErrorEvent(
                externalAppId,
                id,
                midlet,
                errorCode);
            
            throw new RuntimeException(msg);
        }

        return isolate;
    }

    /**
     * Terminates an isolate.
     *
     * @param id Isolate Id
     */
    static void terminateIsolate(int id) {
        Isolate isolate = MIDletProxyUtils.getIsolateFromId(id);
        if (isolate != null) {
            isolate.exit(0);
        }
    }
}

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

import com.sun.midp.configurator.Constants;

import com.sun.cldc.isolate.Isolate;
import com.sun.midp.events.EventQueue;

import com.sun.midp.installer.InternalMIDletSuiteImpl;

import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;

import com.sun.midp.lcdui.DisplayContainer;
import com.sun.midp.lcdui.DisplayEventProducer;
import com.sun.midp.lcdui.DisplayEventHandler;
import com.sun.midp.lcdui.DisplayEventHandlerFactory;
import com.sun.midp.lcdui.LCDUIEventListener;
import com.sun.midp.lcdui.RepaintEventProducer;
import com.sun.midp.lcdui.ItemEventConsumer;

import com.sun.midp.midlet.MIDletEventListener;
import com.sun.midp.midlet.MIDletPeer;
import com.sun.midp.midlet.MIDletStateHandler;
import com.sun.midp.midlet.MIDletSuite;

import com.sun.midp.midletsuite.MIDletSuiteImpl;
import com.sun.midp.midletsuite.MIDletSuiteStorage;

import com.sun.midp.security.Permissions;
import com.sun.midp.security.SecurityToken;
import com.sun.midp.security.SecurityInitializer;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

/**
 * The first class loaded in an application Isolate by the MIDP AMS to
 * initialize internal security and start a MIDlet suite.
 */
public class AppIsolateMIDletSuiteLoader {
    /** Guards against multiple use in an Isolate. */
    static boolean inUse;
    // private static SecurityToken internalSecurityToken;

    /**
     * Called for isolates other than the initial one.
     * Initializes internal security, and starts the MIDlet.
     *
     * @param args arg[0] the suite ID, arg[1] the class name of the MIDlet
     *             (can be null), arg[2] the name of the MIDlet to display
     *             to the user (can be null if arg[1] null),
     *             arg[3] optional MIDlet arg 0, arg[4] optional MIDlet arg 1,
     *             arg[5] optional MIDlet arg 2
     */
    public static void main(String args[]) {
        try {
            new AppIsolateMIDletSuiteLoader().run(args);
        } catch (Throwable t) {
            handleFatalError(t);
        }
    }

    /** Creates a new application MIDlet suite loader. */
    private AppIsolateMIDletSuiteLoader() {};

    /**
     * Native cleanup code, called when this isolate is done,
     * even if killed.
     */
    private native void finalize();

    /**
     * allocate reserved resources for the given isolate.
     *
     * @return     true if the reserved resources are available otherwise false
     */
    public native static boolean allocateReservedResources();

    /**
     * Handle a fatal error
     *
     * @param t the Throwable that caused the fatal error
     */
    private static native void handleFatalError(Throwable t);

    /**
     * Initializes internal security, and starts the MIDlet.
     *
     * @param args arg[0] the suite ID, arg[1] the class name of the MIDlet
     *             (can be null), arg[2] the name of the MIDlet to display
     *             to the user (can be null if arg[1] null),
     *             arg[3] optional MIDlet arg 0, arg[4] optional MIDlet arg 1,
     *             arg[5] optional MIDlet arg 2,
     *             arg[6] external app ID
     */
    private void run(String args[]) {
        int externalAppId = 0;
        String suiteId = null;
        String midletClassName = null;
        MIDletSuite midletSuite = null;
        SecurityToken internalSecurityToken;
        Isolate currentIsolate = Isolate.currentIsolate();

        if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
            Logging.report(Logging.INFORMATION, LogChannels.LC_CORE,
                           "Application MIDlet suite loader started");
        }

        /* This class cannot be used more than once. */
        if (inUse) {
            throw new IllegalStateException();
        } else {
            inUse = true;
        }

        // current isolate & AMS isolate are different objects
        int amsIsolateId = MIDletSuiteLoader.getAmsIsolateId();
        int currentIsolateId = MIDletSuiteLoader.getIsolateId();
        MIDletSuiteLoader.vmBeginStartUp(currentIsolateId);

        // Throws SecurityException if already called.
        internalSecurityToken = SecurityInitializer.init();
        EventQueue eventQueue =
            EventQueue.getEventQueue(internalSecurityToken);

        // create all needed event-related objects but not initialize ...
        MIDletExecuteEventProducer midletEvecuteEventProducer =
            new MIDletExecuteEventProducer(
                internalSecurityToken,
                eventQueue,
                amsIsolateId);

        MIDletControllerEventProducer midletControllerEventProducer =
            new MIDletControllerEventProducer(
                internalSecurityToken,
                eventQueue,
                amsIsolateId,
                currentIsolateId);

        DisplayEventProducer displayEventProducer =
            new DisplayEventProducer(
                internalSecurityToken,
                eventQueue);

        RepaintEventProducer repaintEventProducer = new RepaintEventProducer(
            internalSecurityToken,
            eventQueue);

        DisplayContainer displayContainer = new DisplayContainer(
            internalSecurityToken, currentIsolateId);

        DisplayEventHandler displayEventHandler =
            DisplayEventHandlerFactory.getDisplayEventHandler(
            internalSecurityToken);
        /*
         * Bad style of type casting, but
         * DisplayEventHandlerImpl implement both
         * DisplayEventHandler & ItemEventConsumer I/Fs
         */
        ItemEventConsumer itemEventConsumer =
                (ItemEventConsumer)displayEventHandler;

        MIDletStateHandler midletStateHandler =
            MIDletStateHandler.getMidletStateHandler();

        MIDletEventListener midletEventListener = new MIDletEventListener(
            internalSecurityToken,
            eventQueue,
            displayContainer);

        LCDUIEventListener lcduiEventListener = new LCDUIEventListener(
            internalSecurityToken,
            eventQueue,
            itemEventConsumer);

        // do all initialization for already created event-related objects ...
        AmsUtil.initClassInAppIsolate(midletEvecuteEventProducer);

        displayEventHandler.initDisplayEventHandler(
            internalSecurityToken,
            eventQueue,
            displayEventProducer,
            midletControllerEventProducer,
            repaintEventProducer,
            displayContainer);

        MIDletPeer.initClass(
            internalSecurityToken,
            displayEventHandler,
            midletStateHandler,
            midletControllerEventProducer);
        midletStateHandler.initMIDletStateHandler(
            internalSecurityToken,
            displayEventHandler,
            midletControllerEventProducer,
            displayContainer);

        try {
            String midletDisplayName;
            String arg0;
            String arg1;
            String arg2;
            MIDletSuiteStorage midletSuiteStorage;

            suiteId = args[0];
            midletClassName = args[1];
            midletDisplayName = args[2];
            arg0 = args[3];
            arg1 = args[4];
            arg2 = args[5];
            externalAppId = Integer.parseInt(args[6]);

            if (!allocateReservedResources()) {
                midletControllerEventProducer.sendMIDletStartErrorEvent(
                    externalAppId,
                    suiteId,
                    midletClassName,
                    Constants.MIDLET_RESOURCE_LIMIT);
                return;
            }

            if (suiteId.equals("internal")) {
                // assume a class name of a MIDlet in the classpath
                midletSuite = InternalMIDletSuiteImpl.create(
                    midletDisplayName,
                    suiteId);
            } else {
                midletSuiteStorage = MIDletSuiteStorage.
                    getMIDletSuiteStorage(internalSecurityToken);

                midletSuite = midletSuiteStorage.getMIDletSuite(
                              suiteId, false);
            }

            if (midletSuite == null) {
                midletControllerEventProducer.sendMIDletStartErrorEvent(
                    externalAppId, suiteId, midletClassName,
                    Constants.MIDLET_SUITE_NOT_FOUND);
                return;
            }

            if (!midletSuite.isEnabled()) {
                midletControllerEventProducer.sendMIDletStartErrorEvent(
                    externalAppId, suiteId, midletClassName,
                    Constants.MIDLET_SUITE_DISABLED);
                return;
            }

            // disable verifier in the case MIDlet suite has verified status
            if (Constants.VERIFY_ONCE) {
                if (midletSuite.isVerified()) {
                    MIDletSuiteVerifier.setUseVerifier(false);
                }
            }

            if (arg0 != null) {
                midletSuite.setTempProperty(internalSecurityToken,
                                            "arg-0", arg0);
            }

            if (arg1 != null) {
                midletSuite.setTempProperty(internalSecurityToken,
                                            "arg-1", arg1);
            }

            if (arg2 != null) {
                midletSuite.setTempProperty(internalSecurityToken,
                                            "arg-2", arg2);
            }

            if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                Logging.report(Logging.WARNING, LogChannels.LC_CORE,
                "Application Isolate MIDlet suite loader starting a suite");
            }

            if (midletSuite.checkPermission(
                    Permissions.getName(Permissions.AMS)) != 1) {
                /*
                 * Permission is not allowed.
                 *
                 * Shutdown access to Isolate references before a MIDlet is
                 * loaded. This will not effect the reference already obtained.
                 */
                currentIsolate.setAPIAccess(false);
            }

            MIDletSuiteLoader.vmEndStartUp(currentIsolateId);
            MIDletStateHandler.getMidletStateHandler().startSuite(midletSuite,
                    externalAppId, midletClassName);

            if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                Logging.report(Logging.INFORMATION, LogChannels.LC_CORE,
                   "Application Isolate MIDlet suite loader exiting");
            }
        } catch (Throwable t) {
            int errorCode;
            t.printStackTrace();
            if (t instanceof ClassNotFoundException) {
                errorCode = Constants.MIDLET_CLASS_NOT_FOUND;
            } else if (t instanceof InstantiationException) {
                errorCode = Constants.MIDLET_INSTANTIATION_EXCEPTION;
            } else if (t instanceof IllegalAccessException) {
                errorCode = Constants.MIDLET_ILLEGAL_ACCESS_EXCEPTION;
            } else if (t instanceof OutOfMemoryError) {
                errorCode = Constants.MIDLET_OUT_OF_MEM_ERROR;
            } else {
                errorCode = Constants.MIDLET_CONSTRUCTOR_FAILED;
            }

            if (Logging.TRACE_ENABLED) {
                Logging.trace(t,
                    "Exception caught in AppIsolateMIDletSuiteLoader");
            }

            midletControllerEventProducer.sendMIDletStartErrorEvent(
                externalAppId,
                suiteId,
                midletClassName,
                errorCode);
        } finally {
            if (midletSuite != null) {
                /*
                 * When possible, don't wait for the finalizer to unlock
                 * the suite.
                 */
                midletSuite.close();
            }

            eventQueue.shutdown();
            currentIsolate.exit(0);
        }
    }
}

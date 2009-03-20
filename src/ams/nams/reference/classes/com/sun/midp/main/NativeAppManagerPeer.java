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

package com.sun.midp.main;

import com.sun.midp.events.*;

import com.sun.midp.lcdui.ForegroundEventProducer;

import com.sun.midp.midlet.*;

import com.sun.midp.security.*;

import com.sun.j2me.security.AccessControlContextAdapter;
import com.sun.j2me.security.AccessController;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;
import com.sun.midp.configurator.Constants;
import com.sun.midp.suspend.SuspendSystemListener;
import com.sun.midp.suspend.SuspendSystem;

import com.sun.cldc.isolate.Isolate;

/**
 * This is an implementation of the native application manager peer
 * for the MVM mode of VM capable of running with
 * more than 1 midlet concurrently. It notifies it peer of MIDlet state
 * changes, but not this MIDlet.
 */
public class NativeAppManagerPeer
    implements MIDletProxyListListener, IsolateMonitorListener,
        EventListener, SuspendSystemListener {

    /** If true, the main of this class has already been called. */
    static boolean alreadyCalled = false;

    /** return value from Main, so we know that Main exited normally */
    static final int MAIN_EXIT = 2001;

    /** Singleton native app manager peer reference. */
    private static NativeAppManagerPeer singleton;

    /**
     * Inner class to request security token from SecurityInitializer.
     * SecurityInitializer should be able to check this inner class name.
     */
    static private class SecurityTrusted
        implements ImplicitlyTrustedClass {}

    /**
     * Called at the initial start of the VM.
     * Initializes internal security and any other AMS classes related
     * classes before starting the MIDlet.
     *
     * @param args not used
     */
    public static void main(String args[]) {
        // Since this is public method, guard against multiple calls
        if (alreadyCalled) {
            throw new SecurityException();
        }

        alreadyCalled = true;

        try {
            try {
                singleton = new NativeAppManagerPeer();
            } catch (Throwable exception) {
                notifySystemStartError();
                throw exception;
            }

            notifySystemStart();
            singleton.processNativeAppManagerRequests();
        } catch (Throwable exception) {
            if (Logging.TRACE_ENABLED) {
                Logging.trace(exception,
                             "Exception caught in NativeManagerPeer");
            }
        } finally {
            exitInternal(MAIN_EXIT);
        }
    }

    /** Security token for using protected APIs. */
    private SecurityToken internalSecurityToken;

    /** MIDlet proxy list reference. */
    private MIDletProxyList midletProxyList;

    /** Event queue reference. */
    private EventQueue eventQueue;

    /**
     * Create and initialize a new native app manager peer MIDlet.
     */
    private NativeAppManagerPeer() {
        /*
         * WARNING: Don't add any calls before this !
         *
         * Register AMS Isolate ID in native global variable.
         * Since native functions rely on this value to distinguish
         * whether Java AMS is running, this MUST be called before any
         * other native functions from this Isolate. I.E. This call
         * must be the first thing this main class makes.
         */
        registerAmsIsolateId();

        internalSecurityToken =
            SecurityInitializer.requestToken(new SecurityTrusted());
        eventQueue = EventQueue.getEventQueue(internalSecurityToken);

        // current isolate & AMS isolate is the same object
        int amsIsolateId = MIDletSuiteUtils.getAmsIsolateId();
        int currentIsolateId = MIDletSuiteUtils.getIsolateId();

        // init AMS task resources needed for all tasks
        MIDletSuiteUtils.initAmsResources();

        // create all needed event-related objects but not initialize ...
        MIDletEventProducer midletEventProducer =
            new MIDletEventProducer(eventQueue);

        MIDletControllerEventProducer midletControllerEventProducer =
            new MIDletControllerEventProducer(eventQueue,
                                              amsIsolateId,
                                              currentIsolateId);

        ForegroundEventProducer foregroundEventProducer =
            new ForegroundEventProducer(eventQueue);

        midletProxyList = new MIDletProxyList(eventQueue);

        // do all initialization for already created event-related objects ...
        MIDletProxy.initClass(foregroundEventProducer, midletEventProducer);
        MIDletProxyList.initClass(midletProxyList);

        AmsUtil.initClass(midletProxyList, midletControllerEventProducer);

        midletProxyList.addListener(this);
        midletProxyList.setDisplayController(
            new NativeDisplayControllerPeer(midletProxyList));

        IsolateMonitor.addListener(this);

        SuspendSystem.getInstance(internalSecurityToken).addListener(this);

        eventQueue.registerEventListener(
            EventTypes.NATIVE_MIDLET_EXECUTE_REQUEST, this);
        eventQueue.registerEventListener(
            EventTypes.NATIVE_MIDLET_RESUME_REQUEST, this);
        eventQueue.registerEventListener(
            EventTypes.NATIVE_MIDLET_PAUSE_REQUEST, this);
        eventQueue.registerEventListener(
            EventTypes.NATIVE_MIDLET_DESTROY_REQUEST, this);
        eventQueue.registerEventListener(
            EventTypes.NATIVE_MIDLET_GETINFO_REQUEST, this);
        eventQueue.registerEventListener(
            EventTypes.NATIVE_SET_FOREGROUND_REQUEST, this);

        eventQueue.registerEventListener(
            EventTypes.MIDP_HANDLE_UNCAUGHT_EXCEPTION, this);
        eventQueue.registerEventListener(
            EventTypes.MIDP_HANDLE_OUT_OF_MEMORY, this);
        eventQueue.registerEventListener(
            EventTypes.MIDP_UNCAUGHT_EXCEPTION_HANDLED, this);
        eventQueue.registerEventListener(
            EventTypes.MIDP_OUT_OF_MEMORY_HANDLED, this);

        IndicatorManager.init(midletProxyList);

        AccessController.setAccessControlContext(new MyAccessControlContext());
    }

    /**
     * Called in the main VM thread to keep the thread alive to until
     * shutdown completes.
     */
    private void processNativeAppManagerRequests() {
        midletProxyList.waitForShutdownToComplete();

        /*
         * Shutdown the event queue gracefully to process any events
         * that may be in the queue currently.
         */
        eventQueue.shutdown();
    }

    // ==============================================================
    // ------ Implementation of the MIDletProxyListListener interface

    /**
     * Called when a MIDlet is added to the list.
     *
     * @param midlet The proxy of the MIDlet being added
     */
    public void midletAdded(MIDletProxy midlet) {
        notifyMidletCreated(midlet.getExternalAppId(),
                            midlet.getSuiteId(), midlet.getClassName());
    }

    /**
     * Called when the state of a MIDlet in the list is updated.
     *
     * @param midlet The proxy of the MIDlet that was updated
     * @param fieldId code for which field of the proxy was updated
     */
    public void midletUpdated(MIDletProxy midlet, int fieldId) {
        if (fieldId == MIDLET_STATE) {
            if (midlet.getMidletState() == MIDletProxy.MIDLET_ACTIVE) {
                notifyMidletActive(midlet.getExternalAppId(),
                                   midlet.getSuiteId(), midlet.getClassName());
                return;
            }

            if (midlet.getMidletState() == MIDletProxy.MIDLET_PAUSED) {
                notifyMidletPaused(midlet.getExternalAppId(),
                                   midlet.getSuiteId(), midlet.getClassName());
                return;
            }
        }
    }

    /**
     * Called when a MIDlet is removed from the list.
     *
     * @param midlet The proxy of the removed MIDlet
     */
    public void midletRemoved(MIDletProxy midlet) {
        notifyMidletDestroyed(midlet.getExternalAppId(),
                              midlet.getSuiteId(), midlet.getClassName());
    }

    /**
     * Called when error occurred while starting a MIDlet object.
     *
     * @param externalAppId ID assigned by the external application manager
     * @param suiteId Suite ID of the MIDlet
     * @param className Class name of the MIDlet
     * @param errorCode start error code
     * @param errorDetails start error details
     */
    public void midletStartError(int externalAppId, int suiteId,
                                 String className, int errorCode,
                                 String errorDetails) {
        notifyMidletStartError(externalAppId, suiteId, className, errorCode);
    }

    // ------ End implementation of the MIDletProxyListListener interface
    // ==============================================================

    // ==============================================================
    // ------ Implementation of the IsolateMonitorListener interface

    /**
     * Called when a suite isolate is terminated.
     *
     * @param suiteId ID of the suite
     * @param className class name of the suite
     */
    public void suiteTerminated(int suiteId, String className) {
        notifySuiteTerminated(suiteId, className);
    }


    // ------ End implementation of the IsolateMonitorListener interface
    // ==============================================================

    // ==============================================================
    // ------ Implementation of the EventListener interface

    /**
     * Preprocess an event that is being posted to the event queue.
     * This method will get called in the thread that posted the event.
     *
     * @param event event being posted
     *
     * @param waitingEvent previous event of this type waiting in the
     *     queue to be processed
     *
     * @return true to allow the post to continue, false to not post the
     *     event to the queue
     */
    public boolean preprocess(Event event, Event waitingEvent) {
        return true;
    }

    /**
     * Process an event.
     * This method will get called in the event queue processing thread.
     *
     * @param event event to process
     */
    public void process(Event event) {
        String errorMsg = null;

        NativeEvent nativeEvent = (NativeEvent)event;
        int eventType = nativeEvent.getType();
        MIDletProxy midlet;

        if (eventType == EventTypes.MIDP_HANDLE_UNCAUGHT_EXCEPTION ||
                eventType == EventTypes.MIDP_HANDLE_OUT_OF_MEMORY) {
            /* for these event types the first parameter is isolateId << 16 */
            midlet = midletProxyList.findFirstMIDletProxy(
                                 (nativeEvent.intParam1 >> 16) & 0xffff);
        } else {
            /* for all other types the first parameter is externalAppId */
            midlet = midletProxyList.findMIDletProxy(
                                         nativeEvent.intParam1);
        }

        switch (eventType) {
            case EventTypes.NATIVE_MIDLET_EXECUTE_REQUEST: {
                if (midlet == null) {
                    if (nativeEvent.intParam2 == MIDletSuite.UNUSED_SUITE_ID) {
                        notifyMidletStartError(nativeEvent.intParam1,
                                               MIDletSuite.UNUSED_SUITE_ID,
                                               nativeEvent.stringParam1,
                                               Constants.MIDLET_ID_NOT_GIVEN);
                    } else if (nativeEvent.stringParam1 == null) {
                        notifyMidletStartError(nativeEvent.intParam1,
                                               nativeEvent.intParam2,
                                               null,
                                               Constants.MIDLET_CLASS_NOT_GIVEN);
                    } else {
                        MIDletSuiteUtils.executeWithArgs(internalSecurityToken,
                            nativeEvent.intParam1, nativeEvent.intParam2,
                            nativeEvent.stringParam1, nativeEvent.stringParam2,
                            nativeEvent.stringParam3, nativeEvent.stringParam4,
                            nativeEvent.stringParam5, nativeEvent.intParam3,
                            nativeEvent.intParam4, nativeEvent.intParam5,
                            nativeEvent.stringParam6, false);
                    }
                } else {
                    errorMsg = "Only one instance of a MIDlet can be launched";
                }
                break;
            }

            case EventTypes.NATIVE_MIDLET_RESUME_REQUEST: {
                if (midlet != null) {
                    midlet.activateMidlet();
                } else {
                    errorMsg = "Invalid App Id";
                }
                break;
            }

            case EventTypes.NATIVE_MIDLET_PAUSE_REQUEST: {
                if (midlet != null) {
                    midlet.pauseMidlet();
                } else {
                    errorMsg = "Invalid App Id";
                }
                break;
            }

            case EventTypes.NATIVE_MIDLET_DESTROY_REQUEST: {
                /*
                 * IMPL_NOTE: nativeEvent.intParam2 is a timeout value which
                 *            should be passed to MIDletProxy.destroyMidlet().
                 *
                 */
                if (midlet != null) {
                    midlet.destroyMidlet();
                } else {
                    errorMsg = "Invalid App Id";
                }
                break;
            }

            case EventTypes.NATIVE_MIDLET_GETINFO_REQUEST: {
                Isolate task = getIsolateObjById(midlet.getIsolateId());

                if (task != null) {
                    /* Structure to hold run time information about a midlet. */
                    RuntimeInfo runtimeInfo = new RuntimeInfo();

                    runtimeInfo.memoryTotal    = task.totalMemory();
                    runtimeInfo.memoryReserved = task.reservedMemory();
                    runtimeInfo.usedMemory     = task.usedMemory();
                    runtimeInfo.priority       = task.getPriority();
                    // there is no Isolate API now
                    runtimeInfo.profileName    = null;

                    saveRuntimeInfoInNative(runtimeInfo);
                }

                notifyOperationCompleted(
                    EventTypes.NATIVE_MIDLET_GETINFO_REQUEST,
                        nativeEvent.intParam1, (task == null) ? 1 : 0);
                break;
            }

            case EventTypes.NATIVE_SET_FOREGROUND_REQUEST: {
                // Allow Nams to explicitly set nothing to be in the foreground
                // with special AppId 0
                if (midlet != null ||
                        nativeEvent.intParam1 ==
                            Constants.MIDLET_APPID_NO_FOREGROUND) {
                    if (midletProxyList.getForegroundMIDlet() == midlet &&
                            midlet != null) {
                        // send the notification even if the midlet already has
                        // the foreground
                        NativeDisplayControllerPeer.notifyMidletHasForeground(
                            midlet.getExternalAppId());
                    } else {
                        midletProxyList.setForegroundMIDlet(midlet);
                    }
                } else {
                    errorMsg = "Invalid App Id";
                }
                break;
            }

            case EventTypes.MIDP_HANDLE_UNCAUGHT_EXCEPTION:
            case EventTypes.MIDP_HANDLE_OUT_OF_MEMORY: {
                String midletName;
                int externalAppId;
                
                if (midlet != null) {
                    midletName = midlet.getDisplayName();
                    externalAppId = midlet.getExternalAppId();
                } else {
                    externalAppId = Constants.MIDLET_APPID_NO_FOREGROUND;
                    midletName = "Unknown";
                }

                if (eventType == EventTypes.MIDP_HANDLE_UNCAUGHT_EXCEPTION) {
                    notifyUncaughtException(externalAppId,
                                    midletName,
                                    nativeEvent.stringParam1,
                                    nativeEvent.stringParam2,
                                    (nativeEvent.intParam1 & 0xffff) != 0);
                } else {
                    notifyOutOfMemory(externalAppId,
                                      midletName,
                                      nativeEvent.intParam2,
                                      nativeEvent.intParam3,
                                      nativeEvent.intParam4,
                                      nativeEvent.intParam5,
                                      (nativeEvent.intParam1 & 0xffff) != 0);
                }
                break;
            }

            case EventTypes.MIDP_UNCAUGHT_EXCEPTION_HANDLED:
            case EventTypes.MIDP_OUT_OF_MEMORY_HANDLED: {
                int responseCode = nativeEvent.intParam2;

                if (midlet == null) {
                    errorMsg = "OOM/EXCEPTION_HANDLED: Invalid App Id";
                    break;
                }

                if (responseCode == 0) { // JAVACALL_TERMINATE_MIDLET
                    // terminate the isolate
                    midlet.destroyMidlet();
                } else if ((responseCode == 1) ||  // JAVACALL_TERMINATE_THREAD
                           (responseCode == 2 && (eventType ==         // RETRY
                               EventTypes.MIDP_OUT_OF_MEMORY_HANDLED))) {
                    /*
                     * If the event type MIDP_UNCAUGHT_EXCEPTION_HANDLED, this
                     * response code means that the exception will be ignored
                     * (i.e. the thread from which the exception was thrown
                     * will be terminated).
                     *
                     * If the event type is MIDP_OUT_OF_MEMORY_HANDLED, this
                     * response code means that the VM should retry the memory
                     * allocation attempt; it will be done automatically after
                     * activation of the MIDlet.
                     */
                    Isolate task = getIsolateObjById(midlet.getIsolateId());

                    if (task != null) {
                        task.resume();
                    } else {
                        errorMsg = "OOM/EXCEPTION_HANDLED: Invalid Isolate Id:"
                                       + midlet.getIsolateId();
                    }
                } else {
                    errorMsg = "OOM/EXCEPTION_HANDLED: Invalid response code: "
                                       + responseCode;
                }

                break;
            }

            default: {
                errorMsg = "Unknown event type " + event.getType();
                break;
            }
        }

        if (Logging.REPORT_LEVEL <= Logging.ERROR && errorMsg != null) {
            Logging.report(Logging.ERROR, LogChannels.LC_AMS, errorMsg);
        }
    }

    // ------ End implementation of the EventListener interface
    // ==============================================================

    // ==============================================================
    // ------ Implementation of the SuspendSystemListener interface

    /**
     * Called if MIDP system has been suspended.
     */
    public void midpSuspended() {
        notifySystemSuspended();
    }

    /**
     * Called if MIDP system has been resumed.
     */
    public void midpResumed() {
        notifySystemStart();
    }

    // ------ End implementation of the Listener interface
    // ==============================================================

    /**
     * Finds an Isolate object by the given isolate ID.
     *
     * @param isolateId ID of the isolate
     * @return Isolate object having the given isolate ID
     */
    private Isolate getIsolateObjById(int isolateId) {
        Isolate[] allTasks = Isolate.getIsolates();

        for (int i = 0; i < allTasks.length; i++) {
            if (allTasks[i].id() == isolateId) {
                return allTasks[i];
            }
        }

        return null;
    }

    /**
     * Saves runtime information from the given structure
     * into the native buffer.
     *
     * @param runtimeInfo structure holding the information to save
     */
    private static native void saveRuntimeInfoInNative(RuntimeInfo runtimeInfo);

    /**
     * Notify the native application manager that the system has completed
     * the requested operation and the result (if any) is available.
     *
     * @param operation code of the operation that has completed
     * @param externalAppId ID assigned by the external application manager
     * @param retCode completion code (0 if OK)
     */
    private static native void notifyOperationCompleted(int operation,
                                                        int externalAppId,
                                                        int retCode);

    /**
     * Notify the native application manager that the system had an error
     * starting.
     */
    private static native void notifySystemStartError();

    /**
     * Notify the native application manager of the system start up.
     */
    private static native void notifySystemStart();

    /**
     * Notifies native application manager on MIDP suspension.
     */
    private static native void notifySystemSuspended();

    /**
     * Notify the native application manager of the MIDlet creation.
     *
     * @param externalAppId ID assigned by the external application manager
     * @param suiteId ID of the midlet suite
     * @param className class name of the midlet that failed to start
     * @param error error code
     */
    private static native void notifyMidletStartError(int externalAppId,
                                                      int suiteId,
                                                      String className,
                                                      int error);

    /**
     * Notify the native application manager of the MIDlet creation.
     *
     * @param externalAppId ID assigned by the external application manager
     * @param suiteId ID of the suite the created midlet belongs to
     * @param className class name of the midlet that was created
     */
    private static native void notifyMidletCreated(int externalAppId,
                                                   int suiteId,
                                                   String className);

    /**
     * Notify the native application manager that the MIDlet is active.
     *
     * @param externalAppId ID assigned by the external application manager
     * @param suiteId ID of the suite the started midlet belongs to
     * @param className class name of the midlet that was started
     */
    private static native void notifyMidletActive(int externalAppId,
                                                  int suiteId,
                                                  String className);

    /**
     * Notify the native application manager that the MIDlet is paused.
     *
     * @param externalAppId ID assigned by the external application manager
     * @param suiteId ID of the suite the paused midlet belongs to
     * @param className class name of the midlet that was paused
     */
    private static native void notifyMidletPaused(int externalAppId,
                                                  int suiteId,
                                                  String className);

    /**
     * Notify the native application manager that the MIDlet is destroyed.
     *
     * @param externalAppId ID assigned by the external application manager
     * @param suiteId ID of the suite the destroyed midlet belongs to
     * @param className class name of the midlet that was destroyed
     */
    private static native void notifyMidletDestroyed(int externalAppId,
                                                     int suiteId,
                                                     String className);

    /**
     * Notify the native application manager that the suite is terminated.
     *
     * @param suiteId ID of the MIDlet suite
     * @param className class name of the midlet that was terminated
     */
    private static native void notifySuiteTerminated(int suiteId,
                                                     String className);

    /**
     * Notifies the native application manager about an uncaught exception
     * in a running MIDlet.
     *
     * @param externalAppId ID assigned by the external application manager
     * @param midletName name of the MIDlet in which the exception occured
     * @param className  name of the class containing the method.
     *                   This string is a fully qualified class name
     *                   encoded in internal form (see JVMS 4.2).
     *                   This string is NULL-terminated.
     * @param exceptionMessage exception message
     * @param isLastThread true if this is the last thread of the MIDlet,
     *                     false otherwise
     */
    private static native void notifyUncaughtException(int externalAppId,
                                                       String midletName,
                                                       String className,
                                                       String exceptionMessage,
                                                       boolean isLastThread);

    /**
     * Notifies the native application manager about that VM can't
     * fulfill a memory allocation request.
     *
     * @param externalAppId ID assigned by the external application manager
     * @param memoryLimit in SVM mode - heap capacity, in MVM mode - memory
     *                    limit for the isolate, i.e. the max amount of heap
     *                    memory that can possibly be allocated
     * @param memoryReserved in SVM mode - heap capacity, in MVM mode - memory
     *                       reservation for the isolate, i.e. the max amount of
     *                       heap memory guaranteed to be available
     * @param memoryUsed how much memory is already allocated on behalf of this
     *                   isolate in MVM mode, or for the whole VM in SVM mode
     * @param allocSize the requested amount of memory that the VM failed
     *                  to allocate
     * @param midletName name of the MIDlet in which the exception occured
     * @param isLastThread true if this is the last thread of the MIDlet,
     *                     false otherwise
     */
    private static native void notifyOutOfMemory(int externalAppId,
                                                 String midletName,
                                                 int memoryLimit,
                                                 int memoryReserved,
                                                 int memoryUsed,
                                                 int allocSize,
                                                 boolean isLastThread);

    /**
     * Register the Isolate ID of the AMS Isolate by making a native
     * method call that will call JVM_CurrentIsolateId and set
     * it in the proper native variable.
     */
    private static native void registerAmsIsolateId();

    /**
     * Exit the VM with an error code. Our private version of Runtime.exit.
     * <p>
     * This is needed because the MIDP version of Runtime.exit cannot tell
     * if it is being called from a MIDlet or not, so it always throws an
     * exception.
     * <p>
     *
     * @param status Status code to return.
     */
    private static native void exitInternal(int status);

    // ==============================================================
    // Access Control Context allowing to access all protected APIs

    private class MyAccessControlContext extends AccessControlContextAdapter {
        /**
         * Checks for permission and throw an exception if not allowed.
         * May block to ask the user a question.
         * <p>
         * If the permission check failed because an InterruptedException was
         * thrown, this method will throw a InterruptedSecurityException.
         *
         * @param name name of the requested permission
         * @param resource string to insert into the question, can be null if
         *        no %2 in the question
         * @param extraValue string to insert into the question,
         *        can be null if no %3 in the question
         *
         * @exception SecurityException if the specified permission
         * is not permitted, based on the current security policy
         * @exception InterruptedException if another thread interrupts the
         *   calling thread while this method is waiting to preempt the
         *   display.
         */
        public void checkPermissionImpl(String name, String resource,
            String extraValue) throws SecurityException, InterruptedException {
        }
    }

}

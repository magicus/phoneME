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

import javax.microedition.io.ConnectionNotFoundException;
import javax.microedition.lcdui.Displayable;
import javax.microedition.midlet.MIDlet;
import com.sun.j2me.security.AccessController;
import com.sun.midp.events.*;
import com.sun.midp.log.*;
import com.sun.midp.configurator.Constants;
import com.sun.midp.installer.InternalMIDletSuiteImpl;
import com.sun.midp.lcdui.*;
import com.sun.midp.midlet.*;
import com.sun.midp.midletsuite.*;
import com.sun.midp.security.*;
import com.sun.midp.rms.RmsEnvironment;

/**
 * The first class loaded in VM by midp_run_midlet_with_args to initialize
 * internal security the internal AMS classes and start a MIDlet suite.
 */
public class CdcMIDletSuiteLoader extends AbstractMIDletSuiteLoader
    implements ForegroundController, MIDletStateListener,
    PlatformRequest {

    /** Disable startup error alerts, uninitialized by default */
    protected int disableAlerts = -1;

    /** Holds the ID of the current display, for preempting purposes. */
    protected int currentDisplayId;

    /* Sends foreground events to displays. */
    protected CdcForegroundEventProducer foregroundEventProducer;

    /**
     * Called at the initial start of the VM.
     * Initializes internal security and any other AMS classes related
     * classes before starting the MIDlet.
     *
     *
     * @param args arg[0] the suite ID (-1=rommized MIDlet),
     *             arg[1] the class name of the MIDlet,
     *             arg[2-n] optional MIDlet args, the first labled arg-0,
     *             then second arg-1, etc
     */
    public static void main(String args[]) {
        try {
            CDCInit.init();

            CdcMIDletSuiteLoader loader = new CdcMIDletSuiteLoader();

            if (args.length < 1) {
                System.err.println("The suite ID for the " +
                                   "MIDlet suite was not given");
                throw new IllegalArgumentException("Suite ID absent");
            }

            try {
                loader.suiteId = Integer.parseInt(args[0]);
            } catch (Throwable t) {
                throw new
                    IllegalArgumentException("Suite ID: " + t.getMessage());
            }            

            if (args.length < 2) {
                System.err.println("The class name for the " +
                                   "MIDlet was not given");
                throw new
                    IllegalArgumentException("MIDlet class name absent.");
            }

            loader.midletClassName = args[1];

            int numberOfSuiteArgs = args.length - 2;

            if (numberOfSuiteArgs > 0) {
                loader.args = new String[numberOfSuiteArgs];
                for (int i = 0; i < numberOfSuiteArgs; i++) {
                    loader.args[i] = args[i + 2];
                }
            }

            loader.runMIDletSuite();
        } catch (Throwable t) {
            t.printStackTrace();
        }
    }

    /**
     * Extends base class initialization with initializatons
     * specific for the AMS task
     */
    protected void init() {
        super.init();
    }

    /** Creates environment objects needed to AMS task */
    protected void createSuiteEnvironment() {
        foregroundController = this;

        lcduiEnvironment = new LCDUIEnvironmentForCDC(internalSecurityToken, 
						      eventQueue, isolateId, 
						      foregroundController);
        foregroundEventProducer = 
            new CdcForegroundEventProducer(eventQueue);

        // creates display container, needs foregroundController
        super.createSuiteEnvironment();

        midletStateHandler =
            MIDletStateHandler.getMidletStateHandler();

        MIDletSuiteStorage mss =
            MIDletSuiteStorage.getMIDletSuiteStorage(internalSecurityToken);

        midletStateHandler.initMIDletStateHandler(
            internalSecurityToken,
            this,
            new CdcMIDletLoader(mss),
            this);
        
       MidletSuiteContainer msc = new MidletSuiteContainer(mss);
       
       RmsEnvironment.init( internalSecurityToken, msc); 
    }

    /**
     * Inits created MIDlet suite environment objects and global
     * subsystems needed for all suites.
     * <p>
     * The method also loads MIDlet suite paramaters and arguments
     * from the natively saved <code>CommandState</code> instance.
     */
    protected void initSuiteEnvironment() {
        super.initSuiteEnvironment();

        externalAppId = 0;
        midletDisplayName = null;

        /* Set up permission checking for this suite. */
        AccessController.setAccessControlContext(
            new CdcAccessControlContext(midletSuite));
    }


    /**
     * Creates MIDlet suite instance by suite ID
     *
     * @return MIDlet suite to load
     *
     * @throws Exception in the case MIDlet suite can not be
     *   created because of a security reasons or some problems
     *   related to suite storage
     */
    protected MIDletSuite createMIDletSuite() throws Exception {
        MIDletSuiteStorage storage;
        MIDletSuite suite = null;

        storage = MIDletSuiteStorage.
                      getMIDletSuiteStorage(internalSecurityToken);

        if (suiteId == MIDletSuite.INTERNAL_SUITE_ID) {
            // assume a class name of a MIDlet in the classpath
            suite = InternalMIDletSuiteImpl.create(storage, midletDisplayName,
                        suiteId);
        } else {
            suite = storage.getMIDletSuite(suiteId, false);
            Logging.initLogSettings(suiteId);
        }

        return suite;
    }

    /** Gracefully terminates VM with proper return code */
    protected void exitLoader() {
        System.exit(0);
    }

    /**
     * Displays an exception message to user
     * @param exceptionMsg the message text
     */
    protected void displayException(String exceptionMsg) {
        System.err.println(exceptionMsg);
    }

    /**
     * Updates CommandState status and displays proper
     * exception message to user
     *
     * @param errorCode generic error code
     * @param details text with error details
     */
    protected void reportError(int errorCode, String details) {
        String errorMsg = getErrorMessage(errorCode);
        if (details != null) {
            errorMsg += "\n\n" + details;
        }

        displayException(errorMsg);

        // Error message is always obtained for logging
        if (Logging.REPORT_LEVEL <= Logging.ERROR) {
            Logging.report(Logging.ERROR, LogChannels.LC_CORE, errorMsg);
        }
    }

    /**
     * Handles exceptions happened during MIDlet suite execution.
     * @param t exception instance
     */
    public void handleException(Throwable t) {
        t.printStackTrace();
        int errorCode = getErrorCode(t);

        if (Logging.TRACE_ENABLED) {
            Logging.trace(t,
                "Exception caught in CdcMIDletSuiteLoader");
        }

        reportError(errorCode, t.getMessage());
    }

    /**
     * Gets AMS error message by generic error code.
     *
     * @param errorCode generic error code
     * @return error message
     */
    static protected String getErrorMessage(int errorCode) {
        switch (errorCode) {
            case Constants.MIDLET_SUITE_DISABLED:
                return "MIDlet suite dispabled.";

            case Constants.MIDLET_SUITE_NOT_FOUND:
                return "MIDlet suite not found.";

            case Constants.MIDLET_CLASS_NOT_FOUND:
                return "MIDlet class not found.";

            case Constants.MIDLET_INSTANTIATION_EXCEPTION:
                return "Cannot launch MIDlet due to illegal operation.";

            case Constants.MIDLET_ILLEGAL_ACCESS_EXCEPTION:
                return "Cannot launch MIDlet due to illegal operation.";

            case Constants.MIDLET_OUT_OF_MEM_ERROR:
                return "Out of memory";
        }

        return "MIDlet suite has unexpectedly quit.";
    }

    /**
     * Gets error code by exception type
     *
     * @param t exception instance
     * @return error code
     */
    static protected int getErrorCode(Throwable t) {
        if (t instanceof ClassNotFoundException) {
            return Constants.MIDLET_CLASS_NOT_FOUND;
        } else if (t instanceof InstantiationException) {
            return Constants.MIDLET_INSTANTIATION_EXCEPTION;
        } else if (t instanceof IllegalAccessException) {
            return Constants.MIDLET_ILLEGAL_ACCESS_EXCEPTION;
        } else if (t instanceof OutOfMemoryError) {
            return Constants.MIDLET_OUT_OF_MEM_ERROR;
        } else if (t instanceof MIDletSuiteLockedException) {
            return Constants.MIDLET_INSTALLER_RUNNING;
        } else {
            return Constants.MIDLET_CONSTRUCTOR_FAILED;
        }
    }

    // MIDletStateListener
    /**
     * Called before a MIDlet is created.
     * This implementation does nothing.
     *
     * @param suite reference to the loaded suite
     * @param className class name of the MIDlet to be created
     */
    public void midletPreStart(MIDletSuite suite, String className) {
    }

    /**
     * Called after a MIDlet is successfully created.
     * This implementation does nothing.
     *
     * @param suite reference to the loaded suite
     * @param className Class name of the MIDlet
     * @param externalAppId ID of given by an external application manager
     */
    public void midletCreated(MIDletSuite suite, String className,
                              int externalAppId) {
    }

    /**
     * Called before a MIDlet is activated.
     * This implementation does nothing.
     *
     * @param suite reference to the loaded suite
     * @param className class name of the MIDlet
     */
    public void preActivated(MIDletSuite suite, String className) {
    }

    /**
     * Called after a MIDlet is successfully activated. This is after
     * the startApp method is called.
     * This implementation does nothing.
     *
     * @param suite reference to the loaded suite
     * @param midlet reference to the MIDlet
     */
    public void midletActivated(MIDletSuite suite, MIDlet midlet) {
    }

    /**
     * Called after a MIDlet is successfully paused.
     * This implementation does nothing.
     *
     * @param suite reference to the loaded suite
     * @param className class name of the MIDlet
     */
    public void midletPaused(MIDletSuite suite, String className) {
    }

    /**
     * Called after a MIDlet pauses itself. In this case pauseApp has
     * not been called.
     *
     * @param suite reference to the loaded suite
     * @param className class name of the MIDlet
     */
    public void midletPausedItself(MIDletSuite suite, String className) {
    }

    /**
     * Called when a MIDlet calls MIDlet resume request.
     *
     * @param suite reference to the loaded suite
     * @param className class name of the MIDlet
     */
    public void resumeRequest(MIDletSuite suite, String className) {
        MIDletEventConsumer mec =
            midletStateHandler.getMIDletEventConsumer(internalSecurityToken,
                                                      className);

        if (mec == null) {
            return;
        }

        mec.handleMIDletActivateEvent();
    }

    /**
     * Called after a MIDlet is successfully destroyed.
     * This implementation does nothing.
     *
     * @param suite reference to the loaded suite
     * @param className class name of the MIDlet
     * @param midlet reference to the MIDlet, null if the MIDlet's constructor
     *               was not successful
     */
    public void midletDestroyed(MIDletSuite suite, String className,
                                MIDlet midlet) {
        if (midlet != null) {
            displayContainer.removeDisplaysByOwner(midlet);
        }
    }

    // ForegroundController

    /**
     * Called to register a newly create Display. Must method must
     * be called before the other methods can be called.
     * This implementation does nothing.
     *
     * @param displayId ID of the Display
     * @param ownerClassName Class name of the  that owns the display
     *
     * @return a place holder displayable to used when "getCurrent()==null",
     *         if null is returned an empty form is used
     */
    public Displayable registerDisplay(int displayId, String ownerClassName) {
        currentDisplayId = displayId;

        return null;
    }

    /**
     * Called to request the foreground.
     * This implementation does nothing.
     *
     * @param displayId ID of the Display
     * @param isAlert true if the current displayable is an Alert
     */
    public void requestForeground(int displayId, boolean isAlert) {
        NativeForegroundState.setState(internalSecurityToken, displayId);

        foregroundEventProducer.sendDisplayForegroundNotifyEvent(displayId);
    }

    /**
     * Called to request the background.
     * This implementation does nothing.
     *
     * @param displayId ID of the Display
     */
    public void requestBackground(int displayId) {
    }

    /**
     * Called to start preempting. The given display will preempt all other
     * displays for this isolate.
     *
     * @param displayId ID of the Display
     */
    public void startPreempting(int displayId) {
        foregroundEventProducer.sendDisplayBackgroundNotifyEvent(displayId);
        requestForeground(displayId, true);
    }

    /**
     * Called to end preempting.
     *
     * @param displayId ID of the Display
     */
    public void stopPreempting(int displayId) {
        foregroundEventProducer.sendDisplayBackgroundNotifyEvent(displayId);
        requestForeground(currentDisplayId, false);
    }

    // Platform Request

    /*
     * This is empty.
     *
     * @param URL The URL for the platform to load.
     *
     * @return true if the MIDlet suite MUST first exit before the
     * content can be fetched.
     *
     * @exception ConnectionNotFoundException if
     * the platform cannot handle the URL requested.
     *
     */
    public boolean dispatch(String URL) throws ConnectionNotFoundException {
        throw new ConnectionNotFoundException("not implemented");
    }
}

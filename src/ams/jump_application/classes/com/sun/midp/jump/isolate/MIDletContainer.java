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
package com.sun.midp.jump.isolate;

import java.io.File;

import java.util.Map;

import javax.microedition.io.ConnectionNotFoundException;

import javax.microedition.lcdui.Displayable;

import javax.microedition.midlet.MIDlet;
import javax.microedition.midlet.MIDletStateChangeException;

import com.sun.jump.common.JUMPApplication;
import com.sun.jump.common.JUMPAppModel;

import com.sun.jump.isolate.jvmprocess.JUMPAppContainer;
import com.sun.jump.isolate.jvmprocess.JUMPAppContainerContext;
import com.sun.jump.isolate.jvmprocess.JUMPIsolateProcess;

import com.sun.midp.events.EventQueue;

import com.sun.midp.jump.MIDletApplication;

import com.sun.midp.configurator.Constants;

import com.sun.midp.lcdui.*;

import com.sun.midp.log.*;

import com.sun.midp.main.CDCInit;
import com.sun.midp.main.CdcMIDletLoader;

import com.sun.midp.midlet.*;

import com.sun.midp.midletsuite.*;

import com.sun.midp.security.*;

/**
 * Application Container for the MIDlet app model.
 * <p>
 * The container uses the system property sun.midp.home.path as the
 * directory of the MIDP native library and application database.
 * <p>
 * The container uses the system property sun.midp.library.name as the
 * name of the MIDP native library.
 */
public class MIDletContainer extends JUMPAppContainer implements
    MIDletStateListener, PlatformRequest {

    /**
     * Inner class to request security token from SecurityInitializer.
     * SecurityInitializer should be able to check this inner class name.
     */
    private static class SecurityTrusted implements ImplicitlyTrustedClass {}

    /** This class has a different security domain than the MIDlet suite */
    private SecurityToken internalSecurityToken;

    /** The one and only runtime app ID. */
    private static final int APP_ID = 1;

    /** True, if an app has been started. */
    private boolean appStarted;


    /** Reference to the suite storage. */
    private MIDletSuiteStorage suiteStorage;

    /**
     * Provides interface to lcdui environment.
     */
    protected LCDUIEnvironment lcduiEnvironment;

    /** Starts and controls MIDlets through the lifecycle states. */
    private MIDletStateHandler midletStateHandler;

    /**
     * MIDlet suite instance created and properly initialized for
     * a MIDlet suite invocation.
     */
    private MIDletSuite midletSuite;

    /** Name of the class to start MIDlet suite execution */
    private String midletClassName;

    /** Holds the ID of the current display, for preempting purposes. */
    private int currentDisplayId;

    /** Provides methods to signal app state changes. */
    private JUMPAppContainerContext appContext;

    /** Core initialization of a MIDP environment. */
    public MIDletContainer(JUMPAppContainerContext context) {

        EventQueue eventQueue;

        CDCInit.init(context.getConfigProperty("sun.midp.home.path"));

        appContext = context;

        internalSecurityToken =
            SecurityInitializer.requestToken(new SecurityTrusted());

        // Init security tokens for core subsystems
        SecurityInitializer.initSystem();

        eventQueue = EventQueue.getEventQueue(
            internalSecurityToken);

        lcduiEnvironment = new LCDUIEnvironmentForCDC(internalSecurityToken, 
                                                      eventQueue, 0);

        suiteStorage =
            MIDletSuiteStorage.getMIDletSuiteStorage(internalSecurityToken);

        midletStateHandler =
            MIDletStateHandler.getMidletStateHandler();

        midletStateHandler.initMIDletStateHandler(
            internalSecurityToken,
            this,
            new CdcMIDletLoader(suiteStorage),
            this);
    }

    /**
     * Create a MIDlet and call its startApp method.
     * This method will not return until after the the MIDlet's startApp
     * method has returned.
     *
     * @param app application properties
     * @param args arguments for the app
     *
     * @return runtime application ID or -1 for failure
     */
    public synchronized int startApp(JUMPApplication app, String[] args) {
        try {
            int suiteId;

            if (appStarted) {
                throw new
                    IllegalStateException("Attempt to start a second app");
            }

            appStarted = true;

            suiteId = MIDletApplication.getMIDletSuiteID(app);

            midletSuite = suiteStorage.getMIDletSuite(suiteId, false);

            if (midletSuite == null) {
                throw new IllegalArgumentException("Suite not found");
            }

            Logging.initLogSettings(suiteId);

            if (!midletSuite.isEnabled()) {
                throw new IllegalStateException("Suite is disabled");
            }

            lcduiEnvironment.setTrustedState(midletSuite.isTrusted());

            // set a each arg as property numbered from 0, first arg: "arg-0"
            if (args != null) {
                for (int i = 0; i < args.length; i++) {
                    if (args[i] != null) {
                        midletSuite.setTempProperty(internalSecurityToken,
                                                    "arg-" + i, args[i]);
                    }
                }
            }

            midletClassName = MIDletApplication.getMIDletClassName(app);

            midletStateHandler.startSuite(midletSuite, midletClassName);
        } catch (Throwable e) {
            handleFatalException(e);
            return -1;
        }

        return APP_ID; // only one app can run in this container at a time
    }

    /**
     * Call a MIDlet's pauseApp method.
     * This method will not return until after the the MIDlet's pauseApp
     * method has returned.
     *
     * @param the application ID returned from startApp
     */    
    public void pauseApp(int appId) {
        midletStateHandler.pauseApp();
    }
    
    /**
     * Call a MIDlet's startApp method.
     * This method will not return until after the the MIDlet's startApp
     * method has returned.
     *
     * @param the application ID returned from startApp
     */    
    public void resumeApp(int appId) {
        try {
            midletStateHandler.resumeApp();
        } catch (MIDletStateChangeException msce) {
            // This exception is treated as a runtime exception
            throw new RuntimeException(msce.getMessage());
        }
    }
    
    /**
     * Call a MIDlet's destroyApp method.
     * This method will not return until after the the MIDlet's startApp
     * method has returned.
     * <p>
     * If force = false and the app did not get destroyed,
     * then a RuntimeException must be thrown.
     *
     * @param appId the application ID returned from startApp
     * @param force if false, give the app the option of not being destroyed
     */    
    public void destroyApp(int appId, boolean force) {
        try {
            midletStateHandler.destroyApp(force);
            midletSuite.close();
            appContext.terminateIsolate();
        } catch (Throwable e) {
            if (e instanceof MIDletStateChangeException || !force) {
                throw new RuntimeException(e.getMessage());
            }

            handleFatalException(e);
        }
    }

    /*
     * Standard fatal Throwable handling. Close the suite and terminate
     * the isolate.
     *
     * @param t exception thrown by lower layer
     */
    private void handleFatalException(Throwable t) {
        t.printStackTrace();
        midletSuite.close();
        appContext.terminateIsolate();
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
        appContext.notifyPaused(APP_ID);
    }

    /**
     * Called when a MIDlet calls MIDlet resume request.
     *
     * @param suite reference to the loaded suite
     * @param className class name of the MIDlet
     */
    public void resumeRequest(MIDletSuite suite, String className) {
        appContext.resumeRequest(APP_ID);
    }

    /**
     * Called after a MIDlet is successfully destroyed.
     * This implementation does nothing.
     *
     * @param suite reference to the loaded suite
     * @param className class name of the MIDlet
     */
    public void midletDestroyed(MIDletSuite suite, String className) {
        appContext.notifyDestroyed(APP_ID);
        appContext.terminateIsolate();
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

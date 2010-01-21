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

import com.sun.j2me.security.AccessController;

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
import com.sun.midp.main.CdcAccessControlContext;
import com.sun.midp.main.CdcMIDletLoader;

import com.sun.midp.midlet.*;

import com.sun.midp.midletsuite.*;

import com.sun.midp.security.*;

import com.sun.midp.rms.RmsEnvironment;

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
    ForegroundController, MIDletStateListener,
    PlatformRequest {

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


    /** Stores array of active displays for a MIDlet suite isolate. */
    private DisplayContainer displayContainer;

    /** Reference to the suite storage. */
    private MIDletSuiteStorage suiteStorage;

    /**
     * Provides interface to lcdui environment.
     */
    private LCDUIEnvironment lcduiEnvironment;

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
        appContext = context;
    }

    /** Core initialization of a MIDP environment. */
    private void init() {
        CDCInit.init(appContext.getConfigProperty("sun.midp.home.path"));

        internalSecurityToken =
            SecurityInitializer.requestToken(new SecurityTrusted());

        // Init security tokens for core subsystems
        SecurityInitializer.initSystem();

        suiteStorage =
            MIDletSuiteStorage.getMIDletSuiteStorage(internalSecurityToken);

        midletStateHandler =
            MIDletStateHandler.getMidletStateHandler();

        midletStateHandler.initMIDletStateHandler(
            internalSecurityToken,
            this,
            new CdcMIDletLoader(suiteStorage),
            this);

        EventQueue eventQueue =
            EventQueue.getEventQueue(internalSecurityToken);

        lcduiEnvironment =
            new LCDUIEnvironmentForCDC(internalSecurityToken,
                                       eventQueue, 0, this);

        displayContainer = lcduiEnvironment.getDisplayContainer();
        MidletSuiteContainer msc = new MidletSuiteContainer(suiteStorage);
       
        RmsEnvironment.init( msc); 
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
        init();

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

            AccessController.setAccessControlContext(
                new CdcAccessControlContext(midletSuite));

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

    /**
     * Set foreground display native state, so the native code will know
     * which display can draw. This method will not be needed when
     * JUMP uses GCI.
     *
     * @param displayId Display ID
     */
    private native void setForegroundInNativeState(int displayId);

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
     * @param midlet reference to the MIDlet, null if the MIDlet's constructor
     *               was not successful
     */
    public void midletDestroyed(MIDletSuite suite, String className,
                                MIDlet midlet) {
        appContext.notifyDestroyed(APP_ID);
        appContext.terminateIsolate();
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
        ForegroundEventConsumer fc =
            displayContainer.findForegroundEventConsumer(displayId);

        if (fc == null) {
            return;
        }

        setForegroundInNativeState(displayId);

        fc.handleDisplayForegroundNotifyEvent();
    }

    /**
     * Called to request the background.
     * This implementation does nothing.
     *
     * @param displayId ID of the Display
     */
    public void requestBackground(int displayId) {
        ForegroundEventConsumer fc =
            displayContainer.findForegroundEventConsumer(displayId);

        if (fc == null) {
            return;
        }

        fc.handleDisplayBackgroundNotifyEvent();
    }

    /**
     * Called to start preempting. The given display will preempt all other
     * displays for this isolate.
     *
     * @param displayId ID of the Display
     */
    public void startPreempting(int displayId) {
        requestBackground(currentDisplayId);
        requestForeground(displayId, true);
    }

    /**
     * Called to end preempting.
     *
     * @param displayId ID of the Display
     */
    public void stopPreempting(int displayId) {
        requestBackground(displayId);
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

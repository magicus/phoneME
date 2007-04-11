/*
 *
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
package com.sun.midp.jump.isolate;

import java.io.File;

import java.util.Map;

import javax.microedition.io.ConnectionNotFoundException;

import javax.microedition.lcdui.Displayable;

import javax.microedition.midlet.MIDlet;

import com.sun.jump.common.JUMPApplication;
import com.sun.jump.common.JUMPAppModel;

import com.sun.jump.isolate.jvmprocess.JUMPAppContainer;
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
    MIDletSuiteExceptionListener, ForegroundController, MIDletStateListener,
    PlatformRequest, Runnable {

    /**
     * Inner class to request security token from SecurityInitializer.
     * SecurityInitializer should be able to check this inner class name.
     */
    private static class SecurityTrusted implements ImplicitlyTrustedClass {}

    /** This class has a different security domain than the MIDlet suite */
    private SecurityToken internalSecurityToken;

    /** True, if an app has been started. */
    private boolean appStarted;

    /**
     * Provides interface for display preemption, creation and other
     * functionality that can not be publicly added to a javax package.
     */
    private DisplayEventHandler displayEventHandler;

    /** Stores array of active displays for a MIDlet suite isolate. */
    private DisplayContainer displayContainer;

    /** Reference to the suite storage. */
    private MIDletSuiteStorage suiteStorage;

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

    /** Core initialization of a MIDP environment. */
    public MIDletContainer() {
        EventQueue eventQueue;
        DisplayEventProducer displayEventProducer;
        RepaintEventProducer repaintEventProducer;
        DisplayEventListener displayEventListener;
        ItemEventConsumer itemEventConsumer;
        LCDUIEventListener lcduiEventListener;

        CDCInit.init();

        internalSecurityToken =
            SecurityInitializer.requestToken(new SecurityTrusted());

        // Init security tokens for core subsystems
        SecurityInitializer.initSystem();

        eventQueue = EventQueue.getEventQueue(
            internalSecurityToken);

        displayEventHandler =
            DisplayEventHandlerFactory.getDisplayEventHandler(
               internalSecurityToken);

        displayEventProducer =
            new DisplayEventProducer(
                eventQueue);

        repaintEventProducer =
            new RepaintEventProducer(
                eventQueue);

        displayContainer = new DisplayContainer(
            internalSecurityToken, 0);

        /*
         * Because the display handler is implemented in a javax
         * package it cannot created outside of the package, so
         * we have to get it after the static initializer of display the class
         * has been run and then hook up its objects.
         */
        displayEventHandler.initDisplayEventHandler(
            displayEventProducer,
            this,
            repaintEventProducer,
            displayContainer);

        displayEventListener = new DisplayEventListener(
            eventQueue,
            displayContainer);

        /* Bad style of type casting, but DisplayEventHandlerImpl
         * implements both DisplayEventHandler & ItemEventConsumer IFs */
        itemEventConsumer =
            (ItemEventConsumer)displayEventHandler;

        lcduiEventListener = new LCDUIEventListener(
            internalSecurityToken,
            eventQueue,
            itemEventConsumer);

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

            displayEventHandler.initSuiteData(midletSuite.isTrusted());

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

            //FIXME: This asynchronous call should be synchronous.
            new Thread(this).start();
        } catch (Exception e) {
            e.printStackTrace();
            return -1;
        }

        //DEBUG:System.err.println("**started");
        return 1; // only one app can run in this container at a time
    }

    /**
     * Call a MIDlet's pauseApp method.
     * This method will not return until after the the MIDlet's pauseApp
     * method has returned.
     *
     * @param the application ID returned from startApp
     */    
    public void pauseApp(int appId) {
        try {
            MIDletEventConsumer mec =
                midletStateHandler.getMIDletEventConsumer(
                    internalSecurityToken, midletClassName);

            if (mec == null) {
                return;
            }

            //FIXME: This asynchronous call should be synchronous.
            mec.handleMIDletPauseEvent();
        } catch (Exception e) {
            e.printStackTrace();
        }
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
            MIDletEventConsumer mec =
                midletStateHandler.getMIDletEventConsumer(
                    internalSecurityToken, midletClassName);

            if (mec == null) {
                return;
            }

            //FIXME: This asynchronous call should be synchronous.
            mec.handleMIDletActivateEvent();

        } catch (Exception e) {
            e.printStackTrace();
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
        //Impl note: force=false is not supported.
        try {
            MIDletEventConsumer mec =
                midletStateHandler.getMIDletEventConsumer(
                    internalSecurityToken, midletClassName);

            if (mec == null) {
                return;
            }

            //FIXME: This asynchronous call should be synchronous.
            mec.handleMIDletDestroyEvent();

        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * Set foreground display native state, so the native code will know
     * which display can draw. This method will not be needed when
     * JUMP uses GCI.
     *
     * @param displayId Display ID
     */
    private native void setForegroundInNativeState(int displayId);

    /** Run the MIDletStateHandler. */
    public void run() {
        try {
            midletStateHandler.startSuite(this, midletSuite, 0,
                                          midletClassName);
            midletSuite.close();
        } catch (Throwable t) {
            t.printStackTrace();
        } finally {
            System.exit(0);
        }
    }

    // MIDletSuiteExceptionListener

    /**
     * Handles exception occurred during MIDlet suite execution.
     * @param t exception instance
     */
    public void handleException(Throwable t) {
        t.printStackTrace();
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
        // Impl note: The JUMPApplication API does not support this.
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
        // Impl note: The JUMPApplication API does not support this.
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
     */
    public void midletDestroyed(MIDletSuite suite, String className) {
        // Impl note: The JUMPApplication API does not support this.
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

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

package com.sun.midp.automation;
import com.sun.midp.events.*;
import javax.microedition.lcdui.Image;
import com.sun.midp.main.*;
import java.util.*;

/**
 * Implements Automation class abstract methods.
 */
final class AutomationImpl extends Automation {
    /** The one and only class instance */
    private static AutomationImpl instance = null;

    /** Event queue */
    private EventQueue eventQueue;

    /** Event spying queue */
    private EventQueue eventSpyingQueue = null;

    /** Event factory */
    private AutoEventFactoryImpl eventFactory;

    /** index 0: foreground isolate id, index 1: foreground display id */
    private int[] foregroundIsolateAndDisplay;
    
    /** Takes screenshots */
    private AutoScreenshotTaker screenshotTaker;

    /** Hardware event listeners */
    private Vector hwEventListeners;

    /**
     * Gets instance of AutoSuiteStorage class.
     *
     * @return instance of AutoSuiteStorage class
     */
    public AutoSuiteStorage getStorage() {
        return AutoSuiteStorageImpl.getInstance();
    }

    /**
     * Gets instance of AutoEventFactory class.
     *
     * @return instance of AutoEventFactory class
     */
    public AutoEventFactory getEventFactory() {
        return AutoEventFactoryImpl.getInstance();
    }

    /**
     * Simulates single event.
     *
     * @param event event to simulate
     */    
    public void simulateEvents(AutoEvent event) 
        throws IllegalArgumentException {

        if (event == null) {
            throw new IllegalArgumentException("Event is null");
        }

        // Delay event is a special case
        if (event.getType() == AutoEventType.DELAY) {
            AutoDelayEvent delayEvent = (AutoDelayEvent)event;
            try {
                Thread.sleep(delayEvent.getMsec());
            } catch (InterruptedException e) {
            }

            return;
        }

        AutoEventImplBase eventBase = (AutoEventImplBase)event;
        if (eventBase.isPowerButtonEvent()) {
            // MIDletProxyList.getMIDletProxyList().shutdown();
            sendShutdownEvent();

            return;
        }

        // obtain native event corresponding to this AutoEvent        
        NativeEvent nativeEvent = eventBase.toNativeEvent();
        if (nativeEvent == null) {
            throw new IllegalArgumentException(
                    "Can't simulate this type of event: " + 
                    eventBase.getType().getName());
        }

        // obtain ids of foreground isolate and display
        int foregroundIsolateId;
        int foregroundDisplayId;
        synchronized (foregroundIsolateAndDisplay) {
            getForegroundIsolateAndDisplay(foregroundIsolateAndDisplay);
            foregroundIsolateId = foregroundIsolateAndDisplay[0];
            foregroundDisplayId = foregroundIsolateAndDisplay[1];
        }

        // and send this native event to foreground isolate
        nativeEvent.intParam4 = foregroundDisplayId;
        eventQueue.sendNativeEventToIsolate(nativeEvent, foregroundIsolateId);
    }

    /**
     * Simulates (replays) sequence of events.
     *
     * @param events event sequence to simulate
     * @throws IllegalArgumentException if some of the specified 
     * parameters has illegal value
     */
    public void simulateEvents(AutoEventSequence events) 
        throws IllegalArgumentException {

        if (events == null) {
            throw new IllegalArgumentException("Event sequence is null");
        }
        
        AutoEvent[] arr = events.getEvents();
        for (int i = 0; i < arr.length; ++i) {
            simulateEvents(arr[i]);
        }
    }

    /**
     * Simulates (replays) sequence of events.
     *
     * @param events event sequence to simulate
     * @param delayDivisor a double value for adjusting duration 
     * of delays within the sequence: duration of all delays is 
     * divided by this value. It allows to control the speed of
     * sequence replay. For example, to make it replay two times 
     * faster, specify 2.0 as delay divisor. 
     */
    public void simulateEvents(AutoEventSequence events, 
            double delayDivisor) 
        throws IllegalArgumentException {

        if (events == null) {
            throw new IllegalArgumentException("Event sequence is null");
        }

        if (delayDivisor == 0.0) {
            throw new IllegalArgumentException("Delay divisor is zero");
        }

        AutoEvent[] arr = events.getEvents();
        for (int i = 0; i < arr.length; ++i) {
            AutoEvent event = arr[i];

            if (event.getType() == AutoEventType.DELAY) {
                AutoDelayEvent delayEvent = (AutoDelayEvent)event;
                double msec = delayEvent.getMsec();
                simulateDelayEvent((int)(msec/delayDivisor));
            } else {
                simulateEvents(event);
            }
        }        
    }

    /**
     * Simulates key event. 
     *
     * @param keyCode key code not representable as character 
     * (soft key, for example)
     * @param keyState key state 
     * @param delayMsec delay in milliseconds before simulating the event
     * @throws IllegalArgumentException if some of the specified 
     * parameters has illegal value
     */
    public void simulateKeyEvent(AutoKeyCode keyCode, AutoKeyState keyState, 
            int delayMsec) 
        throws IllegalArgumentException {

        if (delayMsec != 0) {
            simulateDelayEvent(delayMsec);
        }

        AutoEvent e = eventFactory.createKeyEvent(keyCode, keyState);
        simulateEvents(e);
    }
    
    /**
     * Simulates key event. 
     *
     * @param keyChar key character (letter, digit)
     * @param keyState key state 
     * @param delayMsec delay in milliseconds before simulating the event
     * @throws IllegalArgumentException if some of the specified 
     * parameters has illegal value
     */
    public void simulateKeyEvent(char keyChar, AutoKeyState keyState, 
            int delayMsec) 
        throws IllegalArgumentException {

        if (delayMsec != 0) {
            simulateDelayEvent(delayMsec);
        }

        AutoEvent e = eventFactory.createKeyEvent(keyChar, keyState);
        simulateEvents(e);
    }

    /**
     * Simulates key click (key pressed and then released). 
     *
     * @param keyCode key code not representable as character 
     * (soft key, for example)
     * @param delayMsec delay in milliseconds before simulating the click
     * @throws IllegalArgumentException if some of the specified 
     * parameters has illegal value
     */
    public void simulateKeyClick(AutoKeyCode keyCode, int delayMsec) 
        throws IllegalArgumentException {

        AutoEvent e;

        if (delayMsec != 0) {
            simulateDelayEvent(delayMsec);
        }        

        e = eventFactory.createKeyEvent(keyCode, AutoKeyState.PRESSED);
        simulateEvents(e);

        e = eventFactory.createKeyEvent(keyCode, AutoKeyState.RELEASED);
        simulateEvents(e);
    }
    
    /**
     * Simulates key click (key pressed and then released). 
     *
     * @param keyChar key character (letter, digit)
     * @param delayMsec delay in milliseconds before simulating the click
     * @throws IllegalArgumentException if some of the specified 
     * parameters has illegal value
     */
    public void simulateKeyClick(char keyChar, int delayMsec) 
        throws IllegalArgumentException {

        AutoEvent e;

        if (delayMsec != 0) {
            simulateDelayEvent(delayMsec);
        }        

        e = eventFactory.createKeyEvent(keyChar, AutoKeyState.PRESSED);
        simulateEvents(e);

        e = eventFactory.createKeyEvent(keyChar, AutoKeyState.RELEASED);
        simulateEvents(e);
    }

    /**
     * Simulates pen event.
     *
     * @param x x coord of pen tip
     * @param y y coord of pen tip
     * @param penState pen state
     * @param delayMsec delay in milliseconds before simulating the event 
     * @throws IllegalArgumentException if some of the specified 
     * parameters has illegal value
     */
    public void simulatePenEvent(int x, int y, AutoPenState penState, 
            int delayMsec) 
        throws IllegalArgumentException {

        if (delayMsec != 0) {
            simulateDelayEvent(delayMsec);
        }        

        AutoEvent e = eventFactory.createPenEvent(x, y, penState);
        simulateEvents(e);
    }

    /**
     * Simulates pen click (pen tip pressed and then released).
     *
     * @param x x coord of pen tip
     * @param y y coord of pen tip
     * @param delayMsec delay in milliseconds before simulating the click
     * @throws IllegalArgumentException if some of the specified 
     * parameters has illegal value
     */
    public void simulatePenClick(int x, int y, int delayMsec) 
        throws IllegalArgumentException {

        if (delayMsec != 0) {
            simulateDelayEvent(delayMsec);
        }
        
        AutoEvent e;

        e = eventFactory.createPenEvent(x, y, AutoPenState.PRESSED);
        simulateEvents(e);

        e = eventFactory.createPenEvent(x, y, AutoPenState.RELEASED);
        simulateEvents(e);
    }

    /**
     * Simulates delay event.
     *
     * @param msec delay value in milliseconds 
     * @throws IllegalArgumentException if some of the specified 
     * parameters has illegal value
     */
    public void simulateDelayEvent(int msec) 
        throws IllegalArgumentException {

        AutoEvent e =  eventFactory.createDelayEvent(msec);
        simulateEvents(e);        
    }

    /**
     * Adds event listener that allows to monitor hardware events.
     * That is, this listener is called whenever hardware (external)
     * event occurs (like keypress).
     *
     * @param el event listener
     */
    public void addHardwareEventListener(AutoEventListener el) {
        if (eventSpyingQueue == null) {
            NativeEventListener l = new NativeEventListener();
            eventSpyingQueue = EventSpyingQueue.getEventQueue();
            eventSpyingQueue.registerEventListener(EventTypes.KEY_EVENT, l);
            eventSpyingQueue.registerEventListener(EventTypes.PEN_EVENT, l);
        }

        hwEventListeners.addElement(el);
    }

    /**
     * Removes previously added hardware event listener.
     */
    public void removeHardwareEventListener(AutoEventListener el) {
        hwEventListeners.removeElement(el);        
    }
    

    /**
     * Gets screenshot in specified format.
     * IMPL_NOTE: only implemented for putpixel based ports
     *
     * @param format screenshot format 
     * @return screenshot data as byte array, or null if taking
     * screenshot is not implemented
     */   
    public byte[] getScreenshot(int format) {
        screenshotTaker.takeScreenshot();

        byte[] data = screenshotTaker.getScreenshotRGB888();
        if (data == null) {
            return null;
        }

        if (format == SCREENSHOT_FORMAT_BMP) {
            int w = getScreenshotWidth();
            int h = getScreenshotHeight();
            BMPEncoder encoder = new BMPEncoder(data, w, h);
            data = encoder.encode();
        }

        return data;
    }

    /**
     * Gets screenshot width.
     *
     * @return screenshot width
     */
    public int getScreenshotWidth() {
        return screenshotTaker.getScreenshotWidth();
    }

    /**
     * Gets screenshot height.
     *
     * @return screenshot height
     */
    public int getScreenshotHeight() {
        return screenshotTaker.getScreenshotHeight();
    }
    

    /**
     * Gets instance of Automation class.
     *
     * @return instance of Automation class
     * @throws IllegalStateException if Automation API hasn't been
     * initialized or is not permitted to use
     */
    static final synchronized Automation getInstanceImpl() 
        throws IllegalStateException {
        
        AutomationInitializer.guaranteeAutomationInitialized();
        if (instance == null) {
            instance = new AutomationImpl(
                    AutomationInitializer.getEventQueue());
        }
        
        return instance;
    }

    /**
     * EventListener implementation
     */
    private class NativeEventListener implements EventListener {
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
         * @param nativeEvent native event recieved
         */
        public void process(Event nativeEvent) {
            AutoEvent event = eventFactory.createFromNativeEvent(
                    (NativeEvent)nativeEvent);

            notifyHardwareEventListeners(event);
        }
    }

    /**
     * Gets ids of foreground isolate and display.
     *
     * @param foregroundIsolateAndDisplay array to store ids in
     */
    private static native void getForegroundIsolateAndDisplay(
            int[] foregroundIsolateAndDisplay);

    /**
     * Sends global shutdown event
     */
    private static native void sendShutdownEvent();

    /**
     * Notifies hardware event listeners.
     *
     * @param event event to notify about
     */
    private void notifyHardwareEventListeners(AutoEvent event) {
        for (int i = hwEventListeners.size() - 1; i >= 0; i--) {
            AutoEventListener l = 
                (AutoEventListener)hwEventListeners.elementAt(i);

            l.onEvent(event);
        }
    }

    /**
     * Private constructor to prevent creating class instances.
     *
     * @param eventQueue event queue
     */
    private AutomationImpl(EventQueue eventQueue) {
        this.eventQueue = eventQueue;
        this.eventFactory = AutoEventFactoryImpl.getInstance();
        this.foregroundIsolateAndDisplay = new int[2];
        this.screenshotTaker = new AutoScreenshotTaker();
        this.hwEventListeners = new Vector();
    } 

}

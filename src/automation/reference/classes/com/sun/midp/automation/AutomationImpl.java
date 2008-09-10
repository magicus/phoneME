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

final class AutomationImpl extends Automation {
    private static AutomationImpl instance = null;
    private final static Object lock = new Object();

    private EventQueue eventQueue;
    private AutoEventFactoryImpl eventFactory;
    private int[] foregroundIsolateAndDisplay;
    
    private AutomationImpl(EventQueue eventQueue) {
        this.eventQueue = eventQueue;
        this.eventFactory = AutoEventFactoryImpl.getInstance();
        this.foregroundIsolateAndDisplay = new int[2];
    }

    final static Automation getInstanceImpl() 
        throws IllegalStateException {

        synchronized (lock) {
            AutomationInitializer.guaranteeAutomationInitialized();

            if (instance == null) {
                instance = new AutomationImpl(
                        AutomationInitializer.getEventQueue());
            }
        }
        
        return instance;
    }

    /**
     * Gets instance of AutoSuiteStorage class.
     *
     * @return instance of AutoSuiteStorage class
     * @throws IllegalStateException if Automation API hasn't been
     * initialized or is not permitted to use
     */    
    public AutoSuiteStorage getStorage() 
        throws IllegalStateException {

            return AutoSuiteStorageImpl.getInstance();
    }

    public AutoEventFactory getEventFactory() 
        throws IllegalStateException {

        return AutoEventFactoryImpl.getInstance();
    }

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
        NativeEvent nativeEvent = eventBase.toNativeEvent();
        if (nativeEvent == null) {
            throw new IllegalArgumentException(
                    "Can't simulate this type of event: " + 
                    eventBase.getType().getName());
        }

        int forgeroundIsolateId;
        int forgeroundDisplayId;
        synchronized (foregroundIsolateAndDisplay) {
            getForegroundIsolateAndDisplay(foregroundIsolateAndDisplay);
            forgeroundIsolateId = foregroundIsolateAndDisplay[0];
            forgeroundDisplayId = foregroundIsolateAndDisplay[1];
        }

        nativeEvent.intParam4 = forgeroundDisplayId;
        eventQueue.sendNativeEventToIsolate(nativeEvent,forgeroundIsolateId);
    }

    public void simulateEvents(AutoEventSequence events, 
            double speedAdjustment) {

        AutoEvent[] arr = events.getEvents();
        for (int i = 0; i < arr.length; ++i) {
            AutoEvent event = arr[i];

            if (event.getType() == AutoEventType.DELAY) {
                AutoDelayEvent delayEvent = (AutoDelayEvent)event;
                double msec = delayEvent.getMsec();
                simulateDelayEvent((int)(msec/speedAdjustment));
            } else {
                simulateEvents(event);
            }
        }        
    }

    public void simulateEvents(AutoEventSequence events) {
        AutoEvent[] arr = events.getEvents();
        for (int i = 0; i < arr.length; ++i) {
            simulateEvents(arr[i]);
        }
    }
    
    
    public void simulateKeyEvent(AutoKeyCode keyCode, AutoKeyState keyState, 
            int delayMsec) {

        if (delayMsec != 0) {
            simulateDelayEvent(delayMsec);
        }

        AutoEvent e = eventFactory.createKeyEvent(keyCode, keyState);
        simulateEvents(e);
    }
    
    public void simulateKeyEvent(char keyChar, AutoKeyState keyState, 
            int delayMsec) {

        if (delayMsec != 0) {
            simulateDelayEvent(delayMsec);
        }

        AutoEvent e = eventFactory.createKeyEvent(keyChar, keyState);
        simulateEvents(e);
    }

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

    public void simulatePenEvent(int x, int y, AutoPenState penState, 
            int delayMsec) 
        throws IllegalStateException {

        if (delayMsec != 0) {
            simulateDelayEvent(delayMsec);
        }        

        AutoEvent e = eventFactory.createPenEvent(x, y, penState);
        simulateEvents(e);
    }

    public void simulatePenClick(int x, int y, int delayMsec) {
        AutoEvent e;

        e = eventFactory.createPenEvent(x, y, AutoPenState.PRESSED);
        simulateEvents(e);

        e = eventFactory.createPenEvent(x, y, AutoPenState.RELEASED);
        simulateEvents(e);

        if (delayMsec != 0) {
            simulateDelayEvent(delayMsec);
        }
    }

    public void simulatePenClick(int x, int y) {
        simulatePenClick(x, y, 0);
    }


    public void simulateDelayEvent(int msec) {
        AutoEvent e =  eventFactory.createDelayEvent(msec);
        simulateEvents(e);        
    }
    
    private static native void getForegroundIsolateAndDisplay(
            int[] foregroundIsolateAndDisplay);
}

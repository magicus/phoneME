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

final class AutoJavaMEImpl extends AutoJavaME {
    private static AutoJavaMEImpl instance = null;
    private final static Object lock = new Object();

    private EventQueue eventQueue;
    private AutoEventFactoryImpl eventFactory;
    private int[] foregroundIsolateAndDisplay;
    
    private AutoJavaMEImpl(EventQueue eventQueue) {
        this.eventQueue = eventQueue;
        this.eventFactory = AutoEventFactoryImpl.getInstanceImpl();
        this.foregroundIsolateAndDisplay = new int[2];
    }

    final static AutoJavaME getInstanceImpl() 
        throws IllegalStateException {

        synchronized (lock) {
            AutomationInitializer.guaranteeAutomationInitialized();

            if (instance == null) {
                instance = new AutoJavaMEImpl(
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

        AutomationInitializer.guaranteeAutomationInitialized();
        return AutoSuiteStorageImpl.getInstance();        
    }    

    public void injectEvent(AutoEvent event) 
        throws IllegalArgumentException {

        if (event == null) {
            throw new IllegalArgumentException("Event is null");
        }

        AutoEventImplBase eventBase = (AutoEventImplBase)event;
        NativeEvent nativeEvent = eventBase.toNativeEvent();
        if (nativeEvent == null) {
            throw new IllegalArgumentException(
                    "Can't inject this type of event: " + 
                    eventBase.getType().getName());
        }

        getForegroundIsolateAndDisplay(foregroundIsolateAndDisplay);
        int forgeroundIsolateId = foregroundIsolateAndDisplay[0];
        int forgeroundDisplayId = foregroundIsolateAndDisplay[1];

        nativeEvent.intParam4 = forgeroundDisplayId;
        eventQueue.sendNativeEventToIsolate(nativeEvent,forgeroundIsolateId);
    }
    
    public void injectKeyEvent(AutoKeyState keyState, AutoKeyCode keyCode) {
        AutoEvent e = eventFactory.createKeyEvent(keyState, keyCode);
        injectEvent(e);
    }
    
    public void injectKeyEvent(AutoKeyState keyState, char keyChar) {
        AutoEvent e = eventFactory.createKeyEvent(keyState, keyChar);
        injectEvent(e);
    }

    public void replayEvents(AutoEventsSequence events, int speedDivisor) {
    }

    public void replayEvents(AutoEventsSequence events) {
        replayEvents(events, 1);
    }

    private static native void getForegroundIsolateAndDisplay(
            int[] foregroundIsolateAndDisplay);
}

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
import javax.microedition.lcdui.Image;

/**
 * Entry point into Automation API, that is, one starts working
 * with Automation API by obtaining instance of this class.
 * Provides methods for events simulation, getting suites storage 
 * and event factory class instances.
 */
public abstract class Automation {
    /** Screenshot format: RGB888 */
    public static final int SCREENSHOT_FORMAT_RGB888 = 1;

    /** Screenshot format: uncompressed Windows bitmap */ 
    public static final int SCREENSHOT_FORMAT_BMP = 2;

    /**
     * Gets instance of Automation class.
     *
     * @return instance of Automation class
     * @throws IllegalStateException if Automation API hasn't been
     * initialized or is not permitted to use
     */
    public static final Automation getInstance() 
        throws IllegalStateException {

        return AutomationImpl.getInstanceImpl();
    }

    /**
     * Gets instance of AutoSuiteStorage class.
     *
     * @return instance of AutoSuiteStorage class
     */    
    public abstract AutoSuiteStorage getStorage();

    /**
     * Gets instance of AutoEventFactory class.
     *
     * @return instance of AutoEventFactory class
     */
    public abstract AutoEventFactory getEventFactory();


    /*
     * Group of event simulating methods
     */

    /**
     * Simulates single event.
     *
     * @param event event to simulate
     * @throws IllegalArgumentException if some of the specified 
     * parameters has illegal value
     */
    public abstract void simulateEvents(AutoEvent event) 
        throws IllegalArgumentException;

    /**
     * Simulates (replays) sequence of events.
     *
     * @param events event sequence to simulate
     * @throws IllegalArgumentException if some of the specified 
     * parameters has illegal value
     */
    public abstract void simulateEvents(AutoEventSequence events) 
        throws IllegalArgumentException;

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
    public abstract void simulateEvents(AutoEventSequence events, 
            double delayDivisor) 
        throws IllegalArgumentException;

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
    public abstract void simulateKeyEvent(AutoKeyCode keyCode, 
            AutoKeyState keyState, int delayMsec) 
        throws IllegalArgumentException;

    /**
     * Simulates key event. 
     *
     * @param keyChar key character (letter, digit)
     * @param keyState key state 
     * @param delayMsec delay in milliseconds before simulating the event
     * @throws IllegalArgumentException if some of the specified 
     * parameters has illegal value
     */
    public abstract void simulateKeyEvent(char keyChar, AutoKeyState keyState, 
            int delayMsec) 
        throws IllegalArgumentException;

    /**
     * Simulates key click (key pressed and then released). 
     *
     * @param keyCode key code not representable as character 
     * (soft key, for example)
     * @param delayMsec delay in milliseconds before simulating the click
     * @throws IllegalArgumentException if some of the specified 
     * parameters has illegal value
     */
    public abstract void simulateKeyClick(AutoKeyCode keyCode, int delayMsec) 
        throws IllegalArgumentException;
    
    /**
     * Simulates key click (key pressed and then released). 
     *
     * @param keyChar key character (letter, digit)
     * @param delayMsec delay in milliseconds before simulating the click
     * @throws IllegalArgumentException if some of the specified 
     * parameters has illegal value
     */
    public abstract void simulateKeyClick(char keyChar, int delayMsec) 
        throws IllegalArgumentException;

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
    public abstract void simulatePenEvent(int x, int y, AutoPenState penState, 
            int delayMsec) 
        throws IllegalArgumentException;

    /**
     * Simulates pen click (pen tip pressed and then released).
     *
     * @param x x coord of pen tip
     * @param y y coord of pen tip
     * @param delayMsec delay in milliseconds before simulating the click
     * @throws IllegalArgumentException if some of the specified 
     * parameters has illegal value
     */
    public abstract void simulatePenClick(int x, int y, int delayMsec)
        throws IllegalArgumentException;


    /**
     * Simulates delay event.
     *
     * @param msec delay value in milliseconds 
     * @throws IllegalArgumentException if some of the specified 
     * parameters has illegal value
     */
    public abstract void simulateDelayEvent(int msec) 
        throws IllegalArgumentException;


    /*
     * Group of event listening methods
     */

    /**
     * Adds event listener that allows to monitor hardware events.
     * That is, this listener is called whenever hardware (external)
     * event occurs (like keypress).
     *
     * @param el event listener
     */
    public abstract void addHardwareEventListener(AutoEventListener el);

    /**
     * Removes previously added hardware event listener.
     */
    public abstract void removeHardwareEventListener(AutoEventListener el);


    /*
     * Group of screenshot taking methods
     */    

    /**
     * Gets screenshot in specified format.
     * IMPL_NOTE: only implemented for putpixel based ports
     *
     * @param format screenshot format 
     * @return screenshot data as byte array, or null if taking
     * screenshot is not implemented
     */
    public abstract byte[] getScreenshot(int format);

    /**
     * Gets screenshot width.
     *
     * @return screenshot width
     */
    public abstract int getScreenshotWidth();

    /**
     * Gets screenshot height.
     *
     * @return screenshot height
     */
    public abstract int getScreenshotHeight();
}

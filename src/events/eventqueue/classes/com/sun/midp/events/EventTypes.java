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

package com.sun.midp.events;

/**
 * Event type IDs, this should match midpEvents.h.
 */
public final class EventTypes {
    /**
     * MAJOR EVENT TYPES
     */
    
    /**
     * Major ID for an event on a key.
     */
    public static final int KEY_EVENT     =  1;

    /**
     * Major ID for a pointer event.
     */
    public static final int PEN_EVENT     =  2;

    /**
     * Major ID for an Abstract Command.
     */
    public static final int COMMAND_EVENT =  3;

    /**
     * Major ID for a repaint event.
     */
    public static final int REPAINT_EVENT =  4;

    /**
     * Major ID for a screen change event.
     */
    public static final int SCREEN_CHANGE_EVENT =  5;

    /**
     * Major ID for an invalidate event.
     */
    public static final int INVALIDATE_EVENT =  6;

    /**
     * Major ID for an item event.
     */
    public static final int ITEM_EVENT =  7;

    /**
     * Major ID for a item's native peer state changed event.
     */
    public static final int PEER_CHANGED_EVENT =  8;

    /**
     * Major ID for a call serially event.
     */
    public static final int CALL_SERIALLY_EVENT =  9;

    /**
     * Major ID for a foreground notify event.
     * <ul>
     * <li>intParam1 = 1 = has foreground, 0 - background
     * <li>intParam4 = displayID
     * </ul>
     */
    public static final int FOREGROUND_NOTIFY_EVENT = 10;

    /**
     * Major ID for a activate MIDlet event.
     */
    public static final int ACTIVATE_MIDLET_EVENT  =  11;

    /**
     * Major ID for a pause MIDlet event.
     */
    public static final int PAUSE_MIDLET_EVENT  =  12;

    /**
     * Major ID for a destroy MIDlet event.
     */
    public static final int DESTROY_MIDLET_EVENT  =  13;

    /**
     * Major ID for a shutdown event.
     */
    public static final int SHUTDOWN_EVENT  =  14;

    /**
     * Major ID for a pause all event.
     */
    public static final int ACTIVATE_ALL_EVENT  =  15;

    /**
     * Major ID for a pause all event.
     */
    public static final int PAUSE_ALL_EVENT  =  16;

    /**
     * Major ID for a MIDlet created notification.
     */
    public static final int MIDLET_CREATED_NOTIFICATION  =  17;

    /**
     * Major ID for a MIDlet active notification.
     */
    public static final int MIDLET_ACTIVE_NOTIFICATION  =  18;

    /**
     * Major ID for a MIDlet paused notification.
     */
    public static final int MIDLET_PAUSED_NOTIFICATION  =  19;

    /**
     * Major ID for a MIDlet destroyed notification.
     */
    public static final int MIDLET_DESTROYED_NOTIFICATION  =  20;

    /**
     * Major ID for a foreground request event.
     * <ul>
     * <li>intParam1 = IsolateID
     * <li>intParam4 = displayID
     * </ul>
     * @see com.sun.midp.main.MIDletProxyList#foregroundRequestEvent
     */
    public static final int FOREGROUND_REQUEST_EVENT  =  21;

    /**
     * Major ID for a background request event.
     * <ul>
     * <li>intParam1 = IsolateID
     * <li>intParam4 = displayID
     * </ul>
     * @see com.sun.midp.main.MIDletProxyList#backgroundRequestEvent
     */
    public static final int BACKGROUND_REQUEST_EVENT  =  22;

    /**
     * User request to show the main screen, if any.
     * The event parameters are unused.
     * @see com.sun.midp.main.MIDletProxyList#selectForegroundEvent
     */
    public static final int SELECT_FOREGROUND_EVENT  =  23;

    /**
     * Major ID for a preempt event.
     */
    public static final int PREEMPT_EVENT = 24;

    /**
     * Major ID for a MIDlet start error event.
     */
    public static final int MIDLET_START_ERROR_EVENT = 25;

    /**
     * Major ID for a execute MIDlet event.
     * <ul>
     * <li> stringParam1 = id;
     * <li> stringParam2 = midlet;
     * <li> stringParam3 = displayName;
     * <li> stringParam4 = arg0;
     * <li> stringParam5 = arg1;
     * <li> stringParam6 = arg2;
     * </ul>
     */
    public static final int EXECUTE_MIDLET_EVENT  =  26;

    /**
     * Major ID for a request to destroy  a MIDlet event.
     * @see com.sun.midp.main.MIDletProxyList#midletDestroyRequestEvent
     */
    public static final int MIDLET_DESTROY_REQUEST_EVENT  =  27;

    /**
     * Major ID for an foreground handoff event.
     * This is a request to the AMS to transition the foreground 
     * from a current MIDlet to a target MIDlet 
     * (by application ID and classname).
     * <ul>
     * <li>stringParam1 = current FG MIDlet ID
     * <li>stringParam2 = current FG MIDlet class
     * <li>stringParam3 = target MIDlet ID
     * <li>stringParam4 = target MIDlet class
     * </ul>
     * @see com.sun.midp.main.MIDletProxyList#foregroundTransferEvent
     */
    public static final int FOREGROUND_TRANSFER_EVENT = 28;


    /** The event will shutdown the event queue when processed. */
    public static final int EVENT_QUEUE_SHUTDOWN = 29;

    /**
     * This is sent to the AMS isolate when an application isolate
     * has fatal error.
     */
    public static final int FATAL_ERROR_NOTIFICATION = 30;

    /**
     * Major ID for a MM EOM event.
     */
    public static final int MM_EOM_EVENT = 31;

    /**
     * Major ID for a MM SAT event.
     */
    public static final int MM_SAT_EVENT = 32;

    /**
     * Major ID for a MM TONEEOM event.
     */
    public static final int MM_TONEEOM_EVENT = 33;
    
    /**
     * Major ID for a JSR 75 FileConnection disks changed event.
     */
    public static final int FC_DISKS_CHANGED_EVENT = 34;

    /** Reserved for testing. */
    public static final int TEST_EVENT = 35;

    /**
     * Sent to the AMS isolate when a paused MIDlet is requesting to be
     * moved to active state.
     */
    public static final int MIDLET_RESUME_REQUEST = 36;

    /**
     * Sent by the native system to request a MIDlet be created and started.
     */
    public static final int NATIVE_MIDLET_EXECUTE_REQUEST = 37;

    /**
     * Sent by the native system to request a paused MIDlet be resumed.
     */
    public static final int NATIVE_MIDLET_RESUME_REQUEST = 38;

    /**
     * Sent by the native system to request a MIDlet be paused.
     */
    public static final int NATIVE_MIDLET_PAUSE_REQUEST = 40;

    /**
     * Sent by the native system to request a MIDlet be destroyed.
     */
    public static final int NATIVE_MIDLET_DESTROY_REQUEST = 41;

    /**
     * Sent by the native system to request a MIDlet be in the foreground.
     */
    public static final int NATIVE_SET_FOREGROUND_REQUEST = 42;

    
    /**
     * Sent by the Automation API subsystem to request a MIDlet 
     * be in the foreground.
     *
     * <ul>
     * <li>stringParam1 = MIDlet's suite ID
     * <li>stringParam2 = MIDlet's class name
     * </ul>
     */
    public static final int SET_FOREGROUND_BY_NAME_REQUEST = 43;
};

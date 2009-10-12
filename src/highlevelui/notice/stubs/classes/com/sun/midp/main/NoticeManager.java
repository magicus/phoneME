/*
 *
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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
import com.sun.midp.events.EventQueue;


/**
 * Stubbed NoticeManager class. Used if USE_NOTIFICATION=false
 * 
 */
public class NoticeManager {

    /**
     * Common initialization function. Registeres 
     * NOTIFICATION_ANNOUNCEMENT_EVENT handler. Used by every task 
     * initializators. 
     * 
     * 
     * @param queue event queue.
     */
    public static void initCommon(EventQueue queue) {
    }

    /**
     * AMS specific initialization. Registers the manager as {@link 
     * MIDletProxyList} listener. Need to cleanup registered notice 
     * table if the originator MIDlet crashed.
     * 
     * 
     * @param proxyList 
     */
    public static void initWithAMS(MIDletProxyList proxyList) {
    }

    /**
     * Creates (if necessary) and returns NoticeManager singleton.
     * 
     * @return NoticeManager singleton.
     */
    public static NoticeManager getInstance() {
        return new NoticeManager();
    }

    /**
     * Registers notice status listeners.
     * 
     * 
     * @param listener {@link NoticeManagerListener} type listener.
     */
    public synchronized void addListener(NoticeManagerListener listener) {
    }

    /**
     * Remove notice status listener.
     * 
     * 
     * @param listener {@link NoticeManagerListener} type listener.
     */
    public synchronized void removeListener(NoticeManagerListener listener) {
    }
}


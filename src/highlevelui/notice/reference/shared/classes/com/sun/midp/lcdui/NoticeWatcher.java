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

package com.sun.midp.lcdui;

import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Timer;
import java.util.TimerTask;
import java.util.Vector;
import com.sun.midp.main.NoticeManagerListener;
import com.sun.midp.main.NoticeManager;

/**
 * Service class that watches for notices expiration.
 * <p>
 * Every task instances has own watcher that looking for both
 * original notices and notices coming from other midlets. Thus
 * no timeout events are sent through event system.
 * 
 */
public class NoticeWatcher implements NoticeManagerListener {

    /**
     * A timer that handles all timer tasks.
     * 
     */
    Timer timer;

    /**
     * A map that correlate timer task with the notice.
     * 
     */
    Hashtable map;

    /**
     * This class singleton.
     * 
     */
    private static NoticeWatcher singleton;


    /**
     * NoticeWatcher initializer.
     * 
     */
    public static void init() {
        if (null == singleton) {
            singleton = new NoticeWatcher();
        }
    }

    /**
     * Private constructor prevents custom class creation.
     * 
     */
    private NoticeWatcher() {
        NoticeManager.getInstance().addListener(this);
        timer = new Timer();
        map = new Hashtable();
    }

    /**
     * Informs about new information note.
     * 
     * @param notice new information note
     */
    public void notifyNotice(Notice notice) {
        long timeout = notice.getTimeout();
        if (timeout == 0) {
            // 0 means no time limit
            return;
        }
        schedule(notice);
    }

    /**
     * Informs that the notice was updated
     * 
     * @param notice the notice was updated
     */
    public void updateNotice(Notice notice) {
        TimerTask task = (TimerTask)map.get(notice);
        if (null != task) {
            if (task.scheduledExecutionTime() == notice.getTimeout()) {
                return;
            }
            task.cancel();
            map.remove(notice);
        }
        if (0 != notice.getTimeout()) {
            schedule(notice);
        }
    }

    /**
     * Informs about given information note need to be discarded
     * 
     * @param notice information note
     */
    public void removeNotice(Notice notice) {
        TimerTask task = (TimerTask)map.get(notice);
        if (null != task) {
            task.cancel();
            map.remove(notice);
        }
    }


    /**
     * Schedules notice expiration task.
     * 
     * 
     * @param notice to be watched for expiration.
     */
    private void schedule(Notice notice) {
        int duration = (int)(notice.getTimeout() - System.currentTimeMillis());
        if (duration < 0) {
            // remove notice at separate thread
            duration  = 1;
        }
        NoticeCanceler task = new NoticeCanceler(notice);
        map.put(notice, task);
        timer.schedule(task, duration);

    }

    /**
     * Private class that fires the notice at separate thread.
     * 
     */
    private class NoticeCanceler extends TimerTask {
        private Notice note;
        NoticeCanceler(Notice notice) {
            note = notice;
        }
        /**
         * The action to be performed by this timer task.
         */
        public void run() {
            // causes removeNotice call
            note.timeout();
        }
    }
}



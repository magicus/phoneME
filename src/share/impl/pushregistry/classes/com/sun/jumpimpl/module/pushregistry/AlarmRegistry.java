/*
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

package com.sun.jumpimpl.module.pushregistry;

import com.sun.jumpimpl.module.pushregistry.persistence.Store;
import java.io.IOException;
import java.util.Date;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;
import javax.microedition.io.ConnectionNotFoundException;

/**
 * Registry that manages alarms.
 *
 * <p>
 * NOTE: this class is not thread-safe and should be guarded
 * </p>
 */
public final class AlarmRegistry {
    /** Lifecycle management adapter interface. */
    public static interface LifecycleAdapter {
        /**
         * Launches the given <code>MIDlet</code>.
         *
         * <p>
         * NOTE: implementation should be thread-safe as several
         * invocations of the method can be performed in parallel.
         * </p>
         *
         * @param midletSuiteID <code>MIDlet suite</code> ID
         * @param midlet <code>MIDlet</code> class name
         */
        void launchMidlet(int midletSuiteID, String midlet);
    }

    /** Timer to track alarms. */
    private final Timer timer;

    /** Map of registered alarms. */
    private final Map alarms;

    /** Store to save alarm info. */
    private final Store store;

    /** Lifecycle adapter implementation. */
    private final LifecycleAdapter lifecycleAdapter;

    /**
     * Constructs an alarm registry.
     *
     * @param store persistent store to save alarm info into
     * @param lifecycleAdapter adapter to launch <code>MIDlet</code>
     */
    public AlarmRegistry(
            final Store store,
            final LifecycleAdapter lifecycleAdapter) {
        this.timer = new Timer();
        this.alarms = new HashMap();
        this.store = store;
        this.lifecycleAdapter = lifecycleAdapter;
    }

    /**
     * Reads alarms from the persistent store and registers them.
     *
     * <p>
     * NOTE: the store should be initialized with <code>readStore</code>
     * method.
     * </p>
     */
    public void readAlarms() throws IOException {
        store.listAlarms(new Store.AlarmsConsumer() {
            public void consume(final int midletSuiteID, final Map alarms) {
                for (Iterator it = alarms.entrySet().iterator(); it.hasNext();) {
                    final Map.Entry entry = (Map.Entry) it.next();
                    final String midlet = (String) entry.getKey();
                    final Long time = (Long) entry.getValue();
                    scheduleAlarm(new MIDletInfo(midletSuiteID, midlet),
                            time.longValue());
                }
            }
        });
    }

    /**
     * Registers an alarm.
     *
     * @param midletSuiteID <code>MIDlet suite</code> ID
     * @param midlet <code>MIDlet</code> class name
     * @param time alarm time
     *
     * @throws ConnectionNotFoundException if for any reason alarm cannot be
     *  scheduled
     *
     * @return previous alarm time or 0 if none
     */
    public long registerAlarm(
            final int midletSuiteID,
            final String midlet,
            final long time) throws ConnectionNotFoundException {
        final MIDletInfo midletInfo = new MIDletInfo(midletSuiteID, midlet);

        final TimerTask oldTask = (TimerTask) alarms.get(midletInfo);
        long oldTime = 0L;
        if (oldTask != null) {
            oldTime = oldTask.scheduledExecutionTime();
            oldTask.cancel(); // Safe to ignore return
        }

        try {
            store.addAlarm(midletSuiteID, midlet, time);
        } catch (IOException ioe) {
            /*
             * RFC: looks like optimal strategy, but we might simply ignore it
             * (cf. Irbis push_server.c)
             */
            throw new ConnectionNotFoundException();
        }

        scheduleAlarm(midletInfo, time);

        return oldTime;
    }

    /**
     * Removes alarms for the given suite.
     *
     * @param midletSuiteID ID of the suite to remove alarms for
     */
    public void removeSuiteAlarms(final int midletSuiteID) {
        for (Iterator it = alarms.entrySet().iterator(); it.hasNext();) {
            final Map.Entry entry = (Map.Entry) it.next();
            final MIDletInfo midletInfo = (MIDletInfo) entry.getKey();
            if (midletInfo.midletSuiteID == midletSuiteID) {
                // No need to care about retval
                ((TimerTask) entry.getValue()).cancel();
                removeAlarm(midletInfo);
            }
        }
    }

    /**
     * Disposes an alarm registry.
     *
     * <p>
     * This method is needed as <code>Timer</code> creates non daemon thread
     * which would prevent the app from exit.
     * </p>
     */
    public void dispose() {
        timer.cancel();
        alarms.clear();
    }

    /**
     * Scheduleds an alarm.
     *
     * @param midletInfo registration info
     * @param time alarm time
     */
    private void scheduleAlarm(final MIDletInfo midletInfo, final long time) {
        final Date date = new Date(time);
        final TimerTask newTask = new TimerTask() {
            public void run() {
                removeAlarm(midletInfo);
                lifecycleAdapter.launchMidlet(midletInfo.midletSuiteID,
                        midletInfo.midlet);
            }
        };
        alarms.put(midletInfo, newTask);
        timer.schedule(newTask, date);
        /*
         * RFC: according to <code>Timer</code> spec, <quote>if the time is in
         * the past, the task is scheduled for immediate execution</quote>.
         * I hope it's MIDP complaint
         */
    }

    /**
     * Removes an alarm associated info.
     *
     * @param midletInfo defines <code>MIDlet</code> to remove alarm for
     */
    private void removeAlarm(final MIDletInfo midletInfo) {
        alarms.remove(midletInfo);
        try {
            store.removeAlarm(midletInfo.midletSuiteID, midletInfo.midlet);
        } catch (IOException _) {
            // The best thing I can do
        }
    }

    /** Unique identification for <code>MIDlet</code>. */
    private static final class MIDletInfo {
        /*
         * Might go away if better way to bundle both <code>MIDlet suite</code>
         * and <code>MIDlet</code> class name into one piece of data will be
         * found (e.g. appId, but there might be some complications with
         * persistent store and install/uninstall).
         *
         * And most probably such class can be shared.
         */

        /** <code>MIDlet suite</code> ID. */
        public final int midletSuiteID;

        /** <code>MIDlet</code> class name. */
        public final String midlet;

        /**
         * Constructs an instance.
         *
         * @param midletSuiteID <code>MIDlet suite</code> ID
         * @param midlet <code>MIDlet</code> class name
         */
        public MIDletInfo(final int midletSuiteID, final String midlet) {
            this.midletSuiteID = midletSuiteID;
            this.midlet = midlet;
        }

        /**
         * Implements <code>Object.hashCode</code>.
         *
         * @return hash code
         */
        public int hashCode() {
            return midletSuiteID + midlet.hashCode();
        }

        /**
         * Implements <code>Object.equals</code>.
         *
         * @param obj object to compare with
         *
         * @return <code>true</code> iff equal
         */
        public boolean equals(final Object obj) {
            if (!(obj instanceof MIDletInfo)) {
                return false;
            }

            final MIDletInfo rhs = (MIDletInfo) obj;
            return (midletSuiteID == rhs.midletSuiteID)
                && midlet.equals(rhs.midlet);
        }
    }
}

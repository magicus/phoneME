/*
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

package com.sun.midp.push.controller;

import com.sun.midp.push.persistence.Store;
import java.io.IOException;
import java.util.Date;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;
import javax.microedition.io.ConnectionNotFoundException;

/**
 * Controller that manages alarms.
 *
 * <p>
 * IMPORTANT_NOTE:  As this class uses <code>Store</code> to keep
 *  alarms data in persistent store, the clients of this class must ensure
 *  that the underlying content store can be exclusively locked when public
 *  methods of this class get invoked.
 * <p>
 */
final class AlarmController {

    /** Timer to track alarms. */
    private final Timer timer;

    /** Map of registered alarms. */
    private final Map alarms;

    /** Store to save alarm info. */
    private final Store store;

    /** Lifecycle adapter implementation. */
    private final LifecycleAdapter lifecycleAdapter;

    /**
     * Constructs an alarm controller.
     *
     * <p>
     * NOTE: both <code>store</code> and <code>lifecycleAdapter</code>
     * MUST be not <code>null</code>.  There is no checks and passing
     * <code>null</code> leads to undefined behaviour.
     * </p>
     *
     * @param store persistent store to save alarm info into
     * (cannot be <code>null</code>)
     *
     * @param lifecycleAdapter adapter to launch <code>MIDlet</code>
     * (cannot be <code>null</code>)
     */
    public AlarmController(
            final Store store,
            final LifecycleAdapter lifecycleAdapter) {
        this.timer = new Timer();
        this.alarms = new HashMap();
        this.store = store;
        this.lifecycleAdapter = lifecycleAdapter;

        readAlarmsFromStore();
    }

    /**
     * Reads alarms from the persistent store and registers them.
     */
    private synchronized void readAlarmsFromStore() {
        store.listAlarms(new Store.AlarmsConsumer() {
            public void consume(
                    final int midletSuiteID, final Map suiteAlarms) {
                for (Iterator it = suiteAlarms.entrySet().iterator();
                                                            it.hasNext(); ) {
                    final Map.Entry entry = (Map.Entry) it.next();
                    final String midlet = (String) entry.getKey();
                    final Long time = (Long) entry.getValue();
                    scheduleAlarm(new MIDPApp(midletSuiteID, midlet),
                            time.longValue());
                }
            }
        });
    }

    /**
     * Registers an alarm.
     *
     * <p>
     * NOTE: <code>midletSuiteID</code> parameter should refer to a valid
     *  <code>MIDlet</code> suite and <code>midlet</code> should refer to
     *  valid <code>MIDlet</code> from the given suite. <code>timer</code>
     *  parameters is the same as for corresponding <code>Date</code>
     *  constructor.  No checks are performed and no guarantees are
     *  given if parameters are invalid.
     * </p>
     *
     * @param midletSuiteID <code>MIDlet suite</code> ID
     * @param midlet <code>MIDlet</code> class name
     * @param time alarm time
     *
     * @throws ConnectionNotFoundException if for any reason alarm cannot be
     *  registered
     *
     * @return previous alarm time or 0 if none
     */
    public synchronized long registerAlarm(
            final int midletSuiteID,
            final String midlet,
            final long time) throws ConnectionNotFoundException {
        final MIDPApp midpApp = new MIDPApp(midletSuiteID, midlet);

        final AlarmTask oldTask = (AlarmTask) alarms.get(midpApp);
        long oldTime = 0L;
        if (oldTask != null) {
            oldTime = oldTask.getScheduledAlarmTime();
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

        scheduleAlarm(midpApp, time);

        return oldTime;
    }

    /**
     * Removes alarms for the given suite.
     *
     * <p>
     * NOTE: <code>midletSuiteID</code> must refer to valid installed
     *  <code>MIDlet</code> suite.  However, it might refer to the
     *  suite without alarms.
     * </p>
     *
     * @param midletSuiteID ID of the suite to remove alarms for
     */
    public synchronized void removeSuiteAlarms(final int midletSuiteID) {
        for (Iterator it = alarms.entrySet().iterator(); it.hasNext(); ) {
            final Map.Entry entry = (Map.Entry) it.next();
            final MIDPApp midpApp = (MIDPApp) entry.getKey();
            if (midpApp.midletSuiteID == midletSuiteID) {
                // No need to care about retval
                ((AlarmTask) entry.getValue()).cancel();
                removeAlarmFromStore(midpApp);
                it.remove();
            }
        }
    }

    /**
     * Dumps alarms for the given suite.
     *
     * <p>
     * NOTE: <code>midletSuiteID</code> must refer to valid installed
     *  <code>MIDlet</code> suite.  However, it might refer to the
     *  suite without alarms.
     * </p>
     *
     * @param midletSuiteID ID of the suite to dump alarms for
     * @param store dump destination
     * @throws IOException if dump fails
     */
    public synchronized void dumpSuiteAlarms(
            final int midletSuiteID, final Store store) throws IOException {

        class AlarmsConsumer implements Store.AlarmsConsumer {
            private IOException ex;

            AlarmsConsumer() throws IOException {
                AlarmController.this.store.listAlarms(this);
                if (ex != null) {
                    throw ex;
                }
            }

            public void consume(final int suiteID, final Map suiteAlarms) {
                if (midletSuiteID != suiteID) {
                    return;
                }

                try {
                    for (Iterator it = suiteAlarms.entrySet().iterator();
                                                            it.hasNext(); ) {
                        final Map.Entry entry = (Map.Entry) it.next();
                        final String midlet = (String) entry.getKey();
                        final Long time = (Long) entry.getValue();

                        store.addAlarm(midletSuiteID, midlet, time.longValue());
                    }
                } catch (IOException e) {
                    logError(
                        "Failed to write alarm info to the persistent store: "
                        + e);
                    ex = e;
                }
            }
        }

        new AlarmsConsumer();
    }

    /**
     * Disposes an alarm controller.
     *
     * <p>
     * NOTE: This method is needed as <code>Timer</code> creates
     * non daemon thread which would prevent the app from exit.
     * </p>
     *
     * <p>
     * NOTE: after <code>AlarmController</code> is disposed, attempt to perform
     *  any alarms related activity on it leads to undefined behaviour.
     * </p>
     */
    public synchronized void dispose() {
        timer.cancel();
        for (Iterator it = alarms.values().iterator(); it.hasNext(); ) {
            final AlarmTask task = (AlarmTask) it.next();
            task.cancel();
        }
        alarms.clear();
    }

    /**
     * Special class that supports guaranteed canceling of TimerTasks.
     */
    private class AlarmTask extends TimerTask {
        /** <code>MIDlet</code> to run. */
        private final MIDPApp midpApp;

        /** Cancelation status. */
        private boolean cancelled = false;

        /** Scheduled execution time. */
        private long scheduledTime;

        /**
         * Creates a new instance, originally not cancelled.
         *
         * @param midpApp <code>MIDlet</code> to create task for
         * @param scheduledTime scheduled time
         */
        AlarmTask(final MIDPApp midpApp, long scheduledTime) {
            this.midpApp = midpApp;
            this.scheduledTime = scheduledTime;
        }

        /**
         * Returns scheduled time.
         * This is different from what #scheduledExecutionTime() returns
         * as #scheduledExecutionTime() returns <code>0</code>
         * for not scheduled tasks.
         */
        long getScheduledAlarmTime() {
            return scheduledTime;
        }

        /** {@inheritDoc} */
        public void run() {
            synchronized (AlarmController.this) {
                if (cancelled) {
                    return;
                }

                try {
                    lifecycleAdapter.launchMidlet(midpApp.midletSuiteID,
                            midpApp.midlet);
                    removeAlarmFromStore(midpApp);
                } catch (Exception ex) {
                    /*
                     * IMPL_NOTE: need to handle _all_ the exceptions
                     * as otherwise Timer thread gets stuck and alarms
                     * cannot be scheduled anymore.
                     */
                    /*
            * TBD: uncomment when logging can be disabled
                     * (not to interfer with unittests)
            * logError(
                     *       "Failed to launch \"" + midpApp.midlet + "\"" +
                     *       " (suite ID: " + midpApp.midletSuiteID + "): " +
                     *       ex);
                     */
                }
            }
        }

        /** {@inheritDoc} */
        public boolean cancel() {
            cancelled = true;
            return super.cancel();
        }
    }

    /**
     * Scheduleds an alarm.
     *
     * @param midpApp application to register alarm for
     * @param time alarm time
     */
    private void scheduleAlarm(final MIDPApp midpApp, final long time) {
        final Date date = new Date(time);
        final AlarmTask newTask = new AlarmTask(midpApp, date.getTime());
        alarms.put(midpApp, newTask);
        try {
            timer.schedule(newTask, date);
        } catch (IllegalArgumentException e) {
            // Timer javadoc:
            //  throws IllegalArgumentException if time.getTime() is negative

            // register alarm but don't schedule it for execution to pass TCK.
        }
        /*
         * RFC: according to <code>Timer</code> spec, <quote>if the time is in
         * the past, the task is scheduled for immediate execution</quote>.
         * I hope it's MIDP complaint
         */
    }

    /**
     * Removes an alarm from persistent store.
     *
     * @param midpApp application to remove alarm for
     */
    private void removeAlarmFromStore(final MIDPApp midpApp) {
        try {
            store.removeAlarm(midpApp.midletSuiteID, midpApp.midlet);
        } catch (IOException ioex) {
            logError("Failed to remove alarm info from the persistent store: "
                    + ioex);
        }
    }

    /**
     * Logs error message.
     *
     * <p>
     * TBD: common logging
     * </p>
     *
     * @param message message to log
     */
    private static void logError(final String message) {
        System.err.println("ERROR [" + AlarmController.class.getName() + "]: "
                + message);
    }
}

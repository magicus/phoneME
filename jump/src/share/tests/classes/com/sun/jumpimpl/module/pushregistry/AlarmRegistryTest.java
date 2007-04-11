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

import com.sun.jump.module.contentstore.InMemoryContentStore;
import com.sun.jumpimpl.module.pushregistry.persistence.StoreUtils;
import junit.framework.*;
import com.sun.jumpimpl.module.pushregistry.persistence.Store;
import java.io.IOException;
import javax.microedition.io.ConnectionNotFoundException;

public final class AlarmRegistryTest extends TestCase {

    public AlarmRegistryTest(String testName) {
        super(testName);
    }

    private static Store createStore() throws IOException {
        return StoreUtils.createInMemoryPushStore();
    }

    private static class DummyLauncher implements AlarmRegistry.LifecycleAdapter {
        public void launchMidlet(final int midletSuiteID, final String midlet) { }

        static AlarmRegistry createAlarmRegistry() throws IOException {
            return new AlarmRegistry(createStore(), new DummyLauncher());
        }
    }

    private static long registerAlarmWithDelta(
            final AlarmRegistry alarmRegistry,
            final int midletSuiteID,
            final String midlet,
            final long delta) throws ConnectionNotFoundException {
        return alarmRegistry.registerAlarm(midletSuiteID, midlet, System.currentTimeMillis() + delta);
    }

    public void testFirstRegistration() throws IOException {
        final int DUMMY_SUITE_ID = 13;
        final String DUMMY_MIDLET = "foo.bar.Dummy";

        final AlarmRegistry alarmRegistry = DummyLauncher.createAlarmRegistry();
        final long previous = alarmRegistry.registerAlarm(DUMMY_SUITE_ID, DUMMY_MIDLET, 239L);
        alarmRegistry.dispose();

        assertEquals(0L, previous);
    }

    public void testSuiteWithNoAlarmsUninstallingA() throws IOException {
        final int DUMMY_SUITE_ID = 13;

        final AlarmRegistry alarmRegistry = DummyLauncher.createAlarmRegistry();
        alarmRegistry.removeSuiteAlarms(DUMMY_SUITE_ID);
        alarmRegistry.dispose();
    }

    public void testSuiteWithNoAlarmsUninstallingB() throws IOException {
        final int DUMMY_SUITE_ID = 13;
        final String DUMMY_MIDLET = "foo.bar.Dummy";

        final int ANOTHER_SUITE_ID = 17; // should be different from DUMMY_MIDLET
        assert DUMMY_SUITE_ID != ANOTHER_SUITE_ID;

        final AlarmRegistry alarmRegistry = DummyLauncher.createAlarmRegistry();
        alarmRegistry.registerAlarm(DUMMY_SUITE_ID, DUMMY_MIDLET, 239L);
        alarmRegistry.removeSuiteAlarms(ANOTHER_SUITE_ID);
        alarmRegistry.dispose();
    }

    private static class State {
        boolean hasBeenFired = false;
        boolean secondInvocation = false;
        boolean rightSuiteID = false;
        boolean rightMidlet = false;
    }

    public void testAlarmFired() throws IOException, InterruptedException {
        final long ALARM_DELTA = 101L; // in ms
        final long WAIT_DELAY = 3*ALARM_DELTA;

        final int MIDLET_SUITE_ID = 17;
        final String MIDLET = "com.sun.Foo";

        final State state = new State();
        final AlarmRegistry alarmRegistry = new AlarmRegistry(createStore(), new AlarmRegistry.LifecycleAdapter() {
           public void launchMidlet(final int midletSuiteID, final String midlet) {
               synchronized (state) {
                   state.secondInvocation = state.hasBeenFired;
                   state.hasBeenFired = true;
                   state.rightSuiteID = (midletSuiteID == MIDLET_SUITE_ID);
                   state.rightMidlet = midlet.equals(MIDLET);
               }
           }
        });

        registerAlarmWithDelta(alarmRegistry, MIDLET_SUITE_ID, MIDLET, ALARM_DELTA);
        Thread.sleep(WAIT_DELAY);

        alarmRegistry.dispose();

        assertTrue(state.hasBeenFired);
        assertFalse(state.secondInvocation);
        assertTrue(state.rightSuiteID);
        assertTrue(state.rightMidlet);
    }

    private static class FiredChecker {
        boolean hasBeenFired = false;
        final AlarmRegistry.LifecycleAdapter lifecycleAdapter;

        FiredChecker(final int midletSuiteID, final String midlet) {
            this.lifecycleAdapter = new AlarmRegistry.LifecycleAdapter() {
                public void launchMidlet(final int id, final String m) {
                    if ((midletSuiteID == id) && (midlet.equals(m))) {
                        hasBeenFired = true;
                    }
                }
            };
        }

        AlarmRegistry createAlarmRegistry() throws IOException {
            return new AlarmRegistry(createStore(), lifecycleAdapter);
        }
    }

    public void testSecondRegistration() throws IOException {
        final int MIDLET_SUITE_ID = 17;
        final String MIDLET = "com.sun.Foo";

        final FiredChecker firedChecker = new FiredChecker(MIDLET_SUITE_ID, MIDLET);

        final AlarmRegistry alarmRegistry = firedChecker.createAlarmRegistry();

        /*
         * IMPL_NOTE: ALARM_TIME below must be big enough for alarm not
         * to fire before second registration
         */
        final long ALARM_TIME = System.currentTimeMillis() + 10239L;
        alarmRegistry.registerAlarm(MIDLET_SUITE_ID, MIDLET, ALARM_TIME);

        final long previous = alarmRegistry.registerAlarm(MIDLET_SUITE_ID, MIDLET, 2*ALARM_TIME);
        if (firedChecker.hasBeenFired) {
            fail("Test is not reliable: the alarm has been fired.  Please, increase ALARM_TIME");
        }
        alarmRegistry.dispose();

        assertEquals(ALARM_TIME, previous);
    }

    public void testUninstall() throws IOException, InterruptedException {
        /*
         * ALARM_DELTA should be big enough for removeSuiteAlarms to finish
         */
        final long ALARM_DELTA = 1001L; // in ms
        final long WAIT_DELAY = 3*ALARM_DELTA;

        final int MIDLET_SUITE_ID = 17;
        final String MIDLET = "com.sun.Foo";

        final FiredChecker firedChecker = new FiredChecker(MIDLET_SUITE_ID, MIDLET);

        final AlarmRegistry alarmRegistry = firedChecker.createAlarmRegistry();

        registerAlarmWithDelta(alarmRegistry, MIDLET_SUITE_ID, MIDLET, ALARM_DELTA);
        alarmRegistry.removeSuiteAlarms(MIDLET_SUITE_ID);
        if (firedChecker.hasBeenFired) {
            fail("Test is not reliable: the alarm has been fired.  Please, increase ALARM_DELTA");
        }
        Thread.sleep(WAIT_DELAY);

        alarmRegistry.dispose();

        assertFalse(firedChecker.hasBeenFired);
    }

    public void testResetAfterFiring() throws IOException, InterruptedException {
        final long ALARM_DELTA = 101L; // in ms
        final long WAIT_DELAY = 3*ALARM_DELTA;
        final long BIG_DELTA = 10001L; // in ms

        final int MIDLET_SUITE_ID = 17;
        final String MIDLET = "com.sun.Foo";

        final FiredChecker firedChecker = new FiredChecker(MIDLET_SUITE_ID, MIDLET);

        final AlarmRegistry alarmRegistry = firedChecker.createAlarmRegistry();

        registerAlarmWithDelta(alarmRegistry, MIDLET_SUITE_ID, MIDLET, ALARM_DELTA);
        Thread.sleep(WAIT_DELAY);
        if (!firedChecker.hasBeenFired) {
            fail("Test is not reliable: the alarm hasn't been fired");
        }

        final long previous = registerAlarmWithDelta(alarmRegistry, MIDLET_SUITE_ID, MIDLET, BIG_DELTA);

        alarmRegistry.dispose();

        assertEquals(0L, previous);
    }

    public void testCoupleMidletFirstRegistration() throws IOException {
        final long ALARM_DELTA = 1001L; // in ms

        final int MIDLET_SUITE_ID = 17;
        final String MIDLET_1 = "com.sun.Foo";
        final String MIDLET_2 = "com.sun.Bar";

        final FiredChecker firedChecker = new FiredChecker(MIDLET_SUITE_ID, MIDLET_1);
        final AlarmRegistry alarmRegistry = firedChecker.createAlarmRegistry();

        registerAlarmWithDelta(alarmRegistry, MIDLET_SUITE_ID, MIDLET_1, ALARM_DELTA);
        final long previous = registerAlarmWithDelta(alarmRegistry, MIDLET_SUITE_ID, MIDLET_2, 2*ALARM_DELTA);
        if (firedChecker.hasBeenFired) {
            fail("Test is not reliable: the alarm has been fired.  Please, increase ALARM_DELTA");
        }

        alarmRegistry.dispose();

        assertEquals(0L, previous);
    }
}

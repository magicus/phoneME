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

package com.sun.midp.push.controller;

import com.sun.midp.push.persistence.StoreUtils;
import junit.framework.*;
import com.sun.midp.push.persistence.Store;
import java.io.IOException;
import java.util.Map;
import javax.microedition.io.ConnectionNotFoundException;

public final class AlarmControllerTest extends TestCase {

    public AlarmControllerTest(String testName) {
        super(testName);
    }

    private static Store createStore() throws IOException {
        return StoreUtils.getInstance().getStore();
    }

    private static AlarmController createAlarmController(
            final Store store, final LifecycleAdapter lifecycleAdapter)
                throws IOException {
        return new AlarmController(store, lifecycleAdapter);
    }

    private static AlarmController createAlarmController(
            final LifecycleAdapter lifecycleAdapter)
                throws IOException {
        return createAlarmController(createStore(), lifecycleAdapter);
    }

    private static AlarmController createAlarmController(final Store store)
            throws IOException {
        return createAlarmController(store, new ListingLifecycleAdapter());
    }

    private static AlarmController createAlarmController()
            throws IOException {
        return createAlarmController(createStore());
    }

    private static long registerAlarmWithDelta(
            final AlarmController alarmController,
            final int midletSuiteID,
            final String midlet,
            final long delta) throws ConnectionNotFoundException {
        return alarmController.registerAlarm(
                midletSuiteID, midlet, System.currentTimeMillis() + delta);
    }

    public void testFirstRegistration() throws IOException {
        final int DUMMY_SUITE_ID = 13;
        final String DUMMY_MIDLET = "foo.bar.Dummy";

        final AlarmController alarmController = createAlarmController();
        final long previous = alarmController
            .registerAlarm(DUMMY_SUITE_ID, DUMMY_MIDLET, 239L);
        alarmController.dispose();

        assertEquals(0L, previous);
    }

    public void testSuiteWithNoAlarmsUninstallingA() throws IOException {
        final int DUMMY_SUITE_ID = 13;

        final AlarmController alarmController = createAlarmController();
        alarmController.removeSuiteAlarms(DUMMY_SUITE_ID);
        alarmController.dispose();
    }

    public void testSuiteWithNoAlarmsUninstallingB() throws IOException {
        final int DUMMY_SUITE_ID = 13;
        final String DUMMY_MIDLET = "foo.bar.Dummy";

	// should be different from DUMMY_MIDLET
        final int ANOTHER_SUITE_ID = 17; 
        assert DUMMY_SUITE_ID != ANOTHER_SUITE_ID;

        final AlarmController alarmController = createAlarmController();
        alarmController.registerAlarm(DUMMY_SUITE_ID, DUMMY_MIDLET, 239L);
        alarmController.removeSuiteAlarms(ANOTHER_SUITE_ID);
        alarmController.dispose();
    }

    public void testSuiteWithSeveralMidletsUninstall() throws IOException {
        final int MIDLET_SUITE_ID = 17;
        final String MIDLET_1 = "com.sun.Foo";
        final String MIDLET_2 = "com.sun.Bar";
        final String MIDLET_3 = "com.sun.Qux";

        final AlarmController alarmController = createAlarmController();

        registerAlarmWithDelta(alarmController, MIDLET_SUITE_ID, MIDLET_1,
			       1001L);
        registerAlarmWithDelta(alarmController, MIDLET_SUITE_ID, MIDLET_2,
			       2001L);
        registerAlarmWithDelta(alarmController, MIDLET_SUITE_ID, MIDLET_3,
			       3001L);
        alarmController.removeSuiteAlarms(MIDLET_SUITE_ID);
        alarmController.dispose();
    }

    public void testAlarmFired() throws IOException, InterruptedException {
        final long ALARM_DELTA = 101L; // in ms
        final long WAIT_DELAY = 3*ALARM_DELTA;

        final int MIDLET_SUITE_ID = 17;
        final String MIDLET = "com.sun.Foo";

        final ListingLifecycleAdapter lifecycleAdapter =
	    new ListingLifecycleAdapter();

        final AlarmController alarmController =
	    createAlarmController(lifecycleAdapter);

        registerAlarmWithDelta(alarmController, MIDLET_SUITE_ID, MIDLET,
			       ALARM_DELTA);
        Thread.sleep(WAIT_DELAY);
        alarmController.dispose();

        assertTrue(lifecycleAdapter.hasBeenInvokedOnceFor(MIDLET_SUITE_ID,
							  MIDLET));
    }

    private static class FiredChecker {
        final ListingLifecycleAdapter lifecycleAdapter;
        final int midletSuiteID;
        final String midlet;
        final AlarmController alarmController;

        FiredChecker(final int midletSuiteID, final String midlet)
                throws IOException {
            this.midletSuiteID = midletSuiteID;
            this.midlet = midlet;
            this.lifecycleAdapter = new ListingLifecycleAdapter();
            this.alarmController = createAlarmController(lifecycleAdapter);
        }

        FiredChecker(final Store store,
                final int midletSuiteID, final String midlet)
	    throws IOException {
            this.midletSuiteID = midletSuiteID;
            this.midlet = midlet;
            this.lifecycleAdapter = new ListingLifecycleAdapter();
            this.alarmController = createAlarmController(store,
							 lifecycleAdapter);
        }

        long registerCheckedAlarm(final long delta)
                throws ConnectionNotFoundException {
            return registerAlarmWithDelta(
                    alarmController, midletSuiteID, midlet, delta);
        }

        boolean hasBeenFired() {
            return lifecycleAdapter.hasBeenInvokedOnceFor(midletSuiteID,
							  midlet);
        }
    }

    public void testSecondRegistration() throws IOException {
        final int MIDLET_SUITE_ID = 17;
        final String MIDLET = "com.sun.Foo";

        final FiredChecker firedChecker = new FiredChecker(MIDLET_SUITE_ID,
							   MIDLET);

        final AlarmController alarmController = firedChecker.alarmController;

        /*
         * IMPL_NOTE: ALARM_TIME below must be big enough for alarm not
         * to fire before second registration
         */
        final long ALARM_TIME = System.currentTimeMillis() + 10239L;
        alarmController.registerAlarm(MIDLET_SUITE_ID, MIDLET, ALARM_TIME);

        final long previous = alarmController
	    .registerAlarm(MIDLET_SUITE_ID,
			   MIDLET, 2*ALARM_TIME);
        if (firedChecker.hasBeenFired()) {
            fail("Test is not reliable: the alarm has been fired." +
		 "  Please, increase ALARM_TIME");
        }
        alarmController.dispose();

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

        final FiredChecker firedChecker = new FiredChecker(MIDLET_SUITE_ID,
							   MIDLET);

        final AlarmController alarmController = firedChecker.alarmController;

        registerAlarmWithDelta(alarmController, MIDLET_SUITE_ID, MIDLET,
			       ALARM_DELTA);
        alarmController.removeSuiteAlarms(MIDLET_SUITE_ID);
        if (firedChecker.hasBeenFired()) {
            fail("Test is not reliable: the alarm has been fired." +
		 "  Please, increase ALARM_DELTA");
        }
        Thread.sleep(WAIT_DELAY);

        alarmController.dispose();

        assertFalse(firedChecker.hasBeenFired());
    }

    private void registerAndWait(final FiredChecker firedChecker)
                throws ConnectionNotFoundException, InterruptedException {
        final long ALARM_DELTA = 101L; // in ms
        final long WAIT_DELAY = 3*ALARM_DELTA;

        firedChecker.registerCheckedAlarm(ALARM_DELTA);
        Thread.sleep(WAIT_DELAY);
        if (!firedChecker.hasBeenFired()) {
            fail("Test is not reliable: the alarm hasn't been fired");
        }
    }

    public void testResetAfterFiring() throws IOException,
					      InterruptedException {
        final FiredChecker firedChecker = new FiredChecker(17, "com.sun.Foo");

        registerAndWait(firedChecker);
        final long previous = firedChecker.registerCheckedAlarm(10001L);
        firedChecker.alarmController.dispose();

        assertEquals(0L, previous);
    }

    private static boolean checkNoAlarms(final Store store) {
        final boolean noAlarms [] = { true };
        store.listAlarms(new Store.AlarmsConsumer() {
		public void consume(final int suiteId, final Map alarms) {
		    /*
		     * NOTE: store shouldn't report suites without alarms
		     * (empty map)
		     */
		    noAlarms[0] = false;
		}
	    });
        return noAlarms[0];
    }
    
    public void testNoRecordAfterFiring() throws IOException,
						 InterruptedException {
        final Store store = createStore();
        final FiredChecker firedChecker = new FiredChecker(store, 17,
							   "com.sun.Foo");

        registerAndWait(firedChecker);
        firedChecker.alarmController.dispose();

        assertTrue(checkNoAlarms(store));
    }

    public void testNotFiredStored() throws IOException, InterruptedException {
        final int MIDLET_SUITE_ID = 17;
        final String MIDLET = "com.sun.Foo";

        final Store store = createStore();
        final FiredChecker firedChecker = new FiredChecker(store,
							   MIDLET_SUITE_ID,
							   MIDLET);

        final long time = System.currentTimeMillis() + 1001L;
        firedChecker.alarmController.registerAlarm(MIDLET_SUITE_ID, MIDLET,
						   time);
        firedChecker.alarmController.dispose();
        if (firedChecker.hasBeenFired()) {
            fail("Test is not reliable: the alarm has been fired");
        }

        final boolean alarmPresent [] = { false };
        final boolean otherAlarmsPresent [] = { false };
        store.listAlarms(new Store.AlarmsConsumer() {
           public void consume(final int suiteId, final Map alarms) {
               if (suiteId != MIDLET_SUITE_ID) {
                   otherAlarmsPresent[0] = true;
                   return;
               }
               if (alarms.size() > 1) {
                   otherAlarmsPresent[0] = true;
                   return;
               }
               final Long t = (Long) alarms.get(MIDLET);
               if ((t == null) || (t.longValue() != time)) {
                   otherAlarmsPresent[0] = true;
                   return;
               }
               alarmPresent[0] = true;
           }
        });
        assertTrue(alarmPresent[0]);
        assertFalse(otherAlarmsPresent[0]);
    }

    public void testAlarmRecordRead() throws IOException, InterruptedException {
        final int MIDLET_SUITE_ID = 17;
        final String MIDLET = "com.sun.Foo";

        final Store store = createStore();
        final long time = System.currentTimeMillis() + 501L;
        store.addAlarm(MIDLET_SUITE_ID, MIDLET, time);

        final FiredChecker firedChecker = new FiredChecker(store, 
							   MIDLET_SUITE_ID,
							   MIDLET);

        Thread.sleep(3*(time - System.currentTimeMillis()));
        firedChecker.alarmController.dispose();
        assertTrue(firedChecker.hasBeenFired());
    }

    public void testPassedAlarmImmediatelyScheduled()
	throws IOException, InterruptedException {
        final int MIDLET_SUITE_ID = 17;
        final String MIDLET = "com.sun.Foo";

        final Store store = createStore();
        final long time = System.currentTimeMillis() - 501L;
        store.addAlarm(MIDLET_SUITE_ID, MIDLET, time);

        final FiredChecker firedChecker = new FiredChecker(store,
							   MIDLET_SUITE_ID,
							   MIDLET);

        Thread.sleep(101L); // just a small delta
        firedChecker.alarmController.dispose();
        assertTrue(firedChecker.hasBeenFired());
    }

    public void testSuiteUninstallPersistentStore() throws IOException {
        final int MIDLET_SUITE_ID = 17;
        final String MIDLET_1 = "com.sun.Foo";
        final String MIDLET_2 = "com.sun.Bar";

        final Store store = createStore();
        final AlarmController alarmController = createAlarmController(store);

        registerAlarmWithDelta(alarmController, MIDLET_SUITE_ID, MIDLET_1,
			       1001L);
        registerAlarmWithDelta(alarmController, MIDLET_SUITE_ID, MIDLET_2,
			       2001L);
        alarmController.removeSuiteAlarms(MIDLET_SUITE_ID);
        alarmController.dispose();

        assertTrue(checkNoAlarms(store));
    }

    public void testSuiteUninstallPersistentStore2()
	throws IOException, InterruptedException {
        final int MIDLET_SUITE_ID_1 = 17;
        final String MIDLET_11 = "com.sun.Foo";
        final long DELTA = 47L;
        final String MIDLET_12 = "com.sun.Bar";

        final int MIDLET_SUITE_ID_2 = 13;
        final String MIDLET_21 = "com.sun.Qux";

        final Store store = createStore();
        final FiredChecker firedChecker = new FiredChecker(store,
							   MIDLET_SUITE_ID_1,
							   MIDLET_11);

        registerAlarmWithDelta(firedChecker.alarmController,
			       MIDLET_SUITE_ID_1, MIDLET_11, DELTA);
        registerAlarmWithDelta(firedChecker.alarmController, 
			       MIDLET_SUITE_ID_2, MIDLET_21, 3001L);
        registerAlarmWithDelta(firedChecker.alarmController,
			       MIDLET_SUITE_ID_1, MIDLET_12, 2001L);
        Thread.sleep(3*DELTA);
        firedChecker.alarmController.dispose();
        if (!firedChecker.hasBeenFired()) {
            fail("Test is not reliable: the alarm hasn't been fired");
        }

        final boolean [] suite1ok = { false };
        final boolean [] suite2ok = { false };
        final boolean [] noOthers = { true };
        store.listAlarms(new Store.AlarmsConsumer() {
           public void consume(final int suiteId, final Map alarms) {
               switch (suiteId) {
                   case MIDLET_SUITE_ID_1:
                       if (alarms.size() > 1) {
                           noOthers[0] = false;
                       } else if (alarms.containsKey(MIDLET_12)) {
                           suite1ok[0] = true;
                       }
                       break;

                   case MIDLET_SUITE_ID_2:
                       if (alarms.size() > 1) {
                           noOthers[0] = false;
                       } else if (alarms.containsKey(MIDLET_21)) {
                           suite2ok[0] = true;
                       }
                       break;

                   default:
                       noOthers[0] = false;
               }
           }
        });
        assertTrue("Suite 1 is incorrect", suite1ok[0]);
        assertTrue("Suite 2 is incorrect", suite2ok[0]);
        assertTrue("Unexpected registrations", noOthers[0]);
    }

    public void testCoupleMidletFirstRegistration() throws IOException {
        final long ALARM_DELTA = 1001L; // in ms

        final int MIDLET_SUITE_ID = 17;
        final String MIDLET_1 = "com.sun.Foo";
        final String MIDLET_2 = "com.sun.Bar";

        final FiredChecker firedChecker = new FiredChecker(MIDLET_SUITE_ID,
							   MIDLET_1);
        final AlarmController alarmController = firedChecker.alarmController;

        registerAlarmWithDelta(alarmController, MIDLET_SUITE_ID, MIDLET_1,
			       ALARM_DELTA);
        final long previous = registerAlarmWithDelta(alarmController, 
						     MIDLET_SUITE_ID,
						     MIDLET_2, 2*ALARM_DELTA);
        if (firedChecker.hasBeenFired()) {
            fail("Test is not reliable: the alarm has been fired." +
		 "  Please, increase ALARM_DELTA");
        }

        alarmController.dispose();

        assertEquals(0L, previous);
    }

    public void testThrowingLifecycleAdapter()
            throws IOException, InterruptedException {
        final long ALARM_DELTA = 101L; // in ms
        final long WAIT_DELAY = 3*ALARM_DELTA;

        final int MIDLET_SUITE_ID = 17;
        final String MIDLET = "com.sun.Foo";

        final ListingLifecycleAdapter listingLifecycleAdapter =
                new ListingLifecycleAdapter();

        final LifecycleAdapter throwingLifecycleAdapter =
                new ThrowingLifecycleAdapter();

        final ProxyLifecycleAdapter lifecycleAdapter =
                new ProxyLifecycleAdapter();

        final AlarmController alarmController =
                createAlarmController(lifecycleAdapter);

        lifecycleAdapter.setProxy(throwingLifecycleAdapter);
        registerAlarmWithDelta(alarmController, MIDLET_SUITE_ID,
			       MIDLET, ALARM_DELTA);
        Thread.sleep(WAIT_DELAY);

        // And now check that everything is ok despite of previously 
	// thrown adapter
        lifecycleAdapter.setProxy(listingLifecycleAdapter);
        registerAlarmWithDelta(alarmController, MIDLET_SUITE_ID,
			       MIDLET, ALARM_DELTA);
        Thread.sleep(WAIT_DELAY);
        alarmController.dispose();

        assertTrue(listingLifecycleAdapter
		   .hasBeenInvokedOnceFor(MIDLET_SUITE_ID,
					  MIDLET));
    }
}

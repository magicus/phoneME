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

package com.sun.midp.main;

import com.sun.midp.events.EventQueue;
import com.sun.midp.events.ListenerTestEventQueue;

import com.sun.midp.i3test.TestCase;

import com.sun.midp.security.SecurityToken;

import java.util.Random;

/**
 * Unit tests for the MIDletControllerEventProducer class. This class, and
 * event producer classes in general, don't have much logic. However, they do
 * implement the mapping between specific data and generic event fields (e.g.,
 * intParam1 or stringParam2) which is important to test.
 */
public class TestMIDletControllerEventProducer extends TestCase
    implements MIDletControllerEventConsumer {

    private SecurityToken token;

    Random rand = new Random();

    // Constant test data.

    static final int SUITE_ID = 10000;
    static final String CLASS_NAME = "the midlet class name";
    static final String DISPLAY_NAME = "the display-name of the midlet";
    static final int TARGET_SUITE_ID = 10001;
    static final String TARGET_CLASS_NAME = "the target midlet's class name";
    static final String ERROR_DETAILES = "the error details";

    // The following instance variables comprise the test fixture.
    // They are freshly initialized before each test is run.

    int currentIsolateId;
    int amsIsolateId;
    int displayId;
    int displayId2;
    int externalId;
    int errorCode;

    EventQueue queue;
    MIDletControllerEventProducer producer;
    MIDletControllerEventListener listener;

    /**
     * Runs all tests. If a test throw a NullPointerException,
     * the most likely cause is that the listener has not registered with
     * the event queue for that event type.
     */
    public void runTests() throws Throwable {
        token = getSecurityToken();

        setUp();

        declare("testMIDletStartErrorEvent");
        testMIDletStartErrorEvent();

        declare("testMIDletCreateNotifyEvent");
        testMIDletCreateNotifyEvent();

        declare("testMIDletActiveNotifyEvent");
        testMIDletActiveNotifyEvent();

        declare("testMIDletPauseNotifyEvent");
        testMIDletPauseNotifyEvent();

        declare("testMIDletDestroyNotifyEvent");
        testMIDletDestroyNotifyEvent();

        declare("testMIDletResumeRequest");
        testMIDletResumeRequest();

        declare("testMIDletDestroyRequestEvent");
        testMIDletDestroyRequestEvent();

        declare("testMIDletForegroundTransferEvent");
        testMIDletForegroundTransferEvent();

        declare("testDisplayCreateNotifyEvent");
        testDisplayCreateNotifyEvent();

        declare("testDisplayForegroundRequestEvent");
        testDisplayForegroundRequestEvent();

        declare("testDisplayBackgroundRequestEvent");
        testDisplayBackgroundRequestEvent();

        declare("testDisplayPreemptEvents");
        testDisplayPreemptEvents();

        tearDown();
    }

    /**
     * Initializes the test fixture with random data, creates the stub event
     * queue, and creates the MIDletControllerEventProducer under test.
     */
    void setUp() {
        currentIsolateId = rand.nextInt();
        amsIsolateId = rand.nextInt();
        displayId = rand.nextInt();
        displayId2 = rand.nextInt();
        externalId = rand.nextInt();
        errorCode = rand.nextInt();

        queue = new ListenerTestEventQueue();
        producer = new MIDletControllerEventProducer(queue,
            amsIsolateId, currentIsolateId);
        listener = new MIDletControllerEventListener(queue, this);
    }

    /**
     * Nulls out the stub event queue and the event producer.
     */
    void tearDown() {
        queue = null;
        producer = null;
    }
    // the actual tests

    /**
     * Tests sendMIDletStartErrorEvent().
     */
    void testMIDletStartErrorEvent() {
        producer.sendMIDletStartErrorEvent(SUITE_ID, CLASS_NAME, externalId,
                                           errorCode, ERROR_DETAILES);
    }

    /**
     * Tests sendMIDletCreateNotifyEvent().
     */
    void testMIDletCreateNotifyEvent() {
        producer.sendMIDletCreateNotifyEvent(SUITE_ID, CLASS_NAME,
                                             externalId, DISPLAY_NAME);
    }

    /**
     * Tests sendMIDletActiveNotifyEvent().
     */
    void testMIDletActiveNotifyEvent() {
        producer.sendMIDletActiveNotifyEvent(SUITE_ID, CLASS_NAME);
    }

    /**
     * Tests sendMIDletPauseNotifyEvent().
     */
    void testMIDletPauseNotifyEvent() {
        producer.sendMIDletPauseNotifyEvent(SUITE_ID, CLASS_NAME);
    }

    /**
     * Tests sendMIDletDestroyNotifyEvent().
     */
    void testMIDletDestroyNotifyEvent() {
        producer.sendMIDletDestroyNotifyEvent(SUITE_ID, CLASS_NAME);
    }

    /**
     * Tests sendMIDletResumeRequest().
     */
    void testMIDletResumeRequest() {
        producer.sendMIDletResumeRequest(SUITE_ID, CLASS_NAME);
    }

    /**
     * Tests sendMIDletDestroyRequestEvent().
     */
    void testMIDletDestroyRequestEvent() {
        producer.sendMIDletDestroyRequestEvent(displayId);
    }

    /**
     * Tests sendMIDletForegroundTransferEvent().
     */
    void testMIDletForegroundTransferEvent() {
        producer.sendMIDletForegroundTransferEvent(
            SUITE_ID, CLASS_NAME, TARGET_SUITE_ID, TARGET_CLASS_NAME);
    }

    /**
     * Tests sendDisplayCreateNotifyEvent().
     */
    void testDisplayCreateNotifyEvent() {
        producer.sendDisplayCreateNotifyEvent(displayId, CLASS_NAME);
    }

    /**
     * Tests sendDisplayForegroundRequestEvent().
     */
    void testDisplayForegroundRequestEvent() {
        producer.sendDisplayForegroundRequestEvent(displayId, true);
    }

    /**
     * Tests sendDisplayBackgroundRequestEvent().
     */
    void testDisplayBackgroundRequestEvent() {
        producer.sendDisplayBackgroundRequestEvent(displayId);
    }

    /**
     * Tests sendDisplayPreemptStartEvent() and
     * sendDisplayPreemptStopEvent().
     */
    void testDisplayPreemptEvents() {
        producer.sendDisplayPreemptStartEvent(displayId);
        producer.sendDisplayPreemptStopEvent(displayId2);
    }

    /**
     * Process a MIDlet start error event.
     * Notify from last to first added to allow the listener to
     * remove itself without causing a missed notification.
     *
     * MIDletControllerEventConsumer I/F method.
     *
     * @param midletSuiteId ID of the MIDlet suite
     * @param midletClassName Class name of the MIDlet
     * @param midletExternalAppId ID of given by an external application
     *                            manager
     * @param error start error code
     * @param details start error details
     */
    public void handleMIDletStartErrorEvent(
        int midletSuiteId,
        String midletClassName,
        int midletExternalAppId,
        int error,
        String details) {

        assertEquals(SUITE_ID, midletSuiteId);
        assertEquals(CLASS_NAME, midletClassName);
        assertEquals(externalId, midletExternalAppId);
        assertEquals(errorCode, error);
        assertEquals(ERROR_DETAILES, details);
    }

    /**
     * Process a MIDlet created notification.
     * MIDletControllerEventConsumer I/F method.
     *
     * @param midletSuiteId ID of the MIDlet suite
     * @param midletClassName Class name of the MIDlet
     * @param midletIsolateId isolate ID of the sending MIDlet
     * @param midletExternalAppId ID of given by an external application
     *                            manager
     * @param midletDisplayName name to show the user
     */
    public void handleMIDletCreateNotifyEvent(
        int midletSuiteId,
        String midletClassName,
        int midletIsolateId,
        int midletExternalAppId,
        String midletDisplayName) {

        assertEquals(SUITE_ID, midletSuiteId);
        assertEquals(CLASS_NAME, midletClassName);
        assertEquals(currentIsolateId, midletIsolateId);
        assertEquals(externalId, midletExternalAppId);
        assertEquals(DISPLAY_NAME, midletDisplayName);
    }

    /**
     * Process a MIDlet active notification
     * MIDletControllerEventConsumer I/F method.
     *
     * TBD: param midletProxy proxy with information about MIDlet
     *
     * @param midletSuiteId ID of the MIDlet suite
     * @param midletClassName Class name of the MIDlet
     */
    public void handleMIDletActiveNotifyEvent(
        int midletSuiteId,
        String midletClassName) {

        assertEquals(SUITE_ID, midletSuiteId);
        assertEquals(CLASS_NAME, midletClassName);
    }

    /**
     * Process a MIDlet paused notification.
     * MIDletControllerEventConsumer I/F method.
     *
     * TBD: param midletProxy proxy with information about MIDlet
     *
     * @param midletSuiteId ID of the MIDlet suite
     * @param midletClassName Class name of the MIDlet
     */
    public void handleMIDletPauseNotifyEvent(
        int midletSuiteId,
        String midletClassName) {

        assertEquals(SUITE_ID, midletSuiteId);
        assertEquals(CLASS_NAME, midletClassName);
    }

    /**
     * Process a MIDlet destroyed event.
     * MIDletControllerEventConsumer I/F method.
     *
     * @param midletSuiteId ID of the MIDlet suite
     * @param midletClassName Class name of the MIDlet
     */
    public void handleMIDletDestroyNotifyEvent(
        int midletSuiteId,
        String midletClassName) {

        assertEquals(SUITE_ID, midletSuiteId);
        assertEquals(CLASS_NAME, midletClassName);
    }


    /**
     * Processes a MIDLET_RESUME_REQUEST event.
     *
     * MIDletControllerEventConsumer I/F method.
     *
     * @param midletSuiteId ID of the MIDlet suite
     * @param midletClassName Class name of the MIDlet
     */
    public void handleMIDletResumeRequestEvent(int midletSuiteId,
                                               String midletClassName) {

        assertEquals(SUITE_ID, midletSuiteId);
        assertEquals(CLASS_NAME, midletClassName);
    }

    /**
     * Handles notification event of MIDlet resources pause.
     * MIDletControllerEventConsumer I/F method.
     *
     * @param midletSuiteId ID of the MIDlet suite
     * @param midletClassName Class name of the MIDlet
     */
    public void handleMIDletRsPauseNotifyEvent(
        int midletSuiteId,
        String midletClassName) {

        assertEquals(SUITE_ID, midletSuiteId);
        assertEquals(CLASS_NAME, midletClassName);
    }

    /**
     * Process a MIDlet destroy request event.
     * MIDletControllerEventConsumer I/F method.
     *
     * @param midletIsolateId isolate ID of the sending Display
     * @param midletDisplayId ID of the sending Display
     */
    public void handleMIDletDestroyRequestEvent(
        int midletIsolateId,
        int midletDisplayId) {

        assertEquals(currentIsolateId, midletIsolateId);
        assertEquals(displayId, midletDisplayId);
    }

    /**
     * Process an ACTIVATE_ALL_EVENT.
     * MIDletControllerEventConsumer I/F method.
     *
     */
    public void handleActivateAllEvent() {
        assertTrue(true);
    }


    /**
     * Process a PAUSE_ALL_EVENT.
     * MIDletControllerEventConsumer I/F method.
     */
    public void handlePauseAllEvent() {
        assertTrue(true);
    }

    /**
     * Process a SHUTDOWN_ALL_EVENT.
     * MIDletControllerEventConsumer I/F method.
     *
     * It simply calls "shutdown()". In future it shall be merged with
     * "shutdown()" and substitute it.
     */
    public void handleDestroyAllEvent() {
        assertTrue(true);
    }

    /**
     * Processes FATAL_ERROR_NOTIFICATION.
     *
     * MIDletControllerEventConsumer I/F method.
     *
     * @param midletIsolateId isolate ID of the sending isolate
     * @param midletDisplayId ID of the sending Display
     */
    public void handleFatalErrorNotifyEvent(
        int midletIsolateId,
        int midletDisplayId) {

        assertEquals(currentIsolateId, midletIsolateId);
        assertEquals(displayId, midletDisplayId);
    }

    /**
     * Process a Display created notification.
     * MIDletControllerEventConsumer I/F method.
     *
     * @param midletIsolateId isolate ID of the sending Display
     * @param midletDisplayId ID of the sending Display
     * @param midletClassName Class name of the MIDlet that owns the display
     */
    public void handleDisplayCreateNotifyEvent(
        int midletIsolateId,
        int midletDisplayId,
        String midletClassName) {

        assertEquals(currentIsolateId, midletIsolateId);
        assertEquals(displayId, midletDisplayId);
        assertEquals(CLASS_NAME, midletClassName);
    }

    /**
     * Process a foreground request event.
     * MIDletControllerEventConsumer I/F method.
     *
     * @param midletIsolateId isolate ID of the sending Display
     * @param midletDisplayId ID of the sending Display
     * @param isAlert true if the current displayable is an Alert
     */
    public void handleDisplayForegroundRequestEvent(
        int midletIsolateId,
        int midletDisplayId,
        boolean isAlert) {

        assertEquals(currentIsolateId, midletIsolateId);
        assertEquals(displayId, midletDisplayId);
        assertTrue(isAlert);
    }

    /**
     * Process a background request event.
     * MIDletControllerEventConsumer I/F method.
     *
     *
     * @param midletIsolateId isolate ID of the sending Display
     * @param midletDisplayId ID of the sending Display
     */
    public void handleDisplayBackgroundRequestEvent(
        int midletIsolateId,
        int midletDisplayId) {

        assertEquals(currentIsolateId, midletIsolateId);
        assertEquals(displayId, midletDisplayId);
    }

    /**
     * Process a "display preempt start" event.
     * <p>
     * Set the foreground to a given display if a certain display
     * has the foreground. Used to start preempting.
     *
     * MIDletControllerEventConsumer I/F method.
     *
     * @param midletIsolateId isolate ID of the sending Display
     * @param midletDisplayId ID of the sending Display
     */
    public void handleDisplayPreemptStartEvent(
        int midletIsolateId,
        int midletDisplayId) {

        assertEquals(currentIsolateId, midletIsolateId);
        assertEquals(displayId, midletDisplayId);
    }

    /**
     * Process a "display preempt stop" event.
     * <p>
     * Set the foreground to a given display if a certain display
     * has the foreground. Used to end preempting.
     *
     * MIDletControllerEventConsumer I/F method.
     *
     * @param midletIsolateId isolate ID of the sending Display
     * @param midletDisplayId ID of the sending Display
     */
    public void handleDisplayPreemptStopEvent(
        int midletIsolateId,
        int midletDisplayId) {

        assertEquals(currentIsolateId, midletIsolateId);
        assertEquals(displayId2, midletDisplayId);
    }

    /**
     * Process a select foreground event by putting the foreground selector
     * MIDlet in the foreground.
     *
     * MIDletControllerEventConsumer I/F method.
     *
     */
    public void handleMIDletForegroundSelectEvent(int onlyFromLaunched) {
    }

    /**
     * Process an event to transition the foreground from a current display
     * to a target MIDlet by ID and classname. If the source display
     * does not currently own the foreground the request is ignored.
     * If the target MIDlet is found in the active list then it it set
     * as the foreground. If not found, then it should be added as
     * the next display to get the foreground (when it asks).
     *
     * MIDletControllerEventConsumer I/F method.
     *
     * @param originMIDletSuiteId ID of MIDlet from which
     *        to take forefround ownership away,
     * @param originMIDletClassName Name of MIDlet from which
     *        to take forefround ownership away
     * @param targetMIDletSuiteId ID of MIDlet
     *        to give forefround ownership to,
     * @param targetMIDletClassName Name of MIDlet
     *        to give forefround ownership to
     */
    public void handleMIDletForegroundTransferEvent(
        int originMIDletSuiteId,
        String originMIDletClassName,
        int targetMIDletSuiteId,
        String targetMIDletClassName) {

        assertEquals(SUITE_ID, originMIDletSuiteId);
        assertEquals(CLASS_NAME, originMIDletClassName);

        assertEquals(TARGET_SUITE_ID, targetMIDletSuiteId);
        assertEquals(TARGET_CLASS_NAME, targetMIDletClassName);
    }

    /**
     * Processes SET_FOREGROUND_BY_NAME_REQUEST event.
     * <p>
     * Set specified MIDlet to foreground.
     *
     * @param midletSuiteId MIDlet's suite ID
     * @param midletClassName MIDlet's class name
     */
    public void handleSetForegroundByNameRequestEvent(
        int midletSuiteId,
        String midletClassName) {

        assertEquals(SUITE_ID, midletSuiteId);
        assertEquals(CLASS_NAME, midletClassName);
    }
}

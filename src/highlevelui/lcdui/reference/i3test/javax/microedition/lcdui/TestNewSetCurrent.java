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

package javax.microedition.lcdui;

import com.sun.midp.i3test.TestCase;
import com.sun.midp.util.LcduiTestMIDlet;
import com.sun.midp.util.LcduiTestCanvas;
import com.sun.midp.util.LiveTracer;
import com.sun.midp.util.LiveTraceCallback;
import com.sun.midp.util.LiveTraceListener;
import com.sun.midp.util.SerialCallback;

import javax.microedition.midlet.MIDlet;


/**
 * Tests proper behavior of Display.setCurrent() for a single midlet in the
 * foreground. Various combinations of "normal" (i.e., non-Alert) Displayable
 * instances are tested with the one-arg and two-arg Alert forms of the
 * setCurrent() method.
 *
 * The notation used to name tests is as follows.
 *
 * The operations initiated by the tests are:
 *
 * <code>
 * N  setCurrent(normal [non-Alert] Displayable)
 * A  setCurrent(Alert)
 * 2  setCurrent(Alert, Displayable)
 * D  dismiss the current Alert
 * </code>
 *
 * Codes for context or waits for event processing are:
 * <code>
 * 0  tests Display's initial state (current == null)
 * _  wait for a screen-change event to be processed
 * s  suffix indicating a displayable is the same as a previous one
 * </code>
 *
 * For example, testA_2 first calls setCurrent(alert1), waits for the alert to 
 * become current, then calls setCurrent(alert2, disp1). Note that this 
 * notation doesn't include an initial setCurrentWait() call that is typically 
 * used to establish the initial state for the test. In such cases the initial 
 * displayable will be numbered zero, that is, disp0 or cv0. This is in 
 * contrast to displayables and alerts that are used in the operations being 
 * tested, which are numbered starting at one.
 *
 * Quite often it is necessary to execute code atomically on the event thread,
 * that is, ensuring that no events are processed within a group of 
 * statements. Code running on the test thread is arbitrarily interleaved with 
 * code running on the event thread. For example, suppose Displayable d0 is 
 * current. If the following code were executed on the test thread,
 *
 * <code>
 *     dpy.setCurrent(d1);
 *     dpy.setCurrent(alert);
 * </code>
 *
 * there would be a race condition between d1 becoming current and the call to
 * setCurrent(alert). Runnable blocks are used to group a set of statements to
 * be executed atomically.  Runnable blocks are used in two different ways:
 * using callSerially() and using await(). In both cases the run() method is
 * executed on the event thread. This prevents any other events from being
 * dispatched in the midst of the code of the run() method.  For example:
 *
 * <code>
 *     dpy.callSerially(
 *         new Runnable() {
 *             public void run() {
 *                 // (1)
 *             }
 *         });
 * 
 *     scl.await(
 *         new Runnable() {
 *             public void run() {
 *                 // (2)
 *             }
 *         });
 * </code>
 *
 * The code (1) is executed atomically on the event thread, and the calling
 * thread continues. The code (2) is executed atomically on the event thread
 * immediately after the next screen change occurs. The calling thread is
 * blocked until the screen change occurs and the run() method has completed.
 */
public class TestNewSetCurrent extends TestCase {

    Display dpy;
    LiveTraceCallback scl; // scl = screen change listener
    boolean thrown;

    // ===== utility functions =====


    /**
     * Creates a new alert and sets its timeout to FOREVER so that it must be 
     * dismissed explicitly.
     */
    Alert makeAlert(String title) {
        Alert a = new Alert(title);
        a.setTimeout(Alert.FOREVER);
        return a;
    }


    /**
     * Simulates dismissing an alert from this display. The logic here mimics 
     * the logic of what occurs when the Alert times out and executes its 
     * default (DISMISS) command. This allows tests to set the alert timeout 
     * to FOREVER and to call this function in the right sequence, without 
     * having to deal with timing issues.
     */
    void dismiss(Alert alert) {
        synchronized (Display.LCDUILock) {
            alert.lDismiss();
        }
    }


    /**
     * Dismisses the alert and then waits for the resulting screen-change
     * event to be processed.
     */
    void dismissWait(Alert alert) {
        dismiss(alert);
        scl.await();
    }


    /**
     * Checks a variety of invariants on test canvas cv, depending upon 
     * whether the canvas should or should not be current.
     */
    void checkCurrent(String s, LcduiTestCanvas cv, boolean isCurrent) {
        if (isCurrent) {
            assertTrue(s+".showCalled must be true", cv.showCalled());
            assertTrue(s+".isShown must be true", cv.isShown());
            assertSame(s+" must be current", cv, dpy.getCurrent());
        } else {
            assertFalse(s+".showCalled must be false", cv.showCalled());
            assertFalse(s+".isShown must be false", cv.isShown());
            assertNotSame(s+" must not be current", cv, dpy.getCurrent());
        }
    }


    /**
     * Checks a variety of invariants on an alert, depending upon 
     * whether the alert should or should not be current.
     */
    void checkCurrent(String s, Alert alert, boolean shouldBeCurrent) {
        if (shouldBeCurrent) {
            assertTrue(s+".isShown must be true", alert.isShown());
            assertSame(s+" must be current", alert, dpy.getCurrent());
        } else {
            assertFalse(s+".isShown must be false", alert.isShown());
            assertNotSame(s+" must not be current", alert, dpy.getCurrent());
        }
    }


    /**
     * Sets the Displayable to be current, waits for it to become visible, and
     * returns.
     */
    void setCurrentWait(Displayable d) {
        dpy.setCurrent(d);
        scl.await();
    }


    /**
     * Creates the test MIDlet and get its display.
     */
    void init() throws Throwable {
        dpy = new StubDisplay();
        scl = new LiveTraceCallback();
        dpy.liveTracer.add(Display.LTR_SCREENCHANGE_DONE, scl);
    }


    /**
     * Cleans up the test MIDlet.
     */
    void fini() {
        dpy.liveTracer.clear();
        scl.shutdown();
    }


    // ===== the tests =====


    /**
     * Tests whether the display has been initialized properly.
     */
    void testInit() {
        assertNotNull("dpy must be non-null", dpy);
        assertNull("current must be null", dpy.getCurrent());
    }


    /**
     * Case: 0A0A. Tests whether the system returns to the original (null)
     * state after an alert. Does so twice in order to ensure that the first
     * setCurrent doesn't have any side effects.  WARNING: this must be run on
     * a fresh display.
     */
    void test0A0A() {
        final Alert alert1 = makeAlert("alert1");
        final Alert alert2 = makeAlert("alert2");

        assertNull("current null", dpy.getCurrent());

        setCurrentWait(alert1);
        checkCurrent("alert1", alert1, true);

        dismissWait(alert1);
        checkCurrent("alert1", alert1, false);
        assertNull("current null", dpy.getCurrent());

        setCurrentWait(alert2);
        checkCurrent("alert1", alert1, false);
        checkCurrent("alert2", alert2, true);

        dismissWait(alert2);
        checkCurrent("alert1", alert1, false);
        checkCurrent("alert2", alert2, false);
        assertNull("current null", dpy.getCurrent());
    }


    /**
     * Case: N.
     */
    void testN() {
        LcduiTestCanvas cv1 = new LcduiTestCanvas();
        setCurrentWait(cv1);
        checkCurrent("cv1", cv1, true);
    }


    /**
     * Case: A.
     */
    void testA() {
        LcduiTestCanvas cv0 = new LcduiTestCanvas();
        Alert alert = makeAlert("alert");

        // establish initial conditions
        setCurrentWait(cv0);
        checkCurrent("cv0", cv0, true);

        setCurrentWait(alert);
        checkCurrent("alert", alert, true);
        checkCurrent("cv0", cv0, false);

        dismissWait(alert);
        checkCurrent("alert", alert, false);
        checkCurrent("cv0", cv0, true);
    }


    /**
     * Case: 2.
     */
    void test2() {
        LcduiTestCanvas cv0 = new LcduiTestCanvas();
        LcduiTestCanvas cv1 = new LcduiTestCanvas();
        Alert alert = makeAlert("alert");

        // establish initial conditions
        setCurrentWait(cv0);
        checkCurrent("cv0", cv0, true);

        dpy.setCurrent(alert, cv1);
        scl.await();
        checkCurrent("cv0", cv0, false);
        checkCurrent("alert", alert, true);

        dismissWait(alert);
        checkCurrent("cv1", cv1, true);
        checkCurrent("alert", alert, false);
    }


    /**
     * Case: NN.
     */
    void testNN() {
        final LcduiTestCanvas cv1 = new LcduiTestCanvas();
        final LcduiTestCanvas cv2 = new LcduiTestCanvas();

        // Use callSerially to make sure both setCurrent() calls
        // occur before the first screen-change event is processed.

        dpy.callSerially(
            new Runnable() {
                public void run() {
                    dpy.setCurrent(cv1);
                    dpy.setCurrent(cv2);
                }
            });

        // Use await(Runnable) to ensure that the assertion checks
        // are done after the first screen-change event and before
        // the second screen-change event.

        scl.await(
            new Runnable() {
                public void run() {
                    checkCurrent("cv1", cv1, true);
                    checkCurrent("cv2", cv2, false);
                }
            });

        scl.await();
        checkCurrent("cv1", cv1, false);
        checkCurrent("cv2", cv2, true);
    }


    /**
     * Case: NNsN.
     */
    void testNNsN() {
        final LcduiTestCanvas cv1 = new LcduiTestCanvas();
        final LcduiTestCanvas cv2 = new LcduiTestCanvas();

        dpy.callSerially(
            new Runnable() {
                public void run() {
                    dpy.setCurrent(cv1);
                    dpy.setCurrent(cv1);
                    dpy.setCurrent(cv2);
                }
            });

        scl.await(
            new Runnable() {
                public void run() {
                    checkCurrent("cv1", cv1, true);
                    checkCurrent("cv2", cv2, false);
                }
            });

        scl.await();
        checkCurrent("cv1", cv1, false);
        checkCurrent("cv2", cv2, true);
    }


    /**
     * Case: N_N.
     */
    void testN_N() {
        LcduiTestCanvas cv1 = new LcduiTestCanvas();
        LcduiTestCanvas cv2 = new LcduiTestCanvas();

        setCurrentWait(cv1);
        checkCurrent("cv1", cv1, true);

        setCurrentWait(cv2);
        checkCurrent("cv1", cv1, false);
        checkCurrent("cv2", cv2, true);
    }


    /**
     * Case: N_NNs.
     */
    void testN_NNs() {
        final LcduiTestCanvas cv1 = new LcduiTestCanvas();
        final LcduiTestCanvas cv2 = new LcduiTestCanvas();
        
        setCurrentWait(cv1);
        checkCurrent("cv1", cv1, true);

        dpy.callSerially(
            new Runnable() {
                public void run() {
                    dpy.setCurrent(cv2);
                    dpy.setCurrent(cv1);
                }
            });

        scl.await(
            new Runnable() {
                public void run() {
                    checkCurrent("cv1", cv1, false);
                    checkCurrent("cv2", cv2, true);
                }
            });

        scl.await();
        checkCurrent("cv1", cv1, true);
        checkCurrent("cv2", cv2, false);
    }

    /**
     * Case: NA.
     */
    void testNA() {
        final LcduiTestCanvas cv0 = new LcduiTestCanvas();
        final LcduiTestCanvas cv1 = new LcduiTestCanvas();
        final Alert alert = makeAlert("alert");

        // establish initial conditions

        setCurrentWait(cv0);
        checkCurrent("cv0", cv0, true);

        dpy.callSerially(
            new Runnable() {
                public void run() {
                    dpy.setCurrent(cv1);
                    dpy.setCurrent(alert);
                }
            });

        // wait for cv1 to become current

        scl.await(
            new Runnable() {
                public void run() {
                    checkCurrent("alert", alert, false);
                    checkCurrent("cv0", cv0, false);
                    checkCurrent("cv1", cv1, true);
                }
            });

        // wait for alert to become current

        scl.await();
        checkCurrent("alert", alert, true);
        checkCurrent("cv0", cv0, false);
        checkCurrent("cv1", cv1, false);

        dismissWait(alert);

        // This checks for the behavior reported as a CR in
        // CR 6225060: after the alert is dismissed, current
        // returns to cv0.
        // checkCurrent("alert", alert, false);
        // checkCurrent("cv0", cv0, true);
        // checkCurrent("cv1", cv1, false);

        // The following checks for behavior expected by most programs: after 
        // the alert is dismissed, cv1 should become current, 
        // because a setCurrent request on it was issued prior to the 
        // setCurrent request on the alert.

        checkCurrent("alert", alert, false);
        checkCurrent("cv0", cv0, false);
        checkCurrent("cv1", cv1, true);
    }


    /**
     * Case: N2.
     */
    void testN2() {
        final LcduiTestCanvas cv1 = new LcduiTestCanvas();
        final LcduiTestCanvas cv2 = new LcduiTestCanvas();
        final Alert alert = makeAlert("alert");

        dpy.callSerially(
            new Runnable() {
                public void run() {
                    dpy.setCurrent(cv1);
                    dpy.setCurrent(alert, cv2);
                }
            });

        scl.await(
            new Runnable() {
                public void run() {
                    checkCurrent("alert", alert, false);
                    checkCurrent("cv1", cv1, true);
                    checkCurrent("cv2", cv2, false);
                }
            });

        scl.await();
        checkCurrent("alert", alert, true);
        checkCurrent("cv1", cv1, false);
        checkCurrent("cv2", cv2, false);

        dismissWait(alert);
        checkCurrent("alert", alert, false);
        checkCurrent("cv1", cv1, false);
        checkCurrent("cv2", cv2, true);
    }


    /**
     * Case: AN.
     */
    void testAN() {
        final LcduiTestCanvas cv0 = new LcduiTestCanvas();
        final LcduiTestCanvas cv1 = new LcduiTestCanvas();
        final Alert alert = makeAlert("alert");

        // establish initial conditions
        setCurrentWait(cv0);
        checkCurrent("cv0", cv0, true);

        dpy.callSerially(
            new Runnable() {
                public void run() {
                    dpy.setCurrent(alert);
                    dpy.setCurrent(cv1);
                }
            });

        scl.await(
            new Runnable() {
                public void run() {
                    checkCurrent("alert", alert, true);
                    checkCurrent("cv0", cv0, false);
                    checkCurrent("cv1", cv1, false);
                }
            });

        dismissWait(alert);
        checkCurrent("alert", alert, false);
        checkCurrent("cv0", cv0, false);
        checkCurrent("cv1", cv1, true);
    }


    /**
     * Case: AND.
     * Tests alert timeout while it's shown, while another screen change event 
     * is in the queue.
     */
    void testAND() {
        final LcduiTestCanvas cv0 = new LcduiTestCanvas();
        final LcduiTestCanvas cv1 = new LcduiTestCanvas();
        final Alert alert = makeAlert("alert");

        // establish initial conditions
        setCurrentWait(cv0);
        checkCurrent("cv0", cv0, true);

        dpy.callSerially(
            new Runnable() {
                public void run() {
                    dpy.setCurrent(alert);
                    dpy.setCurrent(cv1);
                }
            });

        scl.await(
            new Runnable() {
                public void run() {
                    checkCurrent("alert", alert, true);
                    checkCurrent("cv0", cv0, false);
                    checkCurrent("cv1", cv1, false);
                    dismiss(alert);
                }
            });

        scl.await(
            new Runnable() {
                public void run() {
                    checkCurrent("alert", alert, false);
                    checkCurrent("cv0", cv0, false);
                    checkCurrent("cv1", cv1, true);
                }
            });

        // The setCurrent(cv1) call supersedes the
        // alert, so dismissing the alert should not
        // return to cv0.

        new SerialCallback(dpy).invokeAndWait();
        checkCurrent("alert", alert, false);
        checkCurrent("cv0", cv0, false);
        checkCurrent("cv1", cv1, true);
    }

    /**
     * Case: A_N.
     */
    void testA_N() {
        final LcduiTestCanvas cv0 = new LcduiTestCanvas();
        final LcduiTestCanvas cv1 = new LcduiTestCanvas();
        final Alert alert = makeAlert("alert");

        // establish initial conditions
        setCurrentWait(cv0);
        checkCurrent("cv0", cv0, true);

        dpy.setCurrent(alert);
        scl.await();
        checkCurrent("alert", alert, true);
        checkCurrent("cv0", cv0, false);

        setCurrentWait(cv1);
        checkCurrent("alert", alert, false);
        checkCurrent("cv0", cv0, false);
        checkCurrent("cv1", cv1, true);

        dismiss(alert);
        new SerialCallback(dpy).invokeAndWait();
        checkCurrent("alert", alert, false);
        checkCurrent("cv0", cv0, false);
        checkCurrent("cv1", cv1, true);
    }


    /**
     * Case: AA.
     */
    void testAA() {
        final LcduiTestCanvas cv0 = new LcduiTestCanvas();
        final Alert alert1 = makeAlert("alert1");
        final Alert alert2 = makeAlert("alert2");

        // establish initial conditions
        setCurrentWait(cv0);
        checkCurrent("cv0", cv0, true);

        thrown = false;
        new SerialCallback(dpy) {
            public void run() {
                dpy.setCurrent(alert1);
                try {
                    dpy.setCurrent(alert2);
                } catch (IllegalArgumentException iae) {
                    thrown = true;
                }
            }
        }.invokeAndWait();

        scl.await();
        checkCurrent("alert1", alert1, true);
        assertTrue("IAE should be thrown", thrown);

        dismissWait(alert1);
        checkCurrent("cv0", cv0, true);
        checkCurrent("alert1", alert1, false);
        checkCurrent("alert2", alert2, false);
    }


    /**
     * Case: AAs.
     */
    void testAAs() {
        final LcduiTestCanvas cv0 = new LcduiTestCanvas();
        final Alert alert1 = makeAlert("alert1");

        // establish initial conditions
        setCurrentWait(cv0);
        checkCurrent("cv0", cv0, true);

        thrown = false;
        new SerialCallback(dpy) {
            public void run() {
                dpy.setCurrent(alert1);
                try {
                    dpy.setCurrent(alert1);
                } catch (IllegalArgumentException iae) {
                    thrown = true;
                }
            }
        }.invokeAndWait();

        scl.await();
        checkCurrent("alert1", alert1, true);
        assertFalse("IAE should not be thrown", thrown);

        dismissWait(alert1);
        checkCurrent("cv0", cv0, true);
        checkCurrent("alert1", alert1, false);
    }


    /**
     * Tests setCurrent(Alert) when an alert is already visible.
     * Case: A_A.
     */
    void testA_A() {
        LcduiTestCanvas cv0 = new LcduiTestCanvas();
        Alert alert1 = makeAlert("alert1");
        Alert alert2 = makeAlert("alert2");

        // establish initial conditions
        setCurrentWait(cv0);
        checkCurrent("cv0", cv0, true);

        setCurrentWait(alert1);
        checkCurrent("cv0", cv0, false);
        checkCurrent("alert1", alert1, true);

        thrown = false;
        try {
            dpy.setCurrent(alert2);
        } catch (IllegalArgumentException iae) {
            thrown = true;
        }
        assertTrue("IAE should be thrown", thrown);

        dismissWait(alert1);
        checkCurrent("cv0", cv0, true);
        checkCurrent("alert1", alert1, false);
        checkCurrent("alert2", alert2, false);
    }


    /**
     * Tests setCurrent on the same alert when it is already visible.
     * Case: A_As.
     */
    void testA_As() {
        LcduiTestCanvas cv0 = new LcduiTestCanvas();
        Alert alert1 = makeAlert("alert1");

        // establish initial conditions
        setCurrentWait(cv0);
        checkCurrent("cv0", cv0, true);

        setCurrentWait(alert1);
        checkCurrent("cv0", cv0, false);
        checkCurrent("alert1", alert1, true);

        thrown = false;
        try {
            dpy.setCurrent(alert1);
        } catch (IllegalArgumentException iae) {
            thrown = true;
        }
        assertFalse("IAE should not be thrown", thrown);

        dismissWait(alert1);
        checkCurrent("cv0", cv0, true);
        checkCurrent("alert1", alert1, false);
    }


    /**
     * Case: A2.
     */
    void testA2() {
        final LcduiTestCanvas cv0 = new LcduiTestCanvas();
        final LcduiTestCanvas cv1 = new LcduiTestCanvas();
        final Alert alert1 = makeAlert("alert1");
        final Alert alert2 = makeAlert("alert2");

        // establish initial conditions
        setCurrentWait(cv0);
        checkCurrent("cv0", cv0, true);

        dpy.callSerially(
            new Runnable() {
                public void run() {
                    dpy.setCurrent(alert1);
                    dpy.setCurrent(alert2, cv1);
                }
            });
                    
        scl.await(
            new Runnable() {
                public void run() {
                    checkCurrent("alert1", alert1, true);
                    checkCurrent("alert2", alert2, false);
                    checkCurrent("cv0", cv0, false);
                    checkCurrent("cv1", cv1, false);
                }
            });

        scl.await(
            new Runnable() {
                public void run() {
                    checkCurrent("alert1", alert1, false);
                    checkCurrent("alert2", alert2, true);
                    checkCurrent("cv0", cv0, false);
                    checkCurrent("cv1", cv1, false);
                    dismiss(alert1);
                }
            });

        // Dismissing alert1 should do nothing, since it has
        // been superseded by alert2.

        new SerialCallback(dpy).invokeAndWait();
        checkCurrent("alert1", alert1, false);
        checkCurrent("alert2", alert2, true);
        checkCurrent("cv0", cv0, false);
        checkCurrent("cv1", cv1, false);

        dismissWait(alert2);
        checkCurrent("alert1", alert1, false);
        checkCurrent("alert2", alert2, false);
        checkCurrent("cv0", cv0, false);
        checkCurrent("cv1", cv1, true);
    }


    /**
     * Case: A2s.
     */
    void testA2s() {
        final LcduiTestCanvas cv0 = new LcduiTestCanvas();
        final LcduiTestCanvas cv1 = new LcduiTestCanvas();
        final Alert alert1 = makeAlert("alert1");

        // establish initial conditions
        setCurrentWait(cv0);
        checkCurrent("cv0", cv0, true);

        dpy.callSerially(
            new Runnable() {
                public void run() {
                    dpy.setCurrent(alert1);
                    dpy.setCurrent(alert1, cv1);
                }
            });
                    
        scl.await(
            new Runnable() {
                public void run() {
                    checkCurrent("alert1", alert1, true);
                    checkCurrent("cv0", cv0, false);
                    checkCurrent("cv1", cv1, false);
                }
            });

        // Ensure that the second setCurrent had no effect.

        new SerialCallback(dpy).invokeAndWait();
        checkCurrent("alert1", alert1, true);
        checkCurrent("cv0", cv0, false);
        checkCurrent("cv1", cv1, false);

        dismissWait(alert1);
        checkCurrent("alert1", alert1, false);
        checkCurrent("cv0", cv0, false);
        checkCurrent("cv1", cv1, true);
    }


    /**
     * Case: A_2.
     */
    void testA_2() {
        final LcduiTestCanvas cv0 = new LcduiTestCanvas();
        final LcduiTestCanvas cv1 = new LcduiTestCanvas();
        final Alert alert1 = makeAlert("alert1");
        final Alert alert2 = makeAlert("alert2");

        // establish initial conditions
        setCurrentWait(cv0);
        checkCurrent("cv0", cv0, true);

        dpy.setCurrent(alert1);
        scl.await();
        checkCurrent("alert1", alert1, true);
        checkCurrent("cv0", cv0, false);

        dpy.setCurrent(alert2, cv1);
        scl.await();
        checkCurrent("alert1", alert1, false);
        checkCurrent("alert2", alert2, true);
        checkCurrent("cv0", cv0, false);
        checkCurrent("cv1", cv1, false);

        // Dismiss alert1. Nothing should happen, since it has
        // been superseded by alert2.

        dismiss(alert1);
        new SerialCallback(dpy).invokeAndWait();
        checkCurrent("alert1", alert1, false);
        checkCurrent("alert2", alert2, true);
        checkCurrent("cv0", cv0, false);
        checkCurrent("cv1", cv1, false);

        dismissWait(alert2);
        checkCurrent("alert1", alert1, false);
        checkCurrent("alert2", alert2, false);
        checkCurrent("cv0", cv0, false);
        checkCurrent("cv1", cv1, true);
    }


    /**
     * Case: A_2s.
     */
    void testA_2s() {
        final LcduiTestCanvas cv0 = new LcduiTestCanvas();
        final LcduiTestCanvas cv1 = new LcduiTestCanvas();
        final Alert alert1 = makeAlert("alert1");

        // establish initial conditions
        setCurrentWait(cv0);
        checkCurrent("cv0", cv0, true);

        setCurrentWait(alert1);
        checkCurrent("alert1", alert1, true);
        checkCurrent("cv0", cv0, false);

        // The two-arg call should update the next displayable
        // but should otherwise do nothing.
        dpy.setCurrent(alert1, cv1);
        new SerialCallback(dpy).invokeAndWait();
        checkCurrent("alert1", alert1, true);
        checkCurrent("cv0", cv0, false);
        checkCurrent("cv1", cv1, false);

        dismissWait(alert1);
        checkCurrent("alert1", alert1, false);
        checkCurrent("cv0", cv0, false);
        checkCurrent("cv1", cv1, true);
    }


    /**
     * Case: 2N.
     */
    void test2N() {
        final LcduiTestCanvas cv1 = new LcduiTestCanvas();
        final LcduiTestCanvas cv2 = new LcduiTestCanvas();
        final Alert alert = makeAlert("alert");

        dpy.callSerially(
            new Runnable() {
                public void run() {
                    dpy.setCurrent(alert, cv1);
                    dpy.setCurrent(cv2);
                }
            });

        scl.await(
            new Runnable() {
                public void run() {
                    checkCurrent("alert", alert, true);
                    checkCurrent("cv1", cv1, false);
                    checkCurrent("cv2", cv2, false);
                }
            });
        
        scl.await(
            new Runnable() {
                public void run() {
                    checkCurrent("alert", alert, false);
                    checkCurrent("cv1", cv1, false);
                    checkCurrent("cv2", cv2, true);
                }
            });

        // Dismiss alert. Nothing should happen, since it has
        // been superseded by cv2.

        dismiss(alert);
        new SerialCallback(dpy).invokeAndWait();
        checkCurrent("alert", alert, false);
        checkCurrent("cv1", cv1, false);
        checkCurrent("cv2", cv2, true);
        assertFalse("cv1 should not have been shown", cv1.wasShown());
    }


    /**
     * Case: 2_N.
     */
    void test2_N() {
        final LcduiTestCanvas cv1 = new LcduiTestCanvas();
        final LcduiTestCanvas cv2 = new LcduiTestCanvas();
        final Alert alert = makeAlert("alert");

        dpy.setCurrent(alert, cv1);
        scl.await(
            new Runnable() {
                public void run() {
                    checkCurrent("alert", alert, true);
                    checkCurrent("cv1", cv1, false);
                }
            });
        
        setCurrentWait(cv2);
        checkCurrent("alert", alert, false);
        checkCurrent("cv1", cv1, false);
        checkCurrent("cv2", cv2, true);

        // Dismiss alert. Nothing should happen, since it has
        // been superseded by cv2.
        dismiss(alert);
        new SerialCallback(dpy).invokeAndWait();
        checkCurrent("alert", alert, false);
        checkCurrent("cv1", cv1, false);
        checkCurrent("cv2", cv2, true);
        assertFalse("cv1 should not have been shown", cv1.wasShown());
    }


    /**
     * Case: 2A.
     */
    void test2A() {
        final LcduiTestCanvas cv1 = new LcduiTestCanvas();
        final LcduiTestCanvas cv2 = new LcduiTestCanvas();
        final Alert alert1 = makeAlert("alert1");
        final Alert alert2 = makeAlert("alert2");

        thrown = false;
        dpy.callSerially(
            new Runnable() {
                public void run() {
                    dpy.setCurrent(alert1, cv1);
                    try {
                        dpy.setCurrent(alert2);
                    } catch (IllegalArgumentException IAE) {
                        thrown = true;
                    }
                }
            });

        scl.await(
            new Runnable() {
                public void run() {
                    checkCurrent("alert1", alert1, true);
                    checkCurrent("alert2", alert2, false);
                    checkCurrent("cv1", cv1, false);
                    assertTrue("IAE should be thrown", thrown);
                }
            });

        dismissWait(alert1);
        checkCurrent("alert1", alert1, false);
        checkCurrent("alert2", alert2, false);
        checkCurrent("cv1", cv1, true);
    }

    /**
     * Case: 2_A.
     */
    void test2_A() {
        final LcduiTestCanvas cv1 = new LcduiTestCanvas();
        final LcduiTestCanvas cv2 = new LcduiTestCanvas();
        final Alert alert1 = makeAlert("alert1");
        final Alert alert2 = makeAlert("alert2");

        dpy.setCurrent(alert1, cv1);

        scl.await();
        checkCurrent("alert1", alert1, true);
        checkCurrent("cv1", cv1, false);

        thrown = false;
        try {
            dpy.setCurrent(alert2);
        } catch (IllegalArgumentException IAE) {
            thrown = true;
        }
        assertTrue("IAE should be thrown", thrown);

        dismissWait(alert1);
        checkCurrent("alert1", alert1, false);
        checkCurrent("alert2", alert2, false);
        checkCurrent("cv1", cv1, true);
    }


    /**
     * Case: 22.
     */
    void test22() {
        final LcduiTestCanvas cv1 = new LcduiTestCanvas();
        final LcduiTestCanvas cv2 = new LcduiTestCanvas();
        final Alert alert1 = makeAlert("alert1");
        final Alert alert2 = makeAlert("alert2");

        dpy.callSerially(
            new Runnable() {
                public void run() {
                    dpy.setCurrent(alert1, cv1);
                    dpy.setCurrent(alert2, cv2);
                }
            });

        scl.await(
            new Runnable() {
                public void run() {
                    checkCurrent("alert1", alert1, true);
                    checkCurrent("alert2", alert2, false);
                    checkCurrent("cv1", cv1, false);
                    checkCurrent("cv2", cv2, false);
                }
            });

        scl.await(
            new Runnable() {
                public void run() {
                    checkCurrent("alert1", alert1, false);
                    checkCurrent("alert2", alert2, true);
                    checkCurrent("cv1", cv1, false);
                    checkCurrent("cv2", cv2, false);
                }
            });

        // Dismiss alert1. Nothing should happen, since it has
        // been superseded by cv2.
        dismiss(alert1);
        new SerialCallback(dpy).invokeAndWait();
        checkCurrent("alert1", alert1, false);
        checkCurrent("alert2", alert2, true);
        checkCurrent("cv1", cv1, false);
        checkCurrent("cv2", cv2, false);

        dismissWait(alert2);
        checkCurrent("alert1", alert1, false);
        checkCurrent("alert2", alert2, false);
        checkCurrent("cv1", cv1, false);
        checkCurrent("cv2", cv2, true);
        assertFalse("cv1 should never have been shown", cv1.wasShown());
    }


    /**
     * Case: 2_2.
     */
    void test2_2() {
        final LcduiTestCanvas cv1 = new LcduiTestCanvas();
        final LcduiTestCanvas cv2 = new LcduiTestCanvas();
        final Alert alert1 = makeAlert("alert1");
        final Alert alert2 = makeAlert("alert2");

        dpy.setCurrent(alert1, cv1);
        scl.await();
        checkCurrent("alert1", alert1, true);
        checkCurrent("cv1", cv1, false);

        dpy.setCurrent(alert2, cv2);
        scl.await();
        checkCurrent("alert1", alert1, false);
        checkCurrent("alert2", alert2, true);
        checkCurrent("cv1", cv1, false);
        checkCurrent("cv2", cv2, false);

        // Dismiss alert1. Nothing should happen, since it has
        // been superseded by cv2.

        dismiss(alert1);
        new SerialCallback(dpy).invokeAndWait();
        checkCurrent("alert1", alert1, false);
        checkCurrent("alert2", alert2, true);
        checkCurrent("cv1", cv1, false);
        checkCurrent("cv2", cv2, false);

        dismissWait(alert2);
        checkCurrent("alert1", alert1, false);
        checkCurrent("alert2", alert2, false);
        checkCurrent("cv1", cv1, false);
        checkCurrent("cv2", cv2, true);
        assertFalse("cv1 should never have been shown", cv1.wasShown());
    }


    /**
     * Case 22s. Tests the behavior of two, two-arg setCurrent calls, where 
     * the alert in question is the same in both cases.
     */
    void test22s() {
        final Alert alert = makeAlert("alert");
        final LcduiTestCanvas cv1 = new LcduiTestCanvas();
        final LcduiTestCanvas cv2 = new LcduiTestCanvas();

        dpy.callSerially(
            new Runnable() {
                public void run() {
                    dpy.setCurrent(alert, cv1);
                    dpy.setCurrent(alert, cv2);
                }
            });

        scl.await(
            new Runnable() {
                public void run() {
                    checkCurrent("alert", alert, true);
                    checkCurrent("cv1", cv1, false);
                    checkCurrent("cv2", cv2, false);
                }
            });

        // Dismiss alert, then cv2 should become current.

        dismissWait(alert);
        checkCurrent("alert", alert, false);
        checkCurrent("cv1", cv1, false);
        checkCurrent("cv2", cv2, true);
        assertFalse("cv1 should never have been shown", cv1.wasShown());
    }


    /**
     * Case 2_2s. Tests the behavior of two, two-arg setCurrent calls, where 
     * the alert in question is the same in both cases.
     */
    void test2_2s() {
        final Alert alert = makeAlert("alert");
        final LcduiTestCanvas cv1 = new LcduiTestCanvas();
        final LcduiTestCanvas cv2 = new LcduiTestCanvas();

        dpy.setCurrent(alert, cv1);
        scl.await();
        checkCurrent("alert", alert, true);
        checkCurrent("cv1", cv1, false);

        // Call setCurrent again and dismiss the alert before
        // the event can be processed.

        dpy.callSerially(
            new Runnable() {
                public void run() {
                    dpy.setCurrent(alert, cv2);
                    dismiss(alert);
                }
            });

        scl.await();
        checkCurrent("alert", alert, false);
        checkCurrent("cv1", cv1, false);
        checkCurrent("cv2", cv2, true);
        assertFalse("cv1 should never have been shown", cv1.wasShown());
    }


    /**
     * Tests setCurrent(alert, alert). This is basically an API test.
     */
    void testAlertAlert() {
        Alert alert1 = makeAlert("alert1");
        Alert alert2 = makeAlert("alert2");

        thrown = false;
        try {
            dpy.setCurrent(alert1, alert2);
        } catch (IllegalArgumentException iae) {
            thrown = true;
        }

        assertTrue("IAE should be thrown", thrown);
    }


    /**
     * Case: A_NA.
     */
    void testA_NA() {
        final Alert alert1 = makeAlert("alert1");
        final Alert alert2 = makeAlert("alert2");
        final LcduiTestCanvas cv0 = new LcduiTestCanvas();
        final LcduiTestCanvas cv1 = new LcduiTestCanvas();

        // establish initial conditions
        setCurrentWait(cv0);
        checkCurrent("cv0", cv0, true);

        dpy.setCurrent(alert1);
        scl.await(
            new Runnable() {
                public void run() {
                    checkCurrent("alert1", alert1, true);
                    checkCurrent("cv0", cv0, false);
                    dpy.setCurrent(cv1);
                    dpy.setCurrent(alert2);
                }
            });

        scl.await(
            new Runnable() {
                public void run() {
                    checkCurrent("alert1", alert1, false);
                    checkCurrent("alert2", alert2, false);
                    checkCurrent("cv0", cv0, false);
                    checkCurrent("cv1", cv1, true);
                    dismiss(alert1);
                }
            });
        
        // Dismissing alert1 should be ignored since it has been
        // superseded by cv1; then alert2 should become current.

        scl.await(
            new Runnable() {
                public void run() {
                    checkCurrent("alert1", alert1, false);
                    checkCurrent("alert2", alert2, true);
                    checkCurrent("cv0", cv0, false);
                    checkCurrent("cv1", cv1, false);
                    dismiss(alert2);
                }
            });
        
        scl.await();
        checkCurrent("alert1", alert1, false);
        checkCurrent("alert2", alert2, false);
        checkCurrent("cv0", cv0, false);
        checkCurrent("cv1", cv1, true);
    }


    /**
     * Runs all tests.
     */
    public void runTests() throws Throwable {
        init();
        try {
            declare("testInit");
            testInit();

            declare("test0A0A");
            test0A0A();

            declare("testN");
            testN();

            declare("testA");
            testA();

            declare("test2");
            test2();

            declare("testNN");
            testNN();

            declare("testNNsN");
            testNNsN();

            declare("testN_N");
            testN_N();

            declare("testN_NNs");
            testN_NNs();

            declare("testNA");
            testNA();

            declare("testN2");
            testN2();

            declare("testAN");
            testAN();

            declare ("testAND");
            testAND();

            declare("testA_N");
            testA_N();

            declare("testAA");
            testAA();

            declare("testAAs");
            testAAs();

            declare("testA_A");
            testA_A();

            declare("testA_As");
            testA_As();

            declare("testA2");
            testA2();

            declare("testA2s");
            testA2s();

            declare("testA_2");
            testA_2();

            declare("testA_2s");
            testA_2s();

            declare("test2N");
            test2N();

            declare("test2_N");
            test2_N();

            declare("test2A");
            test2A();

            declare("test2_A");
            test2_A();

            declare("test22");
            test22();

            declare("test2_2");
            test2_2();

            declare("test22s");
            test22s();

            declare("test2_2s");
            test2_2s();

            declare("testAlertAlert");
            testAlertAlert();

            declare("testA_NA");
            testA_NA();
        } finally {
            fini();
        }
    }
}

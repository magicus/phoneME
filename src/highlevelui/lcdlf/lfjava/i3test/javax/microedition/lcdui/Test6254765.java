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
import com.sun.midp.util.Baton;
import com.sun.midp.util.LcduiTestMIDlet;
import com.sun.midp.util.LiveTracer;
import com.sun.midp.util.LiveTraceListener;
import com.sun.midp.util.SerialCallback;

/**
 * Regression tests for CR 6254765. There are actually two bugs mentioned 
 * there. Details are provided for each test case.
 */
public class Test6254765 extends TestCase {

    Display dpy;

    /**
     * Checks all of the itemLFs of the given form to see if their layouts are 
     * valid. Returns true if all are valid, false if any one is invalid.
     */
    boolean checkValidLayout(Form form) {
        FormLFImpl formLF = (FormLFImpl)form.formLF;
        ItemLFImpl itemLF;
        boolean anyInvalid = false;

        for (int ii = 0; ii < formLF.numOfLFs; ii++) {
            itemLF = formLF.itemLFs[ii];
            boolean thisInvalid = 
                itemLF.actualBoundsInvalid[0] ||
                itemLF.actualBoundsInvalid[1] ||
                itemLF.actualBoundsInvalid[2] ||
                itemLF.actualBoundsInvalid[3];
            anyInvalid = anyInvalid || thisInvalid;
            // System.out.println(
            //     "item[" + ii + "] bounds (" +
            //     itemLF.bounds[0] + ", " +
            //     itemLF.bounds[1] + ", " +
            //     itemLF.bounds[2] + ", " +
            //     itemLF.bounds[3] + ")" +
            //     (thisInvalid ? " INVALID" : ""));
        }

        return !anyInvalid;
    }

    /**
     * Regression test for the main CR described in CR 6254765.  This is a
     * race condition between modifying a Form's contents (for example, with
     * append) while the Form is in the process of becoming current.
     */
    void testScreenChangeAppend() {
        StringItem items[] = new StringItem[10];
        final Baton baton = new Baton();

        for (int ii = 0; ii < items.length; ii++) {
            items[ii] = new StringItem(null, Integer.toString(ii % 10));
        }

        Form form = new Form("Test Form");

        // gets called from dpy.callScreenChange() after uCallShow()
        dpy.liveTracer.add(
            Display.LTR_SCREENCHANGE_AFTERSHOW,
            new LiveTraceListener() {
                public void call(String tag) {
                    baton.pass();
                }
            });

        dpy.setCurrent(form);

        for (int ii = 0; ii < items.length; ii++) {
            form.append(items[ii]);
            if (ii == 3) {
                baton.start();
            }
        }
        baton.finish();

        // wait for queued events to be processed
        new SerialCallback(dpy).invokeAndWait();

        assertTrue("layout must be valid", checkValidLayout(form));
        dpy.liveTracer.clear();
    }

    /**
     * This is a regression test for another CR that is also mentioned in 
     * 6254765, which occurs when the form is in an inconsistent state such as 
     * what arose from the initial CR, but which can also arise for other 
     * reasons.
     * 
     * This case is as follows: traverseIndex == -1 and itemTraverse == false,
     * indicating no initial focus; and getNextInteractiveItem returns a value
     * >= 0 indicating that there is a focusable item on screen.
     *
     * The uTraverse() code assumes that if getNextInteractiveItem returns
     * some nonnegative value, there must be a currently focused item (that
     * is, traverseIndex is also nonnegative). However, this is not always the
     * case. This can occur if the form initially has no visible focusable
     * items, and the app adds a focusable item, which triggers an invalidate,
     * and then the user traverses before the invalidate can be processed. 
     * This test simulates that case.
     */
    void testTraversalInconsistency() {
        Form form = new Form("Test Form 2");
        FormLFImpl formLF = (FormLFImpl)form.formLF;
        final Baton baton = new Baton();
        SerialCallback scb = new SerialCallback(dpy);

        // can be any interactive item
        Item item = new Gauge(null, true, 1, 0);

        // Set up a form with no focusable item.

        form.append("String 1");
        dpy.setCurrent(form);
        scb.invokeAndWait();

        // Block the event queue to prevent the invalidate from
        // being processed.

        dpy.callSerially(
            new Runnable() {
                public void run() {
                    baton.pass();
                }
            });
        baton.start();

        // Append a focusable item to the form, and then call
        // uTraverse() directly, as if a key had been pressed at
        // exactly the right moment.

        form.insert(0, item);
        formLF.uTraverse(Canvas.DOWN);
        baton.finish();

        // Wait for the invalidate to finish processing, then
        // check assertions.

        scb.invokeAndWait();
        assertEquals("item 0 should be focused", 0, formLF.traverseIndex);
    }

    // main test driver

    public void runTests() throws Throwable {

        if (!LcduiTestMIDlet.invoke()) {
            throw new RuntimeException("can't start LcduiTestMIDlet");
        }

        try {
            dpy = LcduiTestMIDlet.getDisplay();

            declare("testScreenChangeAppend");
            testScreenChangeAppend();

            declare("testTraversalInconsistency");
            testTraversalInconsistency();

        } finally {
            LcduiTestMIDlet.cleanup();
        }
    }
}


/*
 * A mock LayoutManager. Can be inserted into the LayoutManager class in order
 * to instrument call to the lLayout method, among other things.  To use, 
 * first save away the normal layout manager:
 * 
 * LayoutManager savedLayoutManager = LayoutManager.instance();
 *
 * Then install the mock:
 * 
 * LayoutManager.singleInstance = new MockLayoutManager();
 *
 * After testing, preferably within a finally clause, restore the original 
 * layout manager:
 *
 * LayoutManager.singleInstance = savedLayoutManager;
 *
 * Failure to do this may have side effects on other tests!
 */

// class MockLayoutManager extends LayoutManager {
//     int count; // = 0
// 
//     void lLayout(int layoutMode, 
// 		 ItemLFImpl[] itemLFs, 
// 		 int numOfLFs,
// 		 int inp_viewportWidth,
// 		 int inp_viewportHeight,
// 		 int[] viewable) {
//         count = numOfLFs;
//         super.lLayout(
//             layoutMode,
//             itemLFs, 
//             numOfLFs,
//             inp_viewportWidth,
//             inp_viewportHeight,
//             viewable);
//     }
// }

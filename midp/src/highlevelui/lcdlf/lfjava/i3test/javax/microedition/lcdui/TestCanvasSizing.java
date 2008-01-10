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

import com.sun.midp.chameleon.skins.*;
import com.sun.midp.i3test.TestCase;
import com.sun.midp.util.LcduiTestCanvas;
import com.sun.midp.util.LcduiTestMIDlet;
import  com.sun.midp.configurator.Constants;

/**
 * This test case does extensive testing on the proper sizing
 * of Canvas. It validates the size reported to a Canvas under
 * various conditions, such as a canvas with a title, a ticker,
 * in standard screen mode, in fullscreen mode, as well as a
 * canvas which is newly created versus showing on the screen.
 */
public class TestCanvasSizing extends TestCase {

    /** Test Name */
    static final String testName = "CanvasSizing";
    
    // We define a set of constants that are set by pulling values
    // out of the various skin classes (ie. TitleSkin, TickerSkin, etc).

    /** Screen width in normal mode */ 
    protected static int STD_WIDTH;
    /** Screen height in normal mode */ 
    protected static int STD_HEIGHT;
    
    // Note that although for chameleon's standard spec, the fullscreen
    // height and the standard height are the same, this may not be the
    // case for all implementations. Including both here allows the test
    // to be valid in both circumstances.
    
    /** Screen width in fullsize mode */ 
    protected static int FS_WIDTH;
    /** Screen height in fullsize mode */ 
    protected static int FS_HEIGHT;
    
    /** Title height */ 
    protected static int TITLE_HEIGHT;

    /** Ticker height */ 
    protected static int TICKER_HEIGHT;
    
    /** Softbutton height */ 
    protected static int SOFTBTN_HEIGHT;
    
    /** The Display to use to view our test canvases */
    protected Display display;
    
    /** Test ticker, used for sizing only */
    protected Ticker ticker;
    
    /**
     * Construct a new CanvasSizing test. The constructor
     * initializes all sizing constants from the chameleon
     * skin files.
     */
    public TestCanvasSizing() {
        // Initialize the constants
        
        STD_WIDTH = ScreenSkin.WIDTH;
        STD_HEIGHT = Constants.CHAM_HEIGHT;
        FS_WIDTH = Constants.CHAM_FULLWIDTH;
        FS_HEIGHT = Constants.CHAM_FULLHEIGHT;
        TITLE_HEIGHT = TitleSkin.HEIGHT;
        TICKER_HEIGHT = TickerSkin.HEIGHT;
        SOFTBTN_HEIGHT = SoftButtonSkin.HEIGHT;

        ticker = new Ticker("Canvas sizing test");
    }
    
    /**
     * This is a utility method which retrieves the canvas's size and
     * validates it against the passed in values after making sure 
     * the canvas is visible on the screen.
     *
     * @param canvas the canvas to test
     * @param WIDTH the <b>correct</b> width the canvas should be
     * @param HEIGHT the <b>correct</b> height the canvas should be
     */
    protected void checkCanvasSize(TestCanvas canvas, int WIDTH, int HEIGHT) {
            
        display.setCurrent(canvas);

        if (!canvas.awaitPaint()) {
            fail("checkCanvasSize: canvas not visible");
            return;
        }
            
        int w = canvas.getWidth();
        int h = canvas.getHeight();
        assertEquals("Checking canvas width (shown):", WIDTH, w);
        assertEquals("Checking canvas height (shown):", HEIGHT, h);
    }
    
    /**
     * This test is for a canvas with a title and a ticker set in
     * standard screen mode.
     */
    protected void testOne() {
        declare(testName + " 1: Standard screen mode with title and ticker.");
        
        TestCanvas canvas = new TestCanvas();
        canvas.setTitle("CanvasSizing Test 1");
        canvas.setTicker(ticker);
        
        checkCanvasSize(canvas, STD_WIDTH, 
            STD_HEIGHT - SOFTBTN_HEIGHT - TITLE_HEIGHT - TICKER_HEIGHT);
    }
    
    /**
     * This test is for a canvas with a title but no ticker set in
     * standard screen mode.
     */
    protected void testTwo() {
        declare(testName + " 2: Standard screen mode with title.");
        
        TestCanvas canvas = new TestCanvas();
        canvas.setTitle("CanvasSizing Test 2");
        
        checkCanvasSize(canvas, STD_WIDTH, 
            STD_HEIGHT - SOFTBTN_HEIGHT - TITLE_HEIGHT);
    }
    
    /**
     * This test is for a canvas with a ticker but no title set in
     * standard screen mode.
     */
    protected void testThree() {
        declare(testName + " 3: Standard screen mode with ticker.");
        
        TestCanvas canvas = new TestCanvas();
        canvas.setTicker(ticker);
        
        checkCanvasSize(canvas, STD_WIDTH,
            STD_HEIGHT - SOFTBTN_HEIGHT - TICKER_HEIGHT);
    }
    
    /**
     * This test is for a canvas with no title and no ticker set in
     * standard screen mode.
     */
    protected void testFour() {
        declare(testName + " 4: Standard screen mode, plain.");
        
        TestCanvas canvas = new TestCanvas();
        
        checkCanvasSize(canvas, STD_WIDTH, STD_HEIGHT - SOFTBTN_HEIGHT);
    }
    
    /**
     * This test is for a canvas with a title and a ticker set in
     * fullscreen mode.
     */
    protected void testFive() {
        declare(testName + " 5: Fullsize screen mode with title and ticker.");
        
        TestCanvas canvas = new TestCanvas();
        canvas.setTitle("CanvasSizing Test 5");
        canvas.setTicker(ticker);
        canvas.toggleFullscreen(true);

        // NOTE: In fullscreen mode, the title and ticker shouldn't appear, as
        // per the chameleon UI spec: that area is then used by the canvas
        checkCanvasSize(canvas, FS_WIDTH, FS_HEIGHT);
    }
    
    /**
     * This test is for a canvas with a title but no ticker set in
     * fullscreen mode.
     */
    protected void testSix() {
        declare(testName + " 6: Fullsize screen mode with title.");
        
        TestCanvas canvas = new TestCanvas();
        canvas.setTitle("CanvasSizing Test 6");
        canvas.toggleFullscreen(true);
        
        // NOTE: In fullscreen mode, the title and ticker shouldn't appear, as
        // per the chameleon UI spec: that area is then used by the canvas
        checkCanvasSize(canvas, FS_WIDTH, FS_HEIGHT);
    }
    
    /**
     * This test is for a canvas with a ticker but no title set in
     * fullscreen mode.
     */
    protected void testSeven() {
        declare(testName + " 7: Fullsize screen mode with ticker.");
        
        TestCanvas canvas = new TestCanvas();
        canvas.setTicker(ticker);
        canvas.toggleFullscreen(true);
        
        // NOTE: In fullscreen mode, the title and ticker shouldn't appear, as
        // per the chameleon UI spec: that area is then used by the canvas
        checkCanvasSize(canvas, FS_WIDTH, FS_HEIGHT);
    }
    
    /**
     * This test is for a canvas with no ticker and no title set in
     * fullscreen mode.
     */
    protected void testEight() {
        declare(testName + " 8: Fullsize screen mode, plain.");
        
        TestCanvas canvas = new TestCanvas();
        canvas.toggleFullscreen(true);
        
        // NOTE: In fullscreen mode, the title and ticker shouldn't appear, as
        // per the chameleon UI spec: that area is then used by the canvas
        checkCanvasSize(canvas, FS_WIDTH, FS_HEIGHT);
    }

    /**
     * Overridden from TestCase parent. This method will kick off each
     * individual test
     */
    public void runTests() throws Throwable {
        
        try {
            LcduiTestMIDlet.invoke();
            display = LcduiTestMIDlet.getDisplay();

            testOne();
            testTwo();
            testThree();
            testFour();
            testFive();
            testSix();
            testSeven();
            testEight();
        } finally {
            LcduiTestMIDlet.cleanup();
        }
    }
    
    /**
     * Inner canvas class which notifies the main TestCase when
     * a particular canvas has become visible. This allows the
     * main TestCase to do sizing verification both before the
     * canvas becomes visible, and after.
     */
    class TestCanvas extends LcduiTestCanvas {
        
        /**
         * Used to remotely toggle this canvas's fullscreen mode
         *
         * @param onOff true if this canvas should go to fullscreen mode
         */
        public void toggleFullscreen(boolean onOff) {
            super.setFullScreenMode(onOff);
        }
        
        /** 
         * Just a default white fill. The test programmatically changes
         * screens so it would be too fast to write anything useful anyway,
         * as the user wouldn't have time to read it.
         *
         * @param g the Graphics to paint with
         */
        public void paint1(Graphics g) {
            int c = g.getColor();
            g.setColor(0xFFFFFF);
            g.fillRect(g.getClipX(), g.getClipY(), 
                       g.getClipWidth(), g.getClipHeight());
            g.setColor(c);
        }
    }    
}



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

import com.sun.midp.i3test.*;
import javax.microedition.lcdui.*;
import javax.microedition.midlet.*;
import com.sun.midp.chameleon.skins.*;
import com.sun.midp.midlet.MIDletStateHandler;
import com.sun.midp.util.DummyMIDlet;

/**
 * This test case is meant to verify that the <code>sizeChanged</code>
 * method of a Canvas is called correctly in every instance where it
 * is required by the MIDP specification.
 * It is triggered by the addition or removal of a title, ticker,
 * or both.  It may also be triggered in the event that 
 * the full screen and non-full screen display is toggled.
 *
 * <code>sizeChanged</code>  should be called when the canvas is 
 * visible, or in the event the canvas is not shown, <code>sizeChanged</code>
 * should be called before <code>paint</code> is called.  
 */
public class TestSizeChanged extends TestCase {

    /**
     * Name for this test grouping
     */
    static final String testName = "SizeChanged";
    
    // We define a set of constants that are set by pulling values
    // out of the various skin classes (ie. TitleSkin, TickerSkin, etc).

    /**
     * Width of the screen in non fullscreen mode
     */
    protected static int STD_WIDTH;

    /**
     * Height of the screen in non fullscreen mode
     */
    protected static int STD_HEIGHT;
    
    // Note that although for chameleon's standard spec, the fullscreen
    // height and the standard height are the same, this may not be the
    // case for all implementations. Including both here allows the test
    // to be valid in both circumstances.
    
    /**
     * Height of the screen in fulscreen mode
     */
    protected static int FS_HEIGHT;
    
    /**
     * Height of the title bar
     */
    protected static int TITLE_HEIGHT;

    /**
     * Height of the ticker
     */
    protected static int TICKER_HEIGHT;
    
    /**
     * Height of the soft button bar
     */
    protected static int SOFTBTN_HEIGHT;
    
    /**
     * The Display to use to view our test canvases
     */
    protected Display display;
    
    /**
     * Test ticker, used for sizing only
     */
    protected Ticker ticker;
    
    /**
     * Construct a new SizeChanged test. The constructor
     * initializes all sizing constants from the chameleon
     * skin files.
     */
    public TestSizeChanged() {
        // Initialize the constants
        
        STD_WIDTH = Display.WIDTH;
        STD_HEIGHT = Display.HEIGHT;
        FS_HEIGHT = ScreenSkin.FULLHEIGHT;
        TITLE_HEIGHT = TitleSkin.HEIGHT;
        TICKER_HEIGHT = TickerSkin.HEIGHT;
        SOFTBTN_HEIGHT = SoftButtonSkin.HEIGHT;

        ticker = new Ticker("Canvas sizeChanged() tests...");
    }
    
    /**
     * This is a utility method which checks values passed by 
     * <code>sizeChanged</code> against passed in values. It 
     * ensures that the method should have been called by waiting
     * for the canvas to be visible on the screen.
     *
     * @param canvas the canvas to test
     * @param WIDTH the <b>correct</b> width the canvas should be after the
     *              most recent <code>sizeChanged</code> call
     * @param HEIGHT the <b>correct</b> height the canvas should be after the
     *               most recent <code>sizeChanged</code> call
     */
    protected void checkCanvasSizeChanged(TestCanvas canvas, 
                                          int WIDTH, int HEIGHT) {
        synchronized (canvas) {
            display.setCurrent(canvas);
            try {
                canvas.wait();
            } catch (InterruptedException ie) {
            } catch (IllegalMonitorStateException im) {
                fail("Unexpected monitor exception");
                return;
            }
        }
        
        // Now the canvas should be showing on the screen, we test
        // our assertions about <code>sizeChanged</code> being called
        assertEquals("Checking for Canvas.sizeChanged() width update", 
                     WIDTH, canvas.checkSizeWidth());
        assertEquals("Checking for Canvas.sizeChanged() height update", 
                     HEIGHT, canvas.checkSizeHeight());
    }

    /**
     * This is a utility method which checks values passed by 
     * <code>sizeChanged</code> against passed in values. It 
     * ensures that the method should have been called by waiting
     * for the canvas to be visible on the screen.
     *
     * @param canvas the canvas to test
     * @param WIDTH the <b>correct</b> width the canvas should be after the
     *              most recent <code>sizeChanged</code> call
     * @param HEIGHT the <b>correct</b> height the canvas should be after the
     *               most recent <code>sizeChanged</code> call
     */
    protected void checkCurrentCanvasSizeChanged(TestCanvas canvas, 
                                                 int WIDTH, int HEIGHT) {
        synchronized (canvas) {
            //display.setCurrent(canvas);
            try {
                canvas.wait();
            } catch (InterruptedException ie) {
            } catch (IllegalMonitorStateException im) {
                fail("Unexpected monitor exception");
                return;
            }
        }
        
        // Now the canvas should be showing on the screen, we test
        // our assertions about <code>sizeChanged</code> being called
        assertEquals("Checking for Canvas.sizeChanged() width update", 
                     WIDTH, canvas.checkSizeWidth());
        assertEquals("Checking for Canvas.sizeChanged() height update", 
                     HEIGHT, canvas.checkSizeHeight());
    }
   
    /*
     * Tests 1-4 involve adding components to a Canvas which is not
     * the current display, and checking to see that the proper
     * <code>sizeChanged()</code> occurs whenthe Canvas is brought to 
     * the foreground with a <code>display.setCurrent()</code> call.
     */

    /**
     * This test is for a canvas with no title and no ticker set in
     * standard screen mode.  The canvas is set to the current display 
     * causing <code>sizeChanged</code> to be called.
     */
    protected void testOne() {
        declare(testName + " 1: bg->fg: std-screen, no title, no ticker");
        
        TestCanvas canvas = new TestCanvas();

        checkCanvasSizeChanged(canvas, STD_WIDTH, STD_HEIGHT - SOFTBTN_HEIGHT); 
    }
    
    /**
     * This test is for a canvas with a title set in standard screen mode.
     * The canvas is set to the current display causing <code>sizeChanged</code> 
     * to be called.
     */
    protected void testTwo() {
        declare(testName + " 2: bg->fg: std-screen, add title");
        
        TestCanvas canvas = new TestCanvas();
        canvas.setTitle("SizeChanged Test 2");
        
        checkCanvasSizeChanged(canvas, STD_WIDTH, 
                               STD_HEIGHT - SOFTBTN_HEIGHT - TITLE_HEIGHT);
    }

    /**
     * This test is for a canvas with a title and ticker set in standard 
     * screen mode.  The canvas is set to the current display causing 
     * <code>sizeChanged</code> to be called.
     */
    protected void testThree() {
        declare(testName + " 3: bg->fg: std-screen, add title, add ticker");
        
        TestCanvas canvas = new TestCanvas();
        canvas.setTitle("sizeChanged Test 3");
        canvas.setTicker(ticker);
        
        checkCanvasSizeChanged(canvas, STD_WIDTH, STD_HEIGHT - SOFTBTN_HEIGHT 
                               - TICKER_HEIGHT - TITLE_HEIGHT);
    }

    /**
     * This test is for a canvas with a ticker set in standard screen mode.
     * The canvas is set to the current display causing <code>sizeChanged</code> 
     * to be called.
     */
    protected void testFour() {
        declare(testName + " 4: bg->fg: std-screen, add ticker");
        
        TestCanvas canvas = new TestCanvas();
        canvas.setTicker(ticker);
        
        checkCanvasSizeChanged(canvas, STD_WIDTH, 
                               STD_HEIGHT - SOFTBTN_HEIGHT - TICKER_HEIGHT);
    }

    /*
     * Tests 5-8 involve removing components from a Canvas which is not
     * the current display, and checking to see that the proper
     * <code>sizeChanged()</code> occurs whenthe Canvas is brought to 
     * the foreground with a <code>display.setCurrent()</code> call.
     */
    
    /**
     * This test is for a canvas with a title and a ticker set in
     * standard screen mode.  The canvas is set to the current display 
     * causing <code>sizeChanged</code> to be called.
     */
    protected void testFive() {
        declare(testName + " 5: bg->fg: std-screen, with title & ticker");
        
        TestCanvas tmpCanvas = new TestCanvas();
        TestCanvas canvas = new TestCanvas();

        display.setCurrent(canvas);

        canvas.setTitle("SizeChanged test 5");
        canvas.setTicker(ticker);

        display.setCurrent(tmpCanvas);

        checkCanvasSizeChanged(canvas, STD_WIDTH, STD_HEIGHT - SOFTBTN_HEIGHT -
                               TITLE_HEIGHT - TICKER_HEIGHT); 
    }
    
    /**
     * This test is for a canvas with a title and ticker set in standard 
     * screen mode.  The ticker is removed, and the canvas is set to the 
     * current display causing <code>sizeChanged</code> to be called.
     */
    protected void testSix() {
        declare(testName + " 6: bg->fg: std-screen, with title & ticker." + 
                "  remove ticker");
        
        TestCanvas tmpCanvas = new TestCanvas();
        TestCanvas canvas = new TestCanvas();

        display.setCurrent(canvas);

        canvas.setTitle("SizeChanged Test 6");
        canvas.setTicker(ticker);

        display.setCurrent(tmpCanvas);

        canvas.setTicker(null);
        
        checkCanvasSizeChanged(canvas, STD_WIDTH, STD_HEIGHT - SOFTBTN_HEIGHT 
                               - TITLE_HEIGHT);
    }

    /**
     * This test is for a canvas with a title and ticker set in standard 
     * screen mode.  The title is removed, and the canvas is set to the current 
     * display causing <code>sizeChanged</code> to be called.
     */
    protected void testSeven() {
        declare(testName + " 7: bg->fg: std-screen, with title & ticker." + 
                "  remove title");
        
        TestCanvas tmpCanvas = new TestCanvas();
        TestCanvas canvas = new TestCanvas();

        display.setCurrent(canvas);

        canvas.setTitle("SizeChanged Test 7");
        canvas.setTicker(ticker);

        display.setCurrent(tmpCanvas);

        canvas.setTitle(null);
        
        checkCanvasSizeChanged(canvas, STD_WIDTH, STD_HEIGHT - SOFTBTN_HEIGHT 
                               - TICKER_HEIGHT);
    }

    /**
     * This test is for a canvas with a title and ticker set in standard 
     * screen mode.  The ticker and title are removed, and the canvas is 
     * set to the current display causing <code>sizeChanged</code> 
     * to be called.
     */
    protected void testEight() {
        declare(testName + " 8: bg->fg: std-screen, with title & ticker." + 
                "  remove both ticker and title");
        
        TestCanvas tmpCanvas = new TestCanvas();
        TestCanvas canvas = new TestCanvas();
        
        display.setCurrent(canvas);
        
        canvas.setTitle("SizeChanged Test 8");
        canvas.setTicker(ticker);
        
        display.setCurrent(tmpCanvas);
        
        canvas.setTicker(null);
        canvas.setTitle(null);
        
        checkCanvasSizeChanged(canvas, STD_WIDTH, STD_HEIGHT - SOFTBTN_HEIGHT);
    }

    /**
     * This test is for a canvas with a title and ticker set in standard 
     * screen mode.  FullScreen mode is then set on the Canvas and it is
     * set to the current display causing <code>sizeChanged</code> 
     * to be called.
     */
    protected void testNine() {
        declare(testName + " 9: bg->fg: std-screen, with title & ticker." + 
                "  set fullscreen mode");
        
        TestCanvas tmpCanvas = new TestCanvas();
        TestCanvas canvas = new TestCanvas();
        
        display.setCurrent(canvas);
        
        canvas.setTitle("SizeChanged Test 9");
        canvas.setTicker(ticker);
        
        display.setCurrent(tmpCanvas);
        
        canvas.toggleFullscreen(true);
        
        checkCanvasSizeChanged(canvas, STD_WIDTH, FS_HEIGHT - SOFTBTN_HEIGHT);
    }

    /**
     * This test is for a canvas with a title and ticker set in full 
     * screen mode.  Standard mode is then set on the Canvas and it is
     * set to the current display causing <code>sizeChanged</code> 
     * to be called.
     */
    protected void testTen() {
        declare(testName + " 10: bg->fg: full-screen, with title & ticker." + 
                "  set std-screen mode");
        
        TestCanvas tmpCanvas = new TestCanvas();
        TestCanvas canvas = new TestCanvas();
        
        display.setCurrent(canvas);

        canvas.toggleFullscreen(true);  
        canvas.setTitle("SizeChanged Test 10");
        canvas.setTicker(ticker);

        
        display.setCurrent(tmpCanvas);
        
        canvas.toggleFullscreen(false);
        
        checkCanvasSizeChanged(canvas, STD_WIDTH, FS_HEIGHT - SOFTBTN_HEIGHT -
                               TICKER_HEIGHT - TITLE_HEIGHT);
    }

    /**
     * Test 11 involves removing components from a Canvas which is
     * the current display, and checking to see that the proper
     * <code>sizeChanged()</code> occurs after each removal
     */
    protected void testEleven() {
        declare(testName + " 11: in fg: std-screen, with title & ticker." +
                " remove title, then ticker.");
        
        TestCanvas tmpCanvas = new TestCanvas();
        TestCanvas canvas = new TestCanvas();

        display.setCurrent(tmpCanvas);

        canvas.setTitle("SizeChanged test 11");
        canvas.setTicker(ticker);

        checkCanvasSizeChanged(canvas, STD_WIDTH, STD_HEIGHT - SOFTBTN_HEIGHT -
                               TITLE_HEIGHT - TICKER_HEIGHT); 

        canvas.setTitle(null);
        checkCurrentCanvasSizeChanged(canvas, STD_WIDTH, STD_HEIGHT - 
                                      SOFTBTN_HEIGHT - TICKER_HEIGHT); 

        canvas.setTicker(null);
        checkCurrentCanvasSizeChanged(canvas, STD_WIDTH, STD_HEIGHT - SOFTBTN_HEIGHT); 
    }
    
    /**
     * Test 12 involves adding components from a canvas which is
     * current display, and checking to see that the proper
     * <code>sizeChanged</code> call occurs after each addition.
     */
    protected void testTwelve() {
        declare(testName + " 12: in fg: std-screen.  add title, then ticker");
        
        TestCanvas canvas = new TestCanvas();
        
        checkCanvasSizeChanged(canvas, STD_WIDTH, STD_HEIGHT - SOFTBTN_HEIGHT); 
        
        canvas.setTitle("SizeChanged Test 12");
        checkCurrentCanvasSizeChanged(canvas, STD_WIDTH, STD_HEIGHT - 
                                      SOFTBTN_HEIGHT - TITLE_HEIGHT); 
        
        canvas.setTicker(ticker);
        checkCurrentCanvasSizeChanged(canvas, STD_WIDTH, STD_HEIGHT - 
                                      SOFTBTN_HEIGHT - TITLE_HEIGHT - 
                                      TICKER_HEIGHT); 
    }

    /**
     * Test 13 involves changing to fullScreen mode and back on
     * a canvas which is the current display, and checking to see that the proper
     * <code>sizeChanged</code> call occurs after each change.
     */
    protected void testThirteen() {
        declare(testName + " 13: in fg: std-screen, with title & ticker." +
                " remove title, then ticker.");
        
        TestCanvas tmpCanvas = new TestCanvas();
        TestCanvas canvas = new TestCanvas();

        display.setCurrent(tmpCanvas);

        canvas.setTitle("SizeChanged test 13");
        canvas.setTicker(ticker);
        
        checkCanvasSizeChanged(canvas, STD_WIDTH, STD_HEIGHT - SOFTBTN_HEIGHT -
                               TITLE_HEIGHT - TICKER_HEIGHT); 

        canvas.toggleFullscreen(true);
        checkCurrentCanvasSizeChanged(canvas, STD_WIDTH, STD_HEIGHT - SOFTBTN_HEIGHT); 

        canvas.toggleFullscreen(false);
        checkCurrentCanvasSizeChanged(canvas, STD_WIDTH, STD_HEIGHT - SOFTBTN_HEIGHT
                                      - TITLE_HEIGHT - TICKER_HEIGHT); 
    }

    /**
     * Overridden from TestCase parent. This method will kick off each
     * individual test
     */
    public void runTests() {
        
        MIDletStateHandler msHandler = MIDletStateHandler.getMidletStateHandler();
        // Start a new instance of DummyMIDlet
        try {
            msHandler.startMIDlet("com.sun.midp.util.DummyMIDlet",
                                  "DummyMIDlet");
        } catch (Exception cnf) {
            return;
        }
        
        // Wait for async request to be processed 
        while (DummyMIDlet.midlet == null) {
            try { Thread.sleep(100); } catch (InterruptedException ie) {;}
        }

        MIDlet midlet = DummyMIDlet.midlet;
        display = Display.getDisplay(midlet);
        
        testOne();   
        testTwo();
        testThree();
        testFour();
        testFive();
        testSix();
        testSeven();
        testEight();
        testNine();
        testTen();
        testEleven();
        testTwelve();
        testThirteen();
        
        // Notify the system that our midlet is done
        midlet.notifyDestroyed();
        try {
            ((DummyMIDlet)midlet).destroyApp(true);
        } catch (MIDletStateChangeException ignore) {
        }
    }
    
    /**
     * Inner canvas class which notifies the main TestCase when
     * a particular canvas has become visible. This allows the
     * main TestCase know that <code>sizeChanged</code> should have 
     * been called by a specific time.
     */
    class TestCanvas extends Canvas {

        /**
         * Most recent width value in a <code>sizeChanged</code> call.
         * Reset to -1 when <code>checkSizeWidth</code> is called.
         */
        private int curW = -1;

        /**
         * Most recent height value in a <code>sizeChanged</code> call.
         * Reset to -1 when <code>checkSizeHeight</code> is called.
         */
        private int curH = -1;

        /**
         * Construct a new TestCanvas given the parent test handler.
         */
        public TestCanvas() {
            super();
        }

        /**
         * Returns the most recent width from a <code>sizeChanged</code>
         * call and resets <code>curW</code> to -1
	 * @return width from the most recent sizeChanged call
         */
        public int checkSizeWidth() {       
            int tmp;
            tmp = curW;
            curW = -1;
            return tmp;
        }

        /**
         * Returns the most recent height from a <code>sizeChanged</code>
         * call and resets <code>curH</code> to -1
	 * @return heightfrom the most recent sizeChanged call
         */
        public int checkSizeHeight() {      
            int tmp;
            tmp = curH;
            curH = -1;
            return tmp;
        }
        
        /**
         * Used to remotely toggle this canvas's fullscreen mode
         *
         * @param onOff true if this canvas should go to fullscreen mode
         */
        public void toggleFullscreen(boolean onOff) {
            super.setFullScreenMode(onOff);
        }
        
        /**
         * This notifies the canvas that it's available size for drawing
         * has changed (due to a title, ticker, or fullscreen mode change)
         *
         * @param w the new width in pixels of the drawable area of the
         * <code>Canvas</code>
         * @param h the new height in pixels of the drawable area of
         * the <code>Canvas</code>
         */
        protected void sizeChanged(int w, int h) {
            curW = w;
            curH = h;
        }
        
        /** 
         * Just a default white fill. The test programmatically changes
         * screens so it would be too fast to write anything useful anyway,
         * as the user wouldn't have time to read it.
         *
         * @param g the Graphics to paint with
         */
        protected void paint(Graphics g) {
            int c = g.getColor();

            // IMPL_NOTE: 
	    // remove this once chameleon sizeChanged() code is working.
            // This line is used to fake sizeChanged() being called before paint().
            // It appears sizeChanged is not being called in the chameleon code...
	    // ...but we don't know why.
            this.sizeChanged(this.getWidth(), this.getHeight());

            g.setColor(0xFFFFFF);
            g.fillRect(g.getClipX(), g.getClipY(), 
                       g.getClipWidth(), g.getClipHeight());
            g.setColor(c);

            // The spec guarantees us that paint can only be called when
            // the Canvas is visible, that is, isShown() returns true.
            synchronized (this) {
                try {
                    this.notify();
                } catch (IllegalMonitorStateException e) {
                    // If the other wait()ing thread doesn't get
                    // interrupted, the whole test will block anyway, so
                    // we rely on wait/notify to be working correctly
                }
            }
        }
    }    
}

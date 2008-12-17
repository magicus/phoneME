/*
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
import javax.microedition.midlet.*;

public class CustomItemTest extends TestCase {
    FormLFImpl ownerLFImpl = null;
    int vpY_beforeScroll = 0;
    int vpY_AfterScroll = 0;


    void testCustomItemInternalTraversal() {
	Display display = DisplayTestUtils.createDisplay();
        Form mainForm = new Form("CustomItem Demo");
	CustomItemImpl customItem = 
                new CustomItemImpl("CustomItem", 80, display, false); 
        mainForm.append(customItem);
        display.setCurrent(mainForm);

	// make sure the form is visible
	try {
	    Thread.sleep(2000);
	} catch (Exception e) {}

	assertSame(display.getCurrent(), mainForm);

	CustomItemLF customItemLF = customItem.customItemLF;
	Item myitem = ((CustomItemLFImpl)customItemLF).item;

	// call keypress with "up" keyCode -1
	((CustomItemLFImpl)customItemLF).uCallKeyPressed(-1);
	((CustomItemLFImpl)customItemLF).uCallKeyReleased(-1);

	assertTrue("traverse not called", customItem.traverse_flag);
        Display.displayManagerImpl.uRequestForegroundNone();	
	assertSame(Display.displayManagerImpl.noForeground, 
                             Display.displayManagerImpl.foreground);
    }

    /**
     * check that the viewport scrolls to make the
     * returned vis_Rect from traverse method visible
     *
     * This test is specific to platform_widget. Enable it only
     * for platform-widget if possible.
     *
    void testCustomItemInternalTraversal2() {
 	Display display = DisplayTestUtils.createDisplay();

        Form mainForm = new Form("CustomItem 2 Demo");
	TextField tf = new TextField("Text Field", "some text", 100, 0);

	CustomItemImpl customItem1 = 
               new CustomItemImpl("CustomItem1", 80, display, false); 
        customItem1.setLayout(Item.LAYOUT_LEFT | Item.LAYOUT_NEWLINE_BEFORE |
                Item.LAYOUT_NEWLINE_AFTER);

	CustomItemImpl customItem2 = 
                       new CustomItemImpl("CustomItem2", 80, display, false);
        customItem2.setLayout(Item.LAYOUT_LEFT | Item.LAYOUT_NEWLINE_BEFORE |
                Item.LAYOUT_NEWLINE_AFTER);

        StringItem si = new StringItem("a string", "some text");

        mainForm.append(tf);
        mainForm.append(customItem1);
        mainForm.append(si);
        mainForm.append(customItem2);
 
	display.setCurrentItem(customItem2);

	// make sure the form is visible
	try {
	    Thread.sleep(2000);
	} catch (Exception e) {}

	assertSame(display.getCurrent(), mainForm);

	// part of the custom item is made visible
	CustomItemLF customItemLF = customItem2.customItemLF;
	Item myitem = ((CustomItemLFImpl)customItemLF).item;

	Screen owner = myitem.owner;
	if (owner != null) {
	    ownerLFImpl = 
		(FormLFImpl)owner.getLF();
	}       

	// vpY1 the y coordinate of the top left visible pixel
	// current scroll position
	vpY_beforeScroll = ownerLFImpl.getScrollPosition0();

	// call key down 
       	// KEYCODE_DOWN  Value="-2" 
	((CustomItemLFImpl)customItemLF).uCallKeyPressed(-2);
	((CustomItemLFImpl)customItemLF).uCallKeyReleased(-2);

	assertTrue("traverse not called: first call", 
                                customItem2.traverse_flag);


	((CustomItemLFImpl)customItemLF).uCallKeyPressed(-2);
	((CustomItemLFImpl)customItemLF).uCallKeyReleased(-2);

	assertTrue("traverse not called: second call", 
                                customItem2.traverse_flag);

	// check that the viewport scrolls to make the
	// returned vis_Rect visible
	// vpY1 the y coordinate of the top left visible pixel

	vpY_AfterScroll = ownerLFImpl.getScrollPosition0();

	boolean screenScrolled = 
                     vpY_AfterScroll > vpY_beforeScroll ? true : false;
	assertTrue("screen not Scrolled", screenScrolled);
        Display.displayManagerImpl.uRequestForegroundNone();	
	assertSame(Display.displayManagerImpl.noForeground, 
                             Display.displayManagerImpl.foreground);
    }
    */

    // main test driver

    public void runTests() {
        declare("testCustomItemInternalTraversal");
        testCustomItemInternalTraversal();

	/*
	 * This test is platform_widget specific.
	 *
        declare("testCustomItemInternalTraversal2");
        testCustomItemInternalTraversal2();
	 */
    }
}


class CustomItemImpl extends CustomItem {
    private int size, rgb_bgrnd, rgb_fgrnd, rgb_highlighted_bgrnd,
            rgb_highlighted_fgrnd;
    private int startIndex = 0;
    private int highlightedIndex = 0;
    private int currentIndex = 0;
    private boolean initialTraversal = false;
    private int[] highlightedRect = new int[4];
    int[] visRect = new int[4];
    boolean traverse_flag = false;

    private boolean debugFlag = false;
    private String[] content = { "String0", "String1", "String2", "String3",
              "String4", "String5", "String6", "String7", "String8",
              "String9"
    };

    public CustomItemImpl(String label, int size, Display display,
            boolean debug) {
        super(label);
        debugFlag = debug;
        rgb_bgrnd = display.getColor(Display.COLOR_BACKGROUND);
        rgb_highlighted_bgrnd =
                display.getColor(Display.COLOR_HIGHLIGHTED_BACKGROUND);
        rgb_fgrnd = display.getColor(Display.COLOR_FOREGROUND);
        rgb_highlighted_fgrnd =
                display.getColor(Display.COLOR_HIGHLIGHTED_FOREGROUND);

        this.size = size;
        highlightedRect[0] = 1;
        highlightedRect[1] = 1;
        highlightedRect[2] = 55;
        highlightedRect[3] = 14;
    }

    protected int getMinContentHeight() {
        return size;
    }

    protected int getMinContentWidth() {
        return size;
    }

    protected int getPrefContentHeight(int width) {
        if (debugFlag) {
            System.err.println("getPrefContentHeight called. width = " 
                                           + width);
        }
        return size;
    }

    protected int getPrefContentWidth(int height) {
        if (debugFlag) {
            System.err.println("getPrefContentWidth called. height = "
                    + height);
        }
        return size;
    }

    protected void paint(Graphics g, int w, int h) {
        if (debugFlag) {
	    System.err.println("\n\n" + 
                     getLabel() + "'s paint() called");
            System.err.println("w = " + w + " , h = " + h);
        }
        g.setColor(rgb_bgrnd);
        g.fillRect(0, 0, w, h);
        g.setColor(rgb_fgrnd);
        g.drawRect(0, 0, w - 1, h - 1);
        Font f = g.getFont();

        for (int i = 0; i < 5; i++) {
            g.drawString(content[startIndex + i], 1, 1 + i * f.getHeight(),
                    Graphics.TOP | Graphics.LEFT);
        }

        if (traverse_flag) {
            // change the highlighting
            g.setColor(rgb_highlighted_bgrnd);
            g.fillRect(highlightedRect[0], highlightedRect[1],
                    highlightedRect[2], highlightedRect[3]);
            g.setColor(rgb_highlighted_fgrnd);
            g.drawString(content[currentIndex], highlightedRect[0],
                    highlightedRect[1], Graphics.TOP | Graphics.LEFT);
        }

        if (debugFlag) {
            System.err.println("highlightedRect[0] = " + highlightedRect[0]
                    + "\nhighlightedRect[1] =  " + highlightedRect[1]
                    + "\nhighlightedRect[2] =  " + highlightedRect[2]
		    + "\nhighlightedRect[3] =  " + highlightedRect[3]);

        }
    }

    protected void traverseOut() {
        if (debugFlag) {
           System.err.println("*** traverseOut was called ***");
        }
        traverse_flag = false;
        repaint();
    }

    protected boolean traverse(int dir, int viewportWidth, int viewportHeight,
            int[] visRect_inout) {

        if ((!traverse_flag) && (dir != 0)) {
            traverse_flag = true;
            return true;
        }

	visRect[0] = visRect_inout[0];
	visRect[1] = visRect_inout[1];
	visRect[2] = visRect_inout[2];
	visRect[3] = visRect_inout[3];

        if (debugFlag) {
            System.err.println("\n\n" + "*** traverse was called ***");

            System.err.println("dir = " + dir);
            System.err.println("viewportWidth = " + viewportWidth);
            System.err.println("viewportHeight = " + viewportHeight);
            System.err.println("visRect_inout[0] = " + visRect_inout[0]);

	    System.err.println("visRect_inout[1] = " + visRect_inout[1]);

	    System.err.println("visRect_inout[2] = " + visRect_inout[2]);

	    System.err.println("visRect_inout[3] = " + visRect_inout[3]);
        }

        if (dir == Canvas.DOWN) {
            if (startIndex < 5) {
                if (highlightedIndex < 4) {
                    highlightedRect[1] += 14;
                    highlightedIndex += 1;
                } else {
                    startIndex += 1;
                }
                currentIndex += 1;


            } else {
                return false;
            }
        }

        if (dir == Canvas.UP) {
            if (highlightedIndex > 0) {
                highlightedRect[1] -= 14;
                highlightedIndex -= 1;
            } else if (currentIndex > 0) {
                startIndex -= 1;
            } else {
                return false;
            }
            currentIndex -= 1;
        }
        //  repaint();
        return true;
    }

}

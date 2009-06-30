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

/**
 * This test is designed to test Chameleon's ItemLFImpl.lDoInternalLayout()
 * routine only. It does not test label size or content size calculations.
 * It tests label location and content location calculations.
 * 
 * Internal layout for all items is handled in ItemLFImpl.
 * lDoInternalLayout() is given label and content sizes and available
 * width, height, it should place label and content in correct place.
 * Here is what to expect:
 * If label and content are empty, both bounds are set to 0.
 * If label is empty, content is given as much space as it needs but
 *   not more than available.
 * If content is empty, label is given as much space as it needs but
 *   not more than available.
 * If label is a single line label (its height <= label font height) then
 *   there is an attempt to fit label, content and 3 pixel padding between
 *   them on the same line.
 * Otherwise content is placed under label with 2 pixel horizontal padding 
 *   in between.
 *  
 * Furthermore, if label and content are on the same line than
 *   shorter part (label or content) is vertically centered 
 *   against the taller one (content or label).
 * 
 * Then if there is extra space (item can fit in smaller size than
 * it is given) then the space is distributed based on the layout
 * primitives set.
 *
 * ItemLFImpl and Item area abstract in javax.microedition.lcdui.
 * We extend them to test lDoInternalLayout().
 */

public class TestItemLFImplLDoInternalLayout extends TestCase {

    public void runTests() {

        // no extra space, null label, null content
        declare("test1");
        checkLayout(new TestItemLFImpl(0, 0,  // labelSize
                                       0, 0,  // contentSize
                                       0, 0,  // itemSize
                                       10),   // label font height
                    0, 0,  // checked label location
                    0, 0); // checked content location

        // no extra space, null label, non null content
        declare("test2");
        checkLayout(new TestItemLFImpl(0, 0,      // labelSize
                                       100, 100,  // contentSize
                                       100, 100,  // itemSize
                                       10),   // label font height
                    0, 0,  // checked label location
                    0, 0); // checked content location

        // no extra space, non null label, null content
        declare("test3");
        checkLayout(new TestItemLFImpl(100, 100,  // labelSize
                                       0, 0,      // contentSize
                                       100, 100,  // itemSize
                                       10),   // label font height
                    0, 0,  // checked label location
                    0, 0); // checked content location




        // no extra space, non null label, non null content;
        // label and content are on the same line;
        // label height > content height
        declare("test4");
        checkLayout(new TestItemLFImpl(50, 20,  // labelSize
                                       50, 10,  // contentSize
                                       103, 20, // itemSize
                                       20),     // labelHeight to be single ln 
                    0, 0,  // checked label location
                    53, 5); // checked content location

        // no extra space, non null label, non null content
        // label and content are on the same line
        // label height < content height
        declare("test5");
        checkLayout(new TestItemLFImpl(50, 10,  // labelSize
                                       50, 20,  // contentSize
                                       103, 20, // itemSize
                                       10),     // labelHeight to be single ln 
                    0, 5,  // checked label location
                    53, 0); // checked content location

        // no extra space, non null label, non null content
        // label and content are on different lines (due to width)
        // label width < content width
        declare("test6");
        checkLayout(new TestItemLFImpl(30, 10,  // labelSize
                                       50, 20,  // contentSize
                                       50, 32, // itemSize
                                       10),     // labelHeight to be single ln 
                    0, 0,  // checked label location
                    0, 12); // checked content location

        // no extra space, non null label, non null content
        // label and content are on different lines (due to multiline label)
        // label width < content width
        declare("test7");
        checkLayout(new TestItemLFImpl(30, 20,  // labelSize
                                       50, 30,  // contentSize
                                       50, 52, // itemSize
                                       10),     // labelHeight to be single ln 
                    0, 0,  // checked label location
                    0, 22); // checked content location

        // no extra space, non null label, non null content
        // label and content are on different lines (due to width)
        // label width > content width
        declare("test8");
        checkLayout(new TestItemLFImpl(50, 10,  // labelSize
                                       30, 20,  // contentSize
                                       50, 32, // itemSize
                                       10),     // labelHeight to be single ln 
                    0, 0,  // checked label location
                    0, 12); // checked content location

        // no extra space, non null label, non null content
        // label and content are on different lines (due to mulitline label)
        // label width > content width
        declare("test9");
        checkLayout(new TestItemLFImpl(50, 20,  // labelSize
                                       30, 30,  // contentSize
                                       50, 52, // itemSize
                                       10),     // labelHeight to be single ln 
                    0, 0,  // checked label location
                    0, 22); // checked content location


        // ************ more space HORIZONTALLY  **************************
        // ****************************************************************

        // there IS extra space horizontally, 
        // LAYOUT_CENTER and LAYOUT_RIGHT are NOT set
        // null label, non null content
        declare("test10");
        checkLayout(new TestItemLFImpl(0, 0,    // labelSize
                                       50, 50,  // contentSize
                                       100, 50, // itemSize
                                       10),     // labelHeight to be single ln 
                    0, 0,  // checked label location
                    0, 0); // checked content location
        
        // there IS extra space horizontally, 
        // LAYOUT_CENTER and LAYOUT_RIGHT are NOT set
        // non null label, null content
        declare("test11");
        checkLayout(new TestItemLFImpl(50, 50,   // labelSize
                                       0, 0,     // contentSize
                                       100, 50, // itemSize
                                       10),     // labelHeight to be single ln 
                    0, 0,  // checked label location
                    0, 0); // checked content location



        // there IS extra space horizontally, 
        // LAYOUT_CENTER and LAYOUT_RIGHT are NOT set
        // non null label, non null content
        // label and content on the same line;
        // label height > content height
        declare("test11");
        checkLayout(new TestItemLFImpl(47, 20,    // labelSize
                                       50, 10,  // contentSize
                                       200, 20, // itemSize
                                       20),     // labelHeight to be single ln 
                    0, 0,  // checked label location
                    50, 5); // checked content location

        // there IS extra space horizontally, 
        // LAYOUT_CENTER and LAYOUT_RIGHT are NOT set
        // non null label, non null content
        // label and content on the same line
        // label height < content height
        declare("test12");
        checkLayout(new TestItemLFImpl(47, 10,    // labelSize
                                       50, 20,  // contentSize
                                       200, 20, // itemSize
                                       10),    // labelHeight to be single ln 
                    0, 5,  // checked label location
                    50, 0); // checked content location

        // there IS extra space horizontally, 
        // LAYOUT_CENTER and LAYOUT_RIGHT are NOT set
        // non null label, non null content
        // label and content on different lines
        // label width < content width
        declare("test13");
        checkLayout(new TestItemLFImpl(30, 20,  // labelSize
                                       50, 20,  // contentSize
                                       100, 32, // itemSize
                                       10),     // labelHeight to be single ln 
                    0, 0,  // checked label location
                    0, 22); // checked content location


        // there IS extra space horizontally, 
        // LAYOUT_CENTER and LAYOUT_RIGHT are NOT set
        // non null label, non null content
        // label and content on different lines
        // label width > content width
        declare("test14");
        checkLayout(new TestItemLFImpl(50, 20,  // labelSize
                                       30, 20,  // contentSize
                                       100, 32, // itemSize
                                       10),     // labelHeight to be single ln 
                    0, 0,  // checked label location
                    0, 22); // checked content location


        // **********************************************
        
        // there IS extra space horizontally, 
        // LAYOUT_CENTER is set
        // non null label, non null content
        // label and content on the same line;
        // label height > content height
        declare("test15");
        checkLayout(new TestItemLFImpl(47, 20,    // labelSize
                                       50, 10,  // contentSize
                                       200, 20, // itemSize
                                       20,     // labelHeight to be single ln 
                                       Item.LAYOUT_CENTER),
                    50, 0,  // checked label location
                    100, 5); // checked content location
        
        // there IS extra space horizontally, 
        // LAYOUT_CENTER is set
        // non null label, non null content
        // label and content on the same line
        // label height < content height
        declare("test16");
        checkLayout(new TestItemLFImpl(47, 10,    // labelSize
                                       50, 20,  // contentSize
                                       200, 20, // itemSize
                                       10,     // labelHeight to be single ln 
                                       Item.LAYOUT_CENTER),
                    50, 5,  // checked label location
                    100, 0); // checked content location


        // there IS extra space horizontally, 
        // LAYOUT_CENTER is set
        // non null label, non null content
        // label and content on different lines
        // label width < content width
        declare("test17");
        checkLayout(new TestItemLFImpl(30, 20,  // labelSize
                                       50, 20,  // contentSize
                                       100, 32, // itemSize
                                       10,     // labelHeight to be single ln 
                                       Item.LAYOUT_CENTER),
                    35, 0,  // checked label location
                    25, 22); // checked content location

        // there IS extra space horizontally, 
        // LAYOUT_CENTER is set
        // non null label, non null content
        // label and content on different lines
        // label width > content width
        declare("test18");
        checkLayout(new TestItemLFImpl(50, 20,  // labelSize
                                       30, 20,  // contentSize
                                       100, 32, // itemSize
                                       10,     // labelHeight to be single ln 
                                       Item.LAYOUT_CENTER),
                    25, 0,  // checked label location
                    35, 22); // checked content location


        // *****************


        // there IS extra space horizontally, 
        // LAYOUT_RIGHT is set
        // non null label, non null content
        // label and content on the same line;
        // label height > content height
        declare("test19");
        checkLayout(new TestItemLFImpl(47, 20,    // labelSize
                                       50, 10,  // contentSize
                                       200, 20, // itemSize
                                       20,     // labelHeight to be single ln 
                                       Item.LAYOUT_RIGHT),
                    100, 0,  // checked label location
                    150, 5); // checked content location

        // there IS extra space horizontally, 
        // LAYOUT_RIGHT is set
        // non null label, non null content
        // label and content on the same line
        // label height < content height
        declare("test20");
        checkLayout(new TestItemLFImpl(47, 10,    // labelSize
                                       50, 20,  // contentSize
                                       200, 20, // itemSize
                                       10,     // labelHeight to be single ln 
                                       Item.LAYOUT_RIGHT),
                    100, 5,  // checked label location
                    150, 0); // checked content location

        // there IS extra space horizontally, 
        // LAYOUT_RIGHT is set
        // non null label, non null content
        // label and content on different lines
        // label width < content width
        declare("test21");
        checkLayout(new TestItemLFImpl(30, 20,  // labelSize
                                       50, 20,  // contentSize
                                       100, 32, // itemSize
                                       10,     // labelHeight to be single ln 
                                       Item.LAYOUT_RIGHT),
                    50, 0,  // checked label location
                    50, 22); // checked content location

        // there IS extra space horizontally, 
        // LAYOUT_RIGHT is set
        // non null label, non null content
        // label and content on different lines
        // label width > content width
        declare("test22");
        checkLayout(new TestItemLFImpl(30, 20,  // labelSize
                                       50, 20,  // contentSize
                                       100, 32, // itemSize
                                       10,     // labelHeight to be single ln 
                                       Item.LAYOUT_RIGHT),
                    50, 0,  // checked label location
                    50, 22); // checked content location

        // ************ more space VERTICALLY  **************************
        // ****************************************************************

        // there IS extra space vertically, 
        // LAYOUT_VCENTER and LAYOUT_BOTTOM are NOT set
        // null label, non null content
        declare("test23");
        checkLayout(new TestItemLFImpl(0, 0,    // labelSize
                                       50, 50,  // contentSize
                                       50, 100, // itemSize
                                       10),     // labelHeight to be single ln 
                    0, 0,  // checked label location
                    0, 0); // checked content location
        
        // there IS extra space vertically, 
        // LAYOUT_VCENTER and LAYOUT_BOTTOM are NOT set
        // non null label, null content
        declare("test24");
        checkLayout(new TestItemLFImpl(50, 50,   // labelSize
                                       0, 0,     // contentSize
                                       50, 100, // itemSize
                                       10),     // labelHeight to be single ln 
                    0, 0,  // checked label location
                    0, 0); // checked content location


        // there IS extra space vertically, 
        // LAYOUT_VCENTER and LAYOUT_BOTTOM are NOT set
        // non null label, non null content
        // label and content on the same line;
        // label height > content height
        declare("test25");
        checkLayout(new TestItemLFImpl(47, 20,    // labelSize
                                       50, 10,  // contentSize
                                       100, 100, // itemSize
                                       20),     // labelHeight to be single ln 
                    0, 0,  // checked label location
                    50, 5); // checked content location

        // there IS extra space vertically, 
        // LAYOUT_VCENTER and LAYOUT_BOTTOM are NOT set
        // non null label, non null content
        // label and content on the same line
        // label height < content height
        declare("test26");
        checkLayout(new TestItemLFImpl(47, 10,    // labelSize
                                       50, 20,  // contentSize
                                       100, 100, // itemSize
                                       10),    // labelHeight to be single ln 
                    0, 5,  // checked label location
                    50, 0); // checked content location

        // there IS extra space vertically, 
        // LAYOUT_VCENTER and LAYOUT_BOTTOM are NOT set
        // non null label, non null content
        // label and content are on different lines
        // label width < content width
        declare("test27");
        checkLayout(new TestItemLFImpl(30, 20,  // labelSize
                                       50, 20,  // contentSize
                                       50, 100, // itemSize
                                       10),     // labelHeight to be single ln 
                    0, 0,  // checked label location
                    0, 22); // checked content location


        // there IS extra space vertically, 
        // LAYOUT_VCENTER and LAYOUT_BOTTOM are NOT set
        // non null label, non null content
        // label and content are on different lines
        // label width > content width
        declare("test28");
        checkLayout(new TestItemLFImpl(50, 20,  // labelSize
                                       30, 20,  // contentSize
                                       50, 100, // itemSize
                                       10),     // labelHeight to be single ln 
                    0, 0,  // checked label location
                    0, 22); // checked content location


        // **********************************************
        
        // there IS extra space vertically, 
        // LAYOUT_VCENTER is set
        // non null label, non null content
        // label and content on the same line;
        // label height > content height
        declare("test29");
        checkLayout(new TestItemLFImpl(47, 20,    // labelSize
                                       50, 10,  // contentSize
                                       100, 100, // itemSize
                                       20,     // labelHeight to be single ln 
                                       Item.LAYOUT_VCENTER),
                    0, 40,  // checked label location
                    50, 45); // checked content location
        
        // there IS extra space vertically, 
        // LAYOUT_VCENTER is set
        // non null label, non null content
        // label and content on the same line
        // label height < content height
        declare("test30");
        checkLayout(new TestItemLFImpl(47, 10,    // labelSize
                                       50, 20,  // contentSize
                                       100, 100, // itemSize
                                       10,     // labelHeight to be single ln 
                                       Item.LAYOUT_VCENTER),
                    0, 45,  // checked label location
                    50, 40); // checked content location


        // there IS extra space vertically, 
        // LAYOUT_VCENTER is set
        // non null label, non null content
        // label and content on different lines
        // label width < content width
        declare("test31");
        checkLayout(new TestItemLFImpl(30, 18,  // labelSize
                                       50, 20,  // contentSize
                                       50, 100, // itemSize
                                       10,     // labelHeight to be single ln 
                                       Item.LAYOUT_VCENTER),
                    0, 30,  // checked label location
                    0, 50); // checked content location

        // there IS extra space vertically, 
        // LAYOUT_VCENTER is set
        // non null label, non null content
        // label and content on different lines
        // label width > content width
        declare("test32");
        checkLayout(new TestItemLFImpl(50, 18,  // labelSize
                                       30, 20,  // contentSize
                                       50, 100, // itemSize
                                       10,     // labelHeight to be single ln 
                                       Item.LAYOUT_VCENTER),
                    0, 30,  // checked label location
                    0, 50); // checked content location


        // *************************************************************

        // there IS extra space vertically, 
        // LAYOUT_BOTTOM is set
        // non null label, non null content
        // label and content are on the same line;
        // label height > content height
        declare("test33");
        checkLayout(new TestItemLFImpl(47, 20,    // labelSize
                                       50, 10,  // contentSize
                                       100, 100, // itemSize
                                       20,     // labelHeight to be single ln 
                                       Item.LAYOUT_BOTTOM),
                    0, 80,  // checked label location
                    50, 85); // checked content location

        // there IS extra space vertically, 
        // LAYOUT_BOTTOM is set
        // non null label, non null content
        // label and content on the same line
        // label height < content height
        declare("test34");
        checkLayout(new TestItemLFImpl(47, 10,    // labelSize
                                       50, 20,  // contentSize
                                       100, 100, // itemSize
                                       10,     // labelHeight to be single ln 
                                       Item.LAYOUT_BOTTOM),
                    0, 85,  // checked label location
                    50, 80); // checked content location

        // there IS extra space vertically, 
        // LAYOUT_BOTTOM is set
        // non null label, non null content
        // label and content on different lines
        // label width < content width
        declare("test35");
        checkLayout(new TestItemLFImpl(30, 18,  // labelSize
                                       50, 20,  // contentSize
                                       50, 100, // itemSize
                                       10,     // labelHeight to be single ln 
                                       Item.LAYOUT_BOTTOM),
                    0, 60,  // checked label location
                    0, 80); // checked content location

        // there IS extra space vertically, 
        // LAYOUT_BOTTOM is set
        // non null label, non null content
        // label and content on different lines
        // label width > content width
        declare("test36");
        checkLayout(new TestItemLFImpl(30, 18,  // labelSize
                                       50, 20,  // contentSize
                                       50, 100, // itemSize
                                       10,     // labelHeight to be single ln 
                                       Item.LAYOUT_BOTTOM),
                    0, 60,  // checked label location
                    0, 80); // checked content location

        // ***** more space VERTICALLY and HORIZONTALLY ******************
        // ****************************************************************

        // there IS extra space vertically and horizontally 
        // LAYOUT_CENTER & LAYOUT_VCENTER is set
        // non null label, non null content
        // label and content are on the same line;
        declare("test37");
        checkLayout(new TestItemLFImpl(47, 20,    // labelSize
                                       50, 10,  // contentSize
                                       200, 100, // itemSize
                                       20,     // labelHeight to be single ln 
                                       Item.LAYOUT_CENTER | 
                                       Item.LAYOUT_VCENTER),
                    50, 40,  // checked label location
                    100, 45); // checked content location


        // there IS extra space vertically and horizontally, 
        // LAYOUT_CENTER & LAYOUT_VCENTER is set
        // non null label, non null content
        // label and content are on different lines
        declare("test38");
        checkLayout(new TestItemLFImpl(30, 18,  // labelSize
                                       50, 20,  // contentSize
                                       100, 100, // itemSize
                                       10,     // labelHeight to be single ln 
                                       Item.LAYOUT_CENTER | 
                                       Item.LAYOUT_VCENTER),
                    35, 30,  // checked label location
                    25, 50); // checked content location

        // there IS extra space vertically and horizontally 
        // LAYOUT_RIGHT & LAYOUT_BOTTOM is set
        // non null label, non null content
        // label and content are on the same line;
        declare("test39");
        checkLayout(new TestItemLFImpl(47, 20,    // labelSize
                                       50, 10,  // contentSize
                                       200, 100, // itemSize
                                       20,     // labelHeight to be single ln 
                                       Item.LAYOUT_RIGHT | Item.LAYOUT_BOTTOM),
                    100, 80,  // checked label location
                    150, 85); // checked content location

        // there IS extra space vertically and horizontally, 
        // LAYOUT_RIGHT & LAYOUT_BOTTOM is set
        // non null label, non null content
        // label and content are on different lines
        declare("test40");
        checkLayout(new TestItemLFImpl(30, 18,  // labelSize
                                       50, 20,  // contentSize
                                       100, 100, // itemSize
                                       10,     // labelHeight to be single ln 
                                       Item.LAYOUT_RIGHT | Item.LAYOUT_BOTTOM),
                    50, 60,  // checked label location
                    50, 80); // checked content location
    }

    private void checkLayout(TestItemLFImpl item, int labelX, int labelY,
                             int contentX, int contetnY) {
        item.checkLayout(labelX, labelY, contentX, contetnY);
    }

    class TestItem extends Item {
        TestItem() {
            super(null);
        }
    }

    class TestItemLFImpl extends ItemLFImpl {
        TestItemLFImpl(int labelWidth, int labelHeight,
                       int contentWidth, int contentHeight,
                       int w, int h, 
                       int labelFontHeight) {
            this(labelWidth, labelHeight, contentWidth, contentHeight,
                 w, h, labelFontHeight, Item.LAYOUT_DEFAULT);
        }

        TestItemLFImpl(int labelWidth, int labelHeight,
                       int contentWidth, int contentHeight,
                       int w, int h, 
                       int labelFontHeight, int layout) {
            super(new TestItem());
            item.itemLF = this;

            item.setLayout(layout);

            this.labelFontHeight = labelFontHeight;

            labelBounds[X] = labelBounds[Y] = -1;
            labelBounds[WIDTH] = labelWidth;
            labelBounds[HEIGHT] = labelHeight;

            contentBounds[X] = contentBounds[Y] = -1;
            contentBounds[WIDTH] = contentWidth;
            contentBounds[HEIGHT] = contentHeight;

            // We assume that labelBounds and contentBounds 
            // passed into lDoInternalLayout() are correct
            // that is why we need cachedWidth == w
            cachedWidth = w;

            lDoInternalLayout(labelBounds, contentBounds, w, h);
        }

        void checkLayout(int labelX, int labelY, int contentX, int contentY) {

            assertEquals(labelBounds[X], labelX);
            assertEquals(labelBounds[Y], labelY);
            assertEquals(contentBounds[X], contentX);
            assertEquals(contentBounds[Y], contentY);
        }

	// overriding this function removes dependency on the
	// font height actually used on the platform.
	boolean labelAndContentOnSameLine(int labelHeight) {
	    return labelHeight <= labelFontHeight;
	}

        int getHorizontalPad() {
	    return 3;
	}

	int getVerticalPad() {
	    return 2;
	}

        private int labelFontHeight;
    }
}

/*
 * @(#)FrameXWindow.java	1.13 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
 *
 */

package java.awt;

/** X window used for a Frame.
 @version 1.9, 08/19/02
 @author Nicholas Allen */

class FrameXWindow extends WindowXWindow {
    FrameXWindow (Frame frame) {
        super(frame);
        setTitle(frame.title);
        setIconImage(frame.icon);
        setResizable(frame.resizable);
        topBorder = defaultTopBorder;
        leftBorder = defaultLeftBorder;
        bottomBorder = defaultBottomBorder;
        rightBorder = defaultRightBorder;
    }
	
    native int create(int owner);
	
    void setDefaultBorders(int top, int left, int bottom, int right) {
        defaultTopBorder = top;
        defaultBottomBorder = bottom;
        defaultLeftBorder = left;
        defaultRightBorder = right;
    }
    private static int defaultTopBorder = 20;
    private static int defaultLeftBorder = 5;
    private static int defaultBottomBorder = 5;
    private static int defaultRightBorder = 5;
}

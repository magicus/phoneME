/*
 * @(#)WindowManagementFeedback.java	1.13 06/10/10
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

package sun.porting.windowsystem;

/** 
 * The <code>WindowManagementFeedback</code> interface
 * allows the toolkit to implement some means of interactive window
 * movement for top-level windows.  The window system or graphics library
 * may implement the feedback for these operations by a platform-specific means;
 * it then exposes the functionality safely through this interface.
 *
 * @version 1.8, 08/19/02
 */
public interface WindowManagementFeedback {
    /**
     * Start window management feedback using a rectangle of the given
     * dimensions.  All coordinates are in global (framebuffer) space.
     * @param win The window to be reshaped.  (This allows the window system
     * to show the whole window during moves and/or reshapes).
     * @param x x coordinate of the feedback rectangle's upper-left corner.
     * @param y y coordinate of the feedback rectangle's upper-left corner.
     * @param w Width of the feedback rectangle.
     * @param h Height of the feedback rectangle.
     * @throws IllegalStateException if feedback is already in progress.
     * @throws IllegalArgumentException if win is not a top-level window.
     */
    void startFeedback(Window win, int x, int y, int w, int h) 
        throws IllegalStateException, IllegalArgumentException;
    /**
     * Resize the feedback rectangle to the given dimensions.
     * @param win The window to be reshaped.  (This allows the window system
     * to show the whole window during moves and/or reshapes).
     * @param x x coordinate of the feedback rectangle's upper-left corner.
     * @param y y coordinate of the feedback rectangle's upper-left corner.
     * @param w Width of the feedback rectangle.
     * @param h Height of the feedback rectangle.
     * @throws IllegalStateException if no feedback is in progress.
     * @throws IllegalArgumentException if win is not the same window
     * passed to startFeedback.
     */
    void moveFeedback(Window win, int x, int y, int w, int h)
        throws IllegalStateException, IllegalArgumentException;
    /**
     * End feedback, e.g. hide the feedback rectangle.
     * @param win The window to be reshaped.  (This allows the window system
     * to show the whole window during moves and/or reshapes).
     * @param x x coordinate of the feedback rectangle's upper-left corner.
     * @param y y coordinate of the feedback rectangle's upper-left corner.
     * @param w Width of the feedback rectangle.
     * @param h Height of the feedback rectangle.
     * @throws IllegalStateException if no feedback is in progress.
     * @throws IllegalArgumentException if win is not the same window
     * passed to startFeedback.
     */
    void endFeedback(Window win, int x, int y, int w, int h) 
        throws IllegalStateException, IllegalArgumentException;
}

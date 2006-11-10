/*
 *
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

package javax.microedition.lcdui;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

/**
* This is the look &amps; feel implementation for Screen.
*/
class ScreenLFImpl extends DisplayableLFImpl {

    // ************************************************************
    //  public methods
    // ************************************************************

    /**
     * Override DisplayableLFImpl.lCallHide() to set local variables.
     */
    void lCallHide() {
        // NOTE that resetToTop is also set to false 
        // Display.setCurrentItem() is called.
        // Because of that just knowing current state of DisplayLF
        // is not enough to set it properly in lCallShow.
        // That is why it has to be updated in lCallHide and lCallFreeze
        resetToTop = true;
        super.lCallHide();
    }

    /**
     * Override DisplayableLFImpl.lCallFreeze() to set local variables.
     */
    void lCallFreeze() {
        if (state == SHOWN) {
            resetToTop = false;
        }
        super.lCallFreeze();
    }

    // ************************************************************
    //  package private methods
    // ************************************************************
    
    /**
     * Creates ScreenLF for the passed in screen.
     * @param screen the Screen object associated with this look&feel
     */ 
    ScreenLFImpl(Screen screen) {
        
        super(screen);
        
        viewable = new int[4];
        viewable[X] = 0;
        viewable[Y] = 0;
        viewable[WIDTH] = 0;
        viewable[HEIGHT] = 0;
    }
        
    /**
     * Paint the contents of this Screen
     *
     * @param g the Graphics to paint to
     * @param target the target Object of this repaint
     */
    public void uCallPaint(Graphics g, Object target) {
        if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
            Logging.report(Logging.INFORMATION, LogChannels.LC_HIGHUI,
                           "Screen:Clip: " +
                           g.getClipX() + "," + g.getClipY() + "," +
                           g.getClipWidth() + "," + g.getClipHeight());
        }
    }

    /**
     * Set the vertical scroll position and proportion
     *
     * @param scrollPosition The vertical scroll position to set on a
     *                       scale of 0-100
     * @param scrollProportion The vertical scroll proportion to set on
     *                         a scale of 0-100. For example, if the viewport
     *                         is 25 pixels high and the Displayable is 100
     *                         pixels high, then the scroll proportion would
     *                         be 25, since only 25% of the Displayable can
     *                         be viewed at any one time. This proportion
     *                         value can be used by implementations which
     *                         render scrollbars to indicate scrollability
     *                         to the user.
     */
    void setVerticalScroll(int scrollPosition, int scrollProportion) {
        this.vScrollPosition = scrollPosition;
        this.vScrollProportion = scrollProportion;
            
        if (lIsShown()) {
            currentDisplay.setVerticalScroll(scrollPosition, scrollProportion);
        }
    }
    
    /**
     * Get the current vertical scroll position
     *
     * @return int The vertical scroll position on a scale of 0-100
     */
    int getVerticalScrollPosition() {
        // SYNC NOTE: return of atomic value
        return vScrollPosition;
    }
    
    /**
     * Get the current vertical scroll proportion
     *
     * @return ing The vertical scroll proportion on a scale of 0-100
     */
    int getVerticalScrollProportion() {
        // SYNC NOTE: return of atomic value
        return vScrollProportion;
    }


    /**
     * Set the vertical scroll indicators for this Screen
     */
    void setVerticalScroll() {
        
        if (viewable[HEIGHT] <= viewport[HEIGHT]) {
            setVerticalScroll(0, 100);
        } else {
            setVerticalScroll((viewable[Y] * 100 /
                                  (viewable[HEIGHT] - viewport[HEIGHT])),
                              (viewport[HEIGHT] * 100 / viewable[HEIGHT]));
        }
    }
    /**
     * Paint an Item contained in this Screen. The Item requests a paint
     * in its own coordinate space. Screen translates those coordinates
     * into the overall coordinate space and schedules the repaint
     *
     * @param item the Item requesting the repaint
     * @param x the x-coordinate of the origin of the dirty region
     * @param y the y-coordinate of the origin of the dirty region
     * @param w the width of the dirty region
     * @param h the height of the dirty region
     */
    void lRequestPaintItem(Item item, int x, int y, int w, int h) {
        
        ItemLFImpl iLF = (ItemLFImpl)item.getLF();

        lRequestPaint(iLF.bounds[X] - viewable[X] + x,
                      iLF.bounds[Y] - viewable[Y] + y,
                      w, h);
    }

    // **************************************************************

    // ************************************************************
    //  public member variables - NOT ALLOWED in this class
    // ************************************************************
    
    // ************************************************************
    //  protected member variables - NOT ALLOWED in this class
    // ************************************************************
    
    // ************************************************************
    //  package private member variables
    // ************************************************************
    
    /**
     * An array which holds the scroll location and
     * the overall dimensions of the view being
     * shown in the parent Displayable's viewport
     * Note that the following is always true.
     * 0 <= viewable[X] <= viewable[WIDTH] - viewport[WIDTH]
     * 0 <= viewable[Y] <= viewable[HEIGHT] - viewport[HEIGHT]
     */
    int viewable[];
    
    /**
     * Screens should automatically reset to the top of the when
     * they are shown, except in cases where it is interrupted by
     * a system menu or an off-screen editor - in which case it
     * should be reshown exactly as it was.
     */
    boolean resetToTop = true;

    // ************************************************************
    //  private member variables
    // ************************************************************
    
    /** The vertical scroll position */
    private int vScrollPosition     = 0;
    
    /** The vertical scroll proportion */
    private int vScrollProportion   = 100;

    // ************************************************************
    //  Static initializer, constructor
    // ************************************************************
}

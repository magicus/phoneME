/*
 * $LastChangedDate: 2005-09-18 20:31:12 +0900 (일, 18 9 2005) $  
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package javax.microedition.lcdui;

/**
 * This interface should be implemented by a client that needs a popup keyboard.
 * @author Amir Uval
 */


interface VirtualKeyboardListener {
                
    /**
     * key input callback
     */
    public void virtualKeyEntered(int type, char c);
    
    /**
     * meta key event callback
     */
    public void virtualMetaKeyEntered(int metaKey);
    
    /**
     * a callback used to draw the text entered by the virtual keyboard - on the
     * keyboard text area.
     * In KeyboardLayer, it is implemented by passing the call to 
     * TextField's paint(..)
     */
    public void paintTextOnly(Graphics g, int width, int height);
    
    /**
     * should return the width of the owner Displayable
     */
    public int getAvailableWidth();
    
    /**
     * should return the height of the owner Displayable
     */
    public int getAvailableHeight(); 
    
    /**
     * should trigger a requestRepaint() call to schedule a 
     * paint() of the VirtualKeyboard
     */
    public void repaintVK();
     

}


/*
 * $LastChangedDate: 2005-11-21 02:11:20 +0900 (월, 21 11 2005) $  
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package javax.microedition.lcdui;

import com.sun.midp.lcdui.*;
import com.sun.midp.configurator.Constants;
import com.sun.midp.chameleon.skins.*;
import com.sun.midp.chameleon.layers.PopupLayer;

import java.util.*;

/**
 * This is a popup layer that handles a sub-popup within the text tfContext
 */

class VirtualKeyboard {

    /** indicates whether the virtual keyboard is enabled */
    public static final boolean USE_VIRTUAL_KEYBOARD = true;

    /** indicates whether the virtual keypad is enabled */
    public static final boolean USE_VIRTUAL_KEYPAD = true;

    /** indicates whether the virtual keyboard is opened automatically */
    public static final boolean USE_VIRTUAL_KEYBOARD_OPEN_AUTO = false;

    /** instance of the virtual keyboard listener */
    VirtualKeyboardListener vkl;
    
    // keyboard dimensions
    int kbX;
    int kbY;
    int kbWidth;
    int kbHeight;

    int fontH;       // height of 'M'
    int buttonW;     // width of keyboard
    int buttonH;     // height of keyboard
  
    int currentChar = 0;
    int currentKeyboardIndex = 0; // abc

    int textfieldHeight = 0; // height of text field area, including adornments

    char itemIndexWhenPressed;
    char PRESS_OUT_OF_BOUNDS = 0;

    /** array of all available keys n the keyboard */
    boolean inShift = false;
    boolean textKbd = false;
    Font f;

    /**
     * Virtual Keyboard constructor.
     * 
     * @param keys array of available keys for the keyboard
     * @param vkl the virtual keyboard listener
     * @param displayTextArea flag to indicate whether to display the text area
     */
    public VirtualKeyboard(Vector maps, 
                           VirtualKeyboardListener vkl,
                           boolean displayTextArea) {

        this.keyboardMaps = maps;

        this.vkl = vkl;

        textKbd = displayTextArea;
        if(textKbd){
            changeKeyboad(1);
            PADDING = 4;
        } else {
            changeKeyboad(0);
        }
            
        
        kbWidth = vkl.getAvailableWidth();

        kbHeight = 0;

        for(int h=0; h < keyboardMaps.size(); h++) {
            Vector keyboard = (Vector)keyboardMaps.elementAt(h);
            for(int i=0; i < keyboard.size(); i++) {
    
                Vector line = (Vector)keyboard.elementAt(i);
               for(int j=0; j < line.size(); j++) {
                    Key key = (Key)(line.elementAt(j));
                    int keyBottom = key.getY() + key.getHeight();
                    if ( keyBottom > kbHeight)
                        kbHeight = keyBottom;  
                }
            }
        }

        if (textKbd) {
            kbHeight +=  2*PADDING;
        }

        f = Font.getFont(Font.FACE_PROPORTIONAL, // or SYSTEM
                         Font.STYLE_PLAIN, 
                         Font.SIZE_SMALL);
        fontH = f.getHeight();

        
        if (textKbd) {
            textfieldHeight = fontH + 3 * PADDING;
            kbHeight +=  textfieldHeight;
        } 

        
        kbX = 0;
        kbY = vkl.getAvailableHeight()-kbHeight;
     }

    /**
     * Checks if the virtual keyboard is enabled.
     * @return <code>true</code> if the virtual keyboard is enabled,
     *         <code>false</code> otherwise.
     */
    static boolean isKeyboardEnabled(){
        return USE_VIRTUAL_KEYBOARD;
    }

    /**
     * Checks if the virtual keypad is enabled.
     * @return <code>true</code> if the virtual keypad is enabled,
     *         <code>false</code> otherwise.
     */
    static boolean isKeypadEnabled(){
        return USE_VIRTUAL_KEYPAD;
    }

    /**
     * Checks if the virtual keyboard is opened automatically.
     * @return <code>true</code> if the virtual keyboard is opened automatically,
     *         <code>false</code> otherwise.
     */
    static boolean isAutoOpen(){
        return USE_VIRTUAL_KEYBOARD_OPEN_AUTO;
    }

    /**
     * traverse the virtual keyboard according to key pressed.
     * 
     * @param type type of keypress
     * @param keyCode key code of key pressed
     */
    void traverse(int type, int keyCode) {

        // Soft button means dismiss to the virtual keyboard
        if (type == EventConstants.RELEASED && keyCode == EventConstants.SOFT_BUTTON2) {
            vkl.virtualKeyEntered(type, 0);
            return;
        }

            if (type == EventConstants.RELEASED &&
                keyCode != Constants.KEYCODE_SELECT) {
                // in this case we don't want to traverse on key release
                
            } else {
              switch (keyCode) {
                case Constants.KEYCODE_RIGHT: {
                    int pos = currentLine.indexOf(currentKey);
		    pos++;
		    if (pos >= currentLine.size()) {
			pos = 0;
		    }
                    currentKey = (Key)currentLine.elementAt(pos);
                }; break;
		case Constants.KEYCODE_LEFT: {
                    int pos = currentLine.indexOf(currentKey);
                    pos--;
                    if (pos < 0) {
			pos = currentLine.size()-1;
		    }
                    currentKey = (Key)currentLine.elementAt(pos);
		}; break;
		case Constants.KEYCODE_UP:
                    int xpos = currentLine.indexOf(currentKey);
                    int ypos = currentKeyboard.indexOf(currentLine);

                    ypos--;

                    if (ypos < 0) {
                        ypos = currentKeyboard.size()-1;
		    }
                    currentLine = (Vector)currentKeyboard.elementAt(ypos);
                    if (xpos >= currentLine.size() )
                        xpos = currentLine.size()-1;
                    currentKey = (Key)currentLine.elementAt(xpos);
		    break;
		case Constants.KEYCODE_DOWN:
		    xpos = currentLine.indexOf(currentKey);
                    ypos = currentKeyboard.indexOf(currentLine);

                    ypos++;

                    if (ypos >= currentKeyboard.size()) {
                        ypos = 0;
		    }
                    currentLine = (Vector)currentKeyboard.elementAt(ypos);
                    if (xpos >= currentLine.size() )
                        xpos = currentLine.size()-1;
                    currentKey = (Key)currentLine.elementAt(xpos);
                    break;
              case Constants.KEYCODE_SELECT:
                    if(currentKey!= null) {
                        int key = currentKey.getKey();

                        switch (key) {
                            case SHIFT_META_KEY: //"Shift" - one shot upper case
                                if (type == EventConstants.PRESSED) break;
        			if (currentKeyboardIndex == 1) {  // lower case
                                    changeKeyboad(2);
        			    vkl.virtualMetaKeyEntered(IM_CHANGED_KEY);
                                    inShift = true;
        			} else
        			if (currentKeyboardIndex == 2) {  // lower case
                                    changeKeyboad(1);
        			    vkl.virtualMetaKeyEntered(IM_CHANGED_KEY);
                                    inShift = false;
        			}
        			break;
                            case CAPS_META_KEY: //"CapsL"  (caps lock)
                                if (type == EventConstants.PRESSED) break;
        			if (currentKeyboardIndex == 1) {  // lower case
                                    changeKeyboad(2);
        			} else if (currentKeyboardIndex == 2) {  // upper case
                                    changeKeyboad(1);
        			}
                             vkl.virtualMetaKeyEntered(IM_CHANGED_KEY);
        			break;
        		    case MODE_META_KEY: //"Mode"  
                                if (type == EventConstants.PRESSED) break;
                                changeKeyboad((currentKeyboardIndex+1) % keyboardMaps.size());
                             vkl.virtualMetaKeyEntered(IM_CHANGED_KEY);
        			break;
        		    case BACKSPACE_META_KEY: //"backspace" 
                                if (type == EventConstants.RELEASED) break;
                                vkl.virtualMetaKeyEntered(BACKSPACE_META_KEY);
        			break;
                            case OK_META_KEY: //"ok"
                                if (type == EventConstants.PRESSED) break;
        			vkl.virtualMetaKeyEntered(OK_META_KEY);
        			break;
        		    case CANCEL_META_KEY: //"cancel"
                                if (type == EventConstants.PRESSED) break;
                                vkl.virtualMetaKeyEntered(CANCEL_META_KEY);
        			break;

                            default: 
                                currentKey.setStatus(type==EventConstants.PRESSED?Key.PRESSED:Key.RELEASED);
                                vkl.virtualKeyEntered(type,currentKey.getKey());

                                if (inShift && type == EventConstants.PRESSED) {
                                        //shift is a one-shot upper case
                                        inShift = false;
                                        if(textKbd){
                                            changeKeyboad(1);
                                            vkl.virtualMetaKeyEntered(IM_CHANGED_KEY);			  
                                        } //hk : still need a keyboard displayed
                                        else{
                                            changeKeyboad(0);
                                        }
                                    }
                            
                        }
		    }                    
		}
            }
            

        // triggers paint()
        vkl.repaintVK();
    }

    /**
     * paint the virtual keyboard on the screen
     * 
     * @param g The graphics context to paint to
     */
    protected void paint(Graphics g) {

        g.setFont(f);
        g.setColor(DARK_GRAY);

        if (textKbd) {
         //   g.fillRect(0,0,kbWidth,kbHeight);
            //drawBorder(g,0,0,kbWidth-1,kbHeight-1);
            if (textfieldHeight > 0) {
                drawTextField(g);
                g.translate(0,textfieldHeight+2*PADDING);
            }
        }

        for(int i=0; i < currentKeyboard.size(); i++) {
            Vector line = (Vector) currentKeyboard.elementAt(i);
            for(int j=0; j < line.size(); j++) {
                Key key = (Key)(line.elementAt(j));
                key.paint(g);
                if (key==currentKey) {
                    key.paintSelection(g);
                }
            }  
        }


    }

    /**
     * Draw the text field of the virtual keyboard.
     * 
     * @param g The graphics context to paint to
     */
    void drawTextField(Graphics g) {
        drawSunkedBorder(g,PADDING,PADDING,
                         kbWidth - 2*PADDING, textfieldHeight); 

        g.setClip(0,0,
                         kbWidth - 2*PADDING, textfieldHeight); 


        g.translate(PADDING + 1,PADDING);

        vkl.paintTextOnly(g,kbWidth - 3*PADDING,
                          textfieldHeight - 2*PADDING);

        g.translate(-PADDING - 1,-PADDING);
        g.setClip(0,0,kbWidth,kbHeight);
    }

   

    /**
     * draw a border
     * 
     * @param g The graphics context to paint to
     * @param x1 x-coordinate of the button's location
     * @param y1 y-coordinate of the button's location
     * @param x2 the x-coordinate at the width of the border
     * @param y2 the y-coordinate at the height of the border
     */
    private void drawBorder(Graphics g, int x1, int y1, int x2, int y2) {

        g.setColor(GRAY);
        g.drawLine(x1+2,y1+2,x1+2,y2-3);    // left
        g.drawLine(x1+2,y1+2,x2-2,y1+2);    // top
        g.drawLine(x1+2,y2-1,x2-1,y2-1);    // bottom
        g.drawLine(x2-1,y1+2,x2-1,y2-1);    // right
        g.setColor(WHITE);
        g.drawRect(x1+1,y1+1,x2-x1-3,y2-y1-3);
    }

    /**
     * draw a sunken border
     * 
     * @param g The graphics context to paint to
     * @param x1 x-coordinate of the button's location
     * @param y1 y-coordinate of the button's location
     * @param x2 the x-coordinate at the width of the border
     * @param y2 the y-coordinate at the height of the border
     */
    private void drawSunkedBorder(Graphics g, int x1, int y1, int x2, int y2) {

        g.setColor(WHITE);
        g.fillRect(x1+2,y1+2,x2-x1-2,y2-y1-2);

        g.setColor(GRAY);
        g.drawLine(x1+2,y1+2,x1+2,y2-2);    //left
        g.drawLine(x1+2,y1+2,x2-2,y1+2);    //top
        g.setColor(DARK_GRAY);
        g.drawLine(x1+3,y1+3,x1+3,y2-3);    //left
        g.drawLine(x1+3,y1+3,x2-3,y1+3);    //top

        g.setColor(LIGHT_GRAY);
        g.drawLine(x1+3,y2-2,x2-2,y2-2);    //bottom
        g.drawLine(x2-2,y1+3,x2-2,y2-2);    //right
    }

    /**
     * Handle input from a pen tap. Parameters describe
     * the type of pen event and the x,y location in the
     * layer at which the event occurred. Important : the
     * x,y location of the pen tap will already be translated
     * into the coordinate space of the layer.
     *
     * @param type the type of pen event
     * @param x the x coordinate of the event
     * @param y the y coordinate of the event
     */
    public boolean pointerInput(int type, int x, int y) {


        Vector newLine = null;
        Key newKey = null;

        if (textfieldHeight > 0) {
            y-=(textfieldHeight+2*PADDING);
        }        

        for(int i=0; i < currentKeyboard.size(); i++) {
            Vector line = (Vector) currentKeyboard.elementAt(i);
            for(int j=0; j < line.size(); j++) {
                Key key = (Key)(line.elementAt(j));
                if(key.pointerInput(type,x,y)) {
                    newKey = key;
                    newLine = line;
                }
            }  
        }
        
        switch (type) {
        case EventConstants.PRESSED:
           
            if ( newKey != null) {
                currentKey.setStatus(Key.RELEASED);
                currentKey = newKey;
                currentLine = newLine;
             // press on valid key
                traverse(type,Constants.KEYCODE_SELECT);
                vkl.repaintVK();
               
            }
            break;
        case EventConstants.RELEASED:
            if ( newKey != null) {
                    currentKey.setStatus(Key.RELEASED);
                    currentKey = newKey;
                    currentLine = newLine;
                    traverse(type,Constants.KEYCODE_SELECT);
                    vkl.repaintVK();
               
            }

            break;
        }
        // return true always as menuLayer will capture all of the pointer inputs
        return true;  
    }

    // ********* attributes ********* //

    private final static int WHITE = 0xffffff;

    // blue scheme
    private final static int DARK_GRAY = 0x666666;
    private final static int GRAY = 0x3366cc;//0x8DB0D9;
    private final static int LIGHT_GRAY = 0xcccccc;
    private final static int TEXT_COLOR = 0xCCFFFF;//0xccccff

    /** padding between rows of buttons */
    private int PADDING;
    
    // If you want to change the order of the buttons, just
    // change the serial numbers here:
    final static int OK_META_KEY = -100;
    final static int CANCEL_META_KEY = -101;
    final static int MODE_META_KEY = -102;
    final static int BACKSPACE_META_KEY = -103;
    final static int SHIFT_META_KEY = -104;
    final static int CAPS_META_KEY = -105;

    //When input method is changed, process this key to update UI 
    final static int IM_CHANGED_KEY = -99;

    void changeKeyboad( int which) {
        currentKeyboardIndex = which;
        if (currentKeyboardIndex>=keyboardMaps.size()) {
            currentKeyboardIndex=keyboardMaps.size()-1;
        }
        currentKeyboard = (Vector)keyboardMaps.elementAt(currentKeyboardIndex);
        currentLine = (Vector)currentKeyboard.elementAt(0);
        currentKey = (Key)currentLine.elementAt(0);
    }

    Vector keyboardMaps;
    Vector currentKeyboard;
    Vector currentLine;
    Key currentKey;
}


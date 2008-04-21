/*
 * $LastChangedDate: 2006-03-06 01:36:46 +0900 (월, 06 3 2006) $  
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package javax.microedition.lcdui;

import com.sun.midp.lcdui.*;
import com.sun.midp.configurator.Constants;
import com.sun.midp.chameleon.skins.DateEditorSkin;
import com.sun.midp.chameleon.skins.ChoiceGroupSkin;
import com.sun.midp.chameleon.layers.PopupLayer;
import com.sun.midp.chameleon.input.*;

import com.sun.midp.security.SecurityToken;
import com.sun.midp.security.SecurityInitializer;
import com.sun.midp.security.ImplicitlyTrustedClass;

import com.sun.midp.util.ResourceHandler;

import java.util.Vector;

/**
 * This is a popup layer that handles a sub-popup within the text tfContext
 */
class KeyboardLayer extends PopupLayer implements VirtualKeyboardListener {

    /** the instance of the virtual keyboard */
    VirtualKeyboard vk = null;

    String layerID = null;

    /**
     * Constructs a text tfContext sub-popup layer, which behaves like a
     * popup-choicegroup, given a string array of elements that constitute
     * the available list of choices to select from.
     *
     * @param tf The TextEditor that triggered this popup layer.
     */
    private KeyboardLayer(TextFieldLFImpl tf) {
        super((Image)null, -1); // don't draw a background  

        this.layerID  = "KeyboardLayer";
        tfContext = tf;
        transparent = false;
        //backupstring is set to original text before the kbd was used
        backupString = tfContext.tf.getString();        
        if (vk==null) {
            prepareKeyMapTextField();
            vk = new VirtualKeyboard(Maps, this, true);
        }

        //System.out.println("vk.kbX:"+vk.kbX+",vk.kbY:"+vk.kbY+",vk.kbWidth:"+vk.kbWidth+",vk.kbHeight:"+vk.kbHeight);
        setBounds(vk.kbX, vk.kbY, vk.kbWidth, vk.kbHeight);

        Command keyboardClose = new Command("Close", Command.OK, 1);
        setCommands(new Command[] { keyboardClose });
    }       

    /**
     * Constructs a canvas sub-popup layer, which behaves like a
     * popup-choicegroup, given a string array of elements that constitute
     * the available list of choices to select from.
     *
     * @param canvas The Canvas that triggered this popup layer.
     */
    private KeyboardLayer(CanvasLFImpl canvas) {
        super((Image)null, -1); // don't draw a background  

        this.layerID  = "KeyboardLayer";
        transparent = true;
        setOpaque(false);
        tfContext = null;
        cvContext = canvas;
        if (vk==null) {
            prepareKeyMapCanvas();
            vk = new VirtualKeyboard(Maps, this, false);
        }

        //System.out.println("vk.kbX:"+vk.kbX+",vk.kbY:"+vk.kbY+",vk.kbWidth:"+vk.kbWidth+",vk.kbHeight:"+vk.kbHeight);
	setBounds(vk.kbX,vk.kbY,vk.kbWidth,vk.kbHeight);

        Command keypadClose = new Command("Close", Command.OK, 1);
        setCommands(new Command[] { keypadClose });
    }       
    /**
     * Singleton
     */
    private static KeyboardLayer instanceTF = null;
    // Save the Y position for the virtual keyboard.
    // For some reason, failing to set the bound again ( in each getInstance )
    // Causes the Y position to change. 
    private static int instanceTF_Y= 0;
    private static KeyboardLayer instanceCV = null;

    /**
     * Command to dismiss the keypad without selection.
     */ 

    /**
     * constants for setState()
     */
    public static final int NUMERIC = 0;
    public static final int LOWERCASE = 1;
    public static final int UPPERCASE = 2;
    public static final int SYMBOL = 3;

    /**
     * Sets the state of the keyboard: NUMERIC or ANY
     * Current implementation will set this as the "default" state
     * in which the keyboard opens with.
     * todo 3: constraints - remove unwanted keys according to
     * the constraints of the TextField.
     * (current state is that the keyboard will display the illegal
     * keys, but the TextField not allow to enter them).
     * 
     * @param state the state of the keyboard.
     */
    public void setState(int state) {
        if (vk != null && 
            (state == NUMERIC || 
             state == LOWERCASE ||
             state == UPPERCASE)) {
            vk.changeKeyboad(state);
        }
    }

    public int getState() {
        if (vk != null) {            
            return vk.currentKeyboardIndex;
        }
        return -1;
    }

    public String getIMName() {
        if (vk != null) {
            switch(vk.currentKeyboardIndex)
            {
                case KeyboardLayer.NUMERIC:
                    return "1234";
                case KeyboardLayer.LOWERCASE:
                    return "abcd";
                case KeyboardLayer.UPPERCASE:
                    return "ABCD";
                case KeyboardLayer.SYMBOL:
                    return "Symbol";
            }
        }
        return null;
    }

    /**
     * get TextField Keyboard layer instance
     * 
     * @param tf TextField look/feel instance for the keyboard layer
     * @return a KeyboardLayer instance.
     */
    static KeyboardLayer getInstance(TextFieldLFImpl tf) {
        if ((instanceTF == null) || (instanceTF.tfContext != tf)) {
            instanceTF = new KeyboardLayer(tf);
            instanceTF_Y = instanceTF.bounds[Y];
            instanceCV = null;
            
        }

        instanceTF.tfContext = tf;
        instanceTF.bounds[Y]= instanceTF_Y;

        return instanceTF;
    }

    /**
     * get Canvas Keyboard layer instance
     * 
     * @param canvas Canvas look/feel instance for the keypad layer
     * @return a KeyboardLayer instance.
     */
    static KeyboardLayer getInstance(CanvasLFImpl canvas) {
        if ((instanceCV == null) || (instanceCV.cvContext != canvas)) {
            instanceCV = new KeyboardLayer(canvas);
            instanceTF = null;
        }

        return instanceCV;
    }

    /**
     * Initializes the popup layer.
     */
    protected void initialize() {
        super.initialize();
    }        

    /**
     * Sets the bounds of the popup layer.
     *
     * @param x the x-coordinate of the popup layer location
     * @param y the y-coordinate of the popup layer location
     * @param w the width of this popup layer in open state
     * @param h the height of this popup layer in open state
     */
    public void setBounds(int x, int y, int w, int h) {
        super.setBounds(x, y, w, h);
    }

    /**
     * get the height of the Virtual Keyboard.
     * @return the height of the virtual keyboard.
     */ 
    public int getHeight() {
        return vk.kbHeight + 4;
    }

    /**
     * Handles key event in the open popup.
     *
     * @param type - The type of this key event (pressed, released)
     * @param code - The code of this key event
     * @return true always, since popupLayers swallow all key events
     */
    public boolean keyInput(int type, int code) {

        if ((type == EventConstants.PRESSED ||
             type == EventConstants.RELEASED ||
             type == EventConstants.REPEATED) && 
            (tfContext != null || 
             cvContext != null)) {
            vk.traverse(type,code);
        }
        return true;
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
        return vk.pointerInput(type,x,y);  
    }

    /**
     * Paints the body of the popup layer.
     *
     * @param g The graphics context to paint to
     */
    public void paintBody(Graphics g) {
        vk.paint(g);
    }


    // ********** package private *********** //

    /** Text field look/feel context */
    TextFieldLFImpl tfContext = null;

    /** Canvas look/feel context */
    CanvasLFImpl cvContext = null;

    /** Indicates if this popup layer is shown (true) or hidden (false). */
    boolean open = false;

    /** 
     * Indicates if the Keyboard layer was just opened or if it is already open.
     */
    boolean justOpened = false;

    /** the original text field string in case the user cancels */
    String backupString;

    /** the list of available keys */
    //char[][] keys = null;
    Vector Maps = null;

    /**
     * Prepare key map
     */
    void prepareKeyMapTextField() {


        Maps = new Vector();

        Image icon = getImageFromInternalStorage("key_un_24");
        Image otherIcon = getImageFromInternalStorage("key_sel_24");
        int width = icon.getWidth();
        int height = icon.getHeight();
        int xpos;
        int ypos;
        int pad = 0;

       Vector Keyboard = new Vector();

       Vector Lines = new Vector();
            xpos = 100;
            ypos = 0;
            Lines.addElement(new Key(icon,otherIcon,'1','1',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'2','2',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'3','3',xpos,ypos,width,height)); xpos+=width+pad;
            Keyboard.addElement(Lines);
            Lines = new Vector();

            xpos = 100;
            ypos +=height+pad;

            Lines.addElement(new Key(icon,otherIcon,'4','4',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'5','5',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'6','6',xpos,ypos,width,height)); xpos+=width+pad;
            Keyboard.addElement(Lines);
            Lines = new Vector();

            xpos = 100;
            ypos +=height+pad;

            Lines.addElement(new Key(icon,otherIcon,'7','7',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'8','8',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'9','9',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'0','0',xpos,ypos,width,height)); xpos+=width+pad;

            Keyboard.addElement(Lines);
            Lines = new Vector();

            xpos = 0; //pad+width/3*2;
            ypos +=height+pad;

            Lines.addElement(new Key(icon,otherIcon,VirtualKeyboard.BACKSPACE_META_KEY,'<',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,VirtualKeyboard.OK_META_KEY,'O',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,VirtualKeyboard.CANCEL_META_KEY,'X',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,VirtualKeyboard.MODE_META_KEY,'M',xpos,ypos,width,height)); xpos+=width+pad;

            Keyboard.addElement(Lines);

            Maps.addElement(Keyboard);
            Keyboard = new Vector();
            Lines = new Vector();

            xpos = pad;
            ypos = 0;
            Lines.addElement(new Key(icon,otherIcon,'q','q',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'w','w',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'e','e',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'r','r',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'t','t',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'y','y',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'u','u',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'i','i',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'o','o',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'p','p',xpos,ypos,width,height)); xpos+=width+pad;

            Keyboard.addElement(Lines);

            Lines = new Vector();
            xpos = pad+width/3;
            ypos +=height+pad;
            Lines.addElement(new Key(icon,otherIcon,'a','a',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'s','s',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'d','d',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'f','f',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'g','g',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'h','h',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'j','j',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'k','k',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'l','l',xpos,ypos,width,height)); xpos+=width+pad;

            Keyboard.addElement(Lines);

            Lines = new Vector();
            xpos = pad+width/3*2;
            ypos +=height+pad;
            Lines.addElement(new Key(icon,otherIcon,'z','z',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'x','x',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'c','c',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'v','v',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'b','b',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'m','m',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,' ',' ',xpos,ypos,width,height)); xpos+=width+pad;

            Keyboard.addElement(Lines);

            Lines = new Vector();
            xpos = 0;
            ypos +=height+pad;

            Lines.addElement(new Key(icon,otherIcon,VirtualKeyboard.BACKSPACE_META_KEY,'<',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,VirtualKeyboard.OK_META_KEY,'O',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,VirtualKeyboard.CANCEL_META_KEY,'X',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,VirtualKeyboard.MODE_META_KEY,'M',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,VirtualKeyboard.SHIFT_META_KEY,'S',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,VirtualKeyboard.CAPS_META_KEY,'c',xpos,ypos,width,height)); xpos+=width+pad;

            Keyboard.addElement(Lines);
            Maps.addElement(Keyboard);
            Keyboard = new Vector();
            Lines = new Vector();

            xpos = pad;
            ypos = 0;
            Lines.addElement(new Key(icon,otherIcon,'Q','Q',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'W','W',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'E','E',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'R','R',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'T','T',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'Y','Y',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'U','U',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'I','I',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'O','O',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'P','P',xpos,ypos,width,height)); xpos+=width+pad;

            Keyboard.addElement(Lines);
            Lines = new Vector();

            xpos = pad+width/3;
            ypos +=height+pad;
            Lines.addElement(new Key(icon,otherIcon,'A','A',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'S','S',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'D','D',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'F','F',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'G','G',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'H','H',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'J','J',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'K','K',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'L','L',xpos,ypos,width,height)); xpos+=width+pad;

            Keyboard.addElement(Lines);
            Lines = new Vector();

            xpos = pad+width/3*2;
            ypos +=height+pad;

            Lines.addElement(new Key(icon,otherIcon,'Z','Z',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'X','X',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'C','C',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'V','V',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'B','B',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,'M','M',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,' ',' ',xpos,ypos,width,height)); xpos+=width+pad;

            Keyboard.addElement(Lines);
            Lines = new Vector();

            xpos = 0; //pad+width/3*2;
            ypos +=height+pad;

            Lines.addElement(new Key(icon,otherIcon,VirtualKeyboard.BACKSPACE_META_KEY,'<',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,VirtualKeyboard.OK_META_KEY,'O',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,VirtualKeyboard.CANCEL_META_KEY,'X',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,VirtualKeyboard.MODE_META_KEY,'M',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,VirtualKeyboard.SHIFT_META_KEY,'S',xpos,ypos,width,height)); xpos+=width+pad;
            Lines.addElement(new Key(icon,otherIcon,VirtualKeyboard.CAPS_META_KEY,'c',xpos,ypos,width,height)); xpos+=width+pad;

            Keyboard.addElement(Lines);
            Maps.addElement(Keyboard);
            Keyboard = new Vector();
            Lines = new Vector();


             xpos = 0;
             ypos =0;
             Lines.addElement(new Key(icon,otherIcon,'!','!',xpos,ypos,width,height)); xpos+=width+pad;
             Lines.addElement(new Key(icon,otherIcon,'@','@',xpos,ypos,width,height)); xpos+=width+pad;
             Lines.addElement(new Key(icon,otherIcon,'#','#',xpos,ypos,width,height)); xpos+=width+pad;
             Lines.addElement(new Key(icon,otherIcon,'$','$',xpos,ypos,width,height)); xpos+=width+pad;
             Lines.addElement(new Key(icon,otherIcon,'%','%',xpos,ypos,width,height)); xpos+=width+pad;
             Lines.addElement(new Key(icon,otherIcon,'^','%',xpos,ypos,width,height)); xpos+=width+pad;
             Lines.addElement(new Key(icon,otherIcon,'&','&',xpos,ypos,width,height)); xpos+=width+pad;
             Lines.addElement(new Key(icon,otherIcon,'*','*',xpos,ypos,width,height)); xpos+=width+pad;
             Lines.addElement(new Key(icon,otherIcon,'(','(',xpos,ypos,width,height)); xpos+=width+pad;
             Lines.addElement(new Key(icon,otherIcon,')',')',xpos,ypos,width,height)); xpos+=width+pad;

             Keyboard.addElement(Lines);
             Lines = new Vector();

             xpos = 0;
             ypos +=height+pad;

             Lines.addElement(new Key(icon,otherIcon,'+','+',xpos,ypos,width,height)); xpos+=width+pad;
             Lines.addElement(new Key(icon,otherIcon,'-','-',xpos,ypos,width,height)); xpos+=width+pad;
             Lines.addElement(new Key(icon,otherIcon,'*','*',xpos,ypos,width,height)); xpos+=width+pad;
             Lines.addElement(new Key(icon,otherIcon,'/','/',xpos,ypos,width,height)); xpos+=width+pad;
             Lines.addElement(new Key(icon,otherIcon,'.','.',xpos,ypos,width,height)); xpos+=width+pad;
             Lines.addElement(new Key(icon,otherIcon,',',',',xpos,ypos,width,height)); xpos+=width+pad;
             Lines.addElement(new Key(icon,otherIcon,';',';',xpos,ypos,width,height)); xpos+=width+pad;
             Lines.addElement(new Key(icon,otherIcon,'\'','\'',xpos,ypos,width,height)); xpos+=width+pad;
             Lines.addElement(new Key(icon,otherIcon,'\"','\"',xpos,ypos,width,height)); xpos+=width+pad;
             Lines.addElement(new Key(icon,otherIcon,'~','~',xpos,ypos,width,height)); xpos+=width+pad;

             Keyboard.addElement(Lines);
             Lines = new Vector();

             xpos = 0;
             ypos +=height+pad;

             Lines.addElement(new Key(icon,otherIcon,'_','_',xpos,ypos,width,height)); xpos+=width+pad;

             Keyboard.addElement(Lines);
             Lines = new Vector();

             xpos = 0;
             ypos +=height+pad;

             Lines.addElement(new Key(icon,otherIcon,VirtualKeyboard.BACKSPACE_META_KEY,'<',xpos,ypos,width,height)); xpos+=width+pad;
             Lines.addElement(new Key(icon,otherIcon,VirtualKeyboard.OK_META_KEY,'O',xpos,ypos,width,height)); xpos+=width+pad;
             Lines.addElement(new Key(icon,otherIcon,VirtualKeyboard.CANCEL_META_KEY,'X',xpos,ypos,width,height)); xpos+=width+pad;
             Lines.addElement(new Key(icon,otherIcon,VirtualKeyboard.MODE_META_KEY,'M',xpos,ypos,width,height)); xpos+=width+pad;

             Keyboard.addElement(Lines);
            Maps.addElement(Keyboard);


/*
        // Symbol
        keys[3] = new char[25]; // numerals
        for (char i=0; i<15; i++) {  // !../
            keys[3][i] = (char)(i+33);
        }
        for (char i=0; i<7; i++) {  // :..@
            keys[3][i+15] = (char)(i+58);
        }
        keys[3][22] = '~'; // space
        keys[3][23] = '^'; // space
        keys[3][24] = ' '; // space
        */
    }


    /**
     * Prepare key map for Canvas keypad.
     */
    void prepareKeyMapCanvas() {

        Image otherIcon = null;

        Maps = new Vector();

        int size=33;
        int base=getAvailableWidth()/2;

        Image up_sel   = getImageFromInternalStorage("up_sel_"+size);
        Image up_un    = getImageFromInternalStorage("up_un_"+size);
        Image left_sel = getImageFromInternalStorage("left_sel_"+size);
        Image left_un  = getImageFromInternalStorage("left_un_"+size);
        Image mid_sel  = getImageFromInternalStorage("mid_sel_"+size);
        Image mid_un   = getImageFromInternalStorage("mid_un_"+size);
        Image right_sel  = getImageFromInternalStorage("right_sel_"+size);
        Image right_un   = getImageFromInternalStorage("right_un_"+size);
        Image down_sel = getImageFromInternalStorage("down_sel_"+size);
        Image down_un  = getImageFromInternalStorage("down_un_"+size);
        
        Vector Keyboard = new Vector();

                Vector Lines = new Vector();
                Lines.addElement(new Key(up_un, up_sel,
                                        Constants.KEYCODE_UP,
                                         left_un.getWidth()-1,
                                         0));
                Lines.addElement(new Key(left_un,left_sel,
                                        Constants.KEYCODE_LEFT,
                                         0,
                                         up_un.getHeight()-1));
               /* Lines.addElement(new Key(mid_un,mid_sel,
                                        Constants.KEYCODE_SELECT,
                                         left_un.getWidth()-2,
                                         up_un.getHeight()-2));*/
                Lines.addElement(new Key(down_un, down_sel,
                                         Constants.KEYCODE_DOWN,
                                         left_un.getWidth()-1,
                                         up_un.getHeight()+mid_un.getHeight()-1));
                Lines.addElement(new Key(right_un,right_sel,
                                         Constants.KEYCODE_RIGHT,
                                         left_un.getWidth()+mid_un.getWidth()-1,
                                         up_un.getHeight()-1));


                Lines.addElement(new Key(getImageFromInternalStorage("red_un_"+size),
                                        getImageFromInternalStorage("red_sel_"+size),
                                        '1',base,10));
                Lines.addElement(new Key(getImageFromInternalStorage("green_un_"+size),
                                        getImageFromInternalStorage("green_sel_"+size)
                                        ,'3',base+size*2,10));
                Lines.addElement(new Key(getImageFromInternalStorage("blue_un_"+size),
                                        getImageFromInternalStorage("blue_sel_"+size),
                                        '7',base+size/2,10+size*3/2));
                Lines.addElement(new Key(getImageFromInternalStorage("yellow_un_"+size),
                                        getImageFromInternalStorage("yellow_sel_"+size),
                                        '9',base+size*5/2,10+size*3/2));

        Keyboard.addElement(Lines);

        Maps.addElement(Keyboard);

     }

    /**
     * VirtualKeyboardListener interface
     *
     * MIDlet that wants the receive events from the virtual
     * keyboard needs to implement this interface, and register as
     * a listener.
     * @param c char selected by the user from the virtual keyboard
     *
     */
    public void virtualKeyEntered(int type, int c) {

        // c == 0 - Trying to dismiss the virtual keyboard
        // 
        // 
        if (c == 0) {
            Display disp = null;

            if (tfContext != null) {
                disp = tfContext.tf.owner.getLF().lGetCurrentDisplay();
            } else if (cvContext != null) {
                disp = cvContext.currentDisplay;
            }

            if (disp == null) {
                System.out.println("Could not find display - Can't hide popup");
            } else {
                disp.hidePopup(this);
            }

            open = false;
            justOpened = false;
            return;
        }

        if (tfContext != null) {	    
            if (type != EventConstants.RELEASED) {
                tfContext.uCallKeyPressed(c);
                tfContext.tf.getString();
            }
        } else if (cvContext != null) {

            if (type == EventConstants.RELEASED) {
                cvContext.uCallKeyReleased(c);

                if (!justOpened) {
                    Display disp = cvContext.currentDisplay;
                    if (disp == null) {
                        System.out.println("Could not find display - Can't hide popup");
                    } else {
                        //FIXME: Add option to automatically remove...
                        if( c == 0 )
                            disp.hidePopup(this);
                    }
                    open = false;
                } else {
                    justOpened = false;
                }
            } else {
                cvContext.uCallKeyPressed(c);
                justOpened = false;
            }   
        }
    }

    /**
     * VirtualKeyboardListener interface
     * 
     * @param metaKey special keys: 0=ok; 1=cancel
     *
     */
    public void virtualMetaKeyEntered(int metaKey) {
        Display disp;

        if (tfContext != null) {
            disp = tfContext.tf.owner.getLF().lGetCurrentDisplay();
        } else if (cvContext != null) {
            disp = cvContext.currentDisplay;
        } else {
            return;
        }

        if (metaKey == vk.OK_META_KEY) {
            // ok   
            disp.hidePopup(this);
            open = false;

        } else if (metaKey == vk.CANCEL_META_KEY) {
            // cancel
            if (tfContext != null) {
                tfContext.tf.setString(backupString);
            }
            disp.hidePopup(this);
            open = false;

        } else if (metaKey == vk.BACKSPACE_META_KEY) {

            if (tfContext != null) {
                tfContext.keyClicked(InputMode.KEYCODE_CLEAR);
            }
        } else if (metaKey == vk.IM_CHANGED_KEY) {
            if (tfContext != null) {
                 tfContext.notifyModeChanged();
            }
        }        
        // comment - customer may want backspace event also for Canvas.
        // in this case, we should handle this case here (else.. cvContext.keyPress(..))
    }

    /**
     * paint text only
     * 
     * @param g The graphics context to paint to
     * @param width width of screen to paint
     * @param height height of screen to paint
     */
    public void paintTextOnly(Graphics g, int width, int height) {
        if (tfContext != null) {
            tfContext.lPaintContent(g, width, height);
        }
    }

    /**
     * get available width
     * 
     * @return the available width.
     */
    public int getAvailableWidth() {
        if (tfContext != null) {
            return bounds[W];
            //return tfContext.tf.owner.getWidth();
        } else if (cvContext != null) {
            return cvContext.owner.getWidth();
        }
        return 0;
    }

    /**
     * get available height
     * 
     * @return the available height.
     */
    public int getAvailableHeight() {
        if (tfContext != null) {
            return tfContext.tf.owner.getHeight();
        } else if (cvContext != null) {
            return cvContext.owner.getHeight();
        }
        return 0;
    }

    /**
     * repaint the virtual keyboard.
     */
    public void repaintVK() {
        requestRepaint();
    }


    private Image getImageFromInternalStorage(String imageName) {

        try {
            byte[] imageBytes =
                    ResourceHandler.getSystemImageResource(classSecurityToken, imageName);
    
            if (imageBytes != null) {
                return Image.createImage(imageBytes, 0, imageBytes.length);
            }
        }
        catch (Exception e) {
            System.out.println("Error reading image:"+imageName);
        }

        return null;
    }

        /**
     * Inner class to request security token from SecurityInitializer.
     * SecurityInitializer should be able to check this inner class name.
     */
    static private class SecurityTrusted
        implements ImplicitlyTrustedClass {};

    /** Security token to allow access to implementation APIs */
    private static SecurityToken classSecurityToken =
        SecurityInitializer.requestToken(new SecurityTrusted());


}

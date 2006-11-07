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

import com.sun.midp.lcdui.PhoneDial;

import com.sun.midp.lcdui.DynamicCharacterArray;
import com.sun.midp.lcdui.Text;
import com.sun.midp.lcdui.TextCursor;
import com.sun.midp.lcdui.TextInfo;
import com.sun.midp.lcdui.TextPolicy;
import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

import com.sun.midp.chameleon.CGraphicsUtil;
import com.sun.midp.chameleon.input.*;
import com.sun.midp.chameleon.skins.*;
import com.sun.midp.chameleon.skins.resources.*;

/**
 * This is the look &amps; feel implementation for TextBox.
 */
class TextBoxLFImpl extends TextFieldLFImpl implements TextFieldLF {

    /**
     * Contains line-break information for a blob of text
     */
    protected TextInfo myInfo; 
    
    /**
     * A flag indicating the scroll indicator has been initialized
     * for this textbox. This happens only once when the textbox
     * first paints its contents.
     */
    protected boolean scrollInitialized;

    /**
     * Creates TextFieldLF for the passed in TextField.
     * @param tf The TextField associated with this TextFieldLF
     */
    TextBoxLFImpl(TextField tf) {
        super(tf);

        if (myInfo == null) {
            myInfo = new TextInfo(4); // IMPL NOTE: add initial size to skin
        }
    }

    // *****************************************************
    // Public methods defined in interfaces
    // *****************************************************

    /**
     * Notifies L&F of a content change in the corresponding TextBox.
     */
    public void lSetChars() {
        cursor.index = tf.buffer.length(); // cursor at the end
        cursor.option = Text.PAINT_USE_CURSOR_INDEX;
        myInfo.isModified = true;
        myInfo.topVis = 0; // force recalculate incase new content
	                       // is much shorter
        setVerticalScroll();
        lRequestPaint();
    }

    /**
     * Notifies L&amps;F of a character insertion in the corresponding
     * TextBox.
     * @param data the source of the character data
     * @param offset the beginning of the region of characters copied
     * @param length the number of characters copied
     * @param position the position at which insertion occurred
     */
    public void lInsert(char data[], int offset, int length, int position) {
        if (data == null) {
            return; // -au
        }
        if (position <= cursor.index) {
            cursor.index += length;
            cursor.option = Text.PAINT_USE_CURSOR_INDEX;
        }
        myInfo.isModified = true;
        setVerticalScroll();
        lRequestPaint();
    }

    /**
     * Notifies L&amsp;F of character deletion in the corresponding
     * TextField.
     * @param offset the beginning of the deleted region
     * @param length the number of characters deleted
     *
     * @exception IllegalArgumentException if the resulting contents
     * would be illegal for the current
     * @exception StringIndexOutOfBoundsException if <code>offset</code>
     * and <code>length</code> do not
     * specify a valid range within the contents of the <code>TextField</code>
     */
    public void lDelete(int offset, int length) {
        if (cursor.index >= offset) {
            int diff = cursor.index - offset;
            cursor.index -= (diff < length) ? diff : length;
            cursor.option = Text.PAINT_USE_CURSOR_INDEX;
        } 
        myInfo.isModified = true;
        setVerticalScroll();
        lRequestPaint();
    }

    /**
     * Notifies L&amps;F of a maximum size change in the corresponding
     * TextBox.
     * @param maxSize - the new maximum size
     */
    public void lSetMaxSize(int maxSize) {
        if (cursor.index >= maxSize) {
            cursor.index = maxSize;
            cursor.option = Text.PAINT_USE_CURSOR_INDEX;
        }
        myInfo.isModified = true;
        myInfo.topVis = 0;
        setVerticalScroll();
        lRequestPaint();
    }

    /**
     * Notifies L&amps;F that constraints have to be changed.
     */
    public void lSetConstraints() {
    	setConstraintsCommon(false);
        
        setVerticalScroll();
        lRequestPaint();
    }

    /**
     * Paint the text, linewrapping when necessary
     *
     * @param g the Graphics to use to paint with. If g is null then
     *        only the first four arguments are used and nothing is
     *        painted. Use this to return just the displayed string
     * @param dca the text to paint
     * @param opChar if opChar > 0 then an optional character to paint. 
     * @param constraints text constraints
     * @param font the font to use to paint the text
     * @param fgColor foreground color
     * @param w the available width for the text
     * @param h the available height for the text
     * @param offset the first line pixel offset
     * @param options any of Text.[NORMAL | INVERT | HYPERLINK | TRUNCATE]
     * @param cursor text cursor object to use to draw vertical bar
     * @param info TextInfo structure to use for paint
     */
    public void paint(Graphics g,
                      DynamicCharacterArray dca,
                      char opChar,
                      int constraints,
                      Font font,
                      int fgColor,
                      int w,
                      int h,
                      int offset,
                      int options,
                      TextCursor cursor,
                      TextInfo info) 
    {
        if (opChar != 0) {
            cursor = new TextCursor(cursor);
        }
        
        String str = getDisplayString(dca, opChar, constraints,
                                      cursor, true);

        // MARK: force the info flag to be modified to support the
        // pending input text added in from the input method. 
        // IMPL NOTE: modify it so that the getDisplayString() method updates
        // this boolean if the pending input is non-null


        info.isModified = true;
        
        Text.updateTextInfo(str, font, w, h, offset, options,
                            cursor, info);

        Text.paintText(info, g, str, font, fgColor, 0xffffff - fgColor,
                       w, h, offset, options, cursor);
        
        // just correct cursor index if the charracter has
        // been already committed 
        if (str != null && str.length() > 0) {
            getBufferString(new DynamicCharacterArray(str),
                            constraints, cursor, true);
        }
       
        // We'll double check our anchor point in case the Form
        // has scrolled and we need to update our InputModeLayer's
        // location on the screen
        if (hasFocus) {
            moveInputModeIndicator();
        }

        showPTPopup((int)0, cursor, w, h);
    }    

    /**
     * Sets the content size in the passed in array.
     * Content is calculated based on the availableWidth.
     * size[WIDTH] and size[HEIGHT] should be set by this method.
     * @param size The array that holds Item content size and location 
     *             in Item internal bounds coordinate system.
     * @param availableWidth The width available for this Item
     */
    void lGetContentSize(int size[], int availableWidth) {
        try {
            // We size to the maximum allowed, minus the padding
            // defined in the skin.
            size[WIDTH] = ((DisplayableLFImpl)tf.owner.getLF()).
                getDisplayableWidth() - 2 * TextFieldSkin.BOX_MARGIN;
                       
            // Note: tf.owner is the original TextBox for this LFImpl
            size[HEIGHT] = ((DisplayableLFImpl)tf.owner.getLF()).
                getDisplayableHeight() - 2 * TextFieldSkin.BOX_MARGIN;
        } catch (Throwable t) {
            // NOTE: the above call to getCurrent() will size the textbox
            // appropriately if there is a title, ticker, etc. Calling
            // this method depends on the textbox being current however.
            size[WIDTH] = 100;
            size[HEIGHT] = 100;
            // IMPL NOTE: Log this as an error
        }
    }

    /**
     * Paints the content area of this TextField.
     * Graphics is translated to contents origin.
     * @param g The graphics where Item content should be painted
     * @param width The width available for the Item's content
     * @param height The height available for the Item's content
     */
    void lPaintContent(Graphics g, int width, int height) {
        g.translate(TextFieldSkin.BOX_MARGIN, TextFieldSkin.BOX_MARGIN);

        width -= (2 * TextFieldSkin.BOX_MARGIN);
        height -= ((2 * TextFieldSkin.BOX_MARGIN) +     
                   (inputModeIndicator.getDisplayMode() != null ?
                    Font.getDefaultFont().getHeight() : 0));

        if (editable) {
            if (TextFieldSkin.IMAGE_BG != null) {
                CGraphicsUtil.draw9pcsBackground(g, 0, 0, width, height,
                    TextFieldSkin.IMAGE_BG);
            } else {
                CGraphicsUtil.drawDropShadowBox(g, 0, 0, width, height,
                    TextFieldSkin.COLOR_BORDER,
                    TextFieldSkin.COLOR_BORDER_SHD, 
                    TextFieldSkin.COLOR_BG);
            }
        } else {
            if (TextFieldSkin.IMAGE_BG_UE != null) {
                CGraphicsUtil.draw9pcsBackground(g, 0, 0, width, height,
                    TextFieldSkin.IMAGE_BG_UE);
            } else {
                CGraphicsUtil.drawDropShadowBox(g, 0, 0, width, height,
                    TextFieldSkin.COLOR_BORDER_UE,
                    TextFieldSkin.COLOR_BORDER_SHD_UE, 
                    TextFieldSkin.COLOR_BG_UE);
            }
        }

        // We need to translate by 1 more pixel horizontally 
        // to reserve space for cursor in the empty textfield
        g.translate(TextFieldSkin.PAD_H + 1, TextFieldSkin.PAD_V);

        paint(g, tf.buffer,
              inputSession.getPendingChar(),
              tf.constraints, 
              ScreenSkin.FONT_INPUT_TEXT, 
              (editable ? TextFieldSkin.COLOR_FG : TextFieldSkin.COLOR_FG_UE), 
              width - (2 * (TextFieldSkin.PAD_H)), height, 0,  
              Text.NORMAL, cursor, myInfo); 

        if (!scrollInitialized) {
            setVerticalScroll();
            scrollInitialized = true;
        }
        
        g.translate(-(TextFieldSkin.PAD_H + 1), -(TextFieldSkin.PAD_V));

        if (usePreferredX) {
            cursor.preferredX = cursor.x +
                (myInfo.lineStart[myInfo.cursorLine] == cursor.index ?
                 ScreenSkin.FONT_INPUT_TEXT.charWidth(
                                 tf.buffer.charAt(cursor.index)) :
                 0);
        }

        g.translate(-TextFieldSkin.BOX_MARGIN, -TextFieldSkin.BOX_MARGIN);
    }

    /**
     * Pring debug message 
     * @param s debug message
     */
    void log(String s) {
        //        System.out.println(s);
    }

    /**
     * Used internally to set the vertical scroll position
     */
    void setVerticalScroll() {
        Display d = getCurrentDisplay();
        if (d != null) {
            log("TB.setVertical setting scroll to " + myInfo.getScrollPosition()
                + " " + myInfo.getScrollProportion());
            d.setVerticalScroll(myInfo.getScrollPosition(),
                                myInfo.getScrollProportion());
        }
    }
    
    /**
     * Move the text cursor in the given direction
     *
     * @param dir direction to move
     * @return true if the cursor was moved, false otherwise
     */
    boolean moveCursor(int dir) {

        boolean keyUsed = false;

        switch (dir) {

	case Canvas.LEFT:
	    if (editable) {
                keyClicked(dir);
		if (cursor.index > 0) {
		    cursor.index--;
		    cursor.option = Text.PAINT_USE_CURSOR_INDEX;
		    myInfo.scrollX = true;
                    keyUsed = true; 
		}
	    } else {
		myInfo.scroll(TextInfo.BACK);
                keyUsed = true;
	    }
	    break;

	case Canvas.RIGHT:
	    if (editable) {
                keyClicked(dir);
		if (cursor.index < tf.buffer.length()) {
		    cursor.index++;
		    cursor.option = Text.PAINT_USE_CURSOR_INDEX;
		    myInfo.scrollX = true;
                    keyUsed = true; 
		}
	    } else {
		myInfo.scroll(TextInfo.FORWARD);
                keyUsed = true; 
	    }
	    break;
	    
	case Canvas.UP:
	    if (editable) {
                keyClicked(dir);
		cursor.y -= ScreenSkin.FONT_INPUT_TEXT.getHeight();
		if (cursor.y > 0) {
		    cursor.option = Text.PAINT_GET_CURSOR_INDEX;
		    myInfo.scrollY = true;
                    keyUsed = true;
		} else { 
		    cursor.y += ScreenSkin.FONT_INPUT_TEXT.getHeight();
		}
	    } else {
		myInfo.scroll(TextInfo.BACK);
                keyUsed = true;
	    }
	    break;

	case Canvas.DOWN:
	    if (editable) {
                keyClicked(dir);
		cursor.y += ScreenSkin.FONT_INPUT_TEXT.getHeight();
		if (cursor.y <= myInfo.height) {
		    cursor.option = Text.PAINT_GET_CURSOR_INDEX;
		    myInfo.scrollY = true;
                    keyUsed = true;
		} else {
		    cursor.y -= ScreenSkin.FONT_INPUT_TEXT.getHeight();
		}
	    } else {
		myInfo.scroll(TextInfo.FORWARD);
                keyUsed = true;
	    }
	    break;
	default:
	    // no-op
	    break;
	}

        return keyUsed;
    }
     
    /**
     * Called by the system to notify this Item it is being shown
     *
     * <p>The default implementation of this method updates
     * the 'visible' state
     */
    void lCallShowNotify() {
        super.lCallShowNotify();
        this.scrollInitialized = false;     
    }
    
    /**
     * This is a utility function to calculate the anchor point
     * for the InputModeIndicator layer. Override TextFieldLFImpl
     * version for effeciency.
     * @return anchor (x, y, w, h)
     */
    protected int[] getInputModeAnchor() {
        ScreenLFImpl sLF = (ScreenLFImpl)tf.owner.getLF();
        
        int space = TextFieldSkin.BOX_MARGIN
                    + Font.getDefaultFont().getHeight();
        
        return new int[] {
            sLF.viewport[WIDTH] - TextFieldSkin.BOX_MARGIN - 4
                + getCurrentDisplay().getWindow().getBodyAnchorX(),
            getCurrentDisplay().getWindow().getBodyAnchorY(),
            sLF.viewport[HEIGHT] - space - 4,                    
            space};
    }
}

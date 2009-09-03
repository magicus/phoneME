/*
 * @(#)QtTextFieldPeer.java	1.12 06/10/10
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
package sun.awt.qt;

import java.awt.*;
import sun.awt.peer.*;
import java.awt.event.*;

/**
 *
 *
 * @author Nicholas Allen
 */

class QtTextFieldPeer extends QtTextComponentPeer implements TextFieldPeer
{
  private static native void initIDs();
  
  static
  {
    initIDs();
  }
  
  private static final int padding = 10;
  
  /** Creates a new QtTextFieldPeer. */
  
  QtTextFieldPeer (QtToolkit toolkit, TextField target)
  {
    super (toolkit, target);
    setEchoChar(target.getEchoChar());
  }
  
  protected native void create(QtComponentPeer parentPeer);
  
  public native void setEchoChar(char echoChar);
  
  public Dimension getPreferredSize(int columns) {
    FontMetrics fm = getFontMetrics(target.getFont());
    return new Dimension(fm.charWidth('0') * columns + 20,
                         fm.getMaxDescent() + 
                         fm.getMaxAscent() + padding);
    }
  
  public Dimension getMinimumSize(int columns) 
  {
    return getPreferredSize(columns);
  }
  
    /**
     * DEPRECATED:  Replaced by setEchoChar(char echoChar).
     */
    public void setEchoCharacter(char c) {setEchoChar(c);}

    /**
     * DEPRECATED:  Replaced by getPreferredSize(int).
     */
    public Dimension preferredSize(int cols) {return getPreferredSize(cols);}

    /**
     * DEPRECATED:  Replaced by getMinimumSize(int).
     */
    public Dimension minimumSize(int cols) {return getMinimumSize(cols);}
	
    private void postActionEvent()
    {
        QtToolkit.postEvent(new ActionEvent(target, ActionEvent.ACTION_PERFORMED,
					    getText()));
        /*
         * Fix bug 4818521 (deadlock).
         *
         * Should not use target.getText(), but use
         * QtTextComponentPeer.getText() instead.  Calling target.getText()
         * invloves grabbing the lock associated with the target instance while
         * the thread already owns the qt lock, which can deadlock easily with a
         * thread which calls getText() where the lock sequence is:
         * 
         * [target instance lock], then [qt lock].
         */
    }
 
    public boolean isFocusable() {
        return true;
    }
}


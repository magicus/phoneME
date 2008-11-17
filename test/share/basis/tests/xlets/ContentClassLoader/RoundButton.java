/*
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
 */

import java.lang.*;
import java.util.*;
import java.awt.*;
import java.awt.event.*;

/**
 * RoundButton - a class that produces a lightweight button.
 */
public class RoundButton extends Component {

  String label;                      // The Button's text
  protected boolean pressed = false; // true if the button is detented.

  /**
   * Constructs a RoundButton with the specified label.
   * @param label the label of the button
   */
  public RoundButton(String label) {
      this.label = label;
      enableEvents(AWTEvent.MOUSE_EVENT_MASK);
  }
  
  /**
   * paints the RoundButton
   */
  public void paint(Graphics g) {
      int s = Math.min(getSize().width - 1, getSize().height - 1);
      
      // paint the interior of the button
      if(pressed) {
          g.setColor(getBackground().darker().darker());
      } else {
          g.setColor(getBackground());
      }
      g.fillArc(0, 0, s, s, 0, 360);
      
      // draw the perimeter of the button
      g.setColor(getBackground().darker().darker().darker());
      g.drawArc(0, 0, s, s, 0, 360);
      
      // draw the label centered in the button
      Font f = getFont();
      if(f != null) {
          FontMetrics fm = getFontMetrics(getFont());
          g.setColor(getForeground());
          g.drawString(label,
                       s/2 - fm.stringWidth(label)/2,
                       s/2 + fm.getMaxDescent());
      }
  }
  
  /**
   * The preferred size of the button. 
   */
  public Dimension getPreferredSize() {
      Font f = getFont();
      if(f != null) {
          FontMetrics fm = getFontMetrics(getFont());
          int max = Math.max(fm.stringWidth(label) + 40, fm.getHeight() + 40);
          return new Dimension(max, max);
      } else {
          return new Dimension(100, 100);
      }
  }
  
  /**
   * The minimum size of the button. 
   */
  public Dimension getMinimumSize() {
      return new Dimension(100, 100);
  }
   
   /**
    * Paints the button and distribute an action event to all listeners.
    */
   public void processMouseEvent(MouseEvent e) {
       Graphics g;
       switch(e.getID()) {
          case MouseEvent.MOUSE_PRESSED:
            // render myself inverted....
            pressed = true;

            // Repaint might flicker a bit. To avoid this, you can use
            // double buffering (see the Gauge example).
            repaint(); 
            break;
          case MouseEvent.MOUSE_RELEASED:
            // render myself normal again
            if(pressed == true) {
                pressed = false;
                // Repaint might flicker a bit. To avoid this, you can use
                // double buffering (see the Gauge example).
                repaint();
            }
            fireActionEvent();
            break;
          case MouseEvent.MOUSE_ENTERED:
            break;
          case MouseEvent.MOUSE_EXITED:
            if(pressed == true) {
                // Cancel! Don't send action event.
                pressed = false;

                // Repaint might flicker a bit. To avoid this, you can use
                // double buffering (see the DoubleBufferPanel example above).
                repaint();

                // Note: for a more complete button implementation,
                // you wouldn't want to cancel at this point, but
                // rather detect when the mouse re-entered, and
                // re-highlight the button. There are a few state
                // issues that that you need to handle, which we leave
                // this an an excercise for the reader (I always
                // wanted to say that!)
            }
            break;
       }
       super.processMouseEvent(e);
   }   


   private ActionListener actionListener; 

   public synchronized void addActionListener(ActionListener l) {
       actionListener = AWTEventMulticaster.add(actionListener, l);
   }

   public synchronized void removeActionListener(ActionListener l) {
       actionListener = AWTEventMulticaster.remove(actionListener, l);
   }

   public void fireActionEvent() {
       if (actionListener != null) {
          ActionEvent e = new ActionEvent(this, ActionEvent.ACTION_PERFORMED, label);
          actionListener.actionPerformed(e);
       }
   }
}

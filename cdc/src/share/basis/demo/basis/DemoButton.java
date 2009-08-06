/*
 * @(#)DemoButton.java	1.5 06/10/10
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
package basis;

import java.awt.*;
import java.awt.event.*;
import java.util.ArrayList;
import java.util.EventObject;

public class DemoButton extends Component {
    public static final Font DEFAULT_FONT = new Font("sanserif", Font.PLAIN, 12);
    public static final Color DEFAULT_COLOR = Builder.SUN_BLUE;
    private static boolean pending;
    private String label;
    private boolean mousePressed;
    private boolean keyPressed;
    private boolean inside;
    private ArrayList listeners = new ArrayList();
    private Dimension preferredSize;

    public DemoButton(String label) {
        this.label = label;
        setForeground(DEFAULT_COLOR);
        enableEvents(MouseEvent.MOUSE_CLICKED |
            MouseEvent.MOUSE_ENTERED |
            MouseEvent.MOUSE_EXITED |
            MouseEvent.MOUSE_PRESSED |
            MouseEvent.MOUSE_RELEASED |
            KeyEvent.KEY_PRESSED
// Makes no difference - not needed!
//            FocusEvent.FOCUS_GAINED |
//            FocusEvent.FOCUS_LOST
        );
    }

    public String getLabel() {
        return label;
    }

    public Dimension getPreferredSize() {
        if (preferredSize == null) {
            Graphics g = getGraphics();
            FontMetrics fm = g.getFontMetrics(DEFAULT_FONT);
            int fw = fm.stringWidth(label);
            int fh = fm.getHeight();
            preferredSize = new Dimension(fw + 4, fh + 4);
        }
        return preferredSize;
    }

    public Dimension getMinimumSize() {
        return getPreferredSize();
    }

    public Dimension getMaximumSize() {
        return new Dimension(Integer.MAX_VALUE, Integer.MAX_VALUE);
    }

    public void paint(Graphics g) {
        Dimension d = getSize();
        Color color = getForeground();
        if (inside) {
            if (mousePressed) {
                color = color.darker();
                pending = true;
            }
            if (!pending) {
                color = color.brighter();
            }
        }
        if (keyPressed) {
            color = color.darker();
        }
        g.setColor(color);
        g.fillRect(0, 0, d.width, d.height);
        int w = d.width - 1;
        int h = d.height - 1;
        if ((mousePressed && inside) || keyPressed) {
            g.setColor(Color.white);
        } else {
            g.setColor(Color.black);
        }
        g.drawLine(0, h, w, h);
        g.drawLine(w, 0, w, h);
        if ((mousePressed && inside) || keyPressed) {
            g.setColor(Color.black);
        } else {
            g.setColor(Color.white);
        }
        g.drawLine(0, 0, w, 0);
        g.drawLine(0, 0, 0, h);
        int max = 160;
        if (color.getRed() > max || color.getGreen() > max || color.getBlue() > max) {
             g.setColor(Color.black);
        } else {
            g.setColor(Color.white);
        }
        g.setFont(DEFAULT_FONT);
        FontMetrics fm = g.getFontMetrics(DEFAULT_FONT);
        int fw = fm.stringWidth(label);
        int fa = fm.getAscent();
        g.drawString(label, (d.width - fw) / 2, (d.height + fa) / 2 - 1);
        if (hasFocus()) {
            int margin = 2;
            g.drawRect(margin, margin, w - 2 * margin, h - 2 * margin);
        }
    }

    protected void processMouseEvent(MouseEvent e) {
        switch (e.getID()) {
        case MouseEvent.MOUSE_CLICKED:
            break;
        case MouseEvent.MOUSE_ENTERED:
            inside = true;
            repaint();
            break;
        case MouseEvent.MOUSE_EXITED:
            inside = false;
            repaint();
            break;
        case MouseEvent.MOUSE_PRESSED:
            mousePressed = true;
            // NOTE: Personal doesn't need to request focus,
            // as it is done in Component's dispatchEventImpl(),
            // but Basis and even J2SE do need it...
            requestFocus();
            repaint();
            break;
        case MouseEvent.MOUSE_RELEASED:
            mousePressed = false;
            repaint();
            if (!inside) {
                return;
            }
            pending = false;
            for (int i = 0; i < listeners.size(); i++) {
                DemoButtonListener listener = (DemoButtonListener) listeners.get(i);
                listener.buttonPressed(new EventObject(this));
            }
            break;
        }
        super.processMouseEvent(e);
    }

    protected void processKeyEvent(KeyEvent e) {
        if (e.getKeyCode() == KeyEvent.VK_ENTER) {
            switch (e.getID()) {
            case KeyEvent.KEY_PRESSED:
                keyPressed = true;
                repaint();
                break;
            case KeyEvent.KEY_RELEASED:
                keyPressed = false;
                repaint();
                pending = false;
                for (int i = 0; i < listeners.size(); i++) {
                    DemoButtonListener listener = (DemoButtonListener) listeners.get(i);
                    listener.buttonPressed(new EventObject(this));
                }
                break;
            }
            return;
        }
        super.processKeyEvent(e);
    }

    protected void processFocusEvent(FocusEvent e) {
        switch (e.getID()) {
        case FocusEvent.FOCUS_GAINED:
            repaint();
            break;
        case FocusEvent.FOCUS_LOST:
            keyPressed = false;
            repaint();
            break;
        }
        super.processFocusEvent(e);
    }

    public synchronized void addDemoButtonListener(DemoButtonListener listener) {
        listeners.add(listener);
    }

    public synchronized void removeDemoButtonListener(DemoButtonListener listener) {
        listeners.remove(listener);
    }

    public boolean isFocusTraversable() {
        return true;
    }

    public String toString() {
        return "DemoButton[" + label + "]";
    }
}


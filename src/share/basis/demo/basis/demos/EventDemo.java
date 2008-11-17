/*
 * @(#)EventDemo.java	1.5 06/10/10
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
package basis.demos;

import java.awt.*;
import java.awt.event.*;
import basis.Builder;

public class EventDemo extends Demo implements MouseListener, MouseMotionListener, KeyListener {
    private int w;
    private int h;
    private int border;
    private int corner;
    private int cursorOffset;
    private int cursorHeight;
    private boolean inside;
    private int posX;
    private int posY;
    private Color[] buttonColors = new Color[3];
    private int fontSize;
    private Font font;
    private int buttonWidth;
    private int buttonHeight;
    private String keyCharString = "";
    private String keyCodeString = "";
    private String modifierString = "";

    public EventDemo() {
        for (int i = 0; i < 3; i++) {
            buttonColors[i] = Builder.SUN_BLUE;
        }
        addMouseListener(this);
        addMouseMotionListener(this);
        addKeyListener(this);
    }

    public void paint(Graphics g) {
        Dimension d = getSize();
        w = d.width - 1;
        h = d.height - 1;
        border = (w + h) / 40;
        corner = (w + h) / 15;
        cursorOffset = 2 * h / 3;
        cursorHeight = (h / 3 - border) / 3 - 1;
        g.setColor(Color.white);
        g.fillRect(0, 0, w, h);
        g.setColor(inside ? Builder.SUN_RED : Builder.SUN_BLUE);
        g.fillRect(0, 0, corner, border);
        g.fillRect(corner + 1, 0, w - 2 * (corner + 1), border);
        g.fillRect(w - corner, 0, corner, border);
        g.fillRect(0, 0, border, corner);
        g.fillRect(w - border, 0, border, corner);
        g.fillRect(0, corner + 1, border, h - 2 * (corner + 1));
        g.fillRect(w - border, corner + 1, border, h - 2 * (corner + 1));
        g.fillRect(0, h - corner, border, corner);
        g.fillRect(w - border, h - corner, border, corner);
        g.fillRect(0, h - border, corner, border);
        g.fillRect(corner + 1, h - border, w - 2 * (corner + 1), border);
        g.fillRect(w - corner, h - border, corner, border);
        g.fillRect(border + 1, cursorOffset + 0 * (cursorHeight + 1), w / 2 - border - 2, cursorHeight);
        g.fillRect(border + 1, cursorOffset + 1 * (cursorHeight + 1), w / 2 - border - 2, cursorHeight);
        g.fillRect(border + 1, cursorOffset + 2 * (cursorHeight + 1), w / 2 - border - 2, cursorHeight);
        g.fillRect(w / 2 + 1, cursorOffset + 0 * (cursorHeight + 1), w / 2 - border - 2, cursorHeight);
        g.fillRect(w / 2 + 1, cursorOffset + 1 * (cursorHeight + 1), w / 2 - border - 2, cursorHeight);
        g.fillRect(w / 2 + 1, cursorOffset + 2 * (cursorHeight + 1), w / 2 - border - 2, cursorHeight);
        g.setColor(Color.white);
        int fontWidth = (w - 2 * border) / 2 / 5;
        int fontHeight = (h - 2 * border) / 3 / 4;
        fontSize = fontWidth < fontHeight ? fontWidth : fontHeight;
        font = new Font("Monospaced", Font.BOLD, fontSize);
        g.setFont(font);
        int x = border + 4;
        int y = cursorOffset + cursorHeight - 4;
        g.drawString("Default", x, y);
        y += cursorHeight;
        g.drawString("Hand", x, y);
        y += cursorHeight;
        g.drawString("Text", x, y);
        x = w / 2 + 4;
        y = cursorOffset + cursorHeight - 4;
        g.drawString("Cross", x, y);
        y += cursorHeight;
        g.drawString("Move", x, y);
        y += cursorHeight;
        g.drawString("Wait", x, y);
        paintMouseButtons(g);
        paintMousePosition(g);
        paintKeyStrings(g);
    }

    public void mouseClicked(MouseEvent e) {
        requestFocus();
    }

    public void mouseEntered(MouseEvent e) {
        Toolkit.getDefaultToolkit().beep();
        requestFocus();
        inside = true;
        repaint();
    }

    public void mouseExited(MouseEvent e) {
        inside = false;
        repaint();
    }

    public void mousePressed(MouseEvent e) {
        Toolkit.getDefaultToolkit().beep();
        Color color = Builder.SUN_RED;
        int modifier = e.getModifiers();
        if ((modifier & InputEvent.BUTTON1_MASK) != 0) {
            buttonColors[0] = color;
        }
        if ((modifier & InputEvent.BUTTON2_MASK) != 0) {
            buttonColors[1] = color;
        }
        if ((modifier & InputEvent.BUTTON3_MASK) != 0) {
            buttonColors[2] = color;
        }
        Graphics g = getGraphics();
        paintMouseButtons(g);
    }

    public void mouseReleased(MouseEvent e) {
        Color color = Builder.SUN_BLUE;
        int modifier = e.getModifiers();
        if ((modifier & InputEvent.BUTTON1_MASK) != 0) {
            buttonColors[0] = color;
        }
        if ((modifier & InputEvent.BUTTON2_MASK) != 0) {
            buttonColors[1] = color;
        }
        if ((modifier & InputEvent.BUTTON3_MASK) != 0) {
            buttonColors[2] = color;
        }
        Graphics g = getGraphics();
        paintMouseButtons(g);
    }

    public void mouseDragged(MouseEvent e) {
        mouseMoved(e);
    }

    public void mouseMoved(MouseEvent e) {
        posX = e.getX();
        posY = e.getY();
        Graphics g = getGraphics();
        paintMousePosition(g);
        Cursor cursor = Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR);
        if (posX >= 0 && posX <= w && posY >= 0 && posY <= h) {
            if (posX > corner && posX < w - (corner + 1) && posY < border) {
                cursor = Cursor.getPredefinedCursor(Cursor.N_RESIZE_CURSOR);
            }
            if (posX > corner && posX < w - (corner + 1) && posY > h - (border + 1)) {
                cursor = Cursor.getPredefinedCursor(Cursor.S_RESIZE_CURSOR);
            }
            if (posX > w - (border + 1) && posY > corner && posY < h - (corner + 1)) {
                cursor = Cursor.getPredefinedCursor(Cursor.E_RESIZE_CURSOR);
            }
            if (posX < border && posY > corner && posY < h - (corner + 1)) {
                cursor = Cursor.getPredefinedCursor(Cursor.W_RESIZE_CURSOR);
            }
            if (posX < corner && posY < border || posX < border && posY < corner) {
                cursor = Cursor.getPredefinedCursor(Cursor.NW_RESIZE_CURSOR);
            }
            if (posX > w - (corner + 1) && posY < border || posX > w - (border + 1) && posY < corner) {
                cursor = Cursor.getPredefinedCursor(Cursor.NE_RESIZE_CURSOR);
            }
            if (posX > w - (corner + 1) && posY > h - (border + 1) || posX > w - (border + 1) && posY > h - (corner + 1)) {
                cursor = Cursor.getPredefinedCursor(Cursor.SE_RESIZE_CURSOR);
            }
            if (posX < corner && posY > h - (border + 1) || posX < border && posY > h - (corner + 1)) {
                cursor = Cursor.getPredefinedCursor(Cursor.SW_RESIZE_CURSOR);
            }
            if (posX > border &&
                posX < w / 2 - 1 &&
                posY > cursorOffset + 0 * cursorHeight + 1 &&
                posY < cursorOffset + 1 * cursorHeight + 1) {
                cursor = Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR);
            }
            if (posX > border &&
                posX < w / 2 - 1 &&
                posY > cursorOffset + 1 * cursorHeight + 1 &&
                posY < cursorOffset + 2 * cursorHeight + 1) {
                cursor = Cursor.getPredefinedCursor(Cursor.HAND_CURSOR);
            }
            if (posX > border &&
                posX < w / 2 - 1 &&
                posY > cursorOffset + 2 * cursorHeight + 1 &&
                posY < cursorOffset + 3 * cursorHeight + 1) {
                cursor = Cursor.getPredefinedCursor(Cursor.TEXT_CURSOR);
            }
            if (posX > w / 2 &&
                posX < w - (border + 1) &&
                posY > cursorOffset + 0 * cursorHeight + 1 &&
                posY < cursorOffset + 1 * cursorHeight + 1) {
                cursor = Cursor.getPredefinedCursor(Cursor.CROSSHAIR_CURSOR);
            }
            if (posX > w / 2 &&
                posX < w - (border + 1) &&
                posY > cursorOffset + 1 * cursorHeight + 1 &&
                posY < cursorOffset + 2 * cursorHeight + 1) {
                cursor = Cursor.getPredefinedCursor(Cursor.MOVE_CURSOR);
            }
            if (posX > w / 2 &&
                posX < w - (border + 1) &&
                posY > cursorOffset + 2 * cursorHeight + 1 &&
                posY < cursorOffset + 3 * cursorHeight + 1) {
                cursor = Cursor.getPredefinedCursor(Cursor.WAIT_CURSOR);
            }
        }
        setCursor(cursor);
    }

    public void keyPressed(KeyEvent e) {
        Toolkit.getDefaultToolkit().beep();
        keyCharString = "";
        keyCodeString = "";
        modifierString = "";
        char keyChar = e.getKeyChar();
        if (keyChar == KeyEvent.CHAR_UNDEFINED) {
            keyCharString += "undefined";
        } else {
            keyCharString += "'" + keyChar + "'";
            keyCharString += " (" + (int) keyChar + ")";
        }
        int keyCode = e.getKeyCode();
        Integer key = new Integer(keyCode);
        String value = e.getKeyText(keyCode);
        if (value == null) {
            value = "unknown";
        }
        keyCodeString += value + " (" + keyCode + ")";
        int modifier = e.getModifiers();
        if ((modifier & InputEvent.SHIFT_MASK) != 0) {
            if (keyCode != KeyEvent.VK_SHIFT) {
                modifierString += "SHIFT ";
            }
        }
        if ((modifier & InputEvent.CTRL_MASK) != 0) {
            if (keyCode != KeyEvent.VK_CONTROL) {
                modifierString += "CONTROL ";
            }
        }
        if ((modifier & InputEvent.ALT_MASK) != 0) {
            if (keyCode != KeyEvent.VK_ALT) {
                modifierString += "ALT ";
            }
        }
        if ((modifier & InputEvent.ALT_GRAPH_MASK) != 0) {
            if (keyCode != KeyEvent.VK_ALT_GRAPH) {
                modifierString += "ALT_GRAPH ";
            }
        }
        if ((modifier & InputEvent.META_MASK) != 0) {
            if (keyCode != KeyEvent.VK_META) {
                modifierString += "META ";
            }
        }
        Graphics g = getGraphics();
        paintKeyStrings(g);
    }

    public void keyReleased(KeyEvent e) {}

    public void keyTyped(KeyEvent e) {}

    private void paintMouseButtons(Graphics g) {
        buttonWidth = (w - 2 * (border + 1)) / 3 - 1;
        buttonHeight = (h - 2 * border) / 3;
        for (int i = 0; i < 3; i++) {
            g.setColor(buttonColors[i]);
            g.fillRect(border + 1 + i * (buttonWidth + 1), border + 1, buttonWidth, buttonHeight);
        }
    }

    private void paintMousePosition(Graphics g) {
        int x = border;
        int y = border + 1 + buttonHeight;
        g.setColor(Color.white);
        g.fillRect(x, y, w - 2 * border, fontSize);
        g.setFont(font);
        g.setColor(Color.black);
        x += 1;
        y += 4 * fontSize / 5;
        g.drawString("x:", x, y);
        g.drawString("y:", w / 2, y);
        g.setColor(inside ? Builder.SUN_RED : Builder.SUN_BLUE);
        g.drawString("  " + posX, x, y);
        g.drawString("  " + posY, w / 2, y);
    }

    private void paintKeyStrings(Graphics g) {
        g.setColor(Color.white);
        int x = border;
        int y = border + 1 + buttonHeight + fontSize;
        g.fillRect(x, y, w - 2 * border, 3 * fontSize);
        g.setFont(font);
        x += 1;
        y += fontSize;
        g.setColor(Color.black);
        g.drawString("char: ", x, y - fontSize / 5);
        g.setColor(inside ? Builder.SUN_RED : Builder.SUN_BLUE);
        g.drawString("      " + keyCharString, x, y - fontSize / 5);
        y += fontSize;
        g.setColor(Color.black);
        g.drawString("code:  ", x, y - fontSize / 5);
        g.setColor(inside ? Builder.SUN_RED : Builder.SUN_BLUE);
        g.drawString("       " + keyCodeString, x, y - fontSize / 5);
        y += fontSize;
        g.setColor(Color.black);
        g.drawString("mods: ", x, y - fontSize / 5);
        g.setColor(inside ? Builder.SUN_RED : Builder.SUN_BLUE);
        g.drawString("      " + modifierString, x, y - fontSize / 5);
        g.setColor(Builder.SUN_RED);
   }
}


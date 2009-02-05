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


import java.awt.*;                                                          
import java.awt.event.*;
import javax.microedition.xlet.*;

import sun.awt.Robot;

public class TestXlet1Auto implements Xlet, MouseListener, KeyListener, FocusListener {

    ThreadGroup myThreadGroup;
    boolean failed = false;
    boolean debug  = true;
    Component c;
    XletContext context;
    Robot robot;

    public TestXlet1Auto() {
        System.out.println("Creating TestXlet1Auto:" + Thread.currentThread());
        myThreadGroup = Thread.currentThread().getThreadGroup(); 
    }

    public void initXlet(XletContext context) {
        this.context = context;
        c = new Component() { 
           public Dimension getMinimumSize() { return new Dimension(200,200); }
           public Dimension getPreferredSize() { return new Dimension(200,200); }
           public void paint(Graphics g) {
              g.setColor(Color.RED);
              g.fillRect(50,50,100,100);
           }
        };
        c.setFocusable(true);
        c.addFocusListener(this);
        c.addMouseListener(this);
        c.addKeyListener(this);
        try {
           Container container = context.getContainer();
           container.setSize(200,200);
           container.setLayout(new FlowLayout());
           container.add(c);
           container.validate();
           container.setVisible(true);
        } catch (UnavailableContainerException e) { 
           e.printStackTrace();
           failed = true;
           context.notifyDestroyed();
        }
    }
    public void startXlet() {
        try {
           if (robot == null) robot = new Robot();
           Point loc = c.getLocationOnScreen();
           robot.mouseMove(loc.x + c.getSize().width/2, 
                           loc.y + c.getSize().height/2); 
           robot.delay(100);
           robot.mousePress(MouseEvent.BUTTON1_MASK);
           robot.mouseRelease(MouseEvent.BUTTON1_MASK);
           robot.delay(100);
           robot.keyPress(KeyEvent.VK_A);
           robot.delay(100);
        } catch (Exception e) { 
           e.printStackTrace(); 
           failed = true;
           context.notifyDestroyed();
        }
    }

    public void pauseXlet() {}

    public void destroyXlet(boolean b) {
        debugMessage("TestXlet1Auto.destoryXlet()");
        if (failed) {
           System.out.println("\nTest failed\n");
        }
    }

    public void mousePressed(MouseEvent e) {
        debugMessage("\nTestXlet1Auto-MousePressed:");
        debugMessage("   " + e.getSource() + " "  + Thread.currentThread());
        checkThreadGroup();
    }
    public void mouseReleased(MouseEvent e) {
        debugMessage("\nTestXlet1Auto-MouseReleased:");
        debugMessage("   " + e.getSource() + " "  + Thread.currentThread());
        checkThreadGroup();
    }
    public void mouseEntered(MouseEvent e) {
        debugMessage("\nTestXlet1Auto-MouseEntered:");
        debugMessage("   " + e.getSource() + " "  + Thread.currentThread());
        checkThreadGroup();
    }
    public void mouseExited(MouseEvent e) {
        debugMessage("\nTestXlet1Auto-MouseExited:"); 
        debugMessage("   " + e.getSource() + " "  + Thread.currentThread());
        checkThreadGroup();
    }
    public void mouseClicked(MouseEvent e) {
        debugMessage("\nTestXlet1Auto-MouseClicked:");
        debugMessage("   " + e.getSource() + " "  + Thread.currentThread());
        checkThreadGroup();
    }

    public void keyPressed(KeyEvent e) {
        debugMessage("\nTestXlet1Auto-KeyPressed:");
        debugMessage("   " + e.getSource() + " "  + Thread.currentThread());
        checkThreadGroup();
    }

    public void keyReleased(KeyEvent e) {
        debugMessage("\nTestXlet1Auto-KeyReleased:");
        debugMessage("   " + e.getSource() + " "  + Thread.currentThread());
        checkThreadGroup();
    }

    public void keyTyped(KeyEvent e) {
        debugMessage("\nTestXlet1Auto-KeyTyped:");
        debugMessage("   " + e.getSource() + " "  + Thread.currentThread());
        checkThreadGroup();
    }

    public void focusGained(FocusEvent e) {
        debugMessage("\nTestXlet1Auto-FocusGained:");
        debugMessage("   " + e.getSource() + " "  + Thread.currentThread());
        checkThreadGroup();
    }

    public void focusLost(FocusEvent e) {
        debugMessage("\nTestXlet1Auto-FocusLost:");
        debugMessage("   " + e.getSource() + " "  + Thread.currentThread());
        checkThreadGroup();
    }

    void checkThreadGroup() {
        if (Thread.currentThread().getThreadGroup() != myThreadGroup) {
           System.out.println("Wrong TheadGroup!");
           System.out.println("      Expected TG: " + myThreadGroup);
           System.out.println("      Received TG: " + Thread.currentThread().getThreadGroup());
           new Exception("Just for Debug").printStackTrace();
           failed = true;
           context.notifyDestroyed();
        }
    }
    void debugMessage(String s) {
        if (debug) System.out.println(s);
    }
}         


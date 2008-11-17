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

package tests.fullScreenMode;

import java.awt.*;
import java.awt.event.*;

public class TestFull extends Frame implements MouseListener {
    boolean clicked=false;

    TestFull() {
        addMouseListener(this);

    }

    public void startTest() {
        GraphicsDevice gd = GraphicsEnvironment.getLocalGraphicsEnvironment().
            getDefaultScreenDevice();
        System.out.println("full screen support is " + 
                           gd.isFullScreenSupported());                    
        System.out.println("getFullScreenWindow returns " +
           gd.getFullScreenWindow());
 
        System.out.println("Calling setFullScreen with frame");
        gd.setFullScreenWindow(this);
        while (!clicked) {
            try { wait(300);}
            catch (Exception ex) {}
        }
        System.out.println("Calling setFullScreen with null");
        gd.setFullScreenWindow(null);
    }

    public void paint(Graphics g) {
        g.drawString("Click on the frame to leave full screen mode", 20, 50);
    }

    public void mouseClicked(MouseEvent e) {
        clicked=true;
    }

    public void mousePressed(MouseEvent e) {
    }

    public void mouseReleased(MouseEvent e) {
    }

    public void mouseEntered(MouseEvent e) {
    }

    public void mouseExited(MouseEvent e) {
    }

    public static void main(String args[]) {
        TestFull tf = new TestFull();
        //tf.setLocation(200, 200);

        tf.setVisible(true);
        tf.setLocation(0, 0);
        tf.setSize(200, 200);
        tf.startTest();
    }
	
}

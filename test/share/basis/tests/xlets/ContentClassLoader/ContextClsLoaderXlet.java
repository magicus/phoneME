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


// Originally jdk testcase ContextClsLoader.
// Checks the event delivery's Thread has the
// correct (xlet's) ClassLoader as it's
// ContentClassLoader.
 
// This test uses Robot, so make sure to add
// additional permissions (Ex: -Djava.security.policy=allperm.policy)

import java.awt.AWTException;
import java.awt.Component;
import java.awt.Container;
import java.awt.Dimension;
import java.awt.Point;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseEvent;

import javax.microedition.xlet.*;
import sun.awt.Robot;

public class ContextClsLoaderXlet implements Xlet {

    //private Button b;
    private RoundButton b;

    private boolean done = false;
    private static RuntimeException t;

    public void initXlet(XletContext context) {
	outputContextLoader("initXlet()");

	// Pop up a button and wait for the action.
	//b = new Button("press me");
	b = new RoundButton("press me");
 	b.addActionListener(new ActionListener() {
 	    public void actionPerformed(ActionEvent e) {
		if (!done) {
 		    outputContextLoader("actionPerformed()");
		    done = true;
 		}
 	    }
 	});

        try {
           Container c = context.getContainer();
 
           c.add(b);
  	   b.setSize(100, 100);
  	   c.setSize(100, 100);
  	   c.setLocation(10, 10);
           c.validate();
	   c.setVisible(true);

	   c.paint(c.getGraphics());
        } catch (Exception e) { 
           e.printStackTrace();
           throw new RuntimeException(e);
        }
    }

    public void startXlet() {
	outputContextLoader("startXlet()");


	try {
	    Robot robot = new Robot();
	    //while (!done) {
	    if (!done) {
		Point loc = b.getLocationOnScreen();
		loc.x += (b.getWidth() / 2);
		loc.y += (b.getHeight() / 2);
		robot.mouseMove(loc.x, loc.y);
robot.delay(100);
		robot.mousePress(MouseEvent.BUTTON1_MASK);
		robot.mouseRelease(MouseEvent.BUTTON1_MASK);
	    }
 	} catch (AWTException e) {
	    e.printStackTrace();
	    throw new RuntimeException(e.getMessage());
	}
    }

    public void pauseXlet() {
	outputContextLoader("pauseXlet()");
    }

    public void destroyXlet(boolean unconditional) {
	outputContextLoader("destroyXlet()");
    }

    private static void outputContextLoader(String where) {
	ClassLoader contextLoader =
	    Thread.currentThread().getContextClassLoader();
	System.out.println(where + ": context class loader = " + contextLoader);

	if (! (contextLoader instanceof com.sun.xlet.XletClassLoader)
	    && (t == null)) {
	    t = new RuntimeException(where + " unexpected context class loader: "
				     + contextLoader.toString() + " " + Thread.currentThread());
	    System.err.println("FAILED: " + t.getMessage());
            throw t;
	}
    }
}

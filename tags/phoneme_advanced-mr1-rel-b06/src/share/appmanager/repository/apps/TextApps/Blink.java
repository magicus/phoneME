/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

/*
 * @(#)Blink.java	1.12 03/01/23
 */

/**
 * I love blinking things.
 *
 * @author Arthur van Hoff
 * @modified 04/24/96 Jim Hagen use getBackground
 * @modified 02/05/98 Mike McCloskey removed use of deprecated methods
 * @modified 04/23/99 Josh Bloch, use timer instead of explicit multithreading.
 * @modified 07/10/00 Daniel Peek brought to code conventions, minor changes
 */

import java.awt.*;
import java.util.*;
import javax.microedition.xlet.*;

public class Blink
    extends Panel
    implements Xlet {
    private Timer timer; // Schedules the blinking
    private String labelString; // The label for the window
    private int delay; // the delay time between blinks

    /***************** parameters section taken from applet ***************/
    String speed = "1";
    String lbl = "Welcome to the J2ME CDC Device Top, otherwise known as CDCAms Application Manager.  Application Management at your fingertips!";

    /**********************************************************************/

    public Frame getRootFrame(Container rootContainer) {

        Container tmp = rootContainer;
        while (! (tmp instanceof Frame)) {
            tmp = tmp.getParent();
        }
        return (Frame) tmp;
    }

    public void initXlet(XletContext context) {
        System.err.println("***** INIT_XLET(Blink) *****");

        try {
            Container container = context.getContainer();
            container.add(this);
            init();
            container.setVisible(true);
        }
        catch (UnavailableContainerException e) {
            System.out.println("Error in getting a root container: " + e);
            context.notifyDestroyed();
        }
    }

    public void startXlet() {
        System.err.println("***** START_XLET(Blink) *****");
        start();
    }

    public void pauseXlet() {
        System.err.println("***** PAUSE_XLET(Blink) *****");
        stop();
    }

    public void destroyXlet(boolean unconditional) {
        System.err.println("***** DESTROY_XLET(Blink) *****");
    }

    public void init() {
        String blinkFrequency = speed;
        delay = (blinkFrequency == null) ? 400 :
            (1000 / Integer.parseInt(blinkFrequency));
        labelString = lbl;
        if (labelString == null) {
            labelString = "Blink";
        }
        setBackground(Color.white);
        Font font = new java.awt.Font("Serif", Font.PLAIN, 24);
        setFont(font);
    }

    public void start() {
        timer = new Timer(); //creates a new timer to schedule the blinking
        timer.schedule(new TimerTask() { //creates a timertask to schedule
            // overrides the run method to provide functionality
            public void run() {
                repaint();
            }
        }

        , delay, delay);
    }

    public void paint(Graphics g) {
        int fontSize = g.getFont().getSize();
        int x = 0, y = fontSize + 50, space;
        int red = (int) (50 * Math.random());
        int green = (int) (50 * Math.random());
        int blue = (int) (256 * Math.random());
        Dimension d = getSize();
        g.setColor(Color.black);
        FontMetrics fm = g.getFontMetrics();
        space = fm.stringWidth(" ");
        for (StringTokenizer t = new StringTokenizer(labelString);
             t.hasMoreTokens(); ) {
            String word = t.nextToken();
            int w = fm.stringWidth(word) + space;
            if (x + w > d.width) {
                x = 0;
                y += fontSize + 30; //move word to next line if it doesn't fit
            }
            if (Math.random() < 0.5) {
                g.setColor(new java.awt.Color( (red + y * 30) % 256,
                                              (green + x / 3) % 256, blue));
            }
            else {
                g.setColor(getBackground());
            }
            g.drawString(word, x, y);
            x += w; //shift to the right to draw the next word
        }
    }

    public void stop() {
        timer.cancel(); //stops the timer
    }
}

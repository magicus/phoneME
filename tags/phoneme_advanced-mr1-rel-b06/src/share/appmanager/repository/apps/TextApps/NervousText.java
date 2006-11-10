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
 * @(#)NervousText.java	1.10 03/01/23
 */

import java.awt.event.*;
import java.awt.*;
//import java.applet.Applet;
import javax.microedition.xlet.*;

/**
 * An applet that displays jittering text on the screen.
 *
 * @author Daniel Wyszynski 04/12/95
 * @version 1.10, 02/05/97
 * @modified 05/09/95 kwalrath Changed string; added thread suspension
 * @modified 02/06/98 madbot removed use of suspend and resume and cleaned up
 */

public class NervousText
    extends Panel
    implements Runnable, MouseListener, Xlet {
    String banner; // The text to be displayed
    char bannerChars[]; // The same text as an array of characters
    char attributes[]; // Character attributes ('^' for superscript)
    Thread runner = null; // The thread that is displaying the text
    boolean threadSuspended; // True when thread suspended (via mouse click)

    static final int REGULAR_WD = 15;
    static final int REGULAR_HT = 36;
    static final int SMALL_WD = 12;
    static final int SMALL_HT = 24;

    Font regularFont = new Font("Serif", Font.BOLD, REGULAR_HT);
    Font smallFont = new Font("Serif", Font.BOLD, SMALL_HT);

    String displayText = "CDCAms Application Manager";

    /**
     * Initialize the xlet. Just get the container, set it up and call
     * loadImage.
     */

    public Frame getRootFrame(Container rootContainer) {

        Container tmp = rootContainer;
        while (! (tmp instanceof Frame)) {
            tmp = tmp.getParent();
        }
        return (Frame) tmp;
    }

    public void initXlet(XletContext context) {
        System.err.println("***** INIT_XLET(Nervous Text) *****");

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
        System.err.println("***** START_XLET(Nervous Text) *****");
        start();
    }

    public void pauseXlet() {
        System.err.println("***** PAUSE_XLET(Nervous Text) *****");
        stop();
    }

    public void destroyXlet(boolean unconditional) {
        System.err.println("***** DESTROY_XLET(Nervous Text) *****");
    }

    public void init() {
        setBackground(new Color(176, 177, 203));
        banner = displayText;
        int bannerLength = banner.length();
        StringBuffer bc = new StringBuffer(bannerLength);
        StringBuffer attrs = new StringBuffer(bannerLength);
        int wd = 0;
        for (int i = 0; i < bannerLength; i++) {
            char c = banner.charAt(i);
            char a = 0;
            if (c == '^') {
                i++;
                if (i < bannerLength) {
                    c = banner.charAt(i);
                    a = '^';
                    wd += SMALL_WD - REGULAR_WD;
                }
                else {
                    break;
                }
            }
            bc.append(c);
            attrs.append(a);
            wd += REGULAR_WD;
        }

        bannerLength = bc.length();
        bannerChars = new char[bannerLength];
        attributes = new char[bannerLength];
        bc.getChars(0, bannerLength, bannerChars, 0);
        attrs.getChars(0, bannerLength, attributes, 0);

        threadSuspended = false;
        resize(wd + 10, 50);
        addMouseListener(this);
    }

    public void destroy() {
        removeMouseListener(this);
    }

    public void start() {
        runner = new Thread(this);
        runner.start();
    }

    public synchronized void stop() {
        runner = null;
        if (threadSuspended) {
            threadSuspended = false;
            notify();
        }
    }

    public void run() {
        Thread me = Thread.currentThread();
        while (runner == me) {
            try {
                Thread.sleep(100);
                synchronized (this) {
                    while (threadSuspended) {
                        wait();
                    }
                }
            }
            catch (InterruptedException e) {
            }
            repaint();
        }
    }

    public void paint(Graphics g) {
        int length = bannerChars.length;
        g.setColor(Color.BLUE);
        int height = getHeight();
        for (int i = 0, x = 0; i < length; i++) {
            int wd, ht;
            if (attributes[i] == '^') {
                wd = SMALL_WD;
                ht = SMALL_HT;
                g.setFont(smallFont);
            }
            else {
                wd = REGULAR_WD;
                ht = REGULAR_HT;
                g.setFont(regularFont);
            }
            int px = (int) (10 * Math.random() + x);
            int py = (int) (10 * Math.random() + ht);
            g.drawChars(bannerChars, i, 1, px, (height / 2) - py);
            x += wd;
        }
    }

    public synchronized void mousePressed(MouseEvent e) {
        e.consume();
        threadSuspended = !threadSuspended;
        if (!threadSuspended) {
            notify();
        }
    }

    public void mouseReleased(MouseEvent e) {
    }

    public void mouseEntered(MouseEvent e) {
    }

    public void mouseExited(MouseEvent e) {
    }

    public void mouseClicked(MouseEvent e) {
    }
}

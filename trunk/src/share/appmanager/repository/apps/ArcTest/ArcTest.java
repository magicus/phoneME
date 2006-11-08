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
 * @(#)ArcTest.java	1.10 03/01/23
 */

import java.awt.*;
import java.awt.event.*;
import javax.microedition.xlet.*;

/**
 * An interactive test of the Graphics.drawArc and Graphics.fillArc
 * routines. Can be run either as a standalone application by
 * typing "java ArcTest" or as an applet in the AppletViewer.
 */
public class ArcTest extends Panel implements Xlet {
    ArcControls controls;   // The controls for marking and filling arcs
    ArcCanvas canvas;       // The drawing area to display arcs


    public Frame getRootFrame(Container rootContainer) {

        Container tmp = rootContainer;
        while (! (tmp instanceof Frame)) {
            tmp = tmp.getParent();
        }
        return (Frame) tmp;
    }

    public void initXlet(XletContext context) {
        System.err.println("***** INIT_XLET(ArcTest) *****");

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
        System.err.println("***** START_XLET(ArcTest) *****");
        start();
    }

    public void pauseXlet() {
        System.err.println("***** PAUSE_XLET(ArcTest) *****");
        stop();
    }

    public void destroyXlet(boolean unconditional) {
        System.err.println("***** DESTROY_XLET(ArcTest) *****");
    }

    public void init() {
        setLayout(new BorderLayout());
        canvas = new ArcCanvas();
        add("Center", canvas);
        add("South", controls = new ArcControls(canvas));
    }

    public void destroy() {
        remove(controls);
        remove(canvas);
    }

    public void start() {
        controls.setEnabled(true);
    }

    public void stop() {
        controls.setEnabled(false);
    }

    public void processEvent(AWTEvent e) {
        if (e.getID() == Event.WINDOW_DESTROY) {
            System.exit(0);
        }
    }

    public static void main(String args[]) {
        Frame f = new Frame("ArcTest");
        ArcTest	arcTest = new ArcTest();

        arcTest.init();
        arcTest.start();

        f.add("Center", arcTest);
        f.setSize(300, 300);
        f.show();
    }

    public String getAppletInfo() {
        return "An interactive test of the Graphics.drawArc and \nGraphics.fillArc routines. Can be run \neither as a standalone application by typing 'java ArcTest' \nor as an applet in the AppletViewer.";
    }
}

class ArcCanvas extends Canvas {
    int		startAngle = 0;
    int		endAngle = 45;
    boolean	filled = false;
    Font	font = new java.awt.Font("Courier", Font.BOLD, 12);

    public void paint(Graphics g) {
        Rectangle r = getBounds();
        int hlines = r.height / 10;
        int vlines = r.width / 10;

        g.setColor(Color.pink);
        for (int i = 1; i <= hlines; i++) {
            g.drawLine(0, i * 10, r.width, i * 10);
        }
        for (int i = 1; i <= vlines; i++) {
            g.drawLine(i * 10, 0, i * 10, r.height);
        }

        g.setColor(Color.red);
        if (filled) {
            g.fillArc(0, 0, r.width - 1, r.height - 1, startAngle, endAngle);
        } else {
            g.drawArc(0, 0, r.width - 1, r.height - 1, startAngle, endAngle);
        }

        g.setColor(Color.black);
        g.setFont(font);
        g.drawLine(0, r.height / 2, r.width, r.height / 2);
        g.drawLine(r.width / 2, 0, r.width / 2, r.height);
        g.drawLine(0, 0, r.width, r.height);
        g.drawLine(r.width, 0, 0, r.height);
        int sx = 10;
        int sy = r.height - 28;
        g.drawString("S = " + startAngle, sx, sy);
        g.drawString("E = " + endAngle, sx, sy + 14);
    }

    public void redraw(boolean filled, int start, int end) {
        this.filled = filled;
        this.startAngle = start;
        this.endAngle = end;
        repaint();
    }
}

class ArcControls extends Panel
                  implements ActionListener {
    TextField s;
    TextField e;
    ArcCanvas canvas;

    public ArcControls(ArcCanvas canvas) {
        Button b = null;

        this.canvas = canvas;
        add(s = new TextField("0", 4));
        add(e = new TextField("45", 4));
        b = new Button("Fill");
        b.addActionListener(this);
        add(b);
        b = new Button("Draw");
        b.addActionListener(this);
        add(b);
    }

    public void actionPerformed(ActionEvent ev) {
        String label = ev.getActionCommand();

        canvas.redraw(label.equals("Fill"),
                      Integer.parseInt(s.getText().trim()),
                      Integer.parseInt(e.getText().trim()));
    }
}


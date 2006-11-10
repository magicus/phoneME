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
 * @(#)Clock.java	1.12 03/01/23
 */

import java.util.*;
import java.awt.*;
import java.text.*;
import javax.microedition.xlet.*;

import java.awt.event.MouseListener;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.rmi.NotBoundException;
import javax.microedition.xlet.ixc.IxcRegistry;

    /**
     * Time!
     *
     * @author Rachel Gollub
     * @modified Daniel Peek replaced circle drawing calculation, few more changes
     */
    public class Clock
    extends Panel
    implements Runnable, Xlet {
    private volatile Thread timer; // The thread that displays clock
    private int lastxs, lastys, lastxm,
        lastym, lastxh, lastyh; // Dimensions used to draw hands
    private SimpleDateFormat formatter; // Formats the date displayed
    private String lastdate; // String to hold date displayed
    private Font clockFaceFont; // Font for number display on clock
    private Date currentDate; // Used to get date to display
    private Color handColor; // Color of main hands and dial
    private Color numberColor; // Color of second hand and numbers
    private int xcenter = 80, ycenter = 55; // Center position

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
        System.err.println("***** INIT_XLET(Clock) *****");

        try {
            Container container = context.getContainer();
            container.add(this);
            init();
            container.setVisible(true);

            IxcRegistry regis = IxcRegistry.getRegistry(context);
            try {
               final PhoneService ps = (PhoneService)regis.lookup("phone/dial");
   
               container.addMouseListener(new MouseAdapter() {
                  public void mousePressed(MouseEvent e) { 
                     try {
                        ps.dial(lastdate);
                     } catch (Exception ex) {
                        ex.printStackTrace();
                     }
                  }
               });
             } catch (NotBoundException nbe) {
                System.err.println("Caught NotBoundException.  In Clock, can't find a service under \"phone/dial\" name.");
             } catch (Exception ex) {
                ex.printStackTrace();
             }

        } catch (UnavailableContainerException e) {
            System.out.println("Error in getting a root container: " + e);
            context.notifyDestroyed();
        }
    }

    public void startXlet() {
        System.err.println("***** START_XLET(Clock) *****");
        start();
    }

    public void pauseXlet() {
        System.err.println("***** PAUSE_XLET(Clock) *****");
        stop();
    }

    public void destroyXlet(boolean unconditional) {
        System.err.println("***** DESTROY_XLET(Clock) *****");
    }

    public static void main(String args[]) {
        Frame frame = new Frame("Clock");
        frame.add(new Clock());
        frame.setBounds(0, 0, 170, 150);
        frame.setVisible(true);
    }

    public Clock() {
        //        init();
        //        start();
    }

    public void init() {
        int x, y;
        lastxs = lastys = lastxm = lastym = lastxh = lastyh = 0;
        formatter = new SimpleDateFormat("EEE MMM dd hh:mm:ss yyyy",
                                         Locale.getDefault());
        currentDate = new Date();
        lastdate = formatter.format(currentDate);
        clockFaceFont = new Font("Serif", Font.PLAIN, 14);
        handColor = Color.blue;
        numberColor = Color.darkGray;

        try {
            setBackground(new Color(Integer.parseInt("000000",
                16)));
        }
        catch (NullPointerException e) {
        }
        catch (NumberFormatException e) {
        }
        try {
            handColor = new Color(Integer.parseInt("ff0000",
                16));
        }
        catch (NullPointerException e) {
        }
        catch (NumberFormatException e) {
        }
        try {
            numberColor = new Color(Integer.parseInt("ff00ff",
                16));
        }
        catch (NullPointerException e) {
        }
        catch (NumberFormatException e) {
        }
        resize(300, 300); // Set clock window size
    }

    // Paint is the main part of the program
    public void update(Graphics g) {
        int xh, yh, xm, ym, xs, ys;
        int s = 0, m = 10, h = 10;
        String today;

        currentDate = new Date();

        formatter.applyPattern("s");
        try {
            s = Integer.parseInt(formatter.format(currentDate));
        }
        catch (NumberFormatException n) {
            s = 0;
        }
        formatter.applyPattern("m");
        try {
            m = Integer.parseInt(formatter.format(currentDate));
        }
        catch (NumberFormatException n) {
            m = 10;
        }
        formatter.applyPattern("h");
        try {
            h = Integer.parseInt(formatter.format(currentDate));
        }
        catch (NumberFormatException n) {
            h = 10;
        }

        // Set position of the ends of the hands
        xs = (int) (Math.cos(s * Math.PI / 30 - Math.PI / 2) * 45 + xcenter);
        ys = (int) (Math.sin(s * Math.PI / 30 - Math.PI / 2) * 45 + ycenter);
        xm = (int) (Math.cos(m * Math.PI / 30 - Math.PI / 2) * 40 + xcenter);
        ym = (int) (Math.sin(m * Math.PI / 30 - Math.PI / 2) * 40 + ycenter);
        xh = (int) (Math.cos( (h * 30 + m / 2) * Math.PI / 180 - Math.PI / 2) *
                    30
                    + xcenter);
        yh = (int) (Math.sin( (h * 30 + m / 2) * Math.PI / 180 - Math.PI / 2) *
                    30
                    + ycenter);

        // Get the date to print at the bottom
        formatter.applyPattern("EEE MMM dd HH:mm:ss yyyy");
        today = formatter.format(currentDate);

        g.setFont(clockFaceFont);
        // Erase if necessary
        g.setColor(getBackground());
        if (xs != lastxs || ys != lastys) {
            g.drawLine(xcenter, ycenter, lastxs, lastys);
            g.drawString(lastdate, 5, 125);
        }
        if (xm != lastxm || ym != lastym) {
            g.drawLine(xcenter, ycenter - 1, lastxm, lastym);
            g.drawLine(xcenter - 1, ycenter, lastxm, lastym);
        }
        if (xh != lastxh || yh != lastyh) {
            g.drawLine(xcenter, ycenter - 1, lastxh, lastyh);
            g.drawLine(xcenter - 1, ycenter, lastxh, lastyh);
        }

        // Draw date and hands
        g.setColor(numberColor);
        g.drawString(today, 5, 125);
        g.drawLine(xcenter, ycenter, xs, ys);
        g.setColor(handColor);
        g.drawLine(xcenter, ycenter - 1, xm, ym);
        g.drawLine(xcenter - 1, ycenter, xm, ym);
        g.drawLine(xcenter, ycenter - 1, xh, yh);
        g.drawLine(xcenter - 1, ycenter, xh, yh);
        lastxs = xs;
        lastys = ys;
        lastxm = xm;
        lastym = ym;
        lastxh = xh;
        lastyh = yh;
        lastdate = today;
        currentDate = null;
    }

    public void paint(Graphics g) {
        g.setFont(clockFaceFont);
        // Draw the circle and numbers
        g.setColor(handColor);
        g.drawArc(xcenter - 50, ycenter - 50, 100, 100, 0, 360);
        g.setColor(numberColor);
        g.drawString("9", xcenter - 45, ycenter + 3);
        g.drawString("3", xcenter + 40, ycenter + 3);
        g.drawString("12", xcenter - 5, ycenter - 37);
        g.drawString("6", xcenter - 3, ycenter + 45);

        // Draw date and hands
        g.setColor(numberColor);
        g.drawString(lastdate, 5, 125);
        g.drawLine(xcenter, ycenter, lastxs, lastys);
        g.setColor(handColor);
        g.drawLine(xcenter, ycenter - 1, lastxm, lastym);
        g.drawLine(xcenter - 1, ycenter, lastxm, lastym);
        g.drawLine(xcenter, ycenter - 1, lastxh, lastyh);
        g.drawLine(xcenter - 1, ycenter, lastxh, lastyh);
    }

    public void start() {
        timer = new Thread(this);
        timer.start();
    }

    public void stop() {
        timer = null;
    }

    public void run() {
        Thread me = Thread.currentThread();
        while (timer == me) {
            try {
                Thread.currentThread().sleep(100);
            }
            catch (InterruptedException e) {
            }
            repaint();
        }
    }

    public String getAppletInfo() {
        return "Title: A Clock \n"
            + "Author: Rachel Gollub, 1995 \n"
            + "An analog clock.";
    }

    public String[][] getParameterInfo() {
        String[][] info = {
            {
            "bgcolor", "hexadecimal RGB number",
            "The background color. Default is the color of your browser."}
            , {
            "fgcolor1", "hexadecimal RGB number",
            "The color of the hands and dial. Default is blue."}
            , {
            "fgcolor2", "hexadecimal RGB number",
            "The color of the second hand and numbers. Default is dark gray."}
        };
        return info;
    }
}

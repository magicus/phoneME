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
 * @(#)Ticker.java	1.3 05/10/20
 */

import java.awt.*;
import java.util.Timer;
import java.util.TimerTask;
import java.net.URL;
//import javax.microedition.xlet.*;

public class Ticker
    extends Panel {

    String str[] = new String[] {
        "Missed Calls: 2      Unread Voice Messages: 3",
        "Hotels: Caesars Palace, Las Vegas starting at $89/night.",
        "Dow: 10103.75     SUNW: 3.76",
        "Concerts: Hillary Duff Tix On Sale Today. Tix start at $25.",
        "MLB   A's:  7 Rangers: 6     Giants: 4 Padres: 6",
        "3 new games now available in the Download Store.",
        "News: Martha Stewart involved in daring prison escape!",
        "Box Office: I, Robot tops for the week...",
        "Traffic: Slow going on the 237/880 interchange...",
        "5 new ringtones now available in the Download Store",
        "Music: The Hives \"Tyrannosaurus Hives\" in stores Tuesday.",
        "Weather: Partly Cloudy...  75F",
        "Concerts: Ozzfest coming to Mountain View 7/29. Tix: $30.",
        "Try the new \"Twisted Crust Pizza\" at Pizza Hut today!",
        "TV: A new episode of \"The Simple Life\" airs on Fox tonight",
    };

    Label label = new Label();
    int strIndex = 0;
    private boolean isRunning = false;
    Timer timer;
    Image backgroundImage = null;
    String backgroundImageFile = "TickerBackground.png";

    public static void main(String args[]) {
        Ticker app = new Ticker(0, 550, 480, 40);
    }

    public Ticker(int x, int y, int width, int height) {
        setLayout(new BorderLayout());

        MediaTracker tracker = new MediaTracker(this);

        URL backgroundImageURL = getClass().getResource(
            backgroundImageFile);
        backgroundImage = Toolkit.getDefaultToolkit().createImage(
            backgroundImageURL);

        try {
            tracker.addImage(backgroundImage, 0);
            tracker.waitForAll();
        }
        catch (InterruptedException e) {
            e.printStackTrace();
            return;
        }

        Window window = new Window(new Frame());
        window.setBounds(x, y, width, height);
        window.add(this);
        window.pack();
        window.setVisible(true);

        timer = new Timer();
        timer.scheduleAtFixedRate(new DisplayTextAction(),
                                  (long) 6000,
                                  (long) 6000);
    }

    public void paint(Graphics g) {
        g.drawImage(backgroundImage, 0, 0, null);
        g.setFont(new Font("Monospaced", Font.PLAIN, 12));
        FontMetrics fm = getFontMetrics(getFont());
        int width = (int) fm.stringWidth(str[strIndex]);
        int height = (int) fm.getHeight();
        Dimension size = getSize();
        int x = (int) 5;
        int y = (int) size.height - 13;
        g.drawString(str[strIndex], x, y);

        super.paint(g);
    }

    public void update(Graphics g) {
        paint(g);
    }


    public Dimension getPreferredSize() {
        return new Dimension(480, 40);
    }

    public class DisplayTextAction
        extends TimerTask {

        public DisplayTextAction() {
            super();
        }

        public void run() {
            int length = str.length;
            if (strIndex + 1 == length) {
                strIndex = 0;
            }
            else {
                strIndex++;
            }
            repaint();
        }
    }

}

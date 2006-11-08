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

import java.awt.BorderLayout;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.net.URL;
import java.awt.Container;
import java.awt.Frame;
import java.awt.Point;
import javax.microedition.xlet.*;

public class Phone
    extends MouseAdapter
    implements Xlet {

    private int keypadIndex = 0;
    private Container container = null;
    private Frame frame = null;
    private PhoneImage phoneImage;
    private KeyLocationHash[] keyLocationHash = new KeyLocationHash[2];
    private int offsetX = 0;

    public static void main(String[] args) {
        Phone phone = new Phone();
        phone.init();
        Frame frame = new Frame();
        frame.add(phone.getContainer());
        frame.setBounds(phone.getContainer().getBounds());
        frame.show();
    }

    public URL imageURL(String image) {
        URL url = Phone.class.getResource(image);
        return url;
    }

    public Container getContainer() {
        return container;
    }

    /**
     * Initialize the xlet. Just get the container, set it up and call
     * loadImage.
     */
    public void initXlet(XletContext context) {
	System.err.println("***** INIT_XLET(Phone) *****");

       try {
          container = context.getContainer();
          init();
          container.setVisible(true);
          PhoneServiceImpl.startService(context);
       } catch (UnavailableContainerException e) {
          System.out.println("Error in getting a root container: " + e);
          context.notifyDestroyed();
       }
    }

    public void startXlet() {
	System.err.println("***** START_XLET(Phone) *****");
    }

    public void pauseXlet() {
	System.err.println("***** PAUSE_XLET(Phone) *****");
    }

    public void destroyXlet(boolean unconditional) {
	System.err.println("***** DESTROY_XLET(Phone) *****");
    }

    public Frame getRootFrame(Container rootContainer) {

        Container tmp = rootContainer;
        while( !( tmp instanceof Frame ) ){
            tmp = tmp.getParent();
        }
        return (Frame) tmp;
    }

    private void init() {
        generateKeyLocations();
        phoneImage = new PhoneImage("BasicKeypad.png",
                                    keyLocationHash[0]);
        if (container == null) {
            container = new Container();
        }       
        container.setLayout(new BorderLayout());
        container.add(phoneImage);
        container.setSize(phoneImage.getPreferredSize());
        container.setVisible(true);
        container.addMouseListener(this);
        container.doLayout();
        Point point = new Point(0, 0);

        keyLocationHash[0].setDisplay( (int) point.x + 5,
                                      (int) point.y + 25,
                                      (int) point.x +
                                      (int) container.getSize().width - 15,
                                      (int) point.y + 40);

    }

    public void mousePressed(MouseEvent e) {
        Object o = phoneImage.klh.isItemAssigned(e.getX(), e.getY());
        if (o instanceof String && ( (String) o).equals("home")) {
            // simulate phone dial by displaying numbers right to left
            new Thread(new Runnable() {
                public void run() {
                    String s = phoneImage.numbers;
                    for (int i = 1; i <= s.length(); i++) {
                        phoneImage.numbers = "Dialing.. " + s.substring(0, i);
                        phoneImage.repaint();
                        try {
                            Thread.sleep(200);
                        }
                        catch (InterruptedException e) {}
                    }
                }
            }).start();
        }
        else {
            if (o instanceof String && ( (String) o).equals("<<")) {
                if (phoneImage.numbers.length() > 0) {
                    phoneImage.numbers = phoneImage.numbers.substring(
                        0, phoneImage.numbers.length() - 1);
                }
                phoneImage.shift = 0;
                phoneImage.repaint();
            }
            else if (o instanceof String && ( (String) o).equals(">>")) {
                if (phoneImage.doesntFitDisplayRegion) {
                    phoneImage.shift++;
                    phoneImage.repaint();
                }
            }
            else if (o != null) {
                phoneImage.numbers += o.toString();
                phoneImage.shift = 0;
                phoneImage.repaint();
            }
        }

    }

    public void generateKeyLocations() {

        int buttonWidth = 65;
        int buttonHeight = 30;

        int column1_x = 10;
        int row1_y = 75;

        int column2_x = 90;
        int row2_y = 110;

        int column3_x = 170;
        int row3_y = 152;

        int row4_y = 190;
        int row5_y = 235;

        keyLocationHash[0] = new KeyLocationHash();

        keyLocationHash[0].put(column1_x, row1_y, column1_x + buttonWidth, row1_y + buttonHeight,
                               new Integer(1));
        keyLocationHash[0].put(column2_x, row1_y, column2_x + buttonWidth, row1_y + buttonHeight,
                               new Integer(2));
        keyLocationHash[0].put(column3_x, row1_y, column3_x + buttonWidth, row1_y + buttonHeight,
                               new Integer(3));

        keyLocationHash[0].put(column1_x, row2_y, column1_x + buttonWidth, row2_y + buttonHeight,
                               new Integer(4));
        keyLocationHash[0].put(column2_x, row2_y + 10, column2_x + buttonWidth, row2_y + buttonHeight,
                               new Integer(5));
        keyLocationHash[0].put(column3_x, row2_y, column3_x + buttonWidth, row2_y + buttonHeight,
                               new Integer(6));

        keyLocationHash[0].put(column1_x, row3_y, column1_x + buttonWidth, row3_y + buttonHeight,
                               new Integer(7));
        keyLocationHash[0].put(column2_x, row3_y, column2_x + buttonWidth, row3_y + buttonHeight,
                               new Integer(8));
        keyLocationHash[0].put(column3_x, row3_y, column3_x + buttonWidth, row3_y + buttonHeight,
                               new Integer(9));

        keyLocationHash[0].put(column1_x, row4_y, column1_x + buttonWidth, row4_y + buttonHeight, "*");
        keyLocationHash[0].put(column2_x, row4_y, column2_x + buttonWidth, row4_y + buttonHeight,
                               new Integer(0));
        keyLocationHash[0].put(column3_x, row4_y, column3_x + buttonWidth, row4_y + buttonHeight, "#");


        keyLocationHash[0].put(column1_x, row5_y, column1_x + buttonWidth, row5_y + buttonHeight, "<<");
        keyLocationHash[0].put(column2_x, row5_y, column2_x + buttonWidth, row5_y + buttonHeight, "home");
        keyLocationHash[0].put(column3_x, row5_y, column3_x + buttonWidth, row5_y + buttonHeight, ">>");
    }

}

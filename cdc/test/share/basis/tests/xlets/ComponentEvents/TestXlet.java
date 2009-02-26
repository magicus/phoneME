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


// A test for checking the delivery of ComponentEvent and
// ContainerEvents, to see whether the events are being
// delivery from the xlet's EventQueue.

// For InputEvents, try TwoXlet.

import java.awt.*;
import java.awt.event.*;
import javax.microedition.xlet.*;
   
public class TestXlet implements Xlet{

   MyContainer comp1 = new MyContainer(Color.RED);
   MyContainer comp2= new MyContainer(Color.BLUE);
   XletContext context;
   Container c;

   boolean hasFailed = false;

   public void initXlet(XletContext context) {
      try {
         this.context = context;
         c = context.getContainer();

         c.setSize(640,480);
         c.setLayout(new BorderLayout());
         c.add(comp1, BorderLayout.NORTH);
         c.add(comp2, BorderLayout.SOUTH);
      
         c.validate();
         c.setVisible(true);

      } catch (Exception e) { e.printStackTrace(); }

   }

   public void startXlet() {
       System.out.println("In startXlet()");
       c.setLayout(new FlowLayout());
       c.validate();

       comp1.setVisible(false);
       c.remove(comp1);
       comp2.add(comp1);
       comp1.setVisible(true);
       comp1.validate();

       comp2.setVisible(false);
       c.removeAll();
       comp2.remove(comp1);

       comp1.add(comp2);
       c.add(comp1);
       comp2.setVisible(true);
       comp2.validate();
  
       comp1.repaint();
       comp2.repaint();

       try {    
          Thread.sleep(1000);
       } catch (Exception e) {}
          
       if (!hasFailed)
          System.out.println("Test OK");
   }

   public void pauseXlet() {}
   public void destroyXlet(boolean b) {}

   class MyContainer extends Container {
      ThreadGroup myTG;
      Color myColor;
      public MyContainer(Color c) {
         super();
         //enable ALL AWTEvents, ha!
         enableEvents(0xFFFFF);

         myTG = Thread.currentThread().getThreadGroup();
         myColor = c;
         setFocusable(true);
      }
      public void processEvent(AWTEvent e) {
         ThreadGroup currentTG =  Thread.currentThread().getThreadGroup();
         if (e.getSource() == this && myTG != currentTG) {
            System.out.println("ERROR!!!!" + e + currentTG);
            hasFailed = true;
         } else {
            System.out.println("OK :" + e.paramString());
         }
      }

      public void paint(Graphics g) {
        // This fails, currently.
        // System.out.println("in paint, " + Thread.currentThread());
        // if (myTG != Thread.currentThread().getThreadGroup()) 
        //    new Exception().printStackTrace();
         g.setColor(myColor);
         g.fillRect(0,0,getSize().width,getSize().height);
      }
      
      public Dimension getPreferredSize() { return new Dimension(100,100); }
      public Dimension getMinimumSize() { return new Dimension(100,100); }
   }
}

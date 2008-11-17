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

package tests.volatileImage;

import java.awt.*;
import java.awt.image.*;

public class TestVolatileComponent extends Frame {
    VolatileImage volImage;
    Image origImage=null;
    
    TestVolatileComponent() {  
        MediaTracker mt = new MediaTracker(this);
        origImage = getToolkit().getImage("duke.gif");    
        try {
            mt.addImage(origImage, 0);
            mt.waitForAll();
        } catch (Exception e) {
            e.printStackTrace();
        }
    } 
   
    public void paint(Graphics g) {
        volImage= drawVolatileImage((Graphics2D)g, volImage, 0, 0, origImage);
    }

    // This method draws a volatile image and returns it or possibly a
    // newly created volatile image object. Subsequent calls to this method
    // should always use the returned volatile image.
    // If the contents of the image is lost, it is recreated using orig.
    // img may be null, in which case a new volatile image is created.
    // This one uses the constructor on component.
    public VolatileImage drawVolatileImage(Graphics2D g, VolatileImage img,
                                           int x, int y, Image orig) {
        final int MAX_TRIES = 100;
        for (int i=0; i<MAX_TRIES; i++) {
            if (img != null) {
                // Draw the volatile image
                g.drawImage(img, x, y, null);
    
                // Check if it is still valid
                if (!img.contentsLost()) {
                    return img;
                }
            } else {
                // Create the volatile image
                img = createVolatileImage(orig.getWidth(null), 
                                          orig.getHeight(null));
            }
    
            // Determine how to fix the volatile image
            switch (img.validate(g.getDeviceConfiguration())) {
            case VolatileImage.IMAGE_INCOMPATIBLE:
                // Create a new volatile image object;
                // this could happen if the component was moved to another
                // device
                img.flush();
                img = createVolatileImage( orig.getWidth(null), 
                                            orig.getHeight(null));
            case VolatileImage.IMAGE_OK:
            case VolatileImage.IMAGE_RESTORED:
                // Copy the original image to accelerated image memory
                Graphics2D gc = (Graphics2D)img.createGraphics();
                gc.drawImage(orig, x, y, null);
                gc.dispose();
                break;
            }
        }
    
        // The image failed to be drawn after MAX_TRIES;
        // draw with the non-accelerated image
        System.out.println("Failed to draw accelerated image - falling back " +
                           " to non-accelerated image!");
        g.drawImage(orig, x, y, null);
        return img;
    }
 
    public static void main(String args[]) {
        TestVolatileComponent tv = new TestVolatileComponent();
        tv.setSize(400, 400);
        tv.setVisible(true);
    }
}

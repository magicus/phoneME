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

import java.awt.*;
import java.awt.image.*;
import java.net.URL;
import javax.microedition.xlet.*;

public class PhotoViewer
    extends Container
    implements Xlet {

    public static final String pvImageFile = "AdInsert60.jpg";
    Image pvImage = null;

    public PhotoViewer() {
        MediaTracker tracker = new MediaTracker(this);
        URL pvImgURL = getClass().getResource(pvImageFile);
        pvImage = Toolkit.getDefaultToolkit().createImage(pvImgURL);

        try {
            tracker.addImage(pvImage, 0);
            tracker.waitForID(0);
        }
        catch (InterruptedException e) {
            e.printStackTrace();
            return;
        }
    }

    public void paint(Graphics g) {
        Dimension d = getSize();
        int x = (d.width -  pvImage.getWidth(null)) / 2;
        int y = 0;
        if ((d.height - pvImage.getHeight(null)) > 0) {
            y = (d.height - pvImage.getHeight(null)) / 2;
        }        
        g.drawImage(pvImage, x, y, (ImageObserver)null);
    }

    public void update(Graphics g) {
        paint(g);
    }

    public static void main(String[] args) {
        Frame frame = new Frame();
        PhotoViewer pv = new PhotoViewer();
        frame.add(pv);
        frame.setSize(508, 443);
        frame.show();
    }

    public void initXlet(XletContext context) {
        try {
            System.out.println(
                "***** XLET MESSAGE: initXlet: PhotoViewer *****");
            Container container = context.getContainer();
            container.add(this);
            container.setVisible(true);
        }
        catch (UnavailableContainerException e) {
            System.out.println("Error in getting a root container: " + e);
            context.notifyDestroyed();
        }
    }

    public void startXlet() {
        System.out.println("***** XLET MESSAGE: startXlet: PhotoViewer *****");
    }

    public void pauseXlet() {
        System.out.println("***** XLET MESSAGE: pauseXlet: PhotoViewer *****");
    }

    public void destroyXlet(boolean unconditional) {
        System.out.println("***** XLET MESSAGE: destroyXlet: PhotoViewer *****");
    }
}

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

package com.sun.appmanager.impl.presentation.AwtPDA;

import java.awt.*;
import java.net.URL;

public class ImageScrollPane
    extends ScrollPane {
    MediaTracker tracker = new MediaTracker(this);
    String backgroundImageFile = null;
    URL bgImgURL = null;
    Image backgroundImage = null;
    int width = 0;
    int height = 0;

    public ImageScrollPane(String imageFile, int width, int height) {
        super(ScrollPane.SCROLLBARS_NEVER);
        backgroundImageFile = imageFile;
        bgImgURL = getClass().getResource(backgroundImageFile);
        backgroundImage =
            Toolkit.getDefaultToolkit().createImage(bgImgURL);

        try {
            tracker.addImage(backgroundImage, 0);
            tracker.waitForID(0);
        }
        catch (InterruptedException e) {
            e.printStackTrace();

            return;
        }

        this.width = width;
        this.height = height;
    }

    public Dimension getPreferredSize() {
        return new Dimension( (int) backgroundImage.getWidth(null),
                             (int) backgroundImage.getHeight(null));
    }

    public void paint(Graphics g) {
//            g.drawImage(backgroundImage, 0, 0, null);
        g.drawImage(backgroundImage, 0, 0, width, height, null);
        super.paint(g);
    }

    public void update(Graphics g) {
        paint(g);
    }

}

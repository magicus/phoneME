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

import java.awt.Dialog;
import java.awt.MediaTracker;
import java.awt.Image;
import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Toolkit;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Panel;
import java.net.URL;
import java.util.Timer;
import java.util.TimerTask;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

public class Gauge
    extends Dialog {
    private MediaTracker tracker = null;

    private Image gaugeBackgroundImage = null;

    private final int numGaugeImageFiles = 16;
    private Image[] gaugeImages = new Image[numGaugeImageFiles];

    Timer paintTimer = null;
    boolean drawBackground = true;
    int imageIndex = 0;
    GaugePanel gaugePanel = null;

    boolean clockwise = true;

    AwtPDAPresentationMode presentationMode = null;
    String message = null;

    public Gauge(AwtPDAPresentationMode presentationMode, String message, boolean button) {
        super(presentationMode.getFrame());

        this.presentationMode = presentationMode;
        this.message = message;

        //        setUndecorated(true);
        tracker = new MediaTracker(this);

        String gaugeBackgroundImageFile = "resources/gauge/"
            +
            presentationMode.getString(
                "AwtPDAPresentationMode.gaugeBackgroundImage");
        URL gaugeBackgroundImageURL = getClass().getResource(
            gaugeBackgroundImageFile);
        gaugeBackgroundImage = Toolkit.getDefaultToolkit().createImage(
            gaugeBackgroundImageURL);

        String gaugeBaseImageFile = "resources/gauge/"
            +
            presentationMode.getString("AwtPDAPresentationMode.gaugeImageBase");
        int index = gaugeBaseImageFile.lastIndexOf('.');
        String imageFileBase = gaugeBaseImageFile.substring(0, index);
        String imageFileSuffix = gaugeBaseImageFile.substring(index);
        for (int i = 0; i < numGaugeImageFiles; i++) {
            String imageFile = imageFileBase + (i + 1) + imageFileSuffix;
            URL imageURL = getClass().getResource(imageFile);
            gaugeImages[i] = Toolkit.getDefaultToolkit().createImage(imageURL);
        }

        tracker = new MediaTracker(this);
        try {

            for (int i = 0; i < numGaugeImageFiles; i++) {
                tracker.addImage(gaugeImages[i], i);
            }
            tracker.addImage(gaugeBackgroundImage, numGaugeImageFiles);

            for (int i = 0; i < numGaugeImageFiles + 1; i++) {
                tracker.waitForID(i);
            }
        }
        catch (InterruptedException e) {
            e.printStackTrace();
            return;
        }

        gaugePanel = new GaugePanel();

        gaugePanel.setBackground(Color.white);
        setBackground(Color.white);

        setLayout(new BorderLayout());
        add(gaugePanel, BorderLayout.CENTER);

        if (button) {
            addButton();
            setBounds(120, 200, 250, 200);
        } else {
            setBounds(170, 375, 160, 150);
        }

    }

    public Gauge(AwtPDAPresentationMode presentationMode, boolean button) {
        this(presentationMode, null, button);
    }

    public Gauge(AwtPDAPresentationMode presentationMode, String message) {
        this(presentationMode, message, false);
    }

    public void setMessage(String message) {
        this.message = message;
    }

    public void addButton() {

        // Insert cancel button
        ImageButton cancelButton = new ImageButton(presentationMode.
            mediumButtonImage);
        cancelButton.setLabel(presentationMode.getString(
            "AwtPDAPresentationMode.cancelButtonLabel"), 95, 17);
        cancelButton.setFont(presentationMode.currentUserFont);
        cancelButton.setTextColor(Color.black);
        cancelButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                    stop();
                }
            });
            
        Panel cancelButtonPanel = new Panel() {
           public Dimension getPreferredSize() {
               return new Dimension(50, 30);
           }
        };
        cancelButtonPanel.setLayout(new BorderLayout());
        cancelButtonPanel.add(cancelButton, BorderLayout.CENTER);
        cancelButton.setBackground(Color.white);

        add(cancelButtonPanel, BorderLayout.SOUTH);
        validate();            
    }
     
    public void start() {
        paintTimer = new Timer();
        setVisible(true);
        AnimationAction action = new AnimationAction();
        paintTimer.scheduleAtFixedRate(action, 0, 150);
    }

    public void stop() {
        if (paintTimer != null) {
            paintTimer.cancel();
        }
        setVisible(false);
    }

    class GaugePanel
        extends Panel {
        int x = 0;
        int y = 0;
        int imageWidth = 0;
        int imageHeight = 0;
        boolean firstTime = true;

        public void paint(Graphics g) {

            Dimension size = getSize();
            imageWidth = gaugeBackgroundImage.getWidth(this);
            imageHeight = gaugeBackgroundImage.getHeight(this);
            x = (size.width - imageWidth) / 2;
            //               int y = (size.height - imageHeight) / 2;
            y = 15;

            g.drawImage(gaugeBackgroundImage, x, y, this);
            g.drawImage(gaugeImages[imageIndex], x, y, this);

            if ( (imageIndex == numGaugeImageFiles - 1)) {
                imageIndex = 0;
            }
            else {
                imageIndex++;
            }

            if (message != null) {
                g.setFont(presentationMode.currentUserFont);
                g.drawString(message, 10,
                             (int) getSize().height - 20);
            }

        }

        public void update(Graphics g) {
            g.clearRect(x - 2, y - 2, imageWidth + 2, imageHeight + 2);
            paint(g);
        }

        public Dimension getPreferredSize() {
            return new Dimension(200, 150);
        }

        public Dimension getMinimumSize() {
            return getPreferredSize();
        }

        public Dimension getMaximumSize() {
            return getPreferredSize();
        }

    }

    public class AnimationAction
        extends TimerTask {
        public AnimationAction() {
            super();
        }

        public void run() {
            gaugePanel.repaint();
        }
    }

}

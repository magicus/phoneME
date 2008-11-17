/*
 * @(#)FrameDemo.java	1.5 06/10/10
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
package basis.demos;

import java.awt.*;
import java.awt.event.*;
import java.util.EventObject;
import basis.Builder;
import basis.DemoButton;
import basis.DemoButtonListener;

public class FrameDemo extends Demo {
    static int count;
    Frame frame;

    public FrameDemo() {
        count++;
        setLayout(new GridLayout(3, 2));
        DemoButton newFrameButton = new DemoButton("New");
        DemoButton titleButton = new DemoButton("Title");
        DemoButton sizeButton = new DemoButton("Size");
        DemoButton resizableButton = new DemoButton("Resizable");
        DemoButton locationButton = new DemoButton("Location");
        DemoButton stateButton = new DemoButton("State");
        newFrameButton.addDemoButtonListener(new DemoButtonListener() {
            public void buttonPressed(EventObject e) {
                final Frame frame = new Frame();
                frame.add(new FrameDemo());
                frame.setTitle("" + count);
                frame.setSize(160, 120);
                frame.addWindowListener(new WindowAdapter() {
                    public void windowClosing(WindowEvent e) {
                        frame.setVisible(false);
                        frame.dispose();
                    }
                });
                Toolkit toolkit = frame.getToolkit();
                Dimension d = toolkit.getScreenSize();
                frame.setLocation((int) (Math.random() * 2 * d.width / 3), (int) (Math.random() * 2 * d.height / 3));
                frame.setVisible(true);
            }
        });
        titleButton.addDemoButtonListener(new DemoButtonListener() {
            public void buttonPressed(EventObject e) {
                Frame frame = FrameDemo.this.getFrame();
                frame.setTitle(frame.getTitle() + "*");
            }
        });
        sizeButton.addDemoButtonListener(new DemoButtonListener() {
            public void buttonPressed(EventObject e) {
                Frame frame = FrameDemo.this.getFrame();
                if (!frame.isResizable()) {
                    return;
                }
                Toolkit toolkit = getToolkit();
                Dimension d = toolkit.getScreenSize();
                frame.setSize((int) (Math.random() * d.width / 2) + 60, (int) (Math.random() * d.height / 2) + 60);
                frame.validate();
            }
        });
        resizableButton.addDemoButtonListener(new DemoButtonListener() {
            public void buttonPressed(EventObject e) {
                Frame frame = FrameDemo.this.getFrame();
                if (frame.isResizable()) {
                    frame.setResizable(false);
                    DemoButton b = (DemoButton) e.getSource();
                    b.setForeground(Builder.SUN_RED);
                } else {
                    frame.setResizable(true);
                    DemoButton b = (DemoButton) e.getSource();
                    b.setForeground(DemoButton.DEFAULT_COLOR);
                }
            }
        });
        locationButton.addDemoButtonListener(new DemoButtonListener() {
            public void buttonPressed(EventObject e) {
                Frame frame = FrameDemo.this.getFrame();
                Toolkit toolkit = frame.getToolkit();
                Dimension d = toolkit.getScreenSize();
                frame.setLocation((int) (Math.random() * 2 * d.width / 3), (int) (Math.random() * 2 * d.height / 3));
            }
        });
        stateButton.addDemoButtonListener(new DemoButtonListener() {
            public void buttonPressed(EventObject e) {
                Frame frame = FrameDemo.this.getFrame();
                frame.setState(frame.getState() == Frame.NORMAL ? Frame.ICONIFIED : Frame.NORMAL);
            }
        });
        add(newFrameButton);
        add(titleButton);
        add(sizeButton);
        add(resizableButton);
        add(locationButton);
        add(stateButton);
    }

    Frame getFrame() {
        if (frame == null) {
            Container parent = getParent();
            while (parent.getParent() != null) {
                parent = parent.getParent();
            }
            frame = (Frame) parent;
        }
        return frame;
    }
}


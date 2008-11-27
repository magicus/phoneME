/*
 * @(#)BeanDemo.java	1.5 06/10/10
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
import java.beans.*;
import java.io.*;
import java.util.EventObject;
import basis.DemoButton;
import basis.DemoButtonListener;
import basis.Bean;
import basis.Builder;


public class BeanDemo extends Demo implements PropertyChangeListener, VetoableChangeListener {
    private Bean bean = new Bean();
    private DemoButton textButton = new DemoButton("Text");
    private DemoButton colorButton = new DemoButton("Color");
    private DemoButton dimensionButton = new DemoButton("Dimension");
    private DemoButton saveButton = new DemoButton("save");
    private DemoButton loadButton = new DemoButton("load");
    private static String[] texts = {
        "Pumpkin",
        "Broccoli",
        "Tomato",
        "Peas",
        "Lettuce",
        "Onion",
        "Potato",
        "Radish",
        "Carrot",
        "Cabbage",
        "Cauliflower",
        "Cucumber",
        "Celery",
        "Corn"
    };

    public BeanDemo() {
        setLayout(new FlowLayout());
        add(textButton);
        add(colorButton);
        add(dimensionButton);
        add(saveButton);
        add(loadButton);
        bean.addPropertyChangeListener(this);
        bean.addVetoableChangeListener(this);
        bean.setLocation(20, 20);
        add(bean);
        ButtonListener listener = new ButtonListener();
        textButton.addDemoButtonListener(listener);
        colorButton.addDemoButtonListener(listener);
        dimensionButton.addDemoButtonListener(listener);
        saveButton.addDemoButtonListener(listener);
        loadButton.addDemoButtonListener(listener);
    }

    private String message(PropertyChangeEvent event) {
        String name = event.getPropertyName();
        Object oldValue = null;
        Object newValue = null;
        for (int i = 0; i < 2; i++) {
            Object object = null;
            object = (i == 0) ? event.getOldValue() : event.getNewValue();
            if (object instanceof Color) {
                Color color = (Color) object;
                int r = color.getRed();
                int g = color.getGreen();
                int b = color.getBlue();
                object = "[" + r + "," + g + "," + b + "]";
            }
            if (object instanceof Dimension) {
                Dimension d = (Dimension) object;
                object = "[" + d.width + "," + d.height + "]";
            }
            if (object instanceof Point) {
                Point p = (Point) object;
                object = "[" + p.x + "," + p.y + "]";
            }
            if (i == 0) {
                oldValue = object;
            } else {
                newValue = object;
            }
        }
        return name + ":  " + oldValue + " => " + newValue;
    }

    class ButtonListener implements DemoButtonListener, Serializable {
        public void buttonPressed(EventObject event) {
            DemoButton button = (DemoButton) event.getSource();
            try {
                if (button == textButton) {
                    int index = (int) (Math.random() * texts.length);
                    bean.setText(texts[index]);
                    bean.repaint();
                }
                if (button == colorButton) {
                    bean.setColor(new Color((int) (Math.random() * 255 * 255 * 255)));
                    bean.repaint();
                }
                if (button == dimensionButton) {
                    Dimension size = getSize();
                    int width = 20 + (int) (Math.random() * size.width);
                    int height = 5 + (int) (Math.random() * size.height);
                    bean.setDimension(new Dimension(width, height));
                    bean.repaint();
                }
                if (button == saveButton) {
                    try {
                        FileOutputStream fos = new FileOutputStream("Bean.ser");
                        ObjectOutputStream oos = new ObjectOutputStream(fos);
                        oos.writeObject(bean);
                        oos.close();
                        setStatus("Bean saved.");
                    } catch (IOException ioe) {
                        System.err.println("Error saving Bean: " + ioe);
                        setStatus("Error saving bean!");
                    }
                }
                if (button == loadButton) {
                    try {
                        FileInputStream fis = new FileInputStream("Bean.ser");
                        ObjectInputStream ois = new ObjectInputStream(fis);
                        Bean oldBean = bean;
                        bean = (Bean) ois.readObject();
                        bean.addPropertyChangeListener(BeanDemo.this);
                        bean.addVetoableChangeListener(BeanDemo.this);
                        remove(oldBean);
                        add(bean);
                        BeanDemo.this.repaint();
                        ois.close();
                        setStatus("Bean loaded.");
                    } catch (Exception e) {
                        System.err.println("Error loading Bean: " + e);
                        setStatus("Error loading bean!");
                    }
                }
            } catch (PropertyVetoException pve) {
                PropertyChangeEvent pce = pve.getPropertyChangeEvent();
                setStatus(message(pce) + " VETOED!");
            }
        }
    }

    public void propertyChange(PropertyChangeEvent event) {
        setStatus(message(event));
        String name = event.getPropertyName();
        if (name.equals("Dimension")) {
            validate();
        }
    }

    public void vetoableChange(PropertyChangeEvent event) throws PropertyVetoException {
        String name = event.getPropertyName();
        if (name.equals("Text")) {
            String oldText = (String) event.getOldValue();
            String newText = (String) event.getNewValue();
            if (newText.startsWith("C")) {
                throw new PropertyVetoException("Don't like vegetables beginning with C!", event);
            }
        }
        if (name.equals("Color")) {
            Color color = (Color) event.getNewValue();
            if (color.getRed() + color.getGreen() + color.getBlue() < 300) {
                throw new PropertyVetoException("Too dark!", event);
            }
        }
        if (name.equals("Dimension")) {
            Dimension dimension = (Dimension) event.getNewValue();
            Dimension size = getSize();
            Point point = BeanDemo.this.bean.getLocation();
            if (dimension.width < 40 || dimension.height < 10) {
                throw new PropertyVetoException("Too small!", event);
            }
            if (dimension.width > size.width || dimension.height > size.height) {
                throw new PropertyVetoException("Too big!", event);
            }
        }
    }
}


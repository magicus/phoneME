/*
 * @(#)Builder.java	1.5 06/10/10
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
package basis;

import java.awt.*;
import java.awt.event.*;
import java.lang.reflect.*;
import java.util.ArrayList;
import java.util.EventObject;
import basis.demos.*;

public class Builder extends Container {
    public static final Color SUN_BLUE = new Color(89, 79, 191);
    public static final Color SUN_YELLOW = new Color(251, 226, 73);
    public static final Color SUN_RED = new Color(209, 33, 36);
    public static final Color SUN_LIGHTBLUE = new Color(204, 204, 255);
    private ArrayList demos;
    private Container demoContainer = new Container();
    private CardLayout cardLayout = new CardLayout();
    private Status status;

    public Builder(ArrayList demos) {
        this.demos = demos;
    }

    public void build(Container container) throws Exception {
        container.setBackground(Color.white);
        container.setLayout(new BorderLayout());
        container.add(this, BorderLayout.CENTER);
        status = new Status();
        if (demos.size() > 1) {
            setLayout(new BorderLayout());
            demoContainer.setLayout(cardLayout);
            Container buttonContainer = new Container();
            add(buttonContainer, BorderLayout.NORTH);
            add(demoContainer, BorderLayout.CENTER);
            add(status, BorderLayout.SOUTH);
            buttonContainer.setLayout(new GridLayout(1, demos.size()));
            DemoButtonListener listener = new DemoButtonListener() {
                public void buttonPressed(EventObject e) {
                    DemoButton b = (DemoButton) e.getSource();
                    showDemo(b.getLabel());
                }
            };
            for (int i = 0; i < demos.size(); i++) {
                Class clazz = Class.forName((String) demos.get(i));
                Component component = (Component) clazz.newInstance();
                String label = (String) demos.get(i);
                label = label.substring(label.lastIndexOf(".") + 1, label.lastIndexOf("Demo"));
                if (i == 0) {
                    setStatus(label);
                }
                demoContainer.add(label, component);
                DemoButton b = new DemoButton(label);
                buttonContainer.add(b);
                b.addDemoButtonListener(listener);
            }
        } else {
            String demoName = (String) demos.get(0);
            Class clazz = Class.forName(demoName);
            Demo demo = (Demo) clazz.newInstance();
            setLayout(new BorderLayout());
            add(demo, BorderLayout.CENTER);
            add(status, BorderLayout.SOUTH);
            String label = demoName;
            label = label.substring(label.lastIndexOf(".") + 1, label.lastIndexOf("Demo"));
            setStatus(label);
        }
    }

    public void showDemo(String name) {
        cardLayout.show(demoContainer, name);
        setStatus(name);
    }

    public void setStatus(String text) {
        if (status == null) {
            return;
        }
        if (text.equals(getStatus())) {
            return;
        }
        status.setText(text);
        status.repaint();
    }

    public String getStatus() {
        if (status == null) {
            return null;
        }
        return status.getText();
    }

    class Status extends Component {
        private String text = "";
        private Font font = new Font("sanserif", Font.BOLD, 12);
        private Dimension preferredSize;

        public Status() {
            setBackground(SUN_YELLOW);
            setForeground(SUN_RED);
        }

        public void setText(String text) {
            this.text = text;
            if (status.isShowing()) {
                Graphics g = getGraphics();
                FontMetrics fm = g.getFontMetrics(font);
                int fw = fm.stringWidth(text);
                int fh = fm.getHeight();
                preferredSize = new Dimension(fw + 4, fh + 4);
            }
        }

        public String getText() {
            return text;
        }

        public Dimension getPreferredSize() {
            if (preferredSize == null) {
                Graphics g = getGraphics();
                FontMetrics fm = g.getFontMetrics(font);
                int fw = fm.stringWidth(text);
                int fh = fm.getHeight();
                preferredSize = new Dimension(fw + 4, fh + 4);
            }
            return preferredSize;
        }

        public Dimension getMinimumSize() {
            return getPreferredSize();
        }

        public Dimension getMaximumSize() {
            return new Dimension(Integer.MAX_VALUE, Integer.MAX_VALUE);
        }

        public void paint(Graphics g) {
            Color background = getBackground();
            Color foreground = getForeground();
            Dimension size = getSize();
            g.setColor(background);
            g.fillRect(0, 0, size.width, size.height);
            g.setColor(foreground);
            g.setFont(font);
            FontMetrics fm = g.getFontMetrics(font);
            int w = fm.stringWidth(text);
            g.drawString(text, (size.width - w) / 2, 2 * size.height / 3);
        }
     }
}


/*
 * @(#)WidgetDemo.java	1.6 06/10/10
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
package personal.demos;

import java.awt.*;
import java.awt.datatransfer.*;
import java.awt.event.*;
import java.io.*;
import java.util.ArrayList;
import basis.Builder;
import basis.demos.Demo;

public class WidgetDemo extends Demo implements ActionListener, ItemListener, MouseListener, ClipboardOwner {
    private GridBagLayout gbl;
    private GridBagConstraints gbc;
    private Button button = new Button("Button");
    private Checkbox checkBox = new Checkbox("Checkbox");
    private TextField textField = new TextField();
    private TextArea textArea = new TextArea();
    private List list = new List(3, false);
    private Choice choice = new Choice();
    private Popup popup = new Popup();
    private Clipboard internalClipboard = new Clipboard("Internal");
    private Clipboard systemClipboard;
    private Clipboard clipboard = internalClipboard;
    private Color[] colors = {Builder.SUN_BLUE, Builder.SUN_YELLOW, Builder.SUN_RED};
    private String[] strings = {};

    public static void main(String args[]){
        new WidgetDemo();
    }
    
    public WidgetDemo() {
        for (int i = 1; i <= 10; i++) {
            list.add("List " + i);
        }
        for (int i = 1; i <= 10; i++) {
            choice.add("Choice " + i);
        }
        button.addActionListener(this);
        checkBox.addItemListener(this);
        textField.addActionListener(this);
        list.addActionListener(this);
        list.addItemListener(this);
        choice.addItemListener(this);
        textArea.addMouseListener(this);
        Panel topPanel = new Panel();
        Panel leftPanel = new Panel();
        Panel rightPanel = new Panel();
        leftPanel.setLayout(new GridLayout(3, 1));
        leftPanel.add(button);
        leftPanel.add(checkBox);
        leftPanel.add(choice);
        rightPanel.setLayout(new BorderLayout());
        rightPanel.add("Center", list);
        rightPanel.add("South", textField);
        topPanel.setLayout(new GridLayout(1, 2));
        topPanel.add(leftPanel);
        topPanel.add(rightPanel);
        setLayout(new BorderLayout());
        add("North", topPanel);
        add("Center", textArea);
        add("South", popup);
    }

    private void add(Component component, int gridx, int gridy,
        int gridwidth, int gridheight, int fill,
        int ipadx, int ipady, Insets insets, int anchor,
        double weightx, double weighty) {
        gbc.gridx = gridx;
        gbc.gridy = gridy;
        gbc.gridwidth = gridwidth;
        gbc.gridheight = gridheight;
        gbc.fill = fill;
        gbc.ipadx = ipadx;
        gbc.ipady = ipady;
        gbc.insets = insets;
        gbc.anchor = anchor;
        gbc.weightx = weightx;
        gbc.weighty = weighty;
        gbl.setConstraints(component, gbc);
        add(component);
    }

    public void actionPerformed(ActionEvent event) {
        if (event.getSource() == button) {
            textArea.append("Button.\n");
            int index = 0;
            for (int i = 0; i < colors.length; i++) {
                if (button.getBackground() == colors[i]) {
                    index = ++i < colors.length ? i : 0;
                    break;
                }
            }
            button.setBackground(colors[index]);
            Transferable contents = new TransferableColor(colors[index]);
            clipboard.setContents(contents, this);
            setStatus("Copy to " + clipboard.getName() + " clipboard");
        }
        if (event.getSource() == textField) {
            textArea.append("TextField: " + textField.getText() + "\n");
        }
    }

    public void itemStateChanged(ItemEvent event) {
        if (event.getSource() == checkBox) {
            if ((checkBox.getState()) == true) {
                textArea.append("Checkbox ON.\n");
                if (systemClipboard == null) {
                    Toolkit tk = Toolkit.getDefaultToolkit();
                    systemClipboard = tk.getSystemClipboard();
                }
                clipboard = systemClipboard;
            } else {
                textArea.append("Checkbox OFF.\n");
                clipboard = internalClipboard;
            }
            setStatus("Using " + clipboard.getName() + " clipboard");
        }
        if (event.getSource() == list) {
            textArea.append(list.getSelectedItem() + "\n");
        }
        if (event.getSource() == choice) {
            textArea.append(choice.getSelectedItem() + "\n");
        }
    }

    public void mouseClicked(MouseEvent e) {
        int modifier = e.getModifiers();
        if ((modifier & InputEvent.BUTTON1_MASK) != 0) {
            return;
        }
        if ((modifier & InputEvent.BUTTON2_MASK) != 0) {
            return;
        }
        setStatus("Pasting to " + clipboard.getName() + " clipboard...");
        Transferable contents = clipboard.getContents(null);
        if (contents == null) {
            setStatus(clipboard.getName() + " clipboard empty!");
            return;
        }
        String status = "Paste ";
        boolean found = false;
        DataFlavor[] flavors = contents.getTransferDataFlavors();
        for (int i = 0; i < flavors.length; i++) {
            DataFlavor flavor = (DataFlavor) flavors[i];
            status += flavor.getHumanPresentableName() + " ";
            setStatus(status);
            try {
                Object object = contents.getTransferData(flavor);
                if (object instanceof Color) {
                    status += "(Color) ";
                    setStatus(status);
                    textArea.setBackground((Color) object);
                    found = true;
                    break;
                }
                ArrayList list = new ArrayList();
                if (object instanceof String) {
                    status += "(String) ";
                    setStatus(status);
                    String string = (String) object;
                    String substring = null;
                    while (string != null) {
                        int index = string.indexOf('\n');
                        if (index >= 0) {
                            substring = string.substring(0, index);
                            string = string.substring(index + 1);
                        } else {
                            substring = string;
                            string = null;
                        }
                        list.add(substring);
                    }
                }
                BufferedReader br = null;
                if (object instanceof InputStream) {
                    status += "(InputStream) ";
                    setStatus(status);
                    InputStream is = (InputStream) object;
                    InputStreamReader isr = new InputStreamReader(is);
                    br = new BufferedReader(isr);
                }
                if (object instanceof Reader) {
                    status += "(Reader) ";
                    setStatus(status);
                    Reader reader = (Reader) object;
                    br = new BufferedReader(reader);
                }
                if (object instanceof InputStream || object instanceof Reader) {
                    while (true) {
                        String string = br.readLine();
                        if (string == null) {
                            break;
                        }
                        list.add(string);
                    }
                }
                if (object instanceof String || object instanceof InputStream || object instanceof Reader) {
                    strings = new String[list.size()];
                    for (int j = 0; j < list.size(); j++) {
                        strings[j] = (String) list.get(j);
                    }
                    found = true;
                    break;
                }
            } catch (UnsupportedFlavorException ufe) {
                setStatus("Unsupported flavor!");
            } catch (IOException ioe) {
                setStatus("IOException!");
            }
        }
        if (found) {
            repaint();
        } else {
            setStatus("No data in " + clipboard.getName() + " clipboard!");
        }
    }

    public void mouseEntered(MouseEvent e) {
    }

    public void mouseExited(MouseEvent e) {
    }

    public void mousePressed(MouseEvent e) {
    }

    public void mouseReleased(MouseEvent e) {
    }

    public void lostOwnership(Clipboard clipboard, Transferable contents) {
        setStatus("Lost ownership of " + clipboard.getName() + " clipboard!");
    }

    class Popup extends Component implements ActionListener {
        final Font DEFAULT_FONT = new Font("sanserif", Font.BOLD, 12);
        final Color DEFAULT_COLOR = Builder.SUN_RED;
        private String label = "Popup";
        private Dimension preferredSize;
        PopupMenu popupMenu = new PopupMenu("Popup Menu");

        Popup() {
            setForeground(DEFAULT_COLOR);
            for (int i = 1; i <= 10; i++) {
                MenuItem mi = new MenuItem("Popup " + i);
                mi.addActionListener(this);
                popupMenu.add(mi);
            }
            add(popupMenu);
            enableEvents(AWTEvent.MOUSE_EVENT_MASK);
        }

        public Dimension getPreferredSize() {
            if (preferredSize == null) {
                Graphics g = getGraphics();
                FontMetrics fm = g.getFontMetrics(DEFAULT_FONT);
                int fw = fm.stringWidth(label);
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
            Dimension d = getSize();
            g.setColor(getForeground());
            g.fillRect(0, 0, d.width, d.height);
            g.setColor(Color.white);
            g.setFont(DEFAULT_FONT);
            FontMetrics fm = g.getFontMetrics(DEFAULT_FONT);
            int fw = fm.stringWidth(label);
            int fa = fm.getAscent();
            g.drawString(label, (d.width - fw) / 2, (d.height + fa) / 2 - 1);
        }

        protected void processMouseEvent(MouseEvent event) {
            if (event.isPopupTrigger()) {
                popupMenu.show(this, event.getX(), event.getY());
            }
            super.processMouseEvent(event);
        }

        public void actionPerformed(ActionEvent event) {
            if (event.getSource() == popupMenu) {
                textArea.append(event.getActionCommand() + "\n");
            }
        }
    }
}

class TransferableColor implements Transferable {
    public static DataFlavor colorFlavor = new DataFlavor(Color.class, "Java Color Object");
    private static DataFlavor[] flavors = {colorFlavor, DataFlavor.stringFlavor};
    private Color color;

    public TransferableColor(Color color) {
        this.color = color;
    }

    public DataFlavor[] getTransferDataFlavors() {
        return (DataFlavor[]) flavors.clone();
    }

    public boolean isDataFlavorSupported(DataFlavor flavor) {
        for (int i = 0; i < flavors.length; i++) {
            if (flavor.equals(flavors[i])) {
                return true;
            }
        }
        return false;
    }

    public Object getTransferData(DataFlavor flavor) {
        if (flavor.equals(flavors[0])) {
            return color;
        }
        if (flavor.equals(flavors[1])) {
            return color.toString();
        }
        return null;
    }
}


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

import com.sun.appmanager.impl.CDCAmsAppController;
import com.sun.appmanager.apprepository.AppModule;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.net.URL;
import java.text.SimpleDateFormat;

public class AwtPDAPresentationTaskbar
    extends Panel {

    private CDCAmsAppController controller = null;
    private Panel taskBarPanel = null;

    private Color slotsColor = new Color(176, 177, 203);

    // app id's are the keys, the button is the value
    private Hashtable runningAppsHash = new Hashtable();

    // elements in this vector are app id's, which will
    // then be used as the key into the above hashtable
    private Vector runningAppsVector = new Vector();

    private AwtPDAPresentationMode presentationMode = null;

    private int selectedIndex = 0;
    private int numApps = 0;
    private ScrollPane scrollPane = null;

//  Title bar header
    ImagePanel titleBarHeader = null;

    ImageButton cancelButton = null;
    ImageButton gotoTaskButton = null;
    ImageButton endTaskButton = null;

    public AwtPDAPresentationTaskbar(CDCAmsAppController controller,
                                     AwtPDAPresentationMode
                                     presentationMode) {
        this.controller = controller;
        this.presentationMode = presentationMode;
        initialize();
    }

    public void initialize() {
        setLayout(new BorderLayout());

        String titleBarImageFile = "resources/background/"
            + presentationMode.getString("AwtPDAPresentationMode.titleBarImage");
        titleBarHeader = new ImagePanel(
            titleBarImageFile);
        titleBarHeader.setFont(presentationMode.currentUserFont);
        titleBarHeader.setLabel(presentationMode.getString(
            "AwtPDAPresentationMode.taskbarHeader.label"), -1, 17);
        add(titleBarHeader, BorderLayout.NORTH);

        taskBarPanel = new Panel(new SlotLayout());

        scrollPane = new ScrollPane(ScrollPane.SCROLLBARS_AS_NEEDED);
        scrollPane.add(taskBarPanel);

        add(scrollPane, BorderLayout.CENTER);

        String bottomBarImageFile = "resources/background/"
            +
            presentationMode.getString("AwtPDAPresentationMode.bottomBarImage");
        ImagePanel bottomPanel = new ImagePanel(bottomBarImageFile);
        bottomPanel.setLayout(new GridLayout(1, 3));

        gotoTaskButton = new ImageButton(createImage(presentationMode.getString(
            "AwtPDAPresentationMode.taskbarSelect")));
        gotoTaskButton.addActionListener(new TaskbarAppSwitchActionListener());
        gotoTaskButton.setDepressedImage(createImage(presentationMode.getString(
            "AwtPDAPresentationMode.taskbarSelect.pressed")));
        gotoTaskButton.setLabel("Goto");

        endTaskButton = new ImageButton(createImage(presentationMode.getString(
            "AwtPDAPresentationMode.taskbarEndTask")));
        endTaskButton.addActionListener(new TaskbarEndTaskActionListener());
        endTaskButton.setDepressedImage(createImage(presentationMode.getString(
            "AwtPDAPresentationMode.taskbarEndTask.pressed")));
        endTaskButton.setLabel(presentationMode.getString(
            "AwtPDAPresentationMode.taskbarEndTask.label"));

        cancelButton = new ImageButton(createImage(presentationMode.getString(
            "AwtPDAPresentationMode.cancelButton")));

        cancelButton.addActionListener(new CancelActionListener());
        cancelButton.setLabel(presentationMode.getString(
            "AwtPDAPresentationMode.cancelButton.label"));

        bottomPanel.add(gotoTaskButton);
        bottomPanel.add(endTaskButton);
        bottomPanel.add(cancelButton);
        add(bottomPanel, BorderLayout.SOUTH);
    }

    public void refresh() {

        gotoTaskButton.setFont(presentationMode.currentUserFont);
        endTaskButton.setFont(presentationMode.currentUserFont);
        cancelButton.setFont(presentationMode.currentUserFont);

        TaskBarSlotPanel slot = null;

        int panelHeight = getPreferredSize().height;
        int slotHeight = new TaskBarSlotPanel().getPreferredSize().height;

        int minSlots = panelHeight / slotHeight;
        if (panelHeight % slotHeight != 0) {
            minSlots++;
        }
        taskBarPanel.removeAll();

        selectedIndex = 0;

        int num = 0;
        for (Enumeration e = runningAppsVector.elements(); e.hasMoreElements();
             num++) {
            String appId = (String) e.nextElement();
            Vector appVector = (Vector) runningAppsHash.get( (Object)
                appId);

            ImageButton button = (ImageButton) appVector.get(0);
            String dateString = (String) appVector.get(1);

            slot = new TaskBarSlotPanel(num);
            if (num % 2 == 0) {
                slot.setBackground(Color.white);
            }
            else {
                slot.setBackground(slotsColor);
            }

            slot.setButton(button);
            slot.setAppNameText(button.getLabel());

            slot.setDateText(dateString);
            slot.validate();
            slot.repaint();
            taskBarPanel.add(slot, num);
        }

        numApps = num;

// Add empty slots for a better look if there isn't enough apps
// to fill the entire panel.
        if (num < minSlots) {
            for (int i = num; i < minSlots; i++) {
                slot = new TaskBarSlotPanel(i);
                if (i % 2 == 0) {
                    slot.setBackground(Color.white);
                }
                else {
                    slot.setBackground(slotsColor);
                }
                taskBarPanel.add(slot, i);
            }
        }

        if (numApps > 0) {
            endTaskButton.setVisible(true);
            setSelectedSlot(selectedIndex, true);
        } else {
            gotoTaskButton.setVisible(false);
            endTaskButton.setVisible(false);
        }
        taskBarPanel.validate();
        taskBarPanel.repaint();
    }

    public void setSelectedSlot(int index, boolean val) {

        if (index >= runningAppsVector.size()) {
            return;
        }
        String appId = (String)runningAppsVector.get(index);
        if (appId == null) {
            return;
        }
        AppModule module = controller.getAppModule(appId);
        if (module == null) {
            return;
        }

        TaskBarSlotPanel app = (TaskBarSlotPanel) taskBarPanel.getComponent(
            index);
        app.setSelected(val);
        if (module.getType().equals("XLET")) {
            gotoTaskButton.setVisible(true);
        } else {
            gotoTaskButton.setVisible(false);
        }
    }

    public Dimension getPreferredSize() {
        return new Dimension(480, 462);
    }

    public Dimension getMinimumSize() {
        return getPreferredSize();
    }

    public Dimension getMaximumSize() {
        return getPreferredSize();
    }

    private Image createImage(String filename) {
        String path = "resources/icons/" + filename;
        URL url = getClass().getResource(path);
        return Toolkit.getDefaultToolkit().createImage(url);
    }

    public void addTaskBarButton(ImageButton button, String appId, String str) {

        button.setLabelDisplay(false);

        Date today;
        String dateOutput;
        SimpleDateFormat formatter;
        String pattern = "EEE, MMM d, ''yy h:mm a";
        formatter = new SimpleDateFormat(pattern);
        today = new Date();
        dateOutput = formatter.format(today);

        Vector appVector = new Vector();
        appVector.add(0, button);
        appVector.add(1, dateOutput);

        // associate appId -> button
        runningAppsHash.put(appId, appVector);

        // use the size of the running apps vector as the index
        // used to store the new app within the vector
        int index = runningAppsVector.size();

        // add the appId to the vector... you can then
        // use the appId as the key into the runningAppsHash
        // to get its associated button.
        runningAppsVector.add(index, (Object) appId);

        refresh();
    }

    public void removeAppEntry(String appId) {
        runningAppsHash.remove(appId);
        runningAppsVector.remove(appId);
    }

    public void selectItemUp() {
        if (selectedIndex > 0) {
            TaskBarSlotPanel task = (TaskBarSlotPanel) taskBarPanel.
                getComponent(
                    selectedIndex);
            task.setSelected(false);
            selectedIndex--;
            task = (TaskBarSlotPanel) taskBarPanel.getComponent(selectedIndex);
            task.setSelected(true);

            int visible = scrollPane.getVAdjustable().getVisibleAmount();
            int value = scrollPane.getVAdjustable().getValue();
            int max = scrollPane.getVAdjustable().getMaximum();

            Point point = task.getLocation();

            if (point.y < value) {
                scrollPane.getVAdjustable().setValue(value - 80);
            }
        }
    }

    public void selectItemDown() {
        if (selectedIndex < (numApps - 1)) {
            TaskBarSlotPanel task = (TaskBarSlotPanel) taskBarPanel.
                getComponent(
                    selectedIndex);
            task.setSelected(false);
            selectedIndex++;
            task = (TaskBarSlotPanel) taskBarPanel.getComponent(selectedIndex);
            task.setSelected(true);

            int visible = scrollPane.getVAdjustable().getVisibleAmount();
            int value = scrollPane.getVAdjustable().getValue();
            int max = scrollPane.getVAdjustable().getMaximum();

            Point point = task.getLocation();

            if (point.y > visible) {
                scrollPane.getVAdjustable().setValue(value + 80);
            }
        }
    }

    /*
    public void keyPressed(KeyEvent e) {
        System.out.println("keyPressed: " + e.getKeyText(e.getKeyCode()));
    };

    public void keyTyped(KeyEvent e) {
        System.out.println("keyTyped: " + e.getKeyText(e.getKeyCode()));
    };

    public void keyReleased(KeyEvent e) {
        System.out.println("keyReleased: " + e.getKeyText(e.getKeyCode()));
        System.out.println(e);
        int keyCode = e.getKeyCode();
        switch (keyCode) {
            case 37: * left * {
                break;
            }
            case 38: * up * {
                selectItemUp();
                break;
            }
            case 39: * right * {
                break;
            }
            case 40: * down * {
                selectItemDown();
                break;
            }
        }
    };
    */

    class TaskbarAppSwitchActionListener
        implements ActionListener {
        Image icon;
        String appId;

        protected TaskbarAppSwitchActionListener() {
        }

        public void actionPerformed(ActionEvent e) {
            if (runningAppsVector.isEmpty()) {
                return;
            }
            // remove the current screen
            presentationMode.removeCurrentScreen();
            presentationMode.setFrameMinimized();
            String appId = (String) runningAppsVector.get(
                selectedIndex);
            controller.activate(appId);
        }
    }

    class TaskbarEndTaskActionListener
        implements ActionListener {

        protected TaskbarEndTaskActionListener() {
        }

        public void actionPerformed(ActionEvent e) {
            if (runningAppsVector.isEmpty()) {
                return;
            }
            String appId = (String) runningAppsVector.get(
                selectedIndex);
            removeAppEntry(appId);
            // remove the taskbar screen
            presentationMode.removeCurrentScreen();
            presentationMode.setFrameMinimized();
            controller.killTask(appId);
            controller.reactivateTopTask();
        }
    }

    class CancelActionListener
        implements ActionListener {

        protected CancelActionListener() {
        }

        public void actionPerformed(ActionEvent e) {
            presentationMode.removeCurrentScreen();
            presentationMode.setFrameMinimized();
            controller.reactivateTopTask();
        }
    }

    class TaskBarSlotPanel
        extends Panel implements FocusListener, MouseListener {
        public ImageButton button = null;
        public String appNameLabel = null;

        public String appDateLabel = null;
        private boolean selected = false;
        private final Border selectedBorder =
            new TaskSlotBorder();
        private boolean isSelected = false;
        private int index = 0;

        public TaskBarSlotPanel() {

        }

        public TaskBarSlotPanel(int index) {
            setLayout(new BorderLayout());
            this.index = index;
            addFocusListener(this);
            addMouseListener(this);
        }

        public void clearSlot() {
            if (this.button != null) {
                remove(this.button);
            }
            appNameLabel = null;
            appDateLabel = null;
            validate();
            repaint();
        }

        public void setButton(ImageButton button) {
            if (this.button != null) {
                remove(this.button);
            }
            this.button = button;
            button.setPaintBorders(false);
            add(button, BorderLayout.WEST);
        }

        public ImageButton getButton() {
            return this.button;
        }

        public void setAppNameText(String str) {
            appNameLabel = str;
        }

        public String getAppNameText() {
            return this.appNameLabel;
        }

        public void setDateText(String str) {
            appDateLabel = str;
        }

        public String getDateText() {
            return this.appDateLabel;
        }

        private static final int MAX_TITLE_LENGTH = 16;

        public void paint(Graphics g) {
            g.setFont(presentationMode.currentUserFont);
            super.paint(g);

            if (appNameLabel == null) {
                return;
            }

            String str = null;

            if (appNameLabel.length() > MAX_TITLE_LENGTH) {
                str = appNameLabel.substring(0, MAX_TITLE_LENGTH - 3);
                str += "...";
            }
            else if (appNameLabel.length() < MAX_TITLE_LENGTH) {
                str = appNameLabel;
                int count = MAX_TITLE_LENGTH - appNameLabel.length();
                for (int i = 0; i < count; i++) {
                    str += " ";
                }
            }
            else {
                str = appNameLabel;
            }

            if (str != null) {
                g.drawString(str, 100, 30);
            }
            if (str != null) {
                g.drawString(appDateLabel, 240, 30);
            }
            if (isSelected()) {
                selectedBorder.paint(g, getBackground(), 0, 0, this.getWidth(),
                                     this.getHeight());
            }
        }

        public Dimension getPreferredSize() {
            return new Dimension(460, 50);
        }

        public Dimension getMinimumSize() {
            return getPreferredSize();
        }

        public Dimension getMaximumSize() {
            return getPreferredSize();
        }

        public void setSelected(boolean val) {
            isSelected = val;
            if (val) {
                selectedIndex = index;
            }
            repaint();
        }

        public boolean isSelected() {
            return isSelected;
        }

        public void focusGained(FocusEvent e) {
            if (isSelected()) {
                return;
            }
            else {
                setSelectedSlot(selectedIndex, false);
                setSelectedSlot(this.index, true);
            }
        }

        public void focusLost(FocusEvent e) {}

        public void mouseClicked(MouseEvent e) {}

        public void mouseEntered(MouseEvent e) {}

        public void mouseExited(MouseEvent e) {}

        public void mousePressed(MouseEvent e) {
            requestFocusInWindow();
        }

        public void mouseReleased(MouseEvent e) {}

        /**
         * DefaultImageButtonBorder - a Border, subclassed to set the default border values.
         */
        class TaskSlotBorder
            extends Border {

            public TaskSlotBorder() {
                setBorderThickness(2);
                setType(LINE);
            }
        }
    }

}

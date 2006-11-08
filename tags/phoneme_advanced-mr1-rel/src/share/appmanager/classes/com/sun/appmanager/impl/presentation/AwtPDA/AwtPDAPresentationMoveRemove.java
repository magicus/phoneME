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
import java.awt.event.*;
import java.util.*;
import java.net.URL;
import java.io.File;
import com.sun.appmanager.apprepository.AppModule;
import com.sun.appmanager.apprepository.AppRepository;
import com.sun.appmanager.AppManager;

public class AwtPDAPresentationMoveRemove
    extends Panel {

    private final String pathSeparator = System.getProperty("path.separator");

    private Panel appsPanel = null;
    private Color slotsColor = new Color(173, 175, 233);
    private AwtPDAPresentationMode presentationMode = null;
    private AppSlotBorder selectedBorder = new AppSlotBorder();
    private int selectedIndex = 0;
    private int numApps = 0;
    private int minSlots = 0;
    private ScrollPane scrollPane = null;

//  Title bar header
    ImagePanel titleBarHeader = null;

    ImageButton removeAppButton = null;
    ImageButton cancelButton = null;

    public AwtPDAPresentationMoveRemove(AwtPDAPresentationMode
                                        presentationMode) {

        this.presentationMode = presentationMode;
    }

    public void initialize() {
        setLayout(new BorderLayout());

        String titleBarImageFile = "resources/background/"
            + presentationMode.getString("AwtPDAPresentationMode.titleBarImage");
        titleBarHeader = new ImagePanel(
            titleBarImageFile);
        titleBarHeader.setFont(presentationMode.currentUserFont);
        titleBarHeader.setLabel(presentationMode.getString(
            "AwtPDAPresentationMode.moveRemoveHeader.label"), -1, 17);
        add(titleBarHeader, BorderLayout.NORTH);

        appsPanel = new Panel(new SlotLayout());

        scrollPane = new ScrollPane(ScrollPane.SCROLLBARS_AS_NEEDED);
        scrollPane.add(appsPanel);

        add(scrollPane, BorderLayout.CENTER);

        String bottomBarImageFile = "resources/background/"
            +
            presentationMode.getString("AwtPDAPresentationMode.bottomBarImage");
        ImagePanel bottomPanel = new ImagePanel(bottomBarImageFile);
        bottomPanel.setLayout(new GridLayout(1, 2));

        removeAppButton = new ImageButton(createImage(presentationMode.
            getString(
                "AwtPDAPresentationMode.removeApp")));
        removeAppButton.addActionListener(new RemoveAppAction());
        removeAppButton.setLabel(presentationMode.getString(
            "AwtPDAPresentationMode.removeAppLabel"));

        cancelButton = new ImageButton(createIconImage(presentationMode.
            getString(
                "AwtPDAPresentationMode.cancelButton")));
        cancelButton.addActionListener(new CancelActionListener());
        cancelButton.setLabel(presentationMode.getString(
            "AwtPDAPresentationMode.cancelButton.label"));

        bottomPanel.add(removeAppButton);
        bottomPanel.add(cancelButton);
        add(bottomPanel, BorderLayout.SOUTH);
    }

    public void refresh() {
        removeAppButton.setFont(presentationMode.currentUserFont);
        cancelButton.setFont(presentationMode.currentUserFont);

        Vector appModulesVector = presentationMode.getApplicationAppModules();

        Hashtable bundlesHash = new Hashtable();
        AppModule module;
        int num = 0;

// Since bundles can hold more than one app, we need to be careful.
// When listing apps that can be removed, we are really talking about
// removing bundles since all apps will live in the same jarfile.
// So, by definition you can't remove one without all.
        if (!appModulesVector.isEmpty()) {
            for (Enumeration e = appModulesVector.elements(); e.hasMoreElements();
                 num++) {
                module = (AppModule) e.nextElement();

                String bundle = module.getBundle();
                Vector v = null;

                if (bundlesHash.containsKey(bundle)) {
                    v = (Vector) bundlesHash.get(bundle);
                }
                else {
                    v = new Vector();
                }
                v.add(module);
                bundlesHash.put(bundle, v);
            }
        }

        AppSlotPanel slot = null;

        int panelHeight = getPreferredSize().height;
        int slotHeight = new AppSlotPanel().getPreferredSize().height;

        minSlots = panelHeight / slotHeight - 1;
        if (panelHeight % slotHeight != 0) {
            minSlots++;
        }
        appsPanel.removeAll();

        selectedIndex = 0;

        num = 0;

        if (!appModulesVector.isEmpty()) {
            for (Enumeration e = bundlesHash.keys(); e.hasMoreElements();
                 num++) {
                String bundle = (String) e.nextElement();

                Vector v = (Vector) bundlesHash.get(bundle);
                Object array[] = v.toArray();
                AppModule moduleArray[] = new AppModule[v.size()];
                for (int i = 0; i < array.length; i++) {
                    moduleArray[i] = (AppModule) array[i];
                }

                slot = new AppSlotPanel(bundle, moduleArray, num);
                if (num % 2 == 0) {
                    slot.setBackground(Color.white);
                }
                else {
                    slot.setBackground(slotsColor);
                }

                if (num == 0) {
                    slot.setSelected(true);
                }

                appsPanel.add(slot, num);
            }
            numApps = num;
        }

// Add empty slots for a better look if there isn't enough apps
// to fill the entire panel.
        if (num < minSlots) {
            for (int i = num; i < minSlots; i++) {
                slot = new AppSlotPanel();
                if (i % 2 == 0) {
                    slot.setBackground(Color.white);
                }
                else {
                    slot.setBackground(slotsColor);
                }
                appsPanel.add(slot, i);
            }
        }

        appsPanel.validate();
    }

    public void setSelectedSlot(int index, boolean val) {

        AppSlotPanel app = (AppSlotPanel) appsPanel.getComponent(
            index);
        app.setSelected(val);
    }

    public Dimension getPreferredSize() {
        return new Dimension(480, 430);
    }

    public Dimension getMinimumSize() {
        return getPreferredSize();
    }

    public Dimension getMaximumSize() {
        return getPreferredSize();
    }

    private Image createImage(String filename) {
        String path = "resources/moveremove/" + filename;
        URL url = getClass().getResource(path);
        return Toolkit.getDefaultToolkit().createImage(url);
    }

    private Image createIconImage(String filename) {
        String path = "resources/icons/" + filename;
        URL url = getClass().getResource(path);
        return Toolkit.getDefaultToolkit().createImage(url);
    }

    private long getAppSize(AppModule module) {
        if (module.isSublist()) {
            return 0;
        }
        String modulePath = module.getPath();

        // Get the path to the app bundle's jar file, which is assumed
        // to be the first entry in the classpath.
        String jarPath = null;
        int index = modulePath.indexOf(pathSeparator);
        if (index == -1) {
            jarPath = modulePath;
        }
        else {
            jarPath = modulePath.substring(0, index);
        }

        File jarFile = new File(jarPath);
        if (jarFile.exists()) {
            return jarFile.length();
        }
        else {
            return 0;
        }
    }

    private boolean deleteAppBundle(AppModule module[]) {
        AppRepository repository = AppManager.getAppRepository();
        boolean result = repository.removeApplication(module);
        return result;
    }

    public void selectItemUp() {
        if (selectedIndex > 0) {
            AppSlotPanel app = (AppSlotPanel) appsPanel.getComponent(
                selectedIndex);
            app.setSelected(false);
            selectedIndex--;
            app = (AppSlotPanel) appsPanel.getComponent(selectedIndex);
            app.setSelected(true);

            int visible = scrollPane.getVAdjustable().getVisibleAmount();
            int value = scrollPane.getVAdjustable().getValue();
            int max = scrollPane.getVAdjustable().getMaximum();

            Point point = app.getLocation();

            if (point.y < value) {
                scrollPane.getVAdjustable().setValue(value - 80);
            }
        }

    }

    public void selectItemDown() {
        if (selectedIndex < (numApps - 1)) {
            AppSlotPanel app = (AppSlotPanel) appsPanel.getComponent(
                selectedIndex);
            app.setSelected(false);
            selectedIndex++;
            app = (AppSlotPanel) appsPanel.getComponent(selectedIndex);
            app.setSelected(true);

            int visible = scrollPane.getVAdjustable().getVisibleAmount();
            int value = scrollPane.getVAdjustable().getValue();
            int max = scrollPane.getVAdjustable().getMaximum();

            Point point = app.getLocation();

            if (point.y > visible) {
                scrollPane.getVAdjustable().setValue(value + 80);
            }

        }
    }

    class AppSlotPanel
        extends Panel implements MouseListener, FocusListener {
        private String bundle = null;
        AppModule module[] = null;
        private boolean isSelected = false;
        private int index = 0;
        private String title = null;

        public AppSlotPanel() {
        }

        public AppSlotPanel(String bundle, AppModule module[], int index) {
            this.bundle = bundle;
            this.module = module;
            if (module.length == 1 && module[0] != null) {
                this.title = module[0].getTitle();
            }
            else {
                String str = "";
                for (int i = 0; i < module.length; i++) {
                    str = str + module[i].getTitle();
                    if (i != (module.length - 1)) {
                        str = str + '/';
                    }
                }
                this.title = str;
            }

            this.index = index;
            addMouseListener(this);
            addFocusListener(this);
        }

        public void setIndex(int i) {
            index = i;
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

        public void paint(Graphics g) {
            g.setFont(presentationMode.currentUserFont);
            if (module != null && module[0] != null) {
                if (title != null) {
                    g.drawString(title, 15, 30);
                }
                g.drawString(String.valueOf(getAppSize(module[0])), 360, 30);
            }
            if (isSelected()) {
                selectedBorder.paint(g, getBackground(), 0, 0, this.getWidth(),
                                     this.getHeight());
            }
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

        public AppModule[] getAppModule() {
            return module;
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

    }

    class AppSlotBorder
        extends Border {

        public AppSlotBorder() {
            setBorderThickness(2);
            setType(LINE);
        }
    }

    class RemoveAppAction
        implements ActionListener {

        protected RemoveAppAction() {
        }

        public void actionPerformed(ActionEvent e) {
            AppSlotPanel appSlot = (AppSlotPanel) appsPanel.getComponent(
                selectedIndex);

            AppModule module[] = appSlot.getAppModule();
            if (module == null) {
                return;
            }

            // Remove the apps which are part of the bundle from existence
            // from the appmanager.  A bundle can contain multiple apps,
            // so we need to do this for each
            for (int i = 0; i < module.length; i++) {
                presentationMode.removeApp(module[i]);
            }

            // Now, lets remove the whole bundle itself, meaning that we need
            // to remove the jar file, parent directory, icons, and app
            // descriptors belonging to this bundle.
            deleteAppBundle(module);

            appsPanel.remove(appSlot);
            appsPanel.validate();
            if (numApps > 0) {
                numApps--;
            }

            // If this is the last app in the list, decrement the
            // selectedIndex
            if ( (selectedIndex + 1) >= numApps) {
                if (selectedIndex > 0) {
                    selectedIndex--;
                }
            }

            int i = 0;
            // Update the index value of all remaining slots
            for (i = selectedIndex; i < appsPanel.getComponentCount(); i++) {
                appSlot = (AppSlotPanel) appsPanel.getComponent(i);
                appSlot.setIndex(i);
                if (i % 2 == 0) {
                    appSlot.setBackground(Color.white);
                }
                else {
                    appSlot.setBackground(slotsColor);
                }
            }

            // Add empty slots for a better look if there isn't enough apps
// to fill the entire panel.
            if (i < minSlots) {
                for (int j = i; j < minSlots; j++) {
                    AppSlotPanel slot = new AppSlotPanel();
                    if (j % 2 == 0) {
                        slot.setBackground(Color.white);
                    }
                    else {
                        slot.setBackground(slotsColor);
                    }
                    appsPanel.add(slot, -1);
                }
            }

            // Set the new current selected index
            appSlot = (AppSlotPanel) appsPanel.getComponent(selectedIndex);
            appSlot.setSelected(true);

            appsPanel.validate();
            appsPanel.repaint();
        }
    }

    class CancelActionListener
        implements ActionListener {

        protected CancelActionListener() {
        }

        public void actionPerformed(ActionEvent e) {
            presentationMode.removeCurrentScreen();
        }
    }

}

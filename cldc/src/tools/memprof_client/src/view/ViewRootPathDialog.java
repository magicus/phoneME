/*
 *   
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


package view;

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import com.sun.cldchi.tools.memoryprofiler.data.MPDataProvider;


public class ViewRootPathDialog extends JDialog implements ActionListener {
  private MPDataProvider _provider;
  public static void showDialog(Component frameComp, Object[] objects, String title, MPDataProvider provider) {
        Frame frame = JOptionPane.getFrameForComponent(frameComp);
        ViewRootPathDialog dialog = new ViewRootPathDialog(frame,
                                title, true, objects, provider);
        dialog.setVisible(true);
  }
  
  public ViewRootPathDialog(Frame frame, String title, boolean param, Object[] objects, MPDataProvider provider) {
    super(frame, title, param);
    _provider = provider;
    JButton closeButton = new JButton("Close");
    closeButton.addActionListener(this);
    ViewObjectsPanel vo_panel = new ViewObjectsPanel(provider);
    vo_panel.initUI(false);
    vo_panel.setObjects(objects);
    getContentPane().add(vo_panel, BorderLayout.CENTER);
    getContentPane().add(closeButton, BorderLayout.PAGE_END);
    pack();
    getRootPane().setDefaultButton(closeButton);
  }

  public Dimension getPrefferedSize() {
    return new Dimension(500, 300);
  } 

  public void actionPerformed(ActionEvent e) {
    setVisible(false);
  }
}

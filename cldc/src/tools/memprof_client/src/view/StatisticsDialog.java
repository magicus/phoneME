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
import javax.swing.table.*;
import java.awt.*;
import java.awt.event.*;
import com.sun.cldchi.tools.memoryprofiler.data.*;

public class StatisticsDialog extends JDialog implements ActionListener {
  public static void showDialog(Component frameComp, Object[] objects) {
        Frame frame = JOptionPane.getFrameForComponent(frameComp);
        StatisticsDialog dialog = new StatisticsDialog(frame,
                                "Heap Statistics", true, objects);
        dialog.setVisible(true);
  }
  
  public StatisticsDialog(Frame frame, String title, boolean param, Object[] objects) {
    super(frame, title, param);
    JButton closeButton = new JButton("Close");
    JScrollPane pane = new JScrollPane();
    closeButton.addActionListener(this);
    JTable tbl = new JTable(new StatTableModel(objects));    
    pane.getViewport().setView(tbl);
    getContentPane().add(pane, BorderLayout.CENTER);
    getContentPane().add(closeButton, BorderLayout.PAGE_END);
    pack();
    getRootPane().setDefaultButton(closeButton);
  }

  public Dimension getPrefferedSize() {
    return new Dimension(800, 400);
  } 

  public void actionPerformed(ActionEvent e) {
    setVisible(false);
  }
}

class StatTableModel extends AbstractTableModel {
  private Object[] _data;
  public StatTableModel(Object[] data) {
    _data = data;
  }

  public int getColumnCount() {return 7;}
  private static String columnNames[] = {
    "Class name", "Object number", "Size", "Av. Size", "heap %", 
    "live %", "old gen. %"
  };
  public Object getValueAt(int row, int col) {
    if (_data == null) return null;
    if (row >= _data.length) return null;
    ClassStatistics obj = (ClassStatistics)_data[row];
    if (col == 0) {
       return obj._class_name;
     } else if (col == 1) {
       return new Integer(obj.getCount());
     } else if (col == 2) {
       return new Integer(obj.getTotalSize());
     } else if (col == 3) {
       return new Integer(obj.getAverageSize());
     } else if (col == 4) {
       return displayPerctentage(obj.getHeapPercentage());
     } else if (col == 5) {
       return displayPerctentage(obj.getLivePercentage());
     } else if (col == 6) {
       return displayPerctentage(obj.getOldGenPercentage());
     } else {
       return null;
     }
   }
   public String displayPerctentage(int p) {
     return "" + (p / 100) + "." + (p % 100) + "%";
   }
   public int getRowCount() {
     if (_data == null) {
       return 0;
     } else {
       return _data.length;
     }
   }

   public String getColumnName(int i) {
     return columnNames[i];
   }
}

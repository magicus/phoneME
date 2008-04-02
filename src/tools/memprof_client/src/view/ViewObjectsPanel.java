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
import java.net.*;
import java.awt.*;
import java.awt.event.*;
import com.sun.cldchi.tools.memoryprofiler.data.JavaObject;
import com.sun.cldchi.tools.memoryprofiler.data.MPDataProvider;
import javax.swing.table.*;
import javax.swing.event.*;

public class ViewObjectsPanel extends JPanel {
  private JLabel address_label = new JLabel("address");
  private JTextField address_field = new JTextField();
  private JLabel type_label = new JLabel("type");
  private JTextField type_field = new JTextField();
  private JTable references = new ObjectListTable(new ObjectListTableModel());
  private JTable referees = new ObjectListTable(new ObjectListTableModel());
  private JavaObject _obj = null;
  private MPDataProvider _provider;
  private JButton _root_path;
  private JList _object_list;
  private JButton _stack_location;
  private Object[] _objects;

  public ViewObjectsPanel(MPDataProvider provider) {
    _provider = provider;
  }

  public void initUI(boolean add_show_root_path_button) {
    setLayout(new GridBagLayout());
    JPanel top_panel = new JPanel();
    top_panel.add(address_label);
    top_panel.add(address_field);
    address_field.setEditable(false);
    type_field.setEditable(false);
    address_field.setPreferredSize(new Dimension(80, 20));
    type_field.setPreferredSize(new Dimension(120, 20));

    top_panel.add(type_label);
    top_panel.add(type_field);
    if (add_show_root_path_button) {
      _root_path = new JButton("Show path from the root") {
        public Dimension getPreferredSize() {
          return new Dimension(160, 20);
        }
      };
      _root_path.addActionListener(new ShowRootPathListener());

      _root_path.setFont(_root_path.getFont().deriveFont(9.0f));
      _root_path.setEnabled(false);
      top_panel.add(_root_path);
    } else { //this is show path from the root object
        _stack_location = new JButton("Show stack trace") {
          public Dimension getPreferredSize() {
            return new Dimension(160, 20);
          }
        };
        _stack_location.addActionListener(new ShowStackTraceListener());
  
        _stack_location.setFont(_stack_location.getFont().deriveFont(9.0f));
        _stack_location.setEnabled(false);
        top_panel.add(_stack_location);
  
    }
    add(top_panel, new GridBagConstraints(0, 0, 1, 1, 1, 1,
           GridBagConstraints.NORTHWEST, GridBagConstraints.NONE, new Insets(2, 2, 2, 2), 0, 0));
    JPanel bottom_panel = new JPanel();
    JScrollPane pane = new JScrollPane() {
      public Dimension getPreferredSize() {return new Dimension(100, 220);}
    };
    _object_list = new JList();
    _object_list.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
    _object_list.addListSelectionListener(new ObjectListSelectionListener());
    pane.getViewport().setView(_object_list);

    bottom_panel.setLayout(new GridBagLayout());
    bottom_panel.add(pane, new GridBagConstraints(0, 0, 1, 2, 1, 1,
           GridBagConstraints.NORTHWEST, GridBagConstraints.NONE, new Insets(2, 2, 2, 2), 0, 0));
    bottom_panel.add(new JLabel("Referees"), new GridBagConstraints(1, 0, 1, 1, 1, 1,
           GridBagConstraints.NORTHWEST, GridBagConstraints.NONE, new Insets(2, 2, 2, 2), 0, 0));
    bottom_panel.add(new JLabel("References"), new GridBagConstraints(2, 0, 1, 1, 1, 1,
           GridBagConstraints.NORTHWEST, GridBagConstraints.NONE, new Insets(2, 2, 2, 2), 0, 0));
    pane = new JScrollPane() {
      public Dimension getPreferredSize() {
        return new Dimension(220, 200);
      }
    };
    pane.getViewport().setView(referees);
    bottom_panel.add(pane, new GridBagConstraints(1, 1, 1, 1, 1, 21,
             GridBagConstraints.NORTHWEST, GridBagConstraints.NONE, new Insets(2, 2, 2, 2), 0, 0));
    pane = new JScrollPane() {
        public Dimension getPreferredSize() {
          return new Dimension(220, 200);
        }
      };
    pane.getViewport().setView(references);
    bottom_panel.add(pane, new GridBagConstraints(2, 1, 1, 1, 1, 21,
           GridBagConstraints.NORTHWEST, GridBagConstraints.NONE, new Insets(2, 2, 2, 2), 0, 0));
  
    add(bottom_panel, new GridBagConstraints(0, 1, 1, 1, 1, 20,
           GridBagConstraints.NORTHWEST, GridBagConstraints.NONE, new Insets(2, 2, 2, 2), 0, 0));

  }


  private void setObject(JavaObject obj) {
    _obj = obj;
    ObjectListTableModel refs = (ObjectListTableModel)references.getModel();
    ObjectListTableModel refes = (ObjectListTableModel)referees.getModel();
    if (obj != null) {
      address_field.setText(obj.toString());
      type_field.setText(_provider.getObjectTypeName(obj));
      refs.setData(obj.get_references());
      refes.setData(obj.get_referees());
      if (_root_path != null) {
        _root_path.setEnabled(obj.getRootDistance() != -1);
      }
      if (_stack_location != null) {
        _stack_location.setEnabled(obj.object_type == MPDataProvider.STACK_OBJECT);
      }
    } else {
      address_field.setText("");
      type_field.setText("");
      refs.setData(null);
      refes.setData(null);
      if (_root_path != null) {
        _root_path.setEnabled(false);
      }
    }
    refs.fireTableDataChanged();
    refes.fireTableDataChanged();
  }

  public void setObjects(Object[] objects) {
    _object_list.setListData(objects);
    _objects = objects;
    repaint();
  }

  class ObjectListTableModel extends AbstractTableModel {
      private Object[] _data;
      public void setData(Object[] data) {
          _data = data;
      }
      public int getColumnCount() {return 2;}
      public Object getValueAt(int row, int col) {
          if (_data == null) return "y";
          if (row >= _data.length) return "z";
          if (col == 0) {
              return _data[row].toString();
          } else if (col == 1) {
              return _provider.getObjectTypeName((JavaObject)_data[row]);
          } else {
              throw new RuntimeException();
          }
      }
      public int getRowCount() {
        if (_data == null) {
          return 0;
        } else {
          return _data.length;
        }
      }

      public String getColumnName(int i) {
        if (i == 0) {
            return "address";
        } else if (i == 1) {
            return "type";
        } else {
            throw new RuntimeException("Should not reach here!");
        }
      }
  }

  class ObjectListTable extends JTable {
    public ObjectListTable(TableModel model) {super(model);}

    public String getToolTipText(MouseEvent e) {
      int row = rowAtPoint(e.getPoint());
      int col = columnAtPoint(e.getPoint());
      return (String)getModel().getValueAt(row, col);
    }
  }
  
  class ShowRootPathListener implements ActionListener {
    public void actionPerformed(ActionEvent e) {
      Object[] objs = _provider.pathFromTheRoot(_obj); 
      ViewRootPathDialog.showDialog(ViewObjectsPanel.this, objs, "Path from the Root", _provider);
    }
  } 

  class ObjectListSelectionListener implements ListSelectionListener {
    public void valueChanged(ListSelectionEvent e) {
      JavaObject item = (JavaObject)_object_list.getSelectedValue();
      setObject(item);
    }
  }

  class ShowStackTraceListener implements ActionListener {
    public void actionPerformed(ActionEvent e) {
      for (int i = 0; i < _objects.length-1; i++) {
        if (_objects[i] == _obj) {
          int ptr = ((JavaObject)_objects[i+1]).address;
          String stackTrace = null;
          try {
            stackTrace = _provider.getStackTrace((JavaObject)_objects[i], ptr);
          } catch (SocketException ex) {
            //IMPL_NOTE: need to handle it
          }
          JOptionPane.showMessageDialog(null, stackTrace, "Object location StackTrace", JOptionPane.PLAIN_MESSAGE);         
        }
      }
    }
  }
}

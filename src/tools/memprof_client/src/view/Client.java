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

import com.sun.cldchi.tools.memoryprofiler.data.*;
import com.sun.cldchi.tools.memoryprofiler.jdwp.VMConnection;
import com.sun.cldchi.tools.memoryprofiler.jdwp.VMConnectionFactory;
import java.io.*;
import java.net.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.datatransfer.*;
import javax.swing.*;
import javax.swing.tree.*;
import javax.swing.table.*;
import javax.swing.event.*;
import java.util.Arrays;
import java.nio.channels.*;
import java.nio.*;
import javax.swing.border.LineBorder;
import javax.swing.border.TitledBorder;

public class Client {
  private MPDataProvider _data_provider;
  private JList _classes_list;
  private JList _object_list;
  private JFrame _frame;
  private ViewMemoryPanel _memory_access_panel;
  private ViewObjectsPanel _view_objects_panel;
  private JButton _vm_controller;
  private JButton _connection_controller;
  private JButton _statistics_btn;
  private JLabel _statusLabel;
  private boolean isConnected = false;
  private static String hostName = "localhost";
  private static int    port = 5000;
  private boolean vm_run = false;


  public static void main(String args[]) {
    //Parse arguments
    try{
      if(parseArgs(args) < 0){
        System.exit(1);
      }
    }catch(CommandLineException e){
      System.err.println(e);
      System.exit(1);
    }
    (new Client()).initUI();
  }


/************Connectivity SECTION****************************/
  private void initConnection() throws java.net.ConnectException, java.net.SocketException {    
    _data_provider.connect(hostName, port);
  }
/************UI SECTION****************************/
  private void initUI() {
    _data_provider = MPDataProviderFactory.getMPDataProvider(VMConnectionFactory.getVMConnectionImpl());
    _frame = new JFrame("Java Heap Memory Observe Tool");
    _frame.getAccessibleContext().setAccessibleDescription("Java Heap Memory Observe Tool");
    _frame.getAccessibleContext().setAccessibleName("Java Heap Memory Observe Tool");
    _frame.pack();
    _frame.setLocation(0, 0);
    _frame.setSize(1000, 700);
    _frame.setVisible(true);
    _frame.setResizable(false);
    _frame.getContentPane().setLayout(new GridBagLayout());
    _frame.addWindowListener(new WindowAdapter() {
      public void windowClosing(WindowEvent e) {
        if (isConnected && !vm_run) {
          /* resume VM before exiting, otherwise it will be left unusable */
          try {
            _data_provider.resumeVM();
            /* give KDP proxy time to resend the packet before socket is closed */
            try { Thread.sleep(200); } catch (Exception ex) {}
            System.out.println("Exitting, VM execution resumed.");
          } catch (Exception ex) {
            System.err.println("Exitting, VM execution could not be resumed.");
          }
        }
        _data_provider.closeConnections();
        System.exit(0);
      }
    });

    /* memory access panel*/
    _classes_list = new JList();
    _classes_list.setToolTipText("Classes");
    _classes_list.getAccessibleContext().setAccessibleName("Classes");
    _memory_access_panel = new ViewMemoryPanel(_data_provider);
    _memory_access_panel.setSize(1000, 300);
    _memory_access_panel.update();
    
    _frame.getContentPane().add(_memory_access_panel, new GridBagConstraints(0, 0, 3, 1, 1, 1,
                  GridBagConstraints.NORTHWEST, GridBagConstraints.HORIZONTAL, new Insets(2, 2, 2, 2), 0, 0));

    JPanel lists_panel = new JPanel();
    JScrollPane pane = new JScrollPane() {
      public Dimension getPreferredSize() {return new Dimension(350, 250);}
    };
    _classes_list.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
    _classes_list.addListSelectionListener(new ClassesListSelectionListener());
    pane.getViewport().setView(_classes_list);
    lists_panel.add(pane);
    _frame.getContentPane().add(lists_panel, new GridBagConstraints(0, 1, 2, 1, 1, 1, 
                  GridBagConstraints.NORTHWEST, GridBagConstraints.NONE, new Insets(2, 2, 2, 2), 0, 0));

    _view_objects_panel = new ViewObjectsPanel(_data_provider);
    _view_objects_panel.initUI(true);
    _frame.getContentPane().add(_view_objects_panel, new GridBagConstraints(2, 1, 1, 1, 1, 1,
                  GridBagConstraints.NORTHWEST, GridBagConstraints.BOTH, new Insets(2, 2, 2, 2), 0, 0));


    JPanel button_panel = new JPanel();
    _statistics_btn = new JButton("Statistics");
    _statistics_btn.setMnemonic(KeyEvent.VK_S);
    _statistics_btn.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        StatisticsDialog.showDialog(_frame, _data_provider.calculateStatistics());
      }
    });
    button_panel.add(_statistics_btn);
    _vm_controller = new JButton("Resume");
    _vm_controller.setToolTipText("Resume connection");
    _vm_controller.setMnemonic(KeyEvent.VK_R);;
    _vm_controller.addActionListener(new VMActionListener());
    button_panel.add(_vm_controller);
    _connection_controller = new JButton("Connect");
    _connection_controller.setMnemonic(KeyEvent.VK_C);
    _connection_controller.setToolTipText("Connect to emulator");
    _connection_controller.addActionListener(new ConnectionActionListener());
    _connection_controller.setDefaultCapable(true);
    _connection_controller.setSelected(true);
    button_panel.add(_connection_controller);
    _statusLabel = new JLabel("Disconnected");
    TitledBorder tb = BorderFactory.createTitledBorder("Memory Observer tool status");
    _statusLabel.setBorder(tb);
    JPanel tmpPanel = new JPanel(new BorderLayout());
    tmpPanel.add(BorderLayout.NORTH, button_panel);
    tmpPanel.add(BorderLayout.SOUTH, _statusLabel);

    _frame.getContentPane().add(tmpPanel, new GridBagConstraints(2, 2, 1, 1, 1, 1,
                  GridBagConstraints.SOUTHEAST, GridBagConstraints.NONE, new Insets(2, 2, 2, 2), 0, 0));

    setDisconnected();
    _frame.getRootPane().setDefaultButton(_connection_controller);
    _frame.invalidate();
    _frame.validate();
    _frame.repaint();

  }

  private void update() {
    try {
      _classes_list.setListData(_data_provider.getClassList());
      _memory_access_panel.update();
    } catch (SocketException e) {
      setDisconnected();
    }
  }
/******HANDLERS**************************************/
  private void onClassesSelectionChanged() {
    JavaClass item = (JavaClass)_classes_list.getSelectedValue();
    if (item != null) {
      _memory_access_panel.set_selected_class_id(item);
      Object[] objects = _data_provider.getObjectsOfClass(item);
      _view_objects_panel.setObjects(objects);
    } else {
      _view_objects_panel.setObjects(new Object[0]);      
    }
    _frame.invalidate();
    _frame.validate();
    _frame.repaint();
  }

  private void onObjectSelectionChanged() {
      _frame.invalidate();
      _frame.validate();
      _frame.repaint();
  }
/******PARSE ARGS SECTION****************************/
  private static int parseArgs(String[] args) throws CommandLineException{
        
    //Parse command line parameteres
    boolean portDefined = false;
    boolean hostDefined = false;
    for(int i=0; i<args.length-1 ; i++){
            
      //-port
      if(args[i].equals("-port")) {
        if(portDefined) {
          throw new CommandLineException("Port already defined");
        }
                
        portDefined = true; 
        try{
          port = Integer.valueOf(args[++i]).intValue();
        }catch(NumberFormatException e){
          throw new CommandLineException("Invalid port number");
        }
                
      //-host
      }else if(args[i].equals("-host")){
        if(hostDefined){
          throw new CommandLineException("Host already defined");
        }
        hostDefined = true;
        hostName = args[++i];
                
      //Invalid switch
      }else {
        throw new CommandLineException("Unknown switch " + args[i]);
      }
    }
               
    return 0;
  }

  private static void usage() {
    System.err.println("Usage:\n" + 
      "java view.Client -host <hostname> -port <portname>");
  }   

  class ClassesListSelectionListener implements ListSelectionListener { 
    public void valueChanged(ListSelectionEvent e) {
      onClassesSelectionChanged();
    }
  }

  class ConnectionActionListener implements ActionListener {
    public void actionPerformed(ActionEvent event) {
      if (!isConnected) {
        try {
          initConnection();
          update();
          setConnected();
        } catch (java.net.ConnectException e) {
          JOptionPane.showMessageDialog(null, 
            "Could not connect to the VM! Check parameters! Host:" + hostName + " port:" + port, 
              "Connection Error", JOptionPane.ERROR_MESSAGE);
          System.out.println("");
        } catch (java.net.SocketException e) {
          JOptionPane.showMessageDialog(null, 
            "VM connection was broken during initial update!", 
              "Connection Error", JOptionPane.ERROR_MESSAGE);
        }
      } else { //isConnected
        _data_provider.closeConnections();
        setDisconnected();
      }  
    };
  }


  class VMActionListener implements ActionListener {
    public void actionPerformed(ActionEvent e) {
      try {
        if (!vm_run) {
          _statusLabel.setText("Resuming VM execution..");
          _data_provider.resumeVM();
          _statusLabel.setText("VM running");
        } else {
          _statusLabel.setText("Pausing VM execution..");
          _data_provider.pauseVM();
          _statusLabel.setText("VM paused");
        }      
        vm_run = !vm_run;
      } catch (SocketException e2) {
        vm_run = false;
        setDisconnected();
      }
      if (vm_run) {
        _vm_controller.setText("Pause");
      } else {
        _vm_controller.setText("Resume");
      }
      if (!vm_run) {
        update();
      }

    }
  }

  private void setDisconnected() {
    isConnected = false;
    _classes_list.setListData(new Object[0]);
    _memory_access_panel.update();
    _vm_controller.setEnabled(false);
    _statistics_btn.setEnabled(false);
    _connection_controller.setEnabled(true);
    _connection_controller.setText("Connect");
    _statusLabel.setText("Disconnected");
  }

  private void setConnected() {
    isConnected = true;
    _vm_controller.setEnabled(true);
    _statistics_btn.setEnabled(true);
    _connection_controller.setText("Connected!");
    _statusLabel.setText("Connected");
    //prevent the user from diconnecting, after which he can not connect again
    _connection_controller.setEnabled(false);
    _classes_list.setSelectedIndex(0);
  }
}

class CommandLineException extends Exception {
  public CommandLineException() {
    super();
  }
  public CommandLineException(String s){
    super(s);
  }
}


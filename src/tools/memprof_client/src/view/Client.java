/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

public class Client {
  private MPDataProvider _data_provider;
  private JList _classes_list;
  private JList _object_list;
  private JFrame _frame;
  private ViewMemoryPanel _memory_access_panel;
  private ViewObjectsPanel _view_objects_panel;
  private JButton _vm_controller;
  private static String hostName = "localhost";
  private static int    port = 5000;


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
    (new Client()).start();
  }

  private void start() {
   try {
     initConnection();
   } catch (java.net.ConnectException e) {
     System.out.println("Could not connect to VM! Check parameters!");
     System.exit(1);
   } catch (java.net.SocketException e) {
     e.printStackTrace();
     System.out.println("VM connection was broken during initial update!");
     System.exit(1);
   }
   initUI();
  }

/************Connectivity SECTION****************************/
  private void initConnection() throws java.net.ConnectException, java.net.SocketException {
    _data_provider = MPDataProviderFactory.getMPDataProvider(VMConnectionFactory.getVMConnectionImpl());
    _data_provider.connect(hostName, port);
  }
/************UI SECTION****************************/
  private void initUI() {
    _frame = new JFrame("VM Memory Observe Tool");
    _frame.pack();
    _frame.setLocation(0, 0);
    _frame.setSize(1000, 650);
    _frame.setVisible(true);
    _frame.setResizable(false);
    _frame.getContentPane().setLayout(new GridBagLayout());
    _frame.addWindowListener(new WindowAdapter() {
      public void windowClosing(WindowEvent e) {
        if (_data_provider != null) {
          _data_provider.closeConnections();
        }
        System.exit(0);
      }
    });

    /* memory access panel*/
    try {
      _classes_list = new JList(_data_provider.getClassList());
//      _data_provider.update();
    } catch (SocketException e) {
      System.out.println("Debugger Connection is broken!");
      e.printStackTrace();
      System.exit(-1);    
    }
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
    JButton stat = new JButton("Statistics");
    stat.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        StatisticsDialog.showDialog(_frame, _data_provider.calculateStatistics());
      }
    });
    button_panel.add(stat);
    _vm_controller = new JButton("Resume");
    _vm_controller.addActionListener(new ActionListener() {
      boolean vm_run = false;
      public void actionPerformed(ActionEvent e) {
        vm_run = !vm_run;
        if (vm_run) {
          _vm_controller.setText("Pause");
        } else {
          _vm_controller.setText("Resume");
        }
        try {
          if (vm_run) {
            _data_provider.resumeVM();
          } else {
            _data_provider.pauseVM();
          }

          
        } catch (SocketException e2) {
          System.out.println("Debugger Connection is broken!");
          e2.printStackTrace();
          System.exit(-1);    
        }
        if (!vm_run) {
          try {
            _classes_list.setListData(_data_provider.getClassList());
          } catch (SocketException e1) {
            System.out.println("Debugger Connection is broken!");
            e1.printStackTrace();
            System.exit(-1);            
          }
          _memory_access_panel.update();
        }

      }
    });
    button_panel.add(_vm_controller);

    _frame.getContentPane().add(button_panel, new GridBagConstraints(2, 2, 1, 1, 1, 1,
                  GridBagConstraints.SOUTHEAST, GridBagConstraints.NONE, new Insets(2, 2, 2, 2), 0, 0));

   _frame.invalidate();
   _frame.validate();
   _frame.repaint();

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

}

class CommandLineException extends Exception {
    public CommandLineException() {
        super();
    }
    public CommandLineException(String s){
        super(s);
    }
}

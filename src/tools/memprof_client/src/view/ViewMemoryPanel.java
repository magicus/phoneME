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

import java.awt.*;
import java.awt.event.*;
import java.util.Iterator;
import javax.swing.*;
import com.sun.cldchi.tools.memoryprofiler.data.*;

public class ViewMemoryPanel extends JComponent {
  MPDataProvider _data_provider;

  //paramenters of view
  private static final int _hor_bloc_number = 100;
  private static final int _ver_bloc_number = 20;
  private static final int _block_number = _hor_bloc_number * _ver_bloc_number;
  private int _block_height = -1;
  private int _block_width = -1;

  //the arrays are used for temperature calculation
  private double[] _block_utilization_count = new double[_block_number];

  private JavaClass selected_class = null;
  private int[] _block_object_count = new int[_block_number];

  //various addresses at start_offset and at end_offset
  private int _inline_allocation_top = -1;
  private int _old_generation_end = -1;
  private int _heap_start = -1;
  private int _heap_top = -1;

  public ViewMemoryPanel(MPDataProvider data_provider) {
    _data_provider = data_provider;
    setMinimumSize(new Dimension(600, 300));
    addMouseListener(new ViewMemoryMouseListener());
  }

  public void set_selected_class_id(JavaClass jc) {
    selected_class = jc;
  }
  public void update() {
    _inline_allocation_top = _data_provider.get_allocation_top();
    _old_generation_end = _data_provider.get_old_gen_end();
    _heap_start = _data_provider.get_heap_start();
    _heap_top = _data_provider.get_heap_top();
    update_block_utilization();
    repaint();
  }

  public void paint(Graphics g) {
    Dimension size = getSize();
    _block_width = (size.width - 150) / _hor_bloc_number;
    _block_height = size.height / _ver_bloc_number;
    prepare_blocks();
    for (int i = 0; i < _block_number; i++) {
      draw_block(g, i);
    }
    draw_inline_allocations(g);
    draw_old_generation_end(g);
    draw_agenda(g);
  }
  
  public void draw_agenda(Graphics g) {
    int x = _block_width * _hor_bloc_number + 10;
    int y = 1;
    draw_block_symbol(g, x, y, 100, 0);
    draw_allocation_top_symbol(g, x, y, false);
    g.setColor(Color.BLACK);
    g.drawString("_inline_allocation_top block", x + 2*_block_width, y + _block_height );

    y += 2*_block_height;
    draw_block_symbol(g, x, y, 100, 0);
    draw_old_generation_end_symbol(g, x, y, false);
    g.setColor(Color.BLACK);
    g.drawString("_old_generation_end block", x + 2*_block_width, y + _block_height );

    y += 2*_block_height;
    draw_block_symbol(g, x, y, 1, 0);
    g.setColor(Color.BLACK);
    g.drawString("block with 100% utilization", x + 2*_block_width, y + _block_height );

    y += 2*_block_height;
    draw_block_symbol(g, x, y, 0.5, 0);
    g.setColor(Color.BLACK);
    g.drawString("block with 50% utilization", x + 2*_block_width, y + _block_height );

    y += 2*_block_height;
    draw_block_symbol(g, x, y, 0, 0);
    g.setColor(Color.BLACK);
    g.drawString("block with 0% utilization", x + 2*_block_width, y + _block_height );

    y += 2*_block_height;
    draw_block_symbol(g, x, y, 100, 1);
    g.setColor(Color.BLACK);
    g.drawString("block with object ", x + 2*_block_width, y + _block_height/2 );
    g.drawString("of selected type", x + 2*_block_width, y + (3*_block_height) / 2);

    y += 2*_block_height;
    draw_block_symbol(g, x, y, 100, 110);
    g.setColor(Color.BLACK);
    g.drawString("block with many objects ", x + 2*_block_width, y + _block_height / 2 );
    g.drawString("of selected type", x + 2*_block_width, y + (3*_block_height) / 2 );

  }

  public Dimension getPreferredSize() {
    return new Dimension(900, 300);
  }

  private void draw_old_generation_end(Graphics g) {
    int i = get_block_number(_old_generation_end);
    int x = _block_width * (i % _hor_bloc_number);
    int y = _block_height * (i / _hor_bloc_number);
    draw_old_generation_end_symbol(g, x, y, false);
  }

  private void draw_inline_allocations(Graphics g) {
    int i = get_block_number(_inline_allocation_top);
    int x = _block_width * (i % _hor_bloc_number);
    int y = _block_height * (i / _hor_bloc_number);
    draw_allocation_top_symbol(g, x, y, false);
  }  

  private void draw_block(Graphics g, int i) {
    int x = _block_width * (i % _hor_bloc_number);
    int y = _block_height * (i / _hor_bloc_number);
    draw_block_symbol(g, x, y, _block_utilization_count[i], _block_object_count[i]);
  }
  
  private void prepare_blocks() {
    for (int i = 0; i < _block_number; i++) {
      _block_object_count[i] = 0;
    }

    JavaObject[] objects = _data_provider.getObjectsOfClass(selected_class);
    for (int i = 0; i < objects.length; i++) {
      int address = objects[i].address;
      int block_number = get_block_number(address);
      _block_object_count[block_number]++;
    }
  }

  private void update_block_utilization() {
    for (int i = 0; i < _block_number; i++) {      
      int start = get_address_by_block_num(i);
      int end = get_address_by_block_num(i+1);
      JavaObject[] objects = _data_provider.getObjectsFromTheAddresses(start, end);
      int full_size = 0;
      int real_size = 0;
      for (int j = 0; j < objects.length; j++) {
        full_size += objects[j].size;
        if (objects[j].alive()) real_size += objects[j].size;
      }
      if (full_size == 0) {
        _block_utilization_count[i] = 1;
      } else {
        _block_utilization_count[i] = ((double)real_size) / full_size;
      }
    }

  }


  private int get_block_number(int address) {
    if (address < _heap_start || address > _heap_top) {
      return -1;
    }
    double tmp = ((double)(address - _heap_start)) /(_heap_top - _heap_start);
    int res = (int)(tmp * _block_number);
    return res;
  }

  private int get_block_num_by_mouse_event(MouseEvent e) {    
    return e.getY() / _block_height * _hor_bloc_number + e.getX() / _block_width;       
  }
  
  private int get_address_by_block_num(int block_num) {  
    return (int)(_heap_start + (_heap_top - _heap_start) * (((double)block_num) / _block_number));
  }
  
  private void draw_allocation_top_symbol(Graphics g, int x, int y, boolean gc) {
    g.setColor(new Color(164, 255, 64));    
    g.drawRect(x, y, 1, _block_height);      
  }

  private void draw_old_generation_end_symbol(Graphics g, int x, int y, boolean gc) {
    g.setColor(new Color(64, 64, 255));    
    g.drawRect(x, y, 1, _block_height);      
  }
  private void draw_block_symbol(Graphics g, int x, int y, double utilization, int objects_num) {
    int util = (int)(255*Math.sqrt(1 - utilization));
    g.setColor(new Color(util, 0, 0));
    g.fillRect(x + 1, y + 1, _block_width  - 1, _block_height - 1);
    g.setColor(new Color(255, 255, 0));
    int radius = 0;
    if (objects_num > 0) radius = 1;
    if (objects_num > 10) radius = 2;
    if (objects_num > 100) { radius = 3; x--; }
    g.fillOval(x + _block_width / 2, y + _block_height / 2, radius, radius);
  }

  public boolean contains(int x, int y) {
    if (!super.contains(x, y)) 
       return false;
    if (y > _block_height * _ver_bloc_number || x > _block_width * _hor_bloc_number)
      return false;
    int blockNum = (y / _block_height) * _hor_bloc_number + x / _block_width;
    int address = get_address_by_block_num(blockNum);
    setToolTipText("0x" + Integer.toHexString(address));
    return true;
  }

  class ViewMemoryMouseListener extends MouseAdapter {
    public void mouseClicked(MouseEvent e) {
      if (e.getButton() != MouseEvent.BUTTON1) {
        return;
      }
      int start = get_address_by_block_num(get_block_num_by_mouse_event(e));
      int end = get_address_by_block_num(get_block_num_by_mouse_event(e) + 1);
      String title = "Objects from " + Integer.toHexString(start) + " to " + Integer.toHexString(end);
      ViewRootPathDialog.showDialog(ViewMemoryPanel.this, _data_provider.getObjectsFromTheAddresses(start, end), 
               title, _data_provider);      
    }
  }
}

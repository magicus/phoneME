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


package com.sun.cldchi.tools.memoryprofiler.data;

import com.sun.cldchi.tools.memoryprofiler.jdwp.VMConnection;
import com.sun.cldchi.tools.memoryprofiler.jdwp.VMReply;
import com.sun.cldchi.tools.memoryprofiler.jdwp.BoundException;
import com.sun.cldchi.tools.memoryprofiler.jdwp.DebugeeException;

import java.net.*;
import java.io.*;
import java.util.*;

class GlobalData implements MPDataProvider {
  private VMConnection _connector;
  private int _heap_start;
  private int _heap_top;
  private int _old_gen_end;
  private int _allocation_top;
  private HashMap _allJavaObjects = new HashMap();
  private HashMap _allClasses = new HashMap();

    // memory profiler command constants
  private static final int MPGetGlobalData     = 0x1201;
  private static final int MPGetHeapData       = 0x1202;
  private static final int MPGetClasses        = 0x1203;
  private static final int MPGetRoots          = 0x1204;
  private static final int MPVMSuspend         = 0x1205;
  private static final int MPVMResume          = 0x1206;
  private static final int MPVMStackTrace      = 0x1207;

  //presentation strings
  public static final String InternalObjectName = "VM Internal object";
  public static final String StaticsObjectName =  "Statics of class ";
  public static final String StackObjectName =    "Stack object";

  private static final int  CLASS_ID_OFFSET = 0;
  private static final int  TASK_ID_OFFSET = 16;
  private static final int  OBJ_TYPE_OFFSET = 23;

  public int get_heap_start() {return _heap_start;};
  public int get_heap_top() {return _heap_top;};
  public int get_old_gen_end() {return _old_gen_end;};
  public int get_allocation_top() {return _allocation_top;};

  GlobalData(VMConnection connection) {
    _connector = connection;
  }

  private void update() throws SocketException {
    getClassList();
    getAllData();
    getRoots();
    try {
      VMReply r = _connector.sendReplyCommand(MPGetGlobalData);
      _heap_start =     r.getInt();
      _heap_top =       r.getInt();
      _old_gen_end =    r.getInt();
      _allocation_top = r.getInt();
    }catch(DebugeeException e) {
      reset();
      throw new SocketException(e.getMessage());
    }catch(BoundException e) {
      reset();
      throw new SocketException(e.getMessage());
    }
    calculateDeadObjects();
  }

  public JavaClass[] getClassList() throws SocketException {
    _allClasses.clear();
    JavaClass[] result = null;
    try {
      VMReply r = _connector.sendReplyCommand(MPGetClasses);
      int classesCount = r.getInt();
      result = new JavaClass[classesCount];
      for(int i=0 ; i<classesCount ; i++){
        int class_id = (int)r.getInt();
        String class_name = objectTypeNameFromJNI(r.getString());
        JavaClass new_item = new JavaClass(class_id, class_name);
        _allClasses.put(new Integer(class_id), new_item);
        result[i] = new_item;
      }
    } catch (Exception e) {
      reset();
      throw new SocketException(e.getMessage());
    }
    return result;
  }

  private void getAllData() throws SocketException {
    int read = 1;
    int oread = 1;
    _allJavaObjects.clear();
    try {
      while (true) {
        VMReply r = _connector.sendReplyCommand(MPGetHeapData);
        int object_address = r.getInt();
        while (object_address != -1 && object_address != -2) {
          int size = r.getInt();
          int mp_class_id = r.getInt();
          int object_type = mp_class_id >> OBJ_TYPE_OFFSET;
          int stack_number = -1;
          if (object_type == STACK_OBJECT) {
            stack_number = r.getInt();
            read++;
          } 
          int links = r.getInt();
          int[] refs = new int[links];
          HashMap offsets = new HashMap(links);
          if (object_type == STACK_OBJECT) {
            for (int i = 0; i < links; i++) {
              refs[i] = r.getInt();
              int offset = r.getInt();
              Integer cur_offset = (Integer)offsets.get(new Integer(refs[i]));
              if (cur_offset == null || cur_offset.intValue() < offset) {
                offsets.put(new Integer(refs[i]), new Integer(offset));
              }
            }
            read += links;
          } else {
            for (int i = 0; i < links; i++) {
              refs[i] = r.getInt();
            }
          }
          mp_class_id = mp_class_id & 0x7FFFFF;
          JavaClass class_item = (JavaClass)_allClasses.get(new Integer(mp_class_id));
          int class_id = mp_class_id;
          if (class_item != null) {
            class_id = class_item.id;
          }
          if (object_type == JAVA_OBJECT) {
            _allJavaObjects.put(new Integer(object_address), 
               new JavaObject(object_address, class_id, size, refs, JAVA_OBJECT));
          } else if (object_type == STATICS_OBJECT) {
            _allJavaObjects.put(new Integer(object_address), 
               new JavaObject(object_address, class_id, size, refs, STATICS_OBJECT));
          } else if (object_type == STACK_OBJECT) {
            _allJavaObjects.put(new Integer(object_address), 
               new JavaObject(object_address, -1, size, refs, STACK_OBJECT, offsets, stack_number));
          } else if (object_type == VM_OBJECT) {
            _allJavaObjects.put(new Integer(object_address), 
               new JavaObject(object_address, -1, size, refs, VM_OBJECT));
          } else {
            System.out.println("Wrong response from VM! Unknown object type. Skipped!");
          }
          object_address = r.getInt();
          read += 4;
          read += links;
          oread++;
        }         
        if (object_address == -1) break;
      }
    } catch (Exception e) {
      reset();
      throw new SocketException(e.getMessage());
    }
    updateAllObjects();
  }

  private void updateAllObjects() {
    for (Iterator it = _allJavaObjects.values().iterator(); it.hasNext();) {
      JavaObject obj = (JavaObject)it.next();
      for (int i = 0; i < obj._references_addresses.length; i++) {
        JavaObject ref = getObjectByAddress(obj._references_addresses[i]);
        if (ref != null) {
          obj.add_reference(ref);
          ref.add_referee(obj);
        } 
      }
    }
  }

  private JavaObject getObjectByAddress(int address) {
    Integer key = new Integer(address);
    return (JavaObject)_allJavaObjects.get(key);
  }

  public JavaObject[] getObjectsOfClass(JavaClass jc) {
    if (jc == null) return new JavaObject[0];
    int obj_count = 0;
    int class_id = jc.id;

    for (Iterator it = _allJavaObjects.values().iterator(); it.hasNext(); ) {
      JavaObject elem = (JavaObject)it.next();
      if (elem.object_type == JAVA_OBJECT && elem.class_id == class_id)
        obj_count++;
    } 
    JavaObject result[] = new JavaObject[obj_count];
    obj_count = 0;
    for (Iterator it = _allJavaObjects.values().iterator(); it.hasNext(); ) {
      JavaObject elem = (JavaObject)it.next();
      if (elem.object_type == JAVA_OBJECT && elem.class_id == class_id)
        result[obj_count++] = elem;
    }
    return result;
  }

  public Iterator getObjects() {
    return _allJavaObjects.values().iterator();
  }
  public void connect(String hostName, int port) throws java.net.ConnectException, SocketException  {
    _connector.connect(hostName, port);    
    update();
  }

  public String getObjectTypeName(JavaObject obj) {
    if (obj.object_type == JAVA_OBJECT || obj.object_type == STATICS_OBJECT) {
      JavaClass item = (JavaClass)_allClasses.get(new Integer(obj.class_id)); 
      if (item == null) { //this is just for debug!
        System.out.println(obj.class_id);
        System.out.println(obj.object_type);
        return "null!";
      }
      if (obj.object_type == JAVA_OBJECT) {
        return item.name;
      } else { //statics object
        return StaticsObjectName + item.name;
      }    
    } else if (obj.object_type == STACK_OBJECT) {
      return StackObjectName;
    } else if (obj.object_type == VM_OBJECT) {        
      return InternalObjectName;
    } else {
      return "Wrong object type! Report a bug please!";
    }
  }

  private void getRoots() throws SocketException {
    try {
      VMReply r = _connector.sendReplyCommand(MPGetRoots);
      int root = r.getInt();
      while (root != -1) {
        Integer key = new Integer(root);
        JavaObject obj = (JavaObject)_allJavaObjects.get(key);
        if (obj != null) {
          obj.setRootDistance(0);
        }
        root = r.getInt();
      }
    } catch (Exception e) {
      reset();
      throw new SocketException(e.getMessage());
    }

  }

  private void calculateDeadObjects() {
    boolean was_updated = true;
    while (was_updated) {
      was_updated = false;
      for (Iterator it = _allJavaObjects.values().iterator(); it.hasNext(); ) {
        JavaObject elem = (JavaObject)it.next();
        if (elem.getRootDistance() == -1)
          continue;
        Object[] refs = elem.get_references();
        for (int i = 0; i < refs.length; i++) {
          JavaObject obj = (JavaObject)refs[i];
          if (obj.getRootDistance() == -1) {
            obj.setRootDistance(elem.getRootDistance() + 1);
            was_updated = true;
          } else if (obj.getRootDistance() > 1 + elem.getRootDistance()) {
            obj.setRootDistance(elem.getRootDistance() + 1);
            was_updated = true;
          }
        }
      } 
    }   
  }

  public JavaObject[] pathFromTheRoot(JavaObject obj) {
    if (obj == null) return null;
    if (obj.getRootDistance() == -1) return null;
    JavaObject[] result = new JavaObject[obj.getRootDistance() + 1];
    int dst = obj.getRootDistance();
    while (dst > 0) { 
      result[dst--] = obj;
      Object[] referees = obj.get_referees();
      for (int i = 0; i < referees.length; i++) {
        JavaObject elem = (JavaObject)referees[i];
        if (elem.getRootDistance() == obj.getRootDistance() - 1) {
          obj = elem; break;
        }
      }      
      if (dst != obj.getRootDistance()) throw new RuntimeException("problem here!");      
    }
    result[dst--] = obj;
    return result;
  } 

  public JavaObject[] getObjectsFromTheAddresses(int start, int end) {
    if (start > end) throw new RuntimeException();
    int obj_count = 0;
    for (Iterator it = _allJavaObjects.values().iterator(); it.hasNext(); ) {
      JavaObject obj = (JavaObject)it.next();
      if (obj.address + obj.size > start && obj.address < end) {
        obj_count++;
      }
    }
    JavaObject[] result = new JavaObject[obj_count];
    obj_count = 0;
    for (Iterator it = _allJavaObjects.values().iterator(); it.hasNext(); ) {
      JavaObject obj = (JavaObject)it.next();
      if (obj.address + obj.size > start && obj.address < end) {
        result[obj_count++] = obj;
      }
    }
    Arrays.sort(result, new Comparator() {
     public int	compare(Object o1, Object o2) {
       return ((JavaObject)o1).address - ((JavaObject)o2).address;
     } 
    });
    return result;
  } 

  public void pauseVM() throws SocketException { 
    try {
      _connector.sendCommand(MPVMSuspend);
      update();
    } catch (Exception e ) {
      reset();
      throw new SocketException(e.getMessage());
    }
  } 

  public void resumeVM() throws SocketException { 
    try {
      reset();
      _connector.sendCommand(MPVMResume);
    } catch (Exception e ) {
      reset();
      throw new SocketException(e.getMessage());
    }
  } 

  public ClassStatistics[] calculateStatistics() {

    ClassStatistics.reset();
    int count = _allClasses.values().size() + 1;
    HashMap result = new HashMap(count);
    for (Iterator it = _allClasses.values().iterator(); it.hasNext();) {
      JavaClass item = (JavaClass)it.next();
      result.put(new Integer(item.id), new ClassStatistics(item.name));
    }
    result.put(new Integer(-1), new ClassStatistics("Internal VM Objects"));
    for (Iterator it = _allJavaObjects.values().iterator(); it.hasNext();) {
      JavaObject obj = (JavaObject)it.next();
      int type_id = obj.class_id;      
      if (obj.object_type != JAVA_OBJECT) {
         type_id = -1;
      }
      ClassStatistics cls = (ClassStatistics)result.get(new Integer(type_id));
      if (cls == null) continue; //shall not happend
      cls.add(obj, get_old_gen_end());
    }
    Object[] arr = result.values().toArray();
    Arrays.sort(arr, new Comparator() {
      public int compare(Object obj1, Object obj2) {
        ClassStatistics cls1 = (ClassStatistics)obj1;
        ClassStatistics cls2 = (ClassStatistics)obj2;
        return cls2.getHeapPercentage() - cls1.getHeapPercentage();
      }  
    });
    ClassStatistics[] res = new ClassStatistics[arr.length];
    for (int i = 0; i < res.length; i++) {
      res[i] = (ClassStatistics)arr[i];
    }

    return res;    
  }

  /**
   * Converts object type name from JNI form to <code>javap</code>-like
   * form (for example, <code>Ljava/lang/String;</code> will be converted to
   * <code>java.lang.String</code>). This method is
   * used by <code>fields</code> and <code>methods</code> KJDB commands.
   *
   * @param jniTypeName an object type name in JNI form
   * @return an object type name in <code>javap</code>-like form
   */
  private String objectTypeNameFromJNI(String jniTypeName){
    if(jniTypeName.indexOf("L") != 0 ||
      jniTypeName.lastIndexOf(";") != jniTypeName.length() - 1){
        return jniTypeName;
    }
    char[] chars = new char[jniTypeName.length()-2];
    jniTypeName.getChars(1,jniTypeName.length()-1,chars,0);
    for(int i=0 ; i<chars.length ; i++){
      if(chars[i] == '/'){
        chars[i] = '.';
      }
    }
    return new String(chars);
  }

  public void closeConnections() {
     reset();
    _connector.closeConnections();
  }

  private void setConnector(VMConnection connector) {
    if (_connector == null ) {
      _connector = connector;
      return;
    }
    if (_connector.isConnected()) {
      throw new IllegalStateException("Another connector is connected to the VM");
    }
    _connector = connector;
    reset();
  }

  public String getStackTrace(JavaObject stack_object, int ptr) throws SocketException {
    if (stack_object.object_type != STACK_OBJECT) {
      return "JavaObject of wrong type type was passed to MPDataProvider.getStackTrace.\n" +
                   "Please report a bug!";
    }
    int[] params = new int[2];    
    params[0] = stack_object._stack_id;
    Integer offset = (Integer)stack_object._stack_offsets.get(new Integer(ptr));
    params[1] = offset.intValue();
    String result = null;
    try {
      VMReply reply = _connector.sendReplyCommand(MPVMStackTrace, params);
      result = reply.getString();
    } catch (Exception e) {
      result = "VM connection is broken!";
      reset();
      throw new SocketException(e.getMessage());
    }
    return result;
  }

  private void reset() {
    _heap_start = 0;
    _heap_top = 0;
    _old_gen_end = 0;
    _allocation_top = 0;
    _allJavaObjects.clear();
    _allClasses.clear();
    ClassStatistics.reset();
  }
}

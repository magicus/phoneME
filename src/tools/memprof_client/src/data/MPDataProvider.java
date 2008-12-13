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

import java.net.*;
import java.io.*;
import java.util.*;
import com.sun.cldchi.tools.memoryprofiler.jdwp.VMConnection;
/**
 * The <code>MPDataProvider</code> declares interface which provides data for 
 * data-displaying GUI tool. 
 * For work it requires some implementation of <code>VMConnection</code>.
 * The implementation of this interface is provided by <code>MPDataProviderFactory</code>.
 *
 * To work with this interface you should firstly obtain its realization from <code>MPDataProviderFactory</code>, 
 * passing it an instance of <code>VMConnection</code>.
 * After this you should connect to a KDP 
 * running on hostName:port by calling <code>connect(String hostName, int port)</code>
 * To control VM execution use <code>pauseVM()</code> and <code>resumeVM()</code> funtions. * 
 * All other functions provide processed information.
 *
 * @see com.sun.cldchi.tools.memoryprofiler.data.MPDataProviderFactory
 * @see com.sun.cldchi.tools.memoryprofiler.jdwp.VMConnection
 *
 */
public interface MPDataProvider {


  //internal VM object types
  public static final int JAVA_OBJECT  = 0;
  public static final int STATICS_OBJECT = 1;
  public static final int STACK_OBJECT   = 2;
  public static final int VM_OBJECT      = 3;

  /**
   * Connects to the KDP running on hostName:port using <code>VMConnection</code>, which should 
   * be set before by <code>setConnector(VMConnection connector)</code> call.
   *
   * It will throw <code>java.net.ConnectException</code> unless connection is successful.
   *
   * @param hostName - name of host where KDP runs
   * @param port - number of port where KDP listens
   *
   * @see #closeConnections()   
   */  
  public void connect(String hostName, int port) throws java.net.ConnectException, SocketException ;

  /**
   * Disconnects from the KDP. Does nothing if not connected.
   */  
  public void closeConnections();


  /**
   * Returns list of all classes in systems. All objects in array are <code>JavaClass</code>
   * The provider must be connected to a KDP, see interface description.
   * The function will throw <code>SocketException</code> unless communication with VM is successful
   *
   * @return array of <code>JavaClass</code>
   *
   * @see #connect(String hostName, int port)
   */  
  public JavaClass[] getClassList() throws SocketException;

  /**
   * Pauses the VM execution.
   * The provider must be connected to a kdp, see interface description.
   * The function will throw <code>SocketException</code> unless communication with VM is successful
   *
   * @see #connect(String hostName, int port)
   */  
  public void pauseVM() throws SocketException;

  /**
   * Resumes the VM execution.
   * The provider must be connected to a KDP, see interface description.
   * The function will throw <code>SocketException</code> unless communication with VM is successful
   *
   * @see #connect(String hostName, int port)
   */  
  public void resumeVM() throws SocketException;

  /**
   * Returns value of _heap_start global varaible, which is address where VM heap starts.
   * The provider must be connected to a KDP, see interface description.
   * The function will throw <code>SocketException</code> unless communication with VM is successful
   *
   * @return value of _heap_start global varaible
   *
   * @see #connect(String hostName, int port)
   */  
  public int get_heap_start();

  /**
   * Returns value of _heap_top global varaible, which is address where VM heap ends.
   * The provider must be connected to a KDP, see interface description.
   * The function will throw <code>SocketException</code> unless communication with VM is successful
   *
   * @return value of _heap_top global varaible
   *
   * @see #connect(String hostName, int port)
   */  
  public int get_heap_top();

  /**
   * Returns value of _old_generation_end global varaible, 
   * which is address where old generation of heap objects ends.
   * The provider must be connected to a KDP, see interface description.
   * The function will throw <code>SocketException</code> unless communication with VM is successful
   *
   * @return value of _old_generation_end global varaible
   *
   * @see #connect(String hostName, int port)
   */  
  public int get_old_gen_end();

  /**
   * Returns value of _old_generation_end global varaible, 
   * which is address where old generation of heap objects ends.
   * The provider must be connected to a KDP, see interface description.
   * The function will throw <code>SocketException</code> unless communication with VM is successful
   *
   * @return value of _old_generation_end global varaible
   *
   * @see #connect(String hostName, int port)
   */  
  public int get_allocation_top();

  /**
   * Returns all objects with given extended class id.
   * The provider must be connected to a KDP, see interface description.
   * The function will throw <code>SocketException</code> unless communication with VM is successful
   *
   * @param jc - class which objects you want to find.
   *
   * @return all objects with given extended class id.
   *
   * @see #connect(String hostName, int port)
   */  
  public JavaObject[] getObjectsOfClass(JavaClass jc);

  /**
   * Returns all objects.
   * The provider must be connected to a KDP, see interface description.
   * The function will throw <code>SocketException</code> unless communication with VM is successful
   *
   * @return all objects. Iterator returns instances of <code>JavaObject</code>
   *
   * @see #connect(String hostName, int port)
   */  
  public Iterator getObjects();

  /**
   * Returns name of the type of the object.
   * The provider must be connected to a KDP, see interface description.
   * The function will throw <code>SocketException</code> unless communication with VM is successful
   *
   * @param obj 
   *
   * @return name of the type of the object.
   *
   * @see #connect(String hostName, int port)
   */  
  public String getObjectTypeName(JavaObject obj);

  /**
   * Returns array of linked JavaObjects, where first one is a root object and last one is the parameter.
   * The provider must be connected to a KDP, see interface description.
   * The function will throw <code>SocketException</code> unless communication with VM is successful
   *
   * @param obj 
   *
   * @return array of linked JavaObjects, where first one is a root object and last one is the parameter.
   *
   * @see #connect(String hostName, int port)
   */  
  public JavaObject[] pathFromTheRoot(JavaObject obj);

  /**
   * Returns array JavaObjects, sorted by address from given memory block.
   * The provider must be connected to a KDP, see interface description.
   * The function will throw <code>SocketException</code> unless communication with VM is successful
   *
   * @param start - start address of the zone in heap
   * @param end - end address of the zone in heap
   *
   * @return array JavaObjects, sorted by address from given memory block.
   *
   * @see #connect(String hostName, int port)
   */  
  public JavaObject[] getObjectsFromTheAddresses(int start, int end);

  /**
   * Returns statistics of memory usage for each class in the VM. 
   * There is separate <code>ClassStatistics</code> object int the returned array for each class.
   * The provider must be connected to a KDP, see interface description.
   * The function will throw <code>SocketException</code> unless communication with VM is successful
   *
   * @return array of <code>ClassStatistics</code>.
   *
   * @see #connect(String hostName, int port)
   */  
  public ClassStatistics[] calculateStatistics();  

  /**
   * Returns stacktrace of an address on java stack
   * The provider must be connected to a KDP, see interface description.
   * The function will throw <code>SocketException</code> unless communication with VM is successful
   *
   * @param stackObject - JavaObject of stack type
   * @param ptrAddress - address of the pointer on stack
   *
   * @return String.
   *
   * @see #connect(String hostName, int port)
   */  
  public String getStackTrace(JavaObject stackObject, int ptrAddress) throws SocketException;;  

}

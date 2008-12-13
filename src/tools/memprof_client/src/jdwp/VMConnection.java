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


package com.sun.cldchi.tools.memoryprofiler.jdwp;

import java.net.*;
import java.io.*;
import java.util.*;
/**
 * The <code>VMConnection</code> declares interface between transport 
 * layer and data providing layer of the memory profiler. It allows sending
 * commands to the VM and receiving replies as instances of <code>VMReply</code>.
 * The default implementation of this interface is provided by <code>VMConnectionFactory</code>.
 *
 * @see com.sun.cldchi.tools.memoryprofiler.jdwp.VMConnection
 * @see com.sun.cldchi.tools.memoryprofiler.jdwp.VMConnectionFactory
 *
 */


public interface VMConnection {

  /**
   * Sends a command to the VM, which produces some responce and 
   * returns instance of <code>VMReply</code> for working with the 
   * responce.
   * Could throw <code>DebugeeException</code> unless sending command is successful
   * This functions requires connection to a KDP and will throw <code>DebugeeException</code> otherwise. 
   * You can check it with <code>isConnected()</code> function.
   * To connect to a KDP you must use <code>connect(String hostName, int port)</code> function.
   *
   * @param command - code of command to be sent
   * @param params  - additional command parameters
   *
   * @return instance of <code>VMReply</code> for working with the VM responce
   *
   * @see #sendCommand(int command)
   * @see #isConnected()
   */
  public VMReply sendReplyCommand(int command, int[] params) throws DebugeeException;

   /**
   * Sends a command to the VM which don't needs any parameters, which produces some responce and 
   * returns instance of <code>VMReply</code> for working with the 
   * responce.
   * Could throw <code>DebugeeException</code> unless sending command is successful
   * This functions requires connection to a KDP and will throw <code>DebugeeException</code> otherwise. 
   * You can check it with <code>isConnected()</code> function.
   * To connect to a KDP you must use <code>connect(String hostName, int port)</code> function.
   *
   * @param command - code of command to be sent
   *
   * @return instance of <code>VMReply</code> for working with the VM responce
   *
   * @see #sendCommand(int command)
   * @see #isConnected()
   */
  public VMReply sendReplyCommand(int command) throws DebugeeException;

  /**
   * Sends a command to the VM, which doesn't produces and responce.
   * Could throw <code>DebugeeException</code> unless sending command is successful
   * 
   * This functions requires connection to a KDP and will throw <code>DebugeeException</code> otherwise. 
   * You can check it with <code>isConnected()</code> function.
   * To connect to a kdp you must use <code>connect(String hostName, int port)</code> function.
   *
   * @param command - code of command to be sent
   *
   * @see #sendReplyCommand(int command)
   * @see #isConnected()
   */
  public void sendCommand(int command) throws DebugeeException;

  /**
   * Connects to the KDP running on hostName:port. 
   * Could throw <code>ConnectException</code> unless connection is successful
   *
   * @param hostName - name of host where KDP runs
   * @param port - number of port where KDP listens
   *
   * @see #isConnected()
   * @see #closeConnections()
   */
  public void connect(String hostName, int port) throws ConnectException;

  /**
   * Checks if the instance is connected to a KDP.
   *
   * @return true if is connected to a kdp, false otherwise.
   *
   * @see #connect(String hostName, int port)
   * @see #closeConnections()
   */
  public boolean isConnected();

  /**
   * Disconnects from a KDP.
   *
   * @see #isConnected()
   * @see #connect(String hostName, int port)
   */
  public void closeConnections();
}

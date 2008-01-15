/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

package javax.microedition.io;

import java.security.Permission;

/*
 * An abstract class that is the superclass of all permissions used with the 
 *  Generic Connection Framework. A GCFPermission  consists of a URI string 
 * representing access rights to a connection based on the specified protocol scheme.
 *
 * A specification that extends the Generic Connection Framework with a new protocol scheme 
 * should define a subclass of GCFPermission to guard access to the protocol. 
 */
public abstract class GCFPermission extends Permission {

  private String uri;
  private String protocol;

  /**
   * Constructs a GCFPermission with the specified URI.
   * @param uri - The URI string.
   * @exception IllegalArgumentException - if uri is malformed.
   * @exception NullPointerException - if uri is null.
   */ 
  public GCFPermission(String uri) {
    super(uri);
    if (uri == null) throw new NullPointerException("uri is null!");
    int idx = uri.indexOf(':');
    if (idx == -1) throw new IllegalArgumentException("uri must contain ':', but is " + uri);
    this.protocol = uri.substring(0, idx);
    this.uri = uri;
  }

  /**
   * Returns the URI of this GCFPermission.
   *
   * @return the URI string, identical to Permission.getName().
   */
  public String getURI() {
    return uri;
  }

  /**
   * Returns the protocol scheme of this GCFPermission. 
   * The protocol scheme is the string preceding the first ':' in the URI.
   *
   * @return the protocol scheme portion of the URI string.
   */
  public String getProtocol() {
    return protocol;
  }

  String[] parse_full_info(String full_info) {
    String[] result = new String[3];
    int idx = full_info.indexOf(':');
    if (idx == -1) { // no port information, so we must separate path from the host
      idx = full_info.indexOf('/'); //start of the path
      if (idx == -1) { //only host info presents
        result[0] = full_info;
      } else {
        result[0] = full_info.substring(0, idx - 1);
        result[2] = full_info.substring(idx, full_info.length());
      }
      return result;
    } else if (idx == 0 ) { //server connection
      result[0] = null;
      full_info = full_info.substring(idx + 1, full_info.length());
    } else if (idx == full_info.length()) { // ':' is last symbol
       throw new IllegalArgumentException("uri is malformed. Port info is wrong.");
    } else {
      result[0] = full_info.substring(0, idx);
      full_info = full_info.substring(idx + 1, full_info.length());
    } 

    idx = full_info.indexOf('/'); //start of the path
    if (idx == -1) { //only port info presents
      result[1] = full_info;
    } else {
      result[1] = full_info.substring(0, idx - 1);
      result[2] = full_info.substring(idx, full_info.length());
    }

    return result;
  } 

  String[] parse_host_info(String host_info) {
    String[] result = new String[2];

    int idx = host_info.indexOf('*');
    if (idx == -1) { // no wildcard
      result[1] = host_info;
    } else if (idx == host_info.length() && idx == 0) { // host is *
      result[0] = result[1] = "";
    } else if (idx == 0) { // wildcard
      result[1] = host_info.substring(1, host_info.length()); // we keep host WITHOUT wildcard!!!
      result[0] = "";
    } else { //wildcard is in wrong position
      throw new IllegalArgumentException("uri is malformed. Wildcard is misused.");        
    }

    return result;
  } 

  int[] parse_port_info(String port_info) {
    int[] result = new int[2];
    result[0] = -1;
    result[1] = 0x7fffffff;
    if (port_info != null && port_info != "") {
      int idx = port_info.indexOf('-');
      try {
        if (idx == -1) { //just portnumber        
          int port = Integer.parseInt(port_info, 10);
          result[0] = port;
          result[1] = port;
        } else if (idx == 0) { // -portnumber
          result[1] = Integer.parseInt(port_info.substring(1, port_info.length()), 10);
        } else if (idx == port_info.length() - 1) { // portnumber-
          result[0] =  Integer.parseInt(port_info.substring(0, port_info.length() - 1), 10);
        } else {
          result[0] =  Integer.parseInt(port_info.substring(0, idx), 10);
          result[1] = Integer.parseInt(port_info.substring(idx + 1, port_info.length()), 10);        
        }
      } catch (NumberFormatException ex) {
        throw new IllegalArgumentException("port information is malformed: " + port_info + ex);
      }
    }
    return result;
  }  

}


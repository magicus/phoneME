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
import java.security.PermissionCollection;

import java.util.Enumeration;
import java.util.Vector;

/*
 * An abstract class that is the superclass of all permissions used with the 
 *  Generic Connection Framework. A GCFPermission  consists of a URI string 
 * representing access rights to a connection based on the specified protocol scheme.
 *
 * A specification that extends the Generic Connection Framework with a new protocol scheme 
 * should define a subclass of GCFPermission to guard access to the protocol. 
 */
public abstract class GCFPermission extends Permission {

  private URIParser parser;

  /**
   * Constructs a GCFPermission with the specified URI.
   * @param uri - The URI string.
   * @exception IllegalArgumentException - if uri is malformed.
   * @exception NullPointerException - if uri is null.
   */ 
  public GCFPermission(String uri) {
    super(uri);
    parser = new URIParser(uri, true /* require server authority */);
  }

  /**
   * Returns the URI of this GCFPermission.
   *
   * @return the URI string, identical to Permission.getName().
   */
  public String getURI() {
    return getName();
  }

  /**
   * Returns the protocol scheme of this GCFPermission. 
   * The protocol scheme is the string preceding the first ':' in the URI.
   *
   * @return the protocol scheme portion of the URI string.
   */
  public String getProtocol() {
    return parser.getScheme();
  }

  final int getMinPort() {
    return parser.getPortRange()[0];
  }

  final int getMaxPort() {
    return parser.getPortRange()[1];
  }

  final String getHost() {
    return parser.getHost();
  }

  final String getPath() {
    return parser.getPath();
  }

  final String getSchemeSpecificPart() {
    return parser.getSchemeSpecificPart();
  }

  final boolean impliesByHost(GCFPermission that) {
    URIParser thisParser = this.parser;
    URIParser thatParser = that.parser;

    String thisHost = thisParser.getHost();
    String thatHost = thatParser.getHost();

    // Handle empty host names - server connections
    if ("".equals(thisHost) || "".equals(thatHost)) {
      return thisHost.equals(thatHost);
    }

    String thisCanonicalHostName = thisParser.getCanonicalHostName();
    String thatCanonicalHostName = thatParser.getCanonicalHostName();
    if (thisCanonicalHostName != null &&
      thisCanonicalHostName.equals(thatCanonicalHostName)) {
      return true;
    }

    if (thisHost.startsWith("*") && thatCanonicalHostName != null) {
      return thatCanonicalHostName.endsWith(thisCanonicalHostName);
    }

    return false;    
  }

  final boolean impliesByPorts(GCFPermission p) {
    return 
      (getMinPort() <= p.getMinPort()) && (getMaxPort() >= p.getMaxPort());
  }

  final void checkNoHostPort() {
    parser.checkNoHost();
    parser.checkNoPortRange();
  }

  final void checkHostPortPathOnly() {
    parser.checkNoFragment();
    parser.checkNoUserInfo();
    parser.checkNoQuery();
  }

  final void checkHostPortOnly() {
    parser.checkNoFragment();
    parser.checkNoUserInfo();
    parser.checkNoPath();
    parser.checkNoQuery();
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

/**
 * A GCFPermissionCollection stores a collection
 * of GCF permissions. GCFPermission objects
 * must be stored in a manner that allows them to be inserted in any
 * order, but enable the implies function to evaluate the implies
 * method in an efficient (and consistent) manner.
 *
 * Subclasses should override <code>add</code> method to 
 *
 * @see java.security.Permission
 */

final class GCFPermissionCollection extends PermissionCollection {
  private static final Class gcfPermissionClass;
  private final Class permissionClass;
  private final Vector permissions = new Vector(6);

  static {
    try {
      gcfPermissionClass = 
        Class.forName("javax.microedition.io.GCFPermission");
    } catch (ClassNotFoundException e) {
      throw new Error(e.toString());
    }
  }

  /**
   * Create an empty GCFPermissionCollection object.
   *
   */
  public GCFPermissionCollection(Class clazz) {
    if (!gcfPermissionClass.isAssignableFrom(clazz)) {
      throw new IllegalArgumentException();
    }
    permissionClass = clazz;
  }

  /**
   * Adds a permission to the collection.
   *
   * @param permission the Permission object to add.
   *
   * @exception IllegalArgumentException - if the permission is not a
   *                                       GCFPermission, or if
   *					     the permission is not of the
   *					     same Class as the other
   *					     permissions in this collection.
   *
   * @exception SecurityException - if this GCFPermissionCollection object
   *                                has been marked readonly
   */
  public void add(Permission permission) {
    if (!permissionClass.isInstance(permission)) {
      throw new IllegalArgumentException(
        "Invalid permission class: " + permission);
    }
	
    if (isReadOnly()) {
      throw new SecurityException(
        "Cannot add a Permission to a readonly PermissionCollection");
    }

    permissions.addElement(permission);
  }

  /**
   * Check and see if this set of permissions implies the permissions
   * expressed in "permission".
   *
   * @param p the Permission object to compare
   *
   * @return true if "permission" is a proper subset of a permission in
   * the set, false if not.
   */
  public boolean implies(Permission permission) { 
    if (!permissionClass.isInstance(permission)) {
      return false;
    }

    GCFPermission perm = (GCFPermission) permission;

    int perm_low  = perm.getMinPort();
    int perm_high = perm.getMaxPort();

    Enumeration search = permissions.elements();
    int count = permissions.size();
    int port_low[] = new int[count];
    int port_high[] = new int[count];
    int port_range_count = 0;
    while (search.hasMoreElements()) {
      GCFPermission cur_perm = 
        (GCFPermission)search.nextElement();
      if (cur_perm.impliesByHost(perm)) {
        if (cur_perm.impliesByPorts(perm)) {
          return true;
        }
        
        port_low[port_range_count] = cur_perm.getMinPort();
        port_high[port_range_count] = cur_perm.getMaxPort();
        port_range_count++;
      }
    }

    // now we need to determine if found port ranges cover perm port range.
    // we will sort the port ranges by the low border and will try to found
    // the longest continues range covered by these ranges
    // we will use simple x^2 sort here, cause it is rare situation when we
    // have many port ranges.

    for (int i = 0; i < port_range_count; i++) {
      for (int j = 0; j < port_range_count - 1; j++) {
        if (port_low[j] > port_low[j+1]) {
          int tmp = port_low[j];
          port_low[j] = port_low[j+1];
          port_low[j+1] = tmp;
          tmp = port_high[j];
          port_high[j] = port_high[j+1];
          port_high[j+1] = tmp;
        }
      }
    }

    int current_low = port_low[0];
    int current_high = port_high[0];
    for (int i = 1; i < port_range_count; i++) {
      if (port_low[i] > current_high + 1) { //end of continious range
        if (current_low <= perm_low && current_high >= perm_high) {
          return true;
        } 
        if (perm_low <= current_high) {
          return false;
        }
        current_low = port_low[i];
        current_high = port_high[i];
      } else {
        if (current_high < port_high[i]) {
          current_high = port_high[i];
        }
      }
    }
    return (current_low <= perm_low && current_high >= perm_high);
  }

  /**
   * Returns an enumeration of all the GCFPermission objects in the
   * container.
   *
   * @return an enumeration of all the GCFPermission objects.
   */
  public Enumeration elements() {
    return permissions.elements();
  }
}

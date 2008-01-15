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

import java.security.PermissionCollection;
import java.security.Permission;
import java.util.Enumeration;
import java.util.Hashtable;

/**
 * This class represents access rights to connections via the "comm" protocol. 
 * A CommProtocolPermission consists of a URI string but no actions list.
 *
 * The URI string specifies a logical serial port connection and optional parameters. 
 * It takes the following form:
 *
 *  comm:{port identifier}[{optional parameters}]
 *
 * An asterisk may appear at the end of the URI string to indicate a wildcard match. 
 * Valid examples include "comm:*", "comm:port*" and "comm:port1;*". 
 */

public final class CommProtocolPermission extends GCFPermission {

  boolean has_wildcard = false;
  boolean has_port_wildcard = false;
  String port;
  String options;
  /**
   * Creates a CommProtocolPermission object.
   * @param The URI identifying the comm port.
   */
  public CommProtocolPermission(String uri) {
    super(uri);
    if (!uri.startsWith("comm:")) {
      throw new IllegalArgumentException("uri shall start with 'comm:', but is " + uri);
    }
    String full_info = uri.substring(5/*"comm:".length()*/, uri.length());

    int idx = full_info.indexOf(';');
    if (idx == -1) {
      options = "";
      port = full_info;
    } else if (idx == full_info.length() - 1) {
      throw new IllegalArgumentException("optional parameters are malformed, ';' is the last symbol");
    } else {
      port = full_info.substring(0, idx);
      options = full_info.substring(idx + 1, full_info.length());      
    }

    idx = port.indexOf('*');
    if (idx != -1) {
      if (idx != port.length() - 1) {
        throw new IllegalArgumentException("port identifier is malformed, '*' is in not the last position");
      }

      has_port_wildcard = true;

      if (port.length() == 0) {
        port = "";
      } else {
        port = port.substring(0, port.length() - 1);
      }
      if (options == "") {
        has_wildcard = true;
      }
    }

    idx = options.indexOf('*');
    if (idx != -1) {
      if (idx != options.length() - 1) {
        throw new IllegalArgumentException("options are malformed, '*' is in not the last position");
      }

      has_wildcard = true;

      if (options.length() == 0) {
        options = "";
      } else {
        options = options.substring(0, port.length() - 1);
      }
    }
  }

  /**
   *  Checks if this CommProtocolPermission object "implies" the specified permission.
   *
   *  More specifically, this method returns true if:
   *
   *     * p is an instanceof CommProtocolPermission, and
   *
   *     * p's URI string equals or (in the case of wildcards) is implied by this object's URI string. 
   *                      For example, "comm:*" implies "comm:port1;baudrate=300". 
   *
   * @param p - the permission to check against 
   * @return true if the specified permission is implied by this object, false if not.
   */
  public boolean implies(Permission p) {
    if (!(p instanceof CommProtocolPermission)) {
      return false;
    } 
    CommProtocolPermission p1 = (CommProtocolPermission)p;
    if (has_port_wildcard) {
      if (!p1.port.startsWith(port)) {
        return false;        
      }
    } else {
      if (!p1.port.equals(port) || p1.has_port_wildcard) {
        return false;        
      }
    }

    if (has_wildcard) {
      if (!p1.options.startsWith(options)) {
        return false;        
      }
    } else {
      if (!p1.port.equals(port) || p1.has_wildcard) {
        return false;        
      }
    }
    
    return true;
  }

  /**
   * Checks two CommProtocolPermission objects for equality.
   * @param obj - the object we are testing for equality with this object..
   * @return true if obj is a CommProtocolPermission and has the same URI string 
   *              as this CommProtocolPermission object.
   */
  public boolean equals(Object obj) {
    if (!(obj instanceof CommProtocolPermission)) {
      return false;
    }
    CommProtocolPermission other = (CommProtocolPermission)obj;
    return other.getURI().equals(getURI());
  }

  /**
   * Returns the hash code value for this object.
   * @return  a hash code value for this object.
   */
  public int hashCode() {
    return getURI().hashCode();
  }

  /**
   * Returns the canonical string representation of the actions, which currently 
   * is the empty string "", since there are no actions defined for CommProtocolPermission.
   * @return the empty string "".
   */
  public String getActions() {
    return "";
  }

  /**
   * Returns a new PermissionCollection for storing CommProtocolPermission objects.
   *
   * CommProtocolPermission objects must be stored in a manner that allows them to be inserted 
   * into the collection in any order, but that also enables the PermissionCollection implies 
   * method to be implemented in an efficient (and consistent) manner.  
   *
   * @return a new PermissionCollection suitable for storing CommProtocolPermission objects.
   */
  public PermissionCollection newPermissionCollection() {
    return new CommProtocolPermissionCollection();
  } 
}

/**
 * A GCFPermissionCollection stores a collection
 * of GCFPermission permissions. GCFPermission objects
 * must be stored in a manner that allows them to be inserted in any
 * order, but enable the implies function to evaluate the implies
 * method in an efficient (and consistent) manner.
 *
 * A GCFPermissionCollection handles comparing a permission like "a.b.c.d.e"
 * with a Permission such as "a.b.*", or "*".
 *
 * @see java.security.Permission
 * @see java.security.Permissions
 * @see java.security.PermissionsImpl
 *
 * 
 *
 * @serial include
 */

final class CommProtocolPermissionCollection extends PermissionCollection
{
    private Hashtable perms;

    /**
     * Create an empty GCFPermissionCollection object.
     *
     */

    public CommProtocolPermissionCollection() {
      perms = new Hashtable(11);
    }

    /**
     * Adds a permission to the GCFPermissions. The key for the hash is
     * permission.uri.
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

    public void add(Permission permission)
    {
	if (! (permission instanceof CommProtocolPermission))
	    throw new IllegalArgumentException("invalid permission: "+
					       permission);
	if (isReadOnly())
	    throw new SecurityException("attempt to add a Permission to a readonly PermissionCollection");

	CommProtocolPermission bp = (CommProtocolPermission) permission;

        // No need to synchronize because all adds are done sequentially
	// before any implies() calls

	perms.put(bp.getURI(), permission);
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

    public boolean implies(Permission permission)
    {
	if (! (permission instanceof CommProtocolPermission)) {
	  return false;
        }

	GCFPermission bp = (GCFPermission) permission;


        Enumeration all_permissions = elements();
        while (all_permissions.hasMoreElements()) {
          CommProtocolPermission p = (CommProtocolPermission)all_permissions.nextElement();
          if (p.implies(bp)) {
            return true;
          }
        }  

	// we don't have to check for "*" as it was already checked
	// at the top (all_allowed), so we just return false
	return false;          
    }

    /**
     * Returns an enumeration of all the GCFPermission objects in the
     * container.
     *
     * @return an enumeration of all the GCFPermission objects.
     */

    public Enumeration elements() {
      return perms.elements();
    }

}

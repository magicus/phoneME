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
import java.util.Vector;
/**
 * This class represents access rights to connections via the "file" protocol. 
 * A FileProtocolPermission consists of a URI string indicating a pathname and 
 * a set of actions desired for that pathname.
 *
 * The URI string takes the following form:
 *
 * file://{pathname}
 * 
 *
 * A pathname that ends in "/*" indicates all the files and directories contained in that directory. 
 * A pathname that ends with "/-" indicates (recursively) all files and subdirectories contained in that directory.
 *
 * The actions to be granted are passed to the constructor in a string containing a list of one or more 
 * comma-separated keywords. The possible keywords are "read" and "write". The actions string is converted to lowercase before processing. 
 */

public final class FileProtocolPermission extends GCFPermission {

  private String actions;
  String path;
  boolean has_wildcard;
  boolean has_read_action = false;
  boolean has_write_action = false;
  /**
   * Creates a new FileProtocolPermission with the specified actions. 
   * The specified URI becomes the name of the FileProtocolPermission. 
   * The URI string must conform to the specification given above.
   * @param name - The name of the file.
   * @param actions - The actions string.
   *
   * @throws IllegalArgumentException - if uri or actions is malformed.
   * @throws NullPointerException - if uri or actions is null.
   */
  public FileProtocolPermission(String uri, String actions) {
    super(uri);
    this.actions = actions.toLowerCase().trim();
    parse_actions();
    parse_uri(uri);
  }

  private void parse_actions() {
    String remained_actions = actions;
    int idx = remained_actions.indexOf(',');
    while (idx != -1) {
      if (idx == 0) {
        throw new IllegalArgumentException("action string is malformed:" + actions);
      }
      String action = remained_actions.substring(0, idx - 1).trim();
      if (action.equals("read")) {
        has_read_action = true;
      } else if (action.equals("write")) {
        has_read_action = true;
      } else {
        throw new IllegalArgumentException("action string has illegal keyword: " + action);
      }

      remained_actions = remained_actions.substring(idx + 1, remained_actions.length()).trim();
      idx = remained_actions.indexOf(',');
    }

    //single keyword remained
    if (remained_actions.equals("read")) {
      has_read_action = true;
    } else if (remained_actions.equals("write")) {
      has_read_action = true;
    } else if (remained_actions.equals("")) { // do nothing
    } else {
      throw new IllegalArgumentException("action string has illegal keyword: " + remained_actions);
    }
  }

  private void parse_uri(String uri) {
    if (uri.startsWith("file://")) {
      throw new IllegalArgumentException("uri must start with file://");
    }
    String full_path = uri.substring(7, uri.length());
    int idx = path.indexOf('*');
    if (idx == -1 ) {
      path = full_path;
    }
    has_wildcard = true;
    if (idx != full_path.length())  { //* is not last symbol
      throw new IllegalArgumentException("uri is malformed. * could be only in the end of uri!"); 
    }
    if (idx == 0) { //path is *
      path = "";
    } else {
      path = full_path.substring(0, full_path.length() - 2);
    }
  }
  /**
   * Checks if this FileProtocolPermission object "implies" the specified permission.
   *
   * More specifically, this method returns true if:
   *
   *  * p is an instanceof FileProtocolPermission,
   *
   *  * p's actions are a proper subset of this object's actions, and
   *
   *  * p's pathname is implied by this object's pathname. For example, "/tmp/*" implies "/tmp/foo", 
   * since "/tmp/*" encompasses the "/tmp" directory and all files in that directory, including the one named "foo". 
   * @param p - the permission to check against
   * @return true if the specified permission is implied by this object, false if not.
   */ 
  public boolean implies(Permission p) {
    if (!(p instanceof FileProtocolPermission)) {
      return false;
    } 

    FileProtocolPermission fp = (FileProtocolPermission)p;

    if (fp.has_read_action && !has_read_action) {
      return false;
    }

    if (fp.has_write_action && !has_write_action) {
      return false;
    }
    if (has_wildcard) {
      return fp.path.startsWith(path);
    } else {
      return fp.path.equals(path);
    }
  }

  /**
   * Checks two FileProtocolPermission objects for equality.
   * @param obj - the object we are testing for equality with this object..
   * @return true if obj is a ProtocolPermission and has the same URI string 
   *              as this FileProtocolPermission object.
   */
  public boolean equals(Object obj) {
    if (!(obj instanceof FileProtocolPermission)) {
      return false;
    }
    FileProtocolPermission other = (FileProtocolPermission)obj;
    return other.getURI().equals(getURI()) 
           && has_read_action == other.has_read_action 
           && has_write_action == other.has_write_action;
  }

  /**
   * Returns the hash code value for this object.
   * @return  a hash code value for this object.
   */
  public int hashCode() {
    return getURI().hashCode() ^ actions.hashCode();
  }

  /**
   * Returns the canonical string representation of the actions. 
   * Always returns present actions in the following order: read, write.
   * @return the canonical string representation of the actions..
   */
  public String getActions() {
    if (has_read_action && has_write_action) {
      return "read, write";
    } else if (has_read_action) {
      return "read";
    } else if (has_write_action) {
      return "write";
    }

    return "";
  }

  /**
   * Returns a new PermissionCollection for storing FileProtocolPermission objects.
   *
   * FileProtocolPermission objects must be stored in a manner that allows them to be inserted 
   * into the collection in any order, but that also enables the PermissionCollection implies 
   * method to be implemented in an efficient (and consistent) manner.  
   *
   * @return a new PermissionCollection suitable for storing FileProtocolPermission objects.
   */
  public PermissionCollection newPermissionCollection() {
    return new FileProtocolPermissionCollection();
  } 

}

/**
 * A FileProtocolPermissionCollection stores a collection
 * of FileProtocol permissions. FileProtocolPermission objects
 * must be stored in a manner that allows them to be inserted in any
 * order, but enable the implies function to evaluate the implies
 * method in an efficient (and consistent) manner.
 *
 *
 * @see java.security.Permission
 * @see java.security.Permissions
 * @see java.security.PermissionsImpl
 *
 * 
 *
 */

final class FileProtocolPermissionCollection extends PermissionCollection
{
    private Hashtable read_perms;
    private Hashtable write_perms;
    private Vector    perms;

    /**
     * Create an empty FileProtocolPermissionCollection object.
     *
     */

    public FileProtocolPermissionCollection() {
      read_perms = new Hashtable(6);
      write_perms = new Hashtable(6);
      perms = new Vector(12);
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
	if (! (permission instanceof FileProtocolPermission))
	    throw new IllegalArgumentException("invalid permission: "+
					       permission);
	if (isReadOnly())
	    throw new SecurityException("attempt to add a Permission to a readonly PermissionCollection");

	FileProtocolPermission bp = (FileProtocolPermission) permission;
        perms.addElement(permission);

        if (bp.has_read_action) {
          read_perms.put(bp.path, permission);
        }
        if (bp.has_write_action) {
          read_perms.put(bp.path, permission);
        } 
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
      if (! (permission instanceof FileProtocolPermission)) {
        return false;
      }

      FileProtocolPermission bp = (FileProtocolPermission) permission;
      if (bp.has_read_action) {
        if (!check_permission(read_perms, bp)) {
          return false;
        }
      }
      if (bp.has_write_action) {
        if (!check_permission(write_perms, bp)) {
          return false;
        }
      }
      return true;
    }

    private boolean check_permission(Hashtable perms, FileProtocolPermission bp) {
      String path_to_check = bp.path;
      if (perms.get(path_to_check) != null) {
        FileProtocolPermission perm = (FileProtocolPermission)perms.get(path_to_check);
        if (perm.has_wildcard || !bp.has_wildcard) {
          return true;
        }
      }
      int idx;
      while ((idx = path_to_check.lastIndexOf('/')) != -1) {
        path_to_check = path_to_check.substring(0, idx - 1);
        if (perms.get(path_to_check) != null) {
          FileProtocolPermission perm = (FileProtocolPermission)perms.get(path_to_check);
          if (perm.has_wildcard || !bp.has_wildcard) {
            return true;
          }
        }        
      }
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

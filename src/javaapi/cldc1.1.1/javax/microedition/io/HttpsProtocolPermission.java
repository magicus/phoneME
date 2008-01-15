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
import java.util.Vector;

/**
 * This class represents access rights to connections via the "Https" protocol. 
 * A HttpsProtocolPermission consists of a URI string but no actions list.
 * 
 * The URI string specifies a data resource accessible via Https. It takes the followoing form:
 *
 * Https://{host}[:{portrange}][{pathname}]
 *
 *
 * If the {host} string is a DNS name, an asterisk may appear in the leftmost position to indicate a wildcard 
 * match (e.g., "*.sun.com").
 * 
 * The {portrange} string takes the following form:
 *
 * portnumber | -portnumber | portnumber-[portnumber]
 * 
 *
 * A {portrange} specification of the form "N-" (where N is a port number) signifies all ports numbered N and above, 
 * while a specification of the form "-N" indicates all ports numbered N and below.
 */

public final class HttpsProtocolPermission extends GCFPermission {

  String host;
  String path;
  boolean has_host_wildcard;
  boolean has_path_wildcard;
  int port_low = -1; 
  int port_high = 0x7fffffff;
  boolean all_ports = false;
  

  /**
   * Creates a new HttpsProtocolPermission with the specified URI as its name. 
   * The URI string must conform to the specification given above.
   * @param the URI string.
   * @throws IllegalArgumentException - if uri is malformed.
   * @throws NullPointerException - if uri is null.
   */
  public HttpsProtocolPermission(String uri) {
    super(uri);
    if (!uri.startsWith("https://")) {
      throw new IllegalArgumentException("uri shall start with 'https://', but is " + uri);
    }
    String full_info = uri.substring(8/*"https://".length()*/, uri.length());
 
    String[] infos = parse_full_info(full_info);

    if (infos[0] != null) {
      String[] host_infos = parse_host_info(infos[0]);
      host = host_infos[1];
      has_host_wildcard = (host_infos[0] != null);
    }
    int[] ports = parse_port_info(infos[1]);
    port_low = ports[0];
    port_high = ports[1];
    all_ports = (port_low == -1 && port_high == 0x7fffffff);

    path = infos[2];
  }

  /**
   * Checks if this HttpsProtocolPermission object "implies" the specified permission.
   *
   * More specifically, this method first ensures that all of the following are true (and returns false if any of them are not):
   *
   * * p is an instanceof HttpsProtocolPermission, and
   *
   * * p's port range is included in this port range. 
   *
   * Then implies checks each of the following, in order, and for each returns true if the stated condition is true:
   *
   * * If this object was initialized with a single IP address and one of p's IP addresses is equal to this object's IP address.
   *
   * * If this object is a wildcard domain (such as *.sun.com), and p's canonical name (the name without any preceding *) ends with this object's canonical host name. For example, *.sun.com implies *.eng.sun.com..
   *
   * * If this object was not initialized with a single IP address, and one of this object's IP addresses equals one of p's IP addresses.
   *
   * * If this canonical name equals p's canonical name.   * If none of the above are true, implies returns false. 
   * @param p - the permission to check against 
   * @return true if the specified permission is implied by this object, false if not.
   */
  public boolean implies(Permission p) {
    if (!(p instanceof HttpsProtocolPermission)) {
      return false;
    } 
    HttpsProtocolPermission hp = (HttpsProtocolPermission)p;
    if (port_low > hp.port_low) {
      return false;
    }
    if (port_high < hp.port_high) {
      return false;
    }

    return implies_except_ports(hp);
  }

  boolean implies_except_ports(HttpsProtocolPermission hp) {
    //host checking
    if (has_host_wildcard) {
      if (!hp.host.endsWith(host)) {
        return false;
      }
    } else {
      if (!(hp.host.equals(host) && !hp.has_host_wildcard)) {
        return false;
      }
    }

    //path checking
    if (has_path_wildcard) {
      if (!hp.path.startsWith(path)) {
        return false;
      }
    } else {
      if (!(hp.path.equals(path) && !hp.has_path_wildcard)) {
        return false;
      }
    }
    return true;
  }

  /**
   * Checks two HttpsProtocolPermission objects for equality.
   * @param obj - the object we are testing for equality with this object..
   * @return true if obj is a HttpsProtocolPermission and has the same URI string 
   *              as this HttpsProtocolPermission object.
   */
  public boolean equals(Object obj) {
    if (!(obj instanceof HttpsProtocolPermission)) {
      return false;
    }
    HttpsProtocolPermission other = (HttpsProtocolPermission)obj;
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
   * is the empty string "", since there are no actions defined for HttpsProtocolPermission.
   * @return the empty string "".
   */
  public String getActions() {
    return "";
  }

  /**
   * Returns a new PermissionCollection for storing HttpsProtocolPermission objects.
   *
   * HttpsProtocolPermission objects must be stored in a manner that allows them to be inserted 
   * into the collection in any order, but that also enables the PermissionCollection implies 
   * method to be implemented in an efficient (and consistent) manner.  
   *
   * @return a new PermissionCollection suitable for storing HttpsProtocolPermission objects.
   */
  public PermissionCollection newPermissionCollection() {
    return new HttpsProtocolPermissionCollection();
  } 
}

/**
 * A HttpsProtocolPermissionCollection stores a collection
 * of HttpsProtocol permissions. HttpsProtocolPermission objects
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

final class HttpsProtocolPermissionCollection extends PermissionCollection
{
    private Vector perms;

    /**
     * Create an empty DatagramProtocolPermissionCollection object.
     *
     */

    public HttpsProtocolPermissionCollection() {
      perms = new Vector(15);
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
	if (! (permission instanceof HttpsProtocolPermission))
	    throw new IllegalArgumentException("invalid permission: "+
					       permission);
	if (isReadOnly())
	    throw new SecurityException("attempt to add a Permission to a readonly PermissionCollection");

        perms.addElement(permission);
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
	if (! (permission instanceof HttpsProtocolPermission)) {
	  return false;
        }

	HttpsProtocolPermission bp = (HttpsProtocolPermission) permission;

        String host = bp.host;
        Enumeration search = perms.elements();
        int search_size = perms.size();

        int port_low[] = new int[search_size];
        int port_high[] = new int[search_size];
        int port_range_id = 0;
        while (search.hasMoreElements()) {
          HttpsProtocolPermission cur_perm = (HttpsProtocolPermission)search.nextElement();
          if (cur_perm.implies_except_ports(bp)) {
            if (cur_perm.all_ports) {
              return true;
            }
            port_low[port_range_id] = bp.port_low;
            port_high[port_range_id] = bp.port_high;
          } else {
            port_low[port_range_id] = 0x7fffffff;
            port_high[port_range_id] = 0x7fffffff;
          }       

          port_range_id++;
        }

        // now we need to determine if found port ranges cover bp port range;
        // we will sort the port ranges by the low border and will try to found the  
        // longest continues range covered by these ranges
        // we will use simple x^2 sort here, cause it is rare situation when we have many port ranges.

        for (int i = 0; i < search_size; i++) {
          for (int j = 0; j < search_size - 1; j++) {
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
        for (int i = 1; i < search_size; i++) {
          if (port_low[i] > current_high) { //end of continious range
            if (current_low <= bp.port_low && current_high >= bp.port_high) {
              return true;
            } 
            current_low = bp.port_low;
            current_high = bp.port_high;
          } else {
            if (current_high < port_high[i]) {
              current_high = port_high[i];
            }
          }
        }
	return (current_low <= bp.port_low && current_high >= bp.port_high);        
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

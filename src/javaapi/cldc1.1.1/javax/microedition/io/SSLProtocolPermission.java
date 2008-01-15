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
 * This class represents access rights to connections via the "ssl" protocol. 
 * A SSLProtocolPermission consists of a URI string but no actions list.
 *
 * The URI string specifies a secure socket stream connection. It takes the following form:
 *
 * ssl://{host}[:{portrange}]
 *
 *
 * If the {host} string is a DNS name, an asterisk may appear in the leftmost position to indicate a wildcard match 
 * (e.g., "*.sun.com").
 *
 * The {portrange} string takes the following form:
 *
 * portnumber | -portnumber | portnumber-[portnumber]
 *
 *
 * A {portrange} specification of the form "N-" (where N is a port number) signifies all ports numbered N and above, 
 * while a specification of the form "-N" indicates all ports numbered N and below.
 *
 */

public final class SSLProtocolPermission extends GCFPermission {

   String host = null;
   int port_low = -1;
   int port_high = 0x7fffffff;
   boolean server_mode = false;
   boolean has_wildcard = false;
   boolean all_ports = true;

  /**
   * Creates a new SSLProtocolPermission with the specified URI as its name. 
   * The URI string must conform to the specification given above.
   * @param the URI string.
   * @throws IllegalArgumentException - if uri is malformed.
   * @throws NullPointerException - if uri is null.
   */
  public SSLProtocolPermission(String uri) {
    super(uri);
    if (!uri.startsWith("ssl://")) {
      throw new IllegalArgumentException("uri shall start with 'SSL://', but is " + uri);
    }
    if (uri.length() == 6/*"ssl://".length()*/) {
      server_mode = true;
      return;
    }
    String full_info = uri.substring(6, uri.length());

    String[] infos = parse_full_info(full_info);

    if (infos[0] != null) {
      String[] host_infos = parse_host_info(infos[0]);
      host = host_infos[1];
      has_wildcard = (host_infos[0] != null);
    }

    int[] ports = parse_port_info(infos[1]);
    port_low = ports[0];
    port_high = ports[1];
    all_ports = (port_low == -1 && port_high == 0x7fffffff);

    server_mode = (host == null);
    if (infos[2] != null) {
      throw new IllegalArgumentException("uri is wrong, path is not supported for datagram connection");
    }
  }

  /**
   * Checks if this SSLProtocolPermission object "implies" the specified permission.
   *
   * More specifically, this method first ensures that all of the following are true (and returns false if 
   * any of them are not):
   *
   * * p is an instanceof SSLProtocolPermission, and
   *
   * * p's port range is included in this port range. 
   *
   * Then implies checks each of the following, in order, and for each returns true if the stated condition is true:
   *
   * * If this object was initialized with a single IP address and one of p's IP addresses is equal to this object's IP address.
   *
   * * If this object is a wildcard domain (such as *.sun.com), and p's canonical name (the name without any preceding *) ends with 
   *   this object's canonical host name. For example, *.sun.com implies *.eng.sun.com..
   *
   * * If this object was not initialized with a single IP address, and one of this object's IP addresses equals one of p's IP addresses.
   *
   * * If this canonical name equals p's canonical name. 
   *
   * If none of the above are true, implies returns false. 
   * @param p - the permission to check against 
   * @return true if the specified permission is implied by this object, false if not.
   */
  public boolean implies(Permission p) {
    if (!(p instanceof SSLProtocolPermission)) {
      return false;
    } 
    SSLProtocolPermission perm = (SSLProtocolPermission)p;
    if (server_mode ^ perm.server_mode) { // modes are different
      return false;
    }

    if (!all_ports) { //checking ports
      if (port_low > perm.port_low) {
        return false;
      }
      if (port_high < perm.port_high) {
        return false;
      }
    }
    
    if (has_wildcard) {
      return perm.host.endsWith(host);
    } else {
      if (perm.has_wildcard) {
        return false;
      }
      return host.equals(perm.host);
    }
  }

  /**
   * Checks two SSLProtocolPermission objects for equality.
   * @param obj - the object we are testing for equality with this object..
   * @return true if obj is a SSLProtocolPermission and has the same URI string 
   *              as this SSLProtocolPermission object.
   */
  public boolean equals(Object obj) {
    if (!(obj instanceof SSLProtocolPermission)) {
      return false;
    }
    SSLProtocolPermission other = (SSLProtocolPermission)obj;
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
   * is the empty string "", since there are no actions defined for SSLProtocolPermission.
   * @return the empty string "".
   */
  public String getActions() {
    return "";
  }

  /**
   * Returns a new PermissionCollection for storing SSLProtocolPermission objects.
   *
   * SSLProtocolPermission objects must be stored in a manner that allows them to be inserted 
   * into the collection in any order, but that also enables the PermissionCollection implies 
   * method to be implemented in an efficient (and consistent) manner.  
   *
   * @return a new PermissionCollection suitable for storing SSLProtocolPermission objects.
   */
  public PermissionCollection newPermissionCollection() {
    return new SSLProtocolPermissionCollection();
  } 
}

/**
 * A SSLProtocolPermissionCollection stores a collection
 * of SSLProtocol permissions. SSLProtocolPermission objects
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

final class SSLProtocolPermissionCollection extends PermissionCollection
{
    private Vector client_perms;
    private Vector server_perms;

    /**
     * Create an empty SSLProtocolPermissionCollection object.
     *
     */

    public SSLProtocolPermissionCollection() {
      client_perms = new Vector(6);
      server_perms = new Vector(6);
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
	if (! (permission instanceof SSLProtocolPermission))
	    throw new IllegalArgumentException("invalid permission: "+
					       permission);
	if (isReadOnly())
	    throw new SecurityException("attempt to add a Permission to a readonly PermissionCollection");

	SSLProtocolPermission bp = (SSLProtocolPermission) permission;

        if (bp.server_mode) {
          server_perms.addElement(permission);
        } else {
          client_perms.addElement(permission);
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
	if (! (permission instanceof SSLProtocolPermission)) {
	  return false;
        }

	SSLProtocolPermission bp = (SSLProtocolPermission) permission;

        String host = bp.host;
        Enumeration search = null;
        int search_size = 0;
        if (bp.server_mode) {      
          search = server_perms.elements();
          search_size = server_perms.size();
        } else {
          search = client_perms.elements();
          search_size = client_perms.size();
        }
        int port_low[] = new int[search_size];
        int port_high[] = new int[search_size];
        int port_range_id = 0;
        while (search.hasMoreElements()) {
          SSLProtocolPermission cur_perm = (SSLProtocolPermission)search.nextElement();
          boolean host_implies = false;
          if (cur_perm.has_wildcard) {
            host_implies = bp.host.endsWith(cur_perm.host);
          } else {
            host_implies = bp.host.equals(cur_perm.host) && !cur_perm.has_wildcard;
          }
          
          if (host_implies) {
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
      return new SSLPPCEnumeration(client_perms.elements(), server_perms.elements());
    }
}

class SSLPPCEnumeration implements Enumeration {
  Enumeration en1, en2;

  SSLPPCEnumeration(Enumeration en1, Enumeration en2) {
    this.en1 = en1; this.en2 = en2;
  }

  public boolean  hasMoreElements() {
    if (en1.hasMoreElements()) {
      return true;
    }
    return en2.hasMoreElements();
  }

  public Object nextElement() {
    if (en1.hasMoreElements()) {
      return en1.nextElement();
    }
    return en2.nextElement();
  }
}

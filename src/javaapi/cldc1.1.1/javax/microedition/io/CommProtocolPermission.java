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
 * This class represents access rights to connections via the "comm" protocol.
 * A <code>CommProtocolPermission</code> consists of a URI string but no
 * actions list.
 * <p>
 * The URI string specifies a logical serial port connection and optional
 * parameters.  It takes the following form:
 * <pre>
 * comm:{port identifier}[{optional parameters}]
 * </pre>
 * An asterisk may appear at the end of the URI string to indicate a
 * wildcard match in the <em>port identifer</em> field.
 * Valid examples include "comm:*" and "comm:port*".
 *
 * @see Connector#open
 * @see "javax.microedition.io.CommConnection" in <a href="http://www.jcp.org/en/jsr/detail?id=271">MIDP 3.0 Specification</a>
 */
public final class CommProtocolPermission extends GCFPermission {
  /** Bit flag: 1 stop bits. */
  private final static int serSettingsFlagStopBits1     = 1 << 0;
  /** Bit flag: 2 stop bits. */
  private final static int serSettingsFlagStopBits2     = 1 << 1;
  /** Bit flag: parity none. */
  private final static int serSettingsFlagParityNoneM   = 1 << 2;
  /** Bit flag: parity on. */
  private final static int serSettingsFlagParityOddM    = 1 << 3;
  /** Bit flag: parity even. */
  private final static int serSettingsFlagParityEvenM   = 1 << 4;
  /** Bit flag: RTS rcv flow control. */
  private final static int serSettingsFlagRTSAutoM      = 1 << 5;
  private final static int serSettingsFlagRTSAutoOffM   = 1 << 6;
  /** Bit flag: CTS xmit flow control. */
  private final static int serSettingsFlagCTSAutoM      = 1 << 7;
  private final static int serSettingsFlagCTSAutoOffM   = 1 << 8;
  /** Bit flag: 7 bits/char. */
  private final static int serSettingsFlagBitsPerChar7  = 1 << 9;
  /** Bit flag: 8 bits/char. */
  private final static int serSettingsFlagBitsPerChar8  = 1 << 10;
  /** Bit flag: blocking */
  private final static int serSettingsFlagBlockingOn    = 1 << 11;
  private final static int serSettingsFlagBlockingOff   = 1 << 12;

  /** Port name */
  private String port;
  /** Bit per char. */
  private int bbc      = serSettingsFlagBitsPerChar7 | 
                         serSettingsFlagBitsPerChar8;
  /** Stop bits. */
  private int stop     = serSettingsFlagStopBits1 | 
                         serSettingsFlagStopBits2;
  /** Parity. */
  private int parity   = serSettingsFlagParityNoneM | 
                         serSettingsFlagParityOddM  |
                         serSettingsFlagParityEvenM;
  /** RTS. */
  private int rts      = serSettingsFlagRTSAutoM | 
                         serSettingsFlagRTSAutoOffM;
  /** CTS. */
  private int cts      = serSettingsFlagCTSAutoM | 
                         serSettingsFlagCTSAutoOffM;
  /** Blocking. */
  private int blocking = serSettingsFlagBlockingOn | 
                         serSettingsFlagBlockingOff;
  /** Baud rate. */
  private int baud     = -1;


  /**
   * Creates a new <code>CommProtocolPermission</code> with the specified
   * URI as its name.  The URI string must conform to the specification
   * given above.
   *
   * @param uri the URI string.
   *
   * @throws IllegalArgumentException if <code>uri</code> is malformed.
   * @throws NullPointerException if <code>uri</code> is <code>null</code>.
   *
   * @see #getName
   */
  public CommProtocolPermission(String uri) {
    super(uri);

    if (!"comm".equals(getProtocol())) {
      throw new IllegalArgumentException("Expected comm protocol: " + uri);
    }

    checkHostPortOnly();

    checkNoHostPort();

    String info = getSchemeSpecificPart();

    if (info == null || "".equals(info)) {
      throw new IllegalArgumentException(
        "Expected scheme-specific part: " + uri);
    }

    parseSpecificPart(info);
  }

  private void parseSpecificPart(String info) {
    int p = 0;
    int len = info.length();

    for (p = 0; p < len && isAlphaNum(info.charAt(p)); p++) {}

    // Asterisk can be used to indicate a wildcard match 
    // in the port identifier field
    if (p < len && info.charAt(p) == '*') {
      if (p != len - 1) {
        throw new IllegalArgumentException(
          "Asterisk can appear only at the end: " + info);
      }
      p++;
    }

    if (p == 0) {
      throw new IllegalArgumentException("Expected port identifier: " + info);
    }

    port = info.substring(0, p);

    while (p < len) {
      if (info.charAt(p) != ';') {
        throw new IllegalArgumentException(
          "Expected semi-colon delimiter: " + info);
      }
      p++;
      int q = info.indexOf('=', p);
      if (q == -1) {
        throw new IllegalArgumentException(
          "Malformed optional parameters: " + info);
      }
      String name = info.substring(p, q);
      q++;
      int r = info.indexOf(';', q);
      if (r == -1) {
        r = len;
      }
      String value = info.substring(q, r);
      if ("baudrate".equals(name)) {
        try {
          baud = Integer.parseInt(value);
        } catch (NumberFormatException e) {
          throw new IllegalArgumentException(
            "Invalid baudrate value: " + info);
        }
        if (baud <= 0) {
          throw new IllegalArgumentException(
            "Invalid baudrate value: " + info);
        }
      } else if ("bitsperchar".equals(name)) {
        if ("7".equals(value)) {
          bbc = serSettingsFlagBitsPerChar7;
        } else if ("8".equals(value)) {
          bbc = serSettingsFlagBitsPerChar8;
        } else {
          throw new IllegalArgumentException(
            "Invalid bitsperchar value: " + info);
        }
      } else if ("stopbits".equals(name)) {
        if ("1".equals(value)) {
          stop = serSettingsFlagStopBits1;
        } else if ("2".equals(value)) {
          stop = serSettingsFlagStopBits2;
        } else {
          throw new IllegalArgumentException(
            "Invalid stopbits value: " + info);
        }
      } else if ("parity".equals(name)) {
        if ("odd".equals(value)) {
          stop = serSettingsFlagParityOddM;
        } else if ("even".equals(value)) {
          stop = serSettingsFlagParityEvenM;
        } else if ("none".equals(value)) {
          stop = serSettingsFlagParityNoneM;
        } else {
          throw new IllegalArgumentException(
            "Invalid parity value: " + info);
        }
      } else if ("blocking".equals(name)) {
        if ("on".equals(value)) {
          blocking = serSettingsFlagBlockingOn;
        } else if ("off".equals(value)) {
          blocking = serSettingsFlagBlockingOff;
        } else {
          throw new IllegalArgumentException(
            "Invalid blocking value: " + info);
        }
      } else if ("autocts".equals(name)) {
        if ("on".equals(value)) {
          cts = serSettingsFlagCTSAutoM;
        } else if ("off".equals(value)) {
          cts = serSettingsFlagCTSAutoOffM;
        } else {
          throw new IllegalArgumentException("Invalid autocts value: " + info);
        }
      } else if ("autorts".equals(name)) {
        if ("on".equals(value)) {
          rts = serSettingsFlagRTSAutoM;
        } else if ("off".equals(value)) {
          rts = serSettingsFlagRTSAutoOffM;
        } else {
          throw new IllegalArgumentException("Invalid autorts value: " + info);
        }
      } else {
        throw new IllegalArgumentException("Unknown parameter used: " + info);
      }
      p = r;
    }
  }

  private static boolean isAlphaNum(char c) {
    return Character.isDigit(c) || ('a' <= c && c <= 'z') ||
      ('A' <= c && c <= 'Z');
  }

  /**
   * Checks if this <code>CommProtocolPermission</code> object "implies"
   * the specified permission.
   * <p>
   * More specifically, this method returns <code>true</code> if:
   * <p>
   * <ul>
   * <li> <i>p</i> is an instanceof <code>CommProtocolPermission</code>, and
   * <p>
   * <li> <i>p</i>'s URI string equals or (in the case of wildcards) is
   *   implied by this object's URI string.
   *   For example, "comm:*" implies "comm:port1;baudrate=300".
   * </ul>
   * @param p the permission to check against
   *
   * @return true if the specified permission is implied by this object,
   * false if not.
   */
  public boolean implies(Permission p) {
    if (!(p instanceof CommProtocolPermission)) {
      return false;
    } 
    CommProtocolPermission that = (CommProtocolPermission)p;

    String thisPart = this.getSchemeSpecificPart();
    String thatPart = that.getSchemeSpecificPart();

    if (thisPart.equals(thatPart)) {
      return true;
    }

    if (thisPart.endsWith("*")) {
      String s = thisPart.substring(0, thisPart.length() - 1);
      return thatPart.startsWith(s);
    }

    if (!this.port.equals(that.port)) {
      return false;
    }

    if (this.baud != -1 && this.baud != that.baud) {
      return false;
    }

    int thisParams = 
      this.bbc | this.stop | this.parity | this.rts | this.cts | this.blocking;
    int thatParams = 
      that.bbc | that.stop | that.parity | that.rts | that.cts | that.blocking;

    return (thisParams & thatParams) == thatParams;
  }

  /**
   * Checks two <code>CommProtocolPermission</code> objects for equality.
   * 
   * @param obj the object we are testing for equality with this object.
   *
   * @return <code>true</code> if <code>obj</code> is a
   * <code>CommProtocolPermission</code> and has the same URI string as
   * this <code>CommProtocolPermission</code> object.
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
   *
   * @return a hash code value for this object.
   */
  public int hashCode() {
    return getURI().hashCode();
  }

  /**
   * Returns the canonical string representation of the actions, which
   * currently is the empty string "", since there are no actions defined
   * for <code>CommProtocolPermission</code>.
   *
   * @return the empty string "".
   */
  public String getActions() {
    return "";
  }

  /**
   * Returns a new <code>PermissionCollection</code> for storing
   * <code>CommProtocolPermission</code> objects.
   * <p>
   * <code>CommProtocolPermission</code> objects must be stored in a
   * manner that allows
   * them to be inserted into the collection in any order, but that also
   * enables the <code>PermissionCollection</code> implies method to be
   * implemented in an efficient (and consistent) manner.
   *
   * @return a new <code>PermissionCollection</code> suitable for storing
   * <code>CommProtocolPermission</code> objects.
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
 */

final class CommProtocolPermissionCollection extends PermissionCollection {
  private Vector permissions = new Vector(6);;

  /**
   * Create an empty GCFPermissionCollection object.
   *
   */
  public CommProtocolPermissionCollection() {}

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
  public void add(Permission permission) {
    if (!(permission instanceof CommProtocolPermission)) {
      throw new IllegalArgumentException("invalid permission: "+
                                         permission);
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
    if (!(permission instanceof CommProtocolPermission)) {
      return false;
    }

    Enumeration search = elements();
    while (search.hasMoreElements()) {
      CommProtocolPermission p = (CommProtocolPermission)search.nextElement();
      if (p.implies(permission)) {
        return true;
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
    return permissions.elements();
  }
}

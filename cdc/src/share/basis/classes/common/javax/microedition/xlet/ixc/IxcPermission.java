/*
 * @(#)IxcPermission.java	1.7 06/10/10
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

package javax.microedition.xlet.ixc;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Enumeration;
import java.util.List;
import java.util.Vector;
import java.security.Permission;
import java.security.PermissionCollection;

public final class IxcPermission extends Permission {

    /**
     * Bind action.
     */
    private final static int BIND = 0x1;
    /**
     * Lookup action.
     */
    private final static int LOOKUP   = 0x2;

    /**
     * All actions (bind,lookup)
     */
    private final static int ALL     = BIND|LOOKUP;

    /**
     * No actions.
     */
    private final static int NONE    = 0x0;

    // the actions mask
    private transient int mask;

    // static Strings used by init(int mask)
    private static final char WILD_CHAR = '*';

    // canonicalized dir path. In the case of
    // directories, it is the name "/blah/*" without
    // the last character (the "*" or "-").
    private transient String cpath;

    /**
     * the actions string.
     *
     * @serial
     */
    private String actions; // Left null as long as possible, then
                            // created and re-used in the getAction function.

    // does path include a wildcard?
    private transient boolean wildcard;

    /**
    public String toString() {
        StringBuffer sb = new StringBuffer();
        sb.append("***\n");
        sb.append("cpath = "+cpath+"\n");
        sb.append("mask = "+mask+"\n");
        sb.append("actions = "+getActions()+"\n");
        sb.append("wildcard = "+wildcard+"\n");
        sb.append("***\n");
        return sb.toString();
    }
    **/

   /*
    * Initialize a IxcPermission object. Common to all constructors.
    * Also called during de-serialization.
    *
    * @param mask the actions mask to use.
    */
    private void init(int mask) {
       if ((mask & ALL) != mask)
          throw new IllegalArgumentException("invalid actions mask");
       if (mask == NONE)
          throw new IllegalArgumentException("invalid actions mask");
       if ((cpath = getName()) == null)
          throw new NullPointerException("name can't be null");

       this.mask = mask;
       int len = cpath.length();
       char last = ((len > 0) ? cpath.charAt(len - 1) : 0);

       if (last == WILD_CHAR &&
            (len == 1 || cpath.charAt(len - 2) == '/')) { //6254827
            wildcard = true;
            cpath = cpath.substring(0, --len);
       }       
    }

    /**
     * Converts an actions String to an actions mask.
     *
     * @param action the action string.
     * @return the actions mask.
     */
    private static int getMask(String actions) {
        int mask = NONE;

        if (actions == null) {
           return mask;
        }

        char[] a = actions.toCharArray();

        int i = a.length - 1;
        if (i < 0)
           return mask;

        while (i != -1) {
           char c;

           // skip whitespace
           while ((i!=-1) && ((c = a[i]) == ' ' ||
                               c == '\r' ||
                               c == '\n' ||
                               c == '\f' ||
                               c == '\t'))
               i--;

           // check for the known strings
           int matchlen;
           if (i >= 3 && (a[i-3] == 'b' || a[i-3] == 'B') &&
                         (a[i-2] == 'i' || a[i-2] == 'I') &&
                         (a[i-1] == 'n' || a[i-1] == 'N') &&
                         (a[i] == 'd' || a[i] == 'D')) {
                matchlen = 4;
                mask |= BIND;

           } else if (i >= 5 && (a[i-5] == 'l' || a[i-5] == 'L') &&
                                (a[i-4] == 'o' || a[i-4] == 'O') &&
                                (a[i-3] == 'o' || a[i-3] == 'O') &&
                                (a[i-2] == 'k' || a[i-2] == 'K') &&
                                (a[i-1] == 'u' || a[i-1] == 'U') &&
                                (a[i] == 'p' || a[i] == 'P')) {
               matchlen = 6;
               mask |= LOOKUP;
      
           } else {
               // parse error
               throw new IllegalArgumentException(
                  "invalid permission: " + actions);
           }

           // make sure we didn't just match the tail of a word
           // like "ackbarfaccept".  Also, skip to the comma.
           boolean seencomma = false;
           while (i >= matchlen && !seencomma) {
                switch(a[i-matchlen]) {
                case ',':
                    seencomma = true;
                    /*FALLTHROUGH*/
                case ' ': case '\r': case '\n':
                case '\f': case '\t':
                    break;
                default:
                    throw new IllegalArgumentException(
                            "invalid permission: " + actions);
                }
                i--;
            }

            // point i at the location of the comma minus one (or -1).
            i -= matchlen;
        }

        return mask;
    }

    /**
     * Return the current action mask. Used by the IxcPermissionCollection.
     *
     * @return the actions mask.
     */
    int getMask() {
       return mask;
    }

    public IxcPermission(String name, String actions) {
       super(name);
       init(getMask(actions));
    }

    // package private for use by the IxcPermissionCollection add method
    IxcPermission(String path, int mask) {
        super(path);
        init(mask);
    }

    public boolean implies(Permission p) { 
       if (!(p instanceof IxcPermission))
           return false;

       IxcPermission that = (IxcPermission) p;

       // we get the effective mask. i.e., the "and" of this and that.
       // They must be equal to that.mask for implies to return true.

       return ((this.mask & that.mask) == that.mask) 
                && impliesIgnoreMask(that);
    }
 
    /**
     * Checks if the Permission's actions are a proper subset of the
     * this object's actions. Returns the effective mask iff the
     * this IxcPermission's path also implies that IxcPermission's path.
     *
     * @param that the IxcPermission to check against.
     * @param exact return immediatly if the masks are not equal
     * @return the effective mask
     */
    boolean impliesIgnoreMask(IxcPermission that) {
       if (this.wildcard) {
          if (that.wildcard) {
             // if the permission passed in is a directory
             // specification, make sure that a non-recursive
             // permission (i.e., this object) can't imply a recursive
             // permission.
             if (this.cpath.equals(that.cpath)) 
                return true;
             else if (this.cpath.length() == 0)
                return true;
             else 
                // A/B/* implies A/B/C/*
                // A/*/D is not a valid wildcard
                return (that.cpath.indexOf(this.cpath)==0 && 
                        that.cpath.length() > this.cpath.length());
          } else {
             int last = that.cpath.lastIndexOf('/'); //6254827
             if (last == -1) {
                // Check for the wildcard instead of returning
                // false blindly
                return (this.cpath.length() == 0);
             } else {
                // this.cpath.equals(that.cpath.substring(0, last+1));
                // Use regionMatches to avoid creating new string
                // A/B/* implies A/B/C
                return (this.cpath.length() <= (last + 1)) &&
                        this.cpath.regionMatches(0, that.cpath, 0, this.cpath.length());
                        //this.cpath.regionMatches(0, that.cpath, 0, last+1);
             }
          }

       } else {
           return (this.cpath.equals(that.cpath));

       }
    }

    public boolean equals(Object obj) {
       if (obj == this) 
          return true;
       if (!(obj instanceof IxcPermission)) 
          return false;

       IxcPermission that = (IxcPermission) obj;
       return ((this.mask == that.mask) &&
               this.cpath.equals(that.cpath) &&
               (this.wildcard == that.wildcard));
    }
 
    public int hashCode() { 
        return this.cpath.hashCode();
    }

    private static String getActions(int mask) {
       StringBuffer sb = new StringBuffer();
       boolean comma = false;

       if ((mask & BIND) == BIND) {
            comma = true;
            sb.append("bind");
       }

       if ((mask & LOOKUP) == LOOKUP) {
            if (comma) sb.append(',');
            else comma = true;
            sb.append("lookup");
       }

       return sb.toString();
    }

    public String getActions() { 
       if (actions == null)
          actions = getActions(this.mask);

       return actions;
    }

    public java.security.PermissionCollection newPermissionCollection() {
       return new IxcPermissionCollection();
    }
}

final class IxcPermissionCollection extends PermissionCollection {

   // Not serialized; see serialization section at end of class
    private transient List perms;

 
   /**
    * Create an empty IxcPermissions object.
    */
    public IxcPermissionCollection() {
       perms = new ArrayList();
    }

   /**
    * Adds a permission to the IxcPermissions. The key for the hash is
    * permission.path.
    *
    * @param permission the Permission object to add.
    * @exception IllegalArgumentException - if the permission is not a
    *                                       IxcPermission
    * @exception SecurityException - if this IxcPermissionCollection object
    *                                has been marked readonly
    */

    public void add(Permission permission) {
       if (! (permission instanceof IxcPermission))
          throw new IllegalArgumentException("invalid permission: "+
                                               permission);
       if (isReadOnly())
          throw new SecurityException("attempt to add a Permission to a readonly PermissionCollection");

       // No need to synchronize because all adds are done sequentially
       // before any implies() calls
        perms.add(permission);
    }

   /**
    * Check and see if this set of permissions implies the permissions
    * expressed in "permission".
    *
    * @param p the Permission object to compare
    * @return true if "permission" is a proper subset of a permission in
    * the set, false if not.
    */

    public boolean implies(Permission permission) {
       if (!(permission instanceof IxcPermission))
                return false;

       IxcPermission fp = (IxcPermission) permission;

       int desired = fp.getMask();
       int effective = 0;
       int needed = desired;

       int len = perms.size();
       for (int i = 0; i < len; i++) {
          IxcPermission x = (IxcPermission) perms.get(i);
          if (((needed & x.getMask()) != 0) && x.impliesIgnoreMask(fp)) {
             effective |=  x.getMask();
             if ((effective & desired) == desired)
                return true;
             needed = (desired ^ effective);
          }
       }

       return false;
    }

   /**
    * Returns an enumeration of all the IxcPermission objects in the
    * container.
    *
    * @return an enumeration of all the IxcPermission objects.
    */

    public Enumeration elements() {
       // Convert Iterator into Enumeration
       return Collections.enumeration(perms);
    }
}

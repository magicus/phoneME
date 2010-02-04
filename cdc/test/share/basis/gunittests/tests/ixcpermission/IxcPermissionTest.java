/*
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


package tests.ixcpermission ;

import javax.microedition.xlet.ixc.IxcPermission;
import gunit.framework.*;

// Just a test to check for equals() and implies() on 
// various IxcPermission instances.

public class IxcPermissionTest extends TestCase { 

    public void test_ixcPermissions() {

      String[] bindActions   = {"BIND", "Bind", "bind", "bInD"};
      String[] lookupActions = {"LOOKUP", "Lookup", "lookup", "lOOkUp"};

      IxcPermission[] bindPerms = new IxcPermission[bindActions.length];
      for (int i = 0; i < bindPerms.length; i++) { 
         bindPerms[i] = new IxcPermission("*", bindActions[i]);
      }

      IxcPermission[] lookupPerms = new IxcPermission[lookupActions.length];
      for (int i = 0; i < lookupPerms.length; i++) { 
         lookupPerms[i] = new IxcPermission("*", lookupActions[i]);
      }

      for (int i = 0; i < 4; i++) {
         for (int j = 0; j < 4; j++) {
            if (!bindPerms[i].equals(bindPerms[j]) ||
                !bindPerms[i].implies(bindPerms[j])) {
               System.out.println(bindPerms[i]);
               System.out.println(bindPerms[j]);
               throw new RuntimeException("Failed");
            }
         }
      }

      for (int i = 0; i < 4; i++) {
         for (int j = 0; j < 4; j++) {
            if (!lookupPerms[i].equals(lookupPerms[j]) ||
                !lookupPerms[i].implies(lookupPerms[j])) {
               System.out.println(lookupPerms[i]);
               System.out.println(lookupPerms[j]);
               throw new RuntimeException("Failed");
            }
         }
      }

      IxcPermission[] allPerms = new IxcPermission[bindActions.length];
      for (int i = 0; i < allPerms.length; i++) { 
         allPerms[i] = new IxcPermission("*", new String(bindActions[i] + "," + lookupActions[i]));
      }

      for (int i = 0; i < 4; i++) {
         for (int j = 0; j < 4; j++) {
            if (allPerms[i].equals(bindPerms[j]) ||
                !allPerms[i].implies(bindPerms[j])) {
               System.out.println(allPerms[i]);
               System.out.println(bindPerms[j]);
               throw new RuntimeException("Failed");
            }
         }
      }

      for (int i = 0; i < 4; i++) {
         for (int j = 0; j < 4; j++) {
            if (allPerms[i].equals(lookupPerms[j]) ||
                !allPerms[i].implies(lookupPerms[j])) {
               System.out.println(allPerms[i]);
               System.out.println(lookupPerms[j]);
               throw new RuntimeException("Failed");
            }
         }
      }

      IxcPermission[] allPermsABC = new IxcPermission[bindActions.length];
      for (int i = 0; i < allPermsABC.length; i++) { 
         allPermsABC[i] = new IxcPermission("ABC", new String(bindActions[i] + " , " + lookupActions[i]));
      }

      for (int i = 0; i < 4; i++) {
         for (int j = 0; j < 4; j++) {
            if (allPerms[i].equals(allPermsABC[j]) ||
                !allPerms[i].implies(allPermsABC[j])) {
               System.out.println(allPerms[i]);
               System.out.println(allPermsABC[j]);
               System.out.println("Equals : " +allPerms[i].equals(allPermsABC[j]));
               System.out.println("Implies: " + allPerms[i].implies(allPermsABC[j]));
               throw new RuntimeException("Failed");
            }
         }
      }

      IxcPermission[] bindPermsABC = new IxcPermission[bindActions.length];
      for (int i = 0; i < bindPermsABC.length; i++) { 
         bindPermsABC[i] = new IxcPermission("ABC", bindActions[i]);
      }

      for (int i = 0; i < 4; i++) {
         for (int j = 0; j < 4; j++) {
            if (bindPerms[i].equals(bindPermsABC[j]) ||
                !bindPerms[i].implies(bindPermsABC[j])) {
               System.out.println(bindPerms[i]);
               System.out.println(bindPermsABC[j]);
               throw new RuntimeException("Failed");
            }
         }
      }

      for (int i = 0; i < 4; i++) {
         for (int j = 0; j < 4; j++) {
            if (bindPerms[i].equals(lookupPerms[j]) ||
                bindPerms[i].implies(lookupPerms[j])) {
               System.out.println(bindPerms[i]);
               System.out.println(lookupPerms[j]);
               throw new RuntimeException("Failed");
            }
         }
      }

      IxcPermission[] lookupPermsABC = new IxcPermission[bindActions.length];
      for (int i = 0; i < lookupPermsABC.length; i++) { 
         lookupPermsABC[i] = new IxcPermission("ABC", lookupActions[i]);
      }

      for (int i = 0; i < 4; i++) {
         for (int j = 0; j < 4; j++) {
            if (bindPermsABC[i].equals(lookupPermsABC[j]) ||
                bindPermsABC[i].implies(lookupPermsABC[j])) {
               System.out.println(bindPermsABC[i]);
               System.out.println(lookupPermsABC[j]);
               throw new RuntimeException("Failed");
            }
         }
      }

      // make the previous imply the follower
      String[] variousPath = {"*", "xlet://*", "xlet://tmp/*", "xlet://tmp/ABC"};
      IxcPermission[] lookupPermsVAR = new IxcPermission[variousPath.length];
      for (int i = 0; i < lookupPermsABC.length; i++) { 
         lookupPermsVAR[i] = new IxcPermission(variousPath[i], "lookup");
      }
      for (int i = 0; i < 4; i++) {
         for (int j = i+1; j < 4; j++) {
            if (lookupPermsVAR[i].equals(lookupPermsVAR[j]) ||
                !lookupPermsVAR[i].implies(lookupPermsVAR[j])) {
               System.out.println(lookupPermsVAR[i]);
               System.out.println(lookupPermsVAR[j]);
               System.out.println("Equals : " + lookupPermsVAR[i].equals(lookupPermsVAR[j]));
               System.out.println("Implies: " + lookupPermsVAR[i].implies(lookupPermsVAR[j]));
               throw new RuntimeException("Failed");
            }
         }
      }
      System.out.println("OK");
 
    }
}         


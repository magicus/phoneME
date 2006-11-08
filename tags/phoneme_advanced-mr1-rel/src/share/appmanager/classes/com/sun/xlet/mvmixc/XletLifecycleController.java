/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

package com.sun.xlet.mvmixc;

import java.rmi.*;
import com.sun.appmanager.mtask.Client;
import com.sun.appmanager.appmodel.AppModelController;
import com.sun.appmanager.appmodel.XletAppModelController;

public class XletLifecycleController implements  XletLifecycleInterface {

   private String pid;
   private XletAppModelController controller;
 
   private int waitTime = 1000; // 1 sec 

   XletLifecycleController(String pid, Client client) {
      this.pid = pid;
      controller = (XletAppModelController)AppModelController.getAppModelController(AppModelController.XLET_APP_MODEL, client); 
   }
   
   public boolean postInitXlet() {
      controller.xletInitialize(pid);
      for (int i = 0; i < 10; i++) {
	  try {
	      Thread.sleep(waitTime);
	      if (getState() == XletAppModelController.XLET_PAUSED) {
                  return true;
	      } 
	  } catch (Exception e) {}
      }
      return false;
   }

   public boolean postStartXlet() {
      controller.xletActivate(pid);
      for (int i = 0; i < 10; i++) {
	  try {
	      Thread.sleep(waitTime);
	      if (getState() == XletAppModelController.XLET_ACTIVE) { 
                  return true;
	      }
	  } catch (Exception e) {}
      }
      return false;
   }

   public boolean postPauseXlet() {
      controller.xletDeactivate(pid);
      for (int i = 0; i < 10; i++) {
	  try {
	      Thread.sleep(waitTime);
	      if (getState() == XletAppModelController.XLET_PAUSED) {
		  return true;
	      } 
	  } catch (Exception e) {}
      }
      return false;
   }

   public boolean postDestroyXlet(boolean unconditional) {
      controller.xletDestroy(pid, unconditional);
      for (int i = 0; i < 10; i++) {
	  try {
	      Thread.sleep(waitTime);
	      if (getState() == XletAppModelController.XLET_DESTROYED 
		  || getState() == XletAppModelController.XLET_UNKNOWN) {
		  return true;
	      }
	  } catch (Exception e) {}
      }
      return false;
   }

   public int getState() {
      return controller.xletGetState(pid); 
   }
}
